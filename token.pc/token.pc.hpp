#pragma once
#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/system.hpp>
#include <eosio/transaction.hpp>
#include <math.h>
#include <algorithm>
#include <string>
#include "account.hpp"
#include "stat.hpp"
#include "royalty_holder.hpp"
#include "inheritance.hpp"
#include "pool.hpp"
#include "reverse.hpp"
#include "income.hpp"
#include "deposit.hpp"


using namespace eosio;

class [[eosio::contract("token.pc")]] token : public contract
{
public:
    token(name receiver, name code, datastream<const char *> ds);

    [[eosio::action("migration")]] void migration_data(const uint64_t &id);

    [[eosio::action("setinhdate")]] void set_inheritance_date(const name &owner, const uint32_t &date);
    
    //For managing swap income
    [[eosio::action("addswapinc")]] void add_swap_income(const extended_asset &income);

    [[eosio::action("addswapcash")]] void add_swap_cash(const asset &cash);

    //For managing tokens
    [[eosio::action("create")]] void create_token(const name &issuer, const asset &maximum_supply);

    [[eosio::action("issue")]] void issue_token(const name &to, const asset &quantity, const std::string &memo);

    [[eosio::action("retire")]] void retire_token(const name &owner, const asset &quantity, const std::string &memo);

    [[eosio::action("transfer")]] void transfer_token(const name &from, const name &to, const asset &quantity, const std::string &memo);

    [[eosio::action("open")]] void open_account(const name &owner, const symbol &symbol, const name &ram_payer);

    [[eosio::action("close")]] void close_account(const name &owner, const symbol &symbol);

    [[eosio::action("distrmlnk")]] void distribute_mlnk();

    [[eosio::action("getroyalties")]] void get_royalties(const name &ram_payer);

    [[eosio::action("swapback")]] void swap_back(const name &user, const asset &cash, const uint64_t &id);

    //For royalties managing
    [[eosio::action("addrlthldr")]] void add_royalty_holder(const name &user_name, const asset &royalty);

    [[eosio::action("rmvrlthldr")]] void rmv_royalty_holder(const name &user_name);

    //For init inheritance distribution
    [[eosio::action("dstrinh")]] void distribute_inheritance(const name &initiator, const name &inheritance_owner, const symbol_code &token);

    //For inheritor programm
    [[eosio::action("updinhdate")]] void update_inheritance_date(const name &owner, const uint32_t &inactive_period);

    [[eosio::action("updtokeninhs")]] void update_inheritors(const name &owner, const std::vector<inheritor_record> &inheritors);

    //For incoming payments
    [[eosio::on_notify("*::transfer")]] void on_transfer(const name &from, const name &to, const asset &quantity, const std::string &memo);

    //For notifing
    [[eosio::action("notify")]] void notify(const std::string &action_type, const name &to, const name &from,
                                            const asset &quantity, const std::string &memo);

private:
    void on_transfer_self_token(const name &from, const name &to, const asset &quantity, const std::string &memo);

    void sub_balance(const name &owner, const asset &value);
    void add_balance(const name &owner, const asset &value, const name &ram_payer);

    void distribute_royalty(const asset &quantity);

    std::tuple<pool, bool> get_pool(const checksum256 &hash1, const checksum256 &hash2);

    asset count_share(const asset &quantity, const asset &share);
    bool is_valid_share(const asset &share);
    bool is_valid_share_sum(const asset &sum);
    bool is_valid_inheritors(const std::vector<inheritor_record> &inheritors);
    bool is_valid_royalties_sum(const asset &share);

    bool is_valid_inactive_period(const uint32_t &inactive_period);
    bool is_not_self_in_inheritors(const name &owner, const std::vector<inheritor_record> &inheritors);
    bool is_inheritors_unique(const std::vector<inheritor_record> &inheritors);
    bool is_valid_inheritors_amount(const size_t &size);

    void create_inheritance(const name &owner, const name &ram_payer);
    void close_inheritance(const name &owner);
    void extend_inheritance(const name &owner, const name &ram_payer);

    void add_inh_balances(const name &owner, const asset &value, const std::vector<inheritor_record> &inheritors, const name &ram_payer);
    void add_inh_balance(const name &from, const name &to, const asset &value, const name &ram_payer);

    void send_inheritance(const name &owner, const asset &quantity, const std::vector<inheritor_record> &inheritors,
                          const int64_t &min_amount, const name &ram_payer);

    time_point_sec get_inheritance_exp_date(const uint32_t &inactive_period);

    bool is_valid_deposits(const std::vector<deposit> &deposits);
    std::vector<deposit> parse_deposit_actions(const transaction &trx);
    std::tuple<asset, asset, asset, asset> validation_income_amount(const std::vector<deposit> &deposits, const double &pool_price);
    transaction get_income_trx();

    void create_deposit(const name &owner, const asset &usdt, const asset mlnk, const asset &cash);
    bool is_last_deposit(const deposit &current_deposit, const std::vector<deposit> &deposits);
    bool is_account_exist(const name &owner, const extended_symbol &token);

    time_point_sec get_current_day();

    void send_transfer(const name &contract, const name &to, const asset &quantity, const std::string &memo);
    void send_issue(const name &to, const asset &quantity, const std::string &memo);
    void send_retire(const name &owner, const asset &quantity, const std::string &memo);
    void send_notify(const std::string &action_type, const name &to, const name &from, const asset &quantity, const std::string &memo);
};
