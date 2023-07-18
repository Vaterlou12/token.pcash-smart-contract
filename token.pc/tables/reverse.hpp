#pragma once
#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>

using namespace eosio;

struct [[eosio::contract("token.pc"),eosio::table]] reverse
{
    asset cash;

    uint64_t primary_key() const { return cash.symbol.code().raw(); }
};
using reverse_table = multi_index<name("swapback"), reverse>;