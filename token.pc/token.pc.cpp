#include "token.pc.hpp"

token::token(name receiver, name code, datastream<const char *> ds)
    : contract::contract(receiver, code, ds)
{
}

void token::migration_data(const uint64_t &id)
{
    require_auth(get_self());
    deposits _deposits(get_self(), get_self().value);
    auto it = _deposits.find(id);

    if (it != _deposits.end())
    {
        const auto &deposit = *it;
        _deposits.erase(it);

       _deposits.emplace(get_self(), [&](auto &a) {
        a.id = _deposits.available_primary_key();
        a.owner = deposit.owner;
        a.mlnk_in = deposit.mlnk_in;
        a.usdt_in = deposit.usdt_in;
        a.token_out = deposit.token_out;
        a.creation_date = deposit.creation_date;
    });
    }

}

void token::set_inheritance_date(const name &owner, const uint32_t &inactive_period)
{
    require_auth(get_self());
    inheritance _inheritance(get_self(), get_self().value);
    auto it = _inheritance.find(owner.value);
    check(it != _inheritance.end(), "set_inheritance_date : account is not found");

    _inheritance.modify(it, same_payer, [&](auto &w) {
        w.inheritance_date = time_point_sec(w.inheritance_date.sec_since_epoch() - w.inactive_period + inactive_period);
        w.inactive_period = inactive_period;
    });
}

void token::add_swap_income(const extended_asset &income)
{
    require_auth(get_self());

    auto ext_sym = income.get_extended_symbol();
    check(income.quantity.amount > 0, "add_swap_income : asset amount must be positive");
    check(ext_sym.get_symbol().is_valid(), "add_swap_income : invalid symbol");
    check(ext_sym == USDT, "add_swap_income : income symbol is not USDT symbol");
    check(is_account(ext_sym.get_contract()), "add_swap_income : contract account is not exist");

    swap_table _swap_table(get_self(), get_self().value);
    auto it = _swap_table.find(ext_sym.get_symbol().code().raw());
    if (it == _swap_table.end())
    {
        _swap_table.emplace(get_self(), [&](auto &r) {
            r.income = income;
        });
    }
    else
    {
        _swap_table.modify(it, get_self(), [&](auto &r) {
            r.income = income;
        });
    }
}

void token::add_swap_cash(const asset &cash)
{
    require_auth(get_self());

    stats statstable(get_self(), cash.symbol.code().raw());
    auto existing = statstable.find(cash.symbol.code().raw());
    check(existing != statstable.end(), "add_rate_cash : token with symbol does not exist, create token before add");
    check(cash.symbol == existing->supply.symbol, "issue_token : symbol precision mismatch");
    check(cash.is_valid(), "add_rate_cash : invalid quantity");
    check(cash.amount > 0, "add_rate_cash : must add positive quantity");

    reverse_table _reverse_table(get_self(), get_self().value);
    auto it = _reverse_table.find(cash.symbol.code().raw());

    if (it == _reverse_table.end())
    {
        _reverse_table.emplace(get_self(), [&](auto &a) {
            a.cash = cash;
        });
    }
    else
    {
         _reverse_table.modify(it, get_self(), [&](auto &a) {
            a.cash = cash;
        });
    }
}

void token::create_token(const name &issuer, const asset &maximum_supply)
{
    require_auth(get_self());

    check(is_account(issuer), "create_token : issuer account not exist");

    auto sym = maximum_supply.symbol;
    check(sym.is_valid(), "create_token : invalid symbol name");
    check(maximum_supply.is_valid(), "create_token : invalid supply");
    check(maximum_supply.amount > 0, "create_token : max-supply must be positive");

    stats statstable(get_self(), sym.code().raw());
    auto existing = statstable.find(sym.code().raw());
    check(existing == statstable.end(), "create_token : token with symbol already exists");

    statstable.emplace(get_self(), [&](auto &s) {
        s.supply.symbol = maximum_supply.symbol;
        s.max_supply = maximum_supply;
        s.issuer = issuer;
    });
}

