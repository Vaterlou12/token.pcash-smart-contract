#pragma once
#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/crypto.hpp>

using namespace eosio;

#ifdef DEBUG
    constexpr name TETHER_ACCOUNT("tethertether");
    constexpr name SWAP_PCASH_ACCOUNT("swap.pcash");
    constexpr name MLNK_ACCOUNT("pcash.mlnk");
    constexpr name ROYALTY_ACCOUNT("royalty.pcash");
    constexpr extended_symbol MLNK(symbol("MLNK", 8), SWAP_PCASH_ACCOUNT);
    constexpr extended_symbol USDT(symbol("USDT", 4), TETHER_ACCOUNT);

    const uint32_t min_inh_period = 2;
    const uint32_t initial_period = 2;
    const uint32_t max_inh_period = 5;
    const uint32_t royalty_n_income_period = 60;
#else
    #ifdef PREPROD
        constexpr name TETHER_ACCOUNT("aabw2r.pcash");
        constexpr name SWAP_PCASH_ACCOUNT("testnewswap1");
        constexpr name MLNK_ACCOUNT("mlnkcollectr");
        constexpr name ROYALTY_ACCOUNT("aaeqyy.pcash");
        constexpr extended_symbol MLNK(symbol("MLNK", 8), SWAP_PCASH_ACCOUNT);
        constexpr extended_symbol USDT(symbol("USDT", 4), TETHER_ACCOUNT);

        const uint32_t min_inh_period = 86400; //1 day in secs
        const uint32_t initial_period = 31536000; //1 year in secs
        const uint32_t max_inh_period = 315360000; //10 years in secs
        const uint32_t royalty_n_income_period = 86400; //1 day
    #else
        constexpr name TETHER_ACCOUNT("tethertether");
        constexpr name SWAP_PCASH_ACCOUNT("swap.pcash");
        constexpr name MLNK_ACCOUNT("pcash.mlnk");
        constexpr name ROYALTY_ACCOUNT("rm.pcash");
        constexpr extended_symbol MLNK(symbol("MLNK", 8), SWAP_PCASH_ACCOUNT);
        constexpr extended_symbol USDT(symbol("USDT", 4), TETHER_ACCOUNT);

        const uint32_t min_inh_period = 86400; //1 day in secs
        const uint32_t initial_period = 31536000; //1 year in secs
        const uint32_t max_inh_period = 315360000; //10 years in secs
        const uint32_t royalty_n_income_period = 86400; //1 day
    #endif
#endif

constexpr symbol USDCASH("USDCASH", 5);

constexpr symbol inh_percent("PERCENT", 1);
const asset min_percent(1, inh_percent);
const asset max_percent(1000, inh_percent);

const uint32_t usdcash_package_amount = 10000000;
const uint32_t exchange_multiplier = 100;

struct transfer_action
{
    name from;
    name to;
    asset quantity;
    std::string memo;

    EOSLIB_SERIALIZE(transfer_action, (from)(to)(quantity)(memo))
};

struct deposit
{
    name from;
    extended_asset quantity;
    std::string memo;

    EOSLIB_SERIALIZE(deposit, (from)(quantity)(memo))
};

bool operator==(const deposit &lhs, const deposit &rhs)
{
    return (lhs.from == rhs.from && lhs.quantity.quantity.symbol == rhs.quantity.quantity.symbol && lhs.quantity.quantity.amount == rhs.quantity.quantity.amount && lhs.memo == rhs.memo) ? true : false;
}

std::string to_string(const extended_symbol &token)
{
    std::string str = token.get_symbol().code().to_string() + "@" + token.get_contract().to_string();
    return str;
}

checksum256 to_hash(const extended_symbol &token)
{
    std::string str = to_string(token);
    return sha256(str.data(), str.size());
}

checksum256 to_pair_hash(const extended_symbol &token1, const extended_symbol &token2)
{
    std::string str = to_string(token1) + "/" + to_string(token2);
    return sha256(str.data(), str.size());
}