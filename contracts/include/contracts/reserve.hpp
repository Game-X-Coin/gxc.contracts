#pragma once

#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/singleton.hpp>
#include <misc/contract_wrapper.hpp>
#include <misc/option.hpp>

namespace gxc {

using namespace eosio;

class [[eosio::contract]] reserve : public contract_wrapper<reserve> {
public:
   static constexpr name default_account = "gxc.reserve"_n;

   using contract_wrapper::contract_wrapper;

   struct [[eosio::table]] config {
      extended_asset amount;

      EOSLIB_SERIALIZE(config, (amount))
   };

   struct [[eosio::table]] prepaid {
      name creator;
      name name;
      extended_asset value;

      uint64_t primary_key() const { return name.value; }

      EOSLIB_SERIALIZE(prepaid, (creator)(name)(value))
   };

   struct [[eosio::table]] reserves {
      extended_asset derivative;
      asset underlying;
      double rate;

      uint64_t primary_key()const { return derivative.quantity.symbol.code().raw(); }

      EOSLIB_SERIALIZE( reserves, (derivative)(underlying)(rate) )
   };

   typedef singleton<"config"_n, config> configuration;
   typedef multi_index<"prepaid"_n, prepaid> prepaid_index;
   typedef multi_index<"reserves"_n, reserves> reserves_index;

   void require_prepaid(name creator, name name) {
      prepaid_index ppd(_self, _self.value);
      auto it = ppd.find(name.value);
      check(it != ppd.end(), "prepaid not found");
      check(it->creator == creator, "creator mismatch");
   }

   bool has_reserve(const extended_symbol& symbol) {
      reserves_index rsv(_self, symbol.get_contract().value);
      auto it = rsv.find(symbol.get_symbol().code().raw());
      return it != rsv.end();
   }

   double get_rate(const extended_symbol& symbol) {
      reserves_index rsv(_self, symbol.get_contract().value);
      auto it = rsv.find(symbol.get_symbol().code().raw());
      if (it != rsv.end())
         return it->rate;
      return 0.;
   }

   [[eosio::action]]
   void mint(extended_asset derivative, extended_asset underlying, std::vector<option> opts);

   [[eosio::action]]
   void claim(name owner, extended_asset value);

   [[eosio::action]]
   void setminamount(extended_asset value);

   [[eosio::action]]
   void prepay(name creator, name name, extended_asset value);
};

}
