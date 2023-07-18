#pragma once
#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>

using namespace eosio;

struct [[eosio::contract("token.pc"), eosio::table]] place
{
    uint64_t id;
    name owner;
    asset mlnk_in;
    asset usdt_in;
    asset token_out;
    time_point_sec creation_date;

    uint64_t primary_key() const { return id; }
    uint64_t owner_key() const { return owner.value; }
    uint128_t mlnk_in_and_date_key() const { return ((uint128_t)mlnk_in.amount << 64)|(uint128_t)creation_date.sec_since_epoch(); }
};
using by_mlnk_in_and_date = indexed_by<name("bymlnkdate"), const_mem_fun<place, uint128_t, &place::mlnk_in_and_date_key>>;
using by_owner = indexed_by<name("byowner"), const_mem_fun<place, uint64_t, &place::owner_key>>;
using deposits = multi_index<name("deposits"), place, by_mlnk_in_and_date, by_owner >;