void token::issue_token(const name &to, const asset &quantity, const std::string &memo)
{
    auto sym = quantity.symbol;
    check(sym.is_valid(), "issue_token : invalid symbol name");

    stats statstable(get_self(), sym.code().raw());
    auto existing = statstable.find(sym.code().raw());
    check(existing != statstable.end(), "issue_token : token with symbol does not exist, create token before issue");
    const auto &st = *existing;

    require_auth(st.issuer);
    require_recipient(to);

    check(quantity.is_valid(), "issue_token : invalid quantity");
    check(quantity.amount > 0, "issue_token : must issue positive quantity");

    check(quantity.symbol == st.supply.symbol, "issue_token : symbol precision mismatch");
    check(quantity.amount <= st.max_supply.amount - st.supply.amount, "issue_token : quantity exceeds available supply");

    statstable.modify(st, same_payer, [&](auto &s) {
        s.supply += quantity;
    });

    add_balance(to, quantity, get_self());
}

void token::retire_token(const name &owner, const asset &quantity, const std::string &memo)
{
    require_auth(get_self());

    auto sym = quantity.symbol;
    check(sym.is_valid(), "retire_token : invalid symbol name");
    check(memo.size() <= 256, "retire_token : memo has more than 256 bytes");

    stats statstable(get_self(), sym.code().raw());
    auto existing = statstable.find(sym.code().raw());
    check(existing != statstable.end(), "retire_token : token with symbol does not exist");
    const auto &st = *existing;

    require_recipient(owner);

    check(quantity.is_valid(), "retire_token : invalid quantity");
    check(quantity.amount > 0, "retire_token : must retire positive quantity");
    check(quantity.symbol == st.supply.symbol, "retire_token : symbol precision mismatch");

    statstable.modify(st, same_payer, [&](auto &s) {
        s.supply -= quantity;
    });

    sub_balance(owner, quantity);
}

void token::transfer_token(const name &from, const name &to,
                           const asset &quantity, const std::string &memo)
{
    check(from != to, "transfer_token : cannot transfer to self");
    require_auth(from);
    check(is_account(to), "transfer_token : to account does not exist");
    auto sym = quantity.symbol.code();
    stats statstable(get_self(), sym.raw());
    const auto &st = statstable.get(sym.raw());

    require_recipient(from);
    require_recipient(to);

    check(quantity.is_valid(), "transfer_token : invalid quantity");
    check(quantity.amount > 0, "transfer_token : must transfer positive quantity");
    check(quantity.symbol == st.supply.symbol, "transfer_token : symbol precision mismatch");
    check(memo.size() <= 256, "transfer_token : memo has more than 256 bytes");

    auto payer = has_auth(to) ? to : from;
    
    sub_balance(from, quantity);
    add_balance(to, quantity, payer);

    extend_inheritance(from, payer);

    on_transfer_self_token(from, to, quantity, memo);
}

void token::open_account(const name &owner, const symbol &symbol, const name &ram_payer)
{
    require_auth(ram_payer);

    check(is_account(owner), "open_account : owner account does not exist");

    auto sym_code_raw = symbol.code().raw();
    stats statstable(get_self(), sym_code_raw);
    const auto &st = statstable.get(sym_code_raw, "open_account : symbol does not exist");
    check(st.supply.symbol == symbol, "open_account : symbol precision mismatch");

    accounts acnts(get_self(), owner.value);
    auto it = acnts.find(sym_code_raw);
    if (it == acnts.end())
    {
        acnts.emplace(ram_payer, [&](auto &a) {
            a.balance = asset{0, symbol};
        });

        create_inheritance(owner, ram_payer);
    }
}

void token::close_account(const name &owner, const symbol &symbol)
{
    require_auth(owner);
    accounts acnts(get_self(), owner.value);
    auto it = acnts.find(symbol.code().raw());
    check(it != acnts.end(), "close_account : Balance row already deleted or never existed. Action won't have any effect.");
    check(it->balance.amount == 0, "close_account : Cannot close because the balance is not zero.");
    acnts.erase(it);

    if (acnts.begin() == acnts.end())
    {
        close_inheritance(owner);
    }
}

