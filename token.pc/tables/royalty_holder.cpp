#include "royalty_holder.hpp"

royalty_holder::royalty_holder()
{
}

royalty_holder::royalty_holder(const time_point_sec &_date, const name &_account, const asset &_royalty)
    : date(_date), account(_account), royalty(_royalty)
{
}

uint64_t royalty_holder::primary_key() const
{
    return account.value;
}

time_point_sec royalty_holder::get_date() const
{
    return date;
}

name royalty_holder::get_account() const
{
    return account;
}

asset royalty_holder::get_royalty() const
{
    return royalty;
}

void royalty_holder::set_date(const time_point_sec &_date)
{
    date = _date;
}

void royalty_holder::set_account(const name &_account)
{
    account = _account;
}

void royalty_holder::set_royalty(const asset &_royalty)
{
    royalty = _royalty;
}
