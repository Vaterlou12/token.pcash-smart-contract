#pragma once
#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>

using namespace eosio;

struct [[ eosio::contract("token.pc"), eosio::table]] royalty_holder
{
private:
    time_point_sec date;
    name account;
    asset royalty;

public:
    royalty_holder();
    royalty_holder(const time_point_sec &_date, const name &_account, const asset &_royalty);

    uint64_t primary_key() const;
    time_point_sec get_date() const;
    name get_account() const;
    asset get_royalty() const;

    void set_date(const time_point_sec &_date);
    void set_account(const name &_account);
    void set_royalty(const asset &_royalty);

    EOSLIB_SERIALIZE(royalty_holder, (date)(account)(royalty))
};
using royalties = multi_index<name("royalties"), royalty_holder>;