void token::distribute_mlnk()
{
    require_auth(ROYALTY_ACCOUNT);
    
    asset collect_mlnk = asset{100000, USDCASH};
    sub_balance(ROYALTY_ACCOUNT, collect_mlnk);
    add_balance(get_self(), collect_mlnk, ROYALTY_ACCOUNT);

    send_transfer(get_self(), MLNK_ACCOUNT, collect_mlnk, "collect mlnk");
}

void token::get_royalties(const name &ram_payer)
{
    require_auth(ram_payer);

    royalties _royalties(get_self(), get_self().value);
    accounts _accounts(get_self(), get_self().value);

    std::vector<asset> balances;
    for (const auto &acc : _accounts)
        balances.push_back(acc.balance);

    for (const auto &i : _royalties)
    {
        for (const auto &balance : balances)
        {   
            if (balance.amount >= 1000)
            {
                auto amount = count_share(balance, i.get_royalty());
                send_transfer(get_self(), i.get_account(), amount, "royalty");
            }
        }
    }
}

void token::swap_back(const name &user, const asset &cash, const uint64_t &id)
{
    require_auth(user);
    
    deposits _deposits(get_self(), get_self().value);
    auto it = _deposits.find(id);
    
    check(it != _deposits.end(), "swap_back : deposit information is not found");
    check(it->owner == user, "swap_back : the user is not the owner of the deposit");
    check(cash.symbol == it->token_out.symbol, "swap_back : symbol precision mismatch");
    check(cash.amount >= usdcash_package_amount && cash.amount % usdcash_package_amount == 0, "swap_back : invalid cash amount");
    check(cash.amount <= it->token_out.amount, "swap_back : cash amount must be less then deposit amount");
    check(is_account_exist(user, extended_symbol{USDT}), "swap_back : USDT account is not exist");
    check(is_account_exist(user, extended_symbol{MLNK}), "swap_back : MLNK account is not exist");

    double rate = (double)it->token_out.amount / cash.amount;
    asset send_mlnk = asset(it->mlnk_in.amount / rate, it->mlnk_in.symbol);
    asset send_usdt = asset(it->usdt_in.amount / rate, it->usdt_in.symbol);

    if (it->token_out.amount == cash.amount)
    {
        _deposits.erase(it);
    }
    else
    {
        _deposits.modify(it, same_payer, [&](auto &a) {
            a.token_out -= cash;
            a.usdt_in -= send_usdt;
            a.mlnk_in -= send_mlnk;
        });
    }

    send_retire(user, cash, "");
    send_transfer(TETHER_ACCOUNT, user, send_usdt, "");
    send_transfer(SWAP_PCASH_ACCOUNT, user, send_mlnk, "");
}

void token::add_royalty_holder(const name &user_name, const asset &royalty)
{
    require_auth(get_self());
    check(is_account(user_name), "add_royalty_holder : user_name account not exist");
    check(is_valid_share(royalty), "add_royalty_holder : royalty not valid");
    check(is_valid_royalties_sum(royalty), "add_royalty_holder : royalties sum not valid");
    royalties _royalties(get_self(), get_self().value);
    auto it = _royalties.find(user_name.value);
    if (it == _royalties.end())
    {
        auto current_day = get_current_day();
        _royalties.emplace(get_self(), [&](auto &r) {
            royalty_holder temp(current_day, user_name, royalty);
            r = temp;
        });
    }
    else
    {
        _royalties.modify(it, same_payer, [&](auto &r) {
            r.set_royalty(royalty);
        });
    }
}

void token::rmv_royalty_holder(const name &user_name)
{
    require_auth(get_self());
    royalties _royalties(get_self(), get_self().value);
    auto it = _royalties.find(user_name.value);
    check(it != _royalties.end(), "rmv_royalty_holder : account not exist");
    _royalties.erase(it);
}

