#pragma once

#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/singleton.hpp>
#include <cmath>

using namespace eosio;
using std::string;

namespace gxc {

class [[eosio::contract]] bancor : public contract {
public:
   using contract::contract;

   struct [[eosio::table]] connector {
      extended_symbol smart; // 16
      asset balance;         // 32
      double weight = .5;    // 40

      struct converted {
         extended_asset value;
         asset delta;
         double ratio;
      };

      converted convert_to_smart(const extended_asset& from, const extended_symbol& to, bool reverse = false);
      converted convert_from_smart(const extended_asset& from, const extended_symbol& to, bool reverse = false);

      converted convert_to_exact_smart(const extended_symbol& from, const extended_asset& to) {
         return convert_from_smart(to, from, true);
      }
      converted convert_exact_from_smart(const extended_symbol& from, const extended_asset& to) {
         return convert_to_smart(to, from, true);
      }

      uint64_t primary_key() const { return smart.get_symbol().code().raw(); }

      EOSLIB_SERIALIZE(connector, (smart)(balance)(weight))
   };

   struct base_config {
      uint16_t rate; // permyriad
      asset connected; // if amount != 0, it's considered as fixed amount of conversion fee

      bool is_exempted()const { return connected.amount == 0 && rate == 0; }

      extended_asset get_fee(extended_asset value)const {
         int64_t fee = 0;
         if (rate != 0) {
            int64_t p = 10000 / rate;
            fee = (value.quantity.amount + p - 1) / p;
         }
         fee += connected.amount;
         return {fee, value.get_extended_symbol()};
      }

      extended_asset get_required_fee(extended_asset value)const {
         int64_t fee = 0;
         if (rate != 0) {
            int64_t p = 10000 / rate;
            fee = int64_t((value.quantity.amount + connected.amount) * p / double(p - 1) + 0.9) - value.quantity.amount;
         }
         fee += connected.amount;
         return {fee, value.get_extended_symbol()};
      }

      EOSLIB_SERIALIZE(base_config, (rate)(connected))
   };

   struct [[eosio::table]] charge_policy : base_config {
      extended_symbol smart;

      uint64_t primary_key() const { return smart.get_symbol().code().raw(); }

      EOSLIB_SERIALIZE_DERIVED(charge_policy, base_config, (smart))
   };

   struct [[eosio::table]] config : base_config {
      name connected_contract;
      name admin;

      extended_asset get_connected() const { return { connected, connected_contract }; }
      extended_symbol get_connected_symbol() const { return { connected.symbol, connected_contract }; }

      EOSLIB_SERIALIZE_DERIVED(config, base_config, (connected_contract)(admin))
   };

   typedef multi_index<"connector"_n, connector> connectors;
   typedef multi_index<"charge"_n, charge_policy> charges;
   typedef singleton<"config"_n, config> configuration;

   [[eosio::action]]
   void convert(name sender, extended_asset from, extended_asset to);

   [[eosio::action]]
   void init(name admin, extended_symbol connected);

   [[eosio::action]]
   void connect(extended_symbol smart, extended_asset balance, double weight);

   [[eosio::action]]
   void setcharge(int16_t rate, std::optional<extended_asset> fixed, std::optional<extended_symbol> smart);

   [[eosio::action]]
   void setadmin(name admin);

private:
   extended_asset get_fee(const extended_asset& value, const extended_asset& smart, const config& c, bool required = false);
};

} /// namespace gxc
