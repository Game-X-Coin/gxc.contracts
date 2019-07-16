#pragma once

#include <eosio/name.hpp>

namespace eosio {

struct extended_name {
   name name;
   struct name contract;

   uint128_t raw() const { return (uint128_t)contract.value << 64 | name.value; }

   EOSLIB_SERIALIZE(extended_name, (name)(contract))
};

}