void token::distribute_inheritance(const name &initiator, const name &inheritance_owner, const symbol_code &token)
{
    require_auth(initiator);
    inheritance _inheritance(get_self(), get_self().value);
    auto cur_date = current_time_point().sec_since_epoch();
    auto it = _inheritance.find(inheritance_owner.value);
    check(it != _inheritance.end(), "distribute_inheritance : inheritance_owner is not exist");
    check(it->inheritance_date.sec_since_epoch() < cur_date, "distribute_inheritance : inheritance date is not expired");

    accounts from_acnts(get_self(), it->user_name.value);
    auto iter = from_acnts.find(token.raw());
    check(iter != from_acnts.end(), "distribute_inheritance : token is not exist");
    check(iter->balance.amount > 0, "distribute_inheritance : distribute amount should be positive");

    if (it->inheritors.size() == 1 && it->inheritors.back().inheritor == get_self())
    {
        add_inh_balance(it->user_name, get_self(), iter->balance, initiator);
    }
    else
    {
        add_inh_balances(it->user_name, iter->balance, it->inheritors, initiator);
    }
    sub_balance(it->user_name, iter->balance);
    send_notify("inheritance", it->user_name, name(), -iter->balance, "");
}

void token::update_inheritance_date(const name &owner, const uint32_t &inactive_period)
{
    require_auth(owner);
    inheritance _inheritance(get_self(), get_self().value);
    auto it = _inheritance.find(owner.value);
    check(it != _inheritance.end(), "update_inheritance_date : account is not found");
    check(is_valid_inactive_period(inactive_period), "update_inheritance_date : invalid inactive period");
    auto date = get_inheritance_exp_date(inactive_period);

    _inheritance.modify(it, owner, [&](auto &w) {
        w.inheritance_date = date;
        w.inactive_period = inactive_period;
    });
}

void token::update_inheritors(const name &owner, const std::vector<inheritor_record> &inheritors)
{
    require_auth(owner);
    inheritance _inheritance(get_self(), get_self().value);
    auto it = _inheritance.find(owner.value);
    check(it != _inheritance.end(), "update_inheritors : account is not found");
    check(is_not_self_in_inheritors(owner, inheritors), "update_inheritors : owner can not be in inheritors list");
    check(is_valid_inheritors_amount(inheritors.size()), "update_inheritors : invalid inheritors amount");
    check(is_inheritors_unique(inheritors), "update_inheritors : inheritors must be unique");
    check(is_valid_inheritors(inheritors), "update_inheritors : invalid inheritors shares or accounts");

    _inheritance.modify(it, owner, [&](auto &w) {
        w.inheritors = inheritors;
    });
}

void token::on_transfer(const name &from, const name &to, const asset &quantity, const std::string &memo)
{
    if (to == get_self())
    {
        if (from == MLNK_ACCOUNT && quantity.symbol == MLNK.get_symbol())
        {
            distribute_royalty(quantity);
        }
        else if (get_first_receiver() == SWAP_PCASH_ACCOUNT || get_first_receiver() == TETHER_ACCOUNT)
        {
            auto trx = get_income_trx();
            auto deposits = parse_deposit_actions(trx);
            check(is_valid_deposits(deposits), "invalid deposits");
            deposit current_deposit{from, extended_asset(quantity, get_first_receiver()), memo};

            if (is_last_deposit(current_deposit, deposits))
            {
                std::pair<extended_symbol, extended_symbol> pool_pair = std::make_pair(deposits[0].quantity.get_extended_symbol(), deposits[1].quantity.get_extended_symbol());

                checksum256 hash1 = to_pair_hash(pool_pair.first, pool_pair.second);
                checksum256 hash2 = to_pair_hash(pool_pair.second, pool_pair.first);

                auto [pool, status] = get_pool(hash1, hash2);
                check(status, "on_income : pool by hash is not exist");
                
                double pool_price = 0;
                if (pool.token1.get_extended_symbol() == USDT)
                    pool_price = (double)pool.token2.quantity.amount / (double)pool.token1.quantity.amount;
                else
                    pool_price = (double)pool.token1.quantity.amount / (double)pool.token2.quantity.amount;

                auto [usdt_deposit, mlnk_deposit, usdt_rest, mlnk_rest] = validation_income_amount(deposits, pool_price);

                if (usdt_rest.amount != 0)
                    send_transfer(TETHER_ACCOUNT, from, usdt_rest, "deposit refund");
                
                if (mlnk_rest.amount != 0)
                    send_transfer(SWAP_PCASH_ACCOUNT, from, mlnk_rest, "deposit refund");

                check(is_account_exist(from, extended_symbol{USDCASH, get_self()}), "on_transfer : account is not exist");
                send_issue(from, asset(usdt_deposit.amount * 10, USDCASH), "");

                create_deposit(from, usdt_deposit, mlnk_deposit, asset(usdt_deposit.amount * 10, USDCASH));
            }
        }
    }
}

