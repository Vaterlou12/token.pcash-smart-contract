#pragma once
#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>

using namespace eosio;

struct [[eosio::contract("token.pc"), eosio::table]] account
{
    asset balance;

    uint64_t primary_key() const { return balance.symbol.code().raw(); }
};
using accounts = multi_index< name("accounts"), account>;