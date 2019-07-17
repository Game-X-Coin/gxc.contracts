#pragma once

#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/crypto.hpp>
#include <misc/hash.hpp>

namespace gxc {

using namespace eosio;
using std::string;

class [[eosio::contract]] htlc : public contract {
public:
   using contract::contract;

   struct [[eosio::table("htlc")]] lock_contract {
      string contract_name;
      std::variant<name,checksum160> recipient;
      extended_asset value;
      checksum256 hashlock;
      time_point_sec timelock;

      uint64_t primary_key() const { return std::hash<std::string>()(contract_name); }

      EOSLIB_SERIALIZE(lock_contract, (contract_name)(recipient)(value)(hashlock)(timelock))
   };
   typedef multi_index<"htlc"_n, lock_contract> htlc_index;

   struct [[eosio::table]] config {
      extended_asset min_amount;
      uint32_t min_duration;

      uint64_t primary_key() const { return min_amount.quantity.symbol.code().raw(); }

      EOSLIB_SERIALIZE(config, (min_amount)(min_duration))
   };
   typedef multi_index<"config"_n, config> config_index;

   [[eosio::action]]
   void newcontract(name owner, string contract_name, std::variant<name, checksum160> recipient, extended_asset value, checksum256 hashlock, time_point_sec timelock);

   [[eosio::action]]
   void withdraw(name owner, string contract_name, checksum256 preimage);

   [[eosio::action]]
   void refund(name owner, string contract_name);

   [[eosio::action]]
   void setconfig(extended_asset min_amount, uint32_t min_duration);
};

}