void token::on_transfer_self_token(const name &from, const name &to, const asset &quantity, const std::string &memo)
{
    if (to == get_self())
    {
        reverse_table _reverse_table(get_self(), get_self().value);
        auto r_it = _reverse_table.find(quantity.symbol.code().raw());

        check(r_it != _reverse_table.end(), "is not cash token");
        check(quantity.amount >= r_it->cash.amount && quantity.amount % r_it->cash.amount == 0, "invalid quantity amount");

        if(quantity.symbol == USDCASH)
            check(quantity.amount >= r_it->cash.amount * exchange_multiplier, "invalid quantity amount");

        swap_table _swap_table(get_self(), get_self().value);
        const auto &swap_package = _swap_table.get(USDT.get_symbol().code().raw(), "no swap income object found");
        const auto &swap_pckg_amount = swap_package.income.quantity.amount;

        deposits _deposits(get_self(), get_self().value);
        auto index = _deposits.get_index<name("bymlnkdate")>();

        if (memo == "usdt")
        {
            check(is_account_exist(from, extended_symbol{USDT}), "on_transfer : account is not exist");

            asset sum = asset((double)quantity.amount / r_it->cash.amount * usdcash_package_amount, USDCASH);
            asset usdt_sum = asset(0, USDT.get_symbol());

            for (auto it = index.begin(); it != index.end();)
            {
                if (sum.amount < it->token_out.amount)
                {
                    asset result_usdt = asset(sum.amount / 10, it->usdt_in.symbol);
                    asset result_mlnk = asset(it->mlnk_in.amount / it->usdt_in.amount * result_usdt.amount, it->mlnk_in.symbol);
                    usdt_sum += result_usdt;

                    send_transfer(SWAP_PCASH_ACCOUNT, it->owner, result_mlnk, "return of the deposit");
                    index.modify(it, same_payer, [&](auto &a) {
                        a.token_out -= sum;
                        a.usdt_in -= result_usdt;
                        a.mlnk_in -= result_mlnk;
                    });
                    sum.amount = 0;
                    break;
                }
                else if (sum.amount > it->token_out.amount)
                {
                    send_transfer(SWAP_PCASH_ACCOUNT, it->owner, it->mlnk_in, "return of the deposit");
                    usdt_sum += it->usdt_in;
                    sum -= it->token_out;
                    index.erase(it);
                    it = index.begin();
                }
                else
                {
                    send_transfer(SWAP_PCASH_ACCOUNT, it->owner, it->mlnk_in, "return of the deposit");
                    usdt_sum += it->usdt_in;
                    sum -= it->token_out;
                    index.erase(it);
                    break;
                }
            }
            if (sum.amount != 0)
            {
                asset rest = asset(sum.amount / usdcash_package_amount * r_it->cash.amount, r_it->cash.symbol);

                send_transfer(get_self(), from, rest, "");
                send_transfer(TETHER_ACCOUNT, from, usdt_sum, "");
                send_retire(get_self(), quantity - rest, "");
            }
            else
            {
                send_transfer(TETHER_ACCOUNT, from, usdt_sum, "");
                send_retire(get_self(), quantity, "");
            }
        }
        else
            check(false, "invalid memo");
    }
}

