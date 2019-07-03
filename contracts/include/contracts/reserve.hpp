#pragma once

#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <misc/contract_wrapper.hpp>
#include <misc/option.hpp>

namespace gxc {

using namespace eosio;

class [[eosio::contract]] reserve : public contract_wrapper<reserve> {
public:
   static constexpr name default_account = "gxc.reserve"_n;

   using contract_wrapper::contract_wrapper;

   struct [[eosio::table]] reserves {
      extended_asset derivative;
      asset underlying;
      double rate;

      uint64_t primary_key()const { return derivative.quantity.symbol.code().raw(); }

      EOSLIB_SERIALIZE( reserves, (derivative)(underlying)(rate) )
   };

   typedef multi_index<"reserves"_n, reserves> reserves_index;

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
};

}
