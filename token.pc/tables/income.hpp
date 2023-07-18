#pragma once
#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>

using namespace eosio;

struct [[eosio::contract("token.pc"),eosio::table]] swap
{
    extended_asset income;

    uint64_t primary_key() const { return income.quantity.symbol.code().raw(); }
};
using swap_table = multi_index<name("swap1"), swap>;