void token::notify(const std::string &action_type, const name &to, const name &from, const asset &quantity, const std::string &memo)
{
    require_auth(get_self());
    require_recipient(to);
}

void token::sub_balance(const name &owner, const asset &value)
{
    accounts from_acnts(get_self(), owner.value);

    const auto &from = from_acnts.get(value.symbol.code().raw(), "no balance object found");
    check(from.balance.amount >= value.amount, "overdrawn balance");

    from_acnts.modify(from, same_payer, [&](auto &a) {
        a.balance -= value;
    });
}

void token::add_balance(const name &owner, const asset &value, const name &ram_payer)
{
    accounts to_acnts(get_self(), owner.value);
    auto to = to_acnts.find(value.symbol.code().raw());
    if (to == to_acnts.end())
    {
        to_acnts.emplace(ram_payer, [&](auto &a) {
            a.balance = value;
        });

        create_inheritance(owner, ram_payer);
    }
    else
    {
        to_acnts.modify(to, same_payer, [&](auto &a) {
            a.balance += value;
        });
    }
}

void token::distribute_royalty(const asset &quantity)
{
    accounts from_acnts(MLNK.get_contract(), get_self().value);
    royalties _royalties(get_self(), get_self().value);

    check(quantity.amount >= 1000, "distribute_royalty : invalid distribution token amount");

    asset distributed_sum = asset(0, quantity.symbol);

    for (const auto &i : _royalties)
    {
        auto value = count_share(quantity, i.get_royalty());
        send_transfer(MLNK.get_contract(), i.get_account(), value, "royalty");
        distributed_sum += value;
    }
    send_notify("royalty", get_self(), name(), -distributed_sum, "Total amount of distribution: " + quantity.to_string());
}

std::tuple<pool, bool> token::get_pool(const checksum256 &hash1, const checksum256 &hash2)
{
    pools _pools(SWAP_PCASH_ACCOUNT, SWAP_PCASH_ACCOUNT.value);
    auto index = _pools.get_index<name("bypair")>();

    auto it = index.find(hash1);
    if (it != index.end())
        return std::make_tuple(*it, true);
    else
    {
        it = index.find(hash2);
        if (it != index.end())
            return std::make_tuple(*it, true);
        else
            return std::make_tuple(pool(), false);
    }
}

bool token::is_valid_inactive_period(const uint32_t &inactive_period)
{
    return (inactive_period >= min_inh_period && inactive_period <= max_inh_period) ? true : false;
}

bool token::is_not_self_in_inheritors(const name &owner, const std::vector<inheritor_record> &inheritors)
{
    for (const auto &i : inheritors)
    {
        if (i.inheritor == owner)
        {
            return false;
        }
    }
    return true;
}

bool token::is_inheritors_unique(const std::vector<inheritor_record> &inheritors)
{
    std::map<name, asset> mymap;
    for (auto const &it : inheritors)
    {
        mymap.emplace(it.inheritor, it.share);
    }
    return (mymap.size() == inheritors.size() ? true : false);
}

bool token::is_valid_inheritors_amount(const size_t &size)
{
    return ((size >= 1 && size <= 3) ? true : false);
}

bool token::is_valid_share(const asset &share)
{
    return (share.symbol == inh_percent && share >= min_percent && share <= max_percent) ? true : false;
}

bool token::is_valid_share_sum(const asset &sum)
{
    return (sum == max_percent ? true : false);
}

bool token::is_valid_inheritors(const std::vector<inheritor_record> &inheritors)
{
    asset share_sum(0, inh_percent);
    for (auto &it : inheritors)
    {
        if (!is_account(it.inheritor) || !is_valid_share(it.share))
        {
            return false;
        }
        share_sum += it.share;
    }

    return (!is_valid_share_sum(share_sum) ? false : true);
}

asset token::count_share(const asset &quantity, const asset &share)
{
    double result = (double)quantity.amount * (double)share.amount;
    result /= (double)max_percent.amount;
    return asset(result, quantity.symbol);
}

bool token::is_valid_royalties_sum(const asset &share)
{
    royalties _royalties(get_self(), get_self().value);

    asset sum(0, inh_percent);
    for (const auto &it : _royalties)
    {
        sum += it.get_royalty();
    }
    sum += share;

    return (sum <= max_percent) ? true : false;
}

void token::create_inheritance(const name &owner, const name &ram_payer)
{
    inheritance _inheritance(get_self(), get_self().value);
    auto it = _inheritance.find(owner.value);
    if (it == _inheritance.end())
    {
        _inheritance.emplace(ram_payer, [&](auto &a) {
            a.user_name = owner;
            a.inheritance_date = time_point_sec(current_time_point().sec_since_epoch() + initial_period);
            a.inactive_period = initial_period;
            a.inheritors = std::vector<inheritor_record>{{get_self(), max_percent}};
        });
    }
}

void token::close_inheritance(const name &owner)
{
    inheritance _inheritance(get_self(), get_self().value);
    auto inh = _inheritance.find(owner.value);
    if (inh != _inheritance.end())
    {
        _inheritance.erase(inh);
    }
}

void token::extend_inheritance(const name &owner, const name &ram_payer)
{
    inheritance _inheritance(get_self(), get_self().value);
    auto it = _inheritance.find(owner.value);
    if (it != _inheritance.end())
    {
        time_point_sec new_inh_date(current_time_point().sec_since_epoch() + it->inactive_period);

        _inheritance.modify(it, ram_payer, [&](auto &r) {
            r.inheritance_date = new_inh_date;
        });
    }
}

void token::add_inh_balances(const name &owner, const asset &value, const std::vector<inheritor_record> &inheritors, const name &ram_payer)
{
    send_inheritance(owner, value, inheritors, 1, ram_payer);
}

void token::add_inh_balance(const name &from, const name &to, const asset &value, const name &ram_payer)
{
    add_balance(to, value, ram_payer);
    send_notify("inheritance", to, from, value, "");
}

void token::send_inheritance(const name &owner, const asset &quantity,
                            const std::vector<inheritor_record> &inheritors,
                            const int64_t &min_amount, const name &ram_payer)
{
    if (quantity.amount >= min_amount)
    {
        asset sum(0, quantity.symbol);
        for (auto it = inheritors.rbegin(); it != inheritors.rend(); ++it)
        {
            auto amount = count_share(quantity, it->share);
            sum += amount;
            if (it != --inheritors.rend())
            {
                add_inh_balance(owner, it->inheritor, amount, ram_payer);
            }
            else
            {
                asset rest = quantity - sum;
                add_inh_balance(owner, it->inheritor, amount + rest, ram_payer);
            }
        }
    }
    else
    {
        add_inh_balance(owner, inheritors.begin()->inheritor, quantity, ram_payer);
    }
}

time_point_sec token::get_inheritance_exp_date(const uint32_t &inactive_period)
{
    auto current_time = current_time_point().sec_since_epoch();
    time_point_sec temp(current_time + inactive_period);
    return temp;
}

bool token::is_valid_deposits(const std::vector<deposit> &deposits)
{
    return (deposits.size() == 2 && deposits[0].from == deposits[1].from &&
           ((deposits[0].quantity.get_extended_symbol() == MLNK && deposits[1].quantity.get_extended_symbol() == USDT) ||
           (deposits[0].quantity.get_extended_symbol() == USDT && deposits[1].quantity.get_extended_symbol() == MLNK))) ? true : false;
}

std::vector<deposit>
token::parse_deposit_actions(const transaction &trx)
{
    std::vector<deposit> result;

    for (auto act : trx.actions)
    {
        if (act.name == name("transfer"))
        {
            auto data = act.data_as<transfer_action>();
            if (data.to == get_self())
            {
                result.push_back({data.from, extended_asset(data.quantity, act.account), data.memo});
            }
        }
    }

    return result;
}

std::tuple<asset, asset, asset, asset> token::validation_income_amount(const std::vector<deposit> &deposits, const double &pool_price)
{
    asset income_usdt = asset();
    asset income_mlnk = asset();

    if (deposits[0].quantity.get_extended_symbol() == USDT)
    {
        income_usdt = deposits[0].quantity.quantity;
        income_mlnk = deposits[1].quantity.quantity;
    }
    else
    {
        income_usdt = deposits[1].quantity.quantity;
        income_mlnk = deposits[0].quantity.quantity;
    }

    swap_table _swap_table(get_self(), get_self().value);
    const auto &swap_package = _swap_table.get(income_usdt.symbol.code().raw(), "no swap income object found");
    const auto &swap_pckg_amount = swap_package.income.quantity.amount;
    
    int64_t mlnk_in_swap_pckg_amount = ceil(swap_pckg_amount * pool_price);

    if (income_usdt.amount >= swap_pckg_amount && income_mlnk.amount >= mlnk_in_swap_pckg_amount)
    {
        int64_t lots = std::min(income_usdt.amount / swap_pckg_amount, income_mlnk.amount / mlnk_in_swap_pckg_amount);
        asset usdt_deposit = asset(lots * swap_pckg_amount, income_usdt.symbol);
        asset mlnk_deposit = asset(lots * mlnk_in_swap_pckg_amount, income_mlnk.symbol);
        return std::make_tuple(usdt_deposit, mlnk_deposit, income_usdt - usdt_deposit, income_mlnk - mlnk_deposit);
    }
    else
        check(false, "invalid income amount");
}

transaction token::get_income_trx()
{
    auto size = transaction_size();
    char buff[size];
    auto readed_size = read_transaction(buff, size);
    check(readed_size == size, "get_income_trx : read transaction failed");
    transaction trx = unpack<transaction>(buff, size);
    return trx;
}

void token::create_deposit(const name &owner, const asset &usdt, const asset mlnk, const asset &cash)
{
    deposits _deposits(get_self(), get_self().value);
    _deposits.emplace(get_self(), [&](auto &a) {
        a.id = _deposits.available_primary_key();
        a.owner = owner;
        a.mlnk_in = mlnk;
        a.usdt_in = usdt;
        a.token_out = cash;
        a.creation_date = time_point_sec(current_time_point().sec_since_epoch());
    });
}

bool token::is_last_deposit(const deposit &current_deposit, const std::vector<deposit> &deposits)
{
    return current_deposit == deposits[1] ? true : false;
}

bool token::is_account_exist(const name &owner, const extended_symbol &token)
{
    accounts _accounts(token.get_contract(), owner.value);
    auto it = _accounts.find(token.get_symbol().code().raw());
    return it != _accounts.end() ? true : false;
}

time_point_sec token::get_current_day()
{
    time_point_sec date = current_time_point();
    auto delta = date.utc_seconds % royalty_n_income_period;
    return date - delta;
}

void token::send_transfer(const name &contract, const name &to, const asset &quantity, const std::string &memo)
{
    action(
        permission_level{get_self(), name("active")},
        contract,
        name("transfer"),
        std::make_tuple(get_self(), to, quantity, memo))
        .send();
}

void token::send_issue(const name &to, const asset &quantity, const std::string &memo)
{
    action(
        permission_level{get_self(), name("active")},
        get_self(),
        name("issue"),
        std::make_tuple(to, quantity, memo))
        .send();
}

void token::send_retire(const name &owner, const asset &quantity, const std::string &memo)
{
    action(
        permission_level{get_self(), name("active")},
        get_self(),
        name("retire"),
        std::make_tuple(owner, quantity, memo))
        .send();
}

void token::send_notify(const std::string &action_type, const name &to, const name &from, const asset &quantity, const std::string &memo)
{
    action(
        permission_level{get_self(), name("active")},
        get_self(),
        name("notify"),
        std::make_tuple(action_type, to, from, quantity, memo))
        .send();
}