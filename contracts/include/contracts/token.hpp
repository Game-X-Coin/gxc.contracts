#pragma once

#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <sio4/symbol.hpp>
#include <sio4/binary_extension.hpp>
#include <misc/hash.hpp>
#include <misc/option.hpp>
#include <misc/contract_wrapper.hpp>

namespace gxc {

using namespace eosio;
using sio4::extended_symbol_code;

class [[eosio::contract]] token : public contract_wrapper<token> {
public:
   static constexpr name default_account = "gxc.token"_n;

   using contract_wrapper::contract_wrapper;

   // TABLE
   struct [[eosio::table]] accounts {
      asset balance;
      name issuer_;
      sio4::binary_extension<asset> deposit;

      enum opt {
         frozen = 0,
         whitelist
      };

      name issuer() const { return name(issuer_.value & ~0xFULL); }
      void issuer(name issuer) { issuer_ = name(issuer.value | (issuer_.value & 0xFULL)); }

      bool option(opt n) const { return !!(issuer_.value & (0x1 << n)); }
      void option(opt n, bool val) {
         if (val) issuer_.value |= 0x1 << n;
         else     issuer_.value &= ~(0x1 << n);
      }

      uint64_t primary_key() const { return std::hash<extended_symbol_code>()(extended_symbol_code{balance.symbol.code(), issuer()}); }
      uint64_t by_issuer() const { return issuer().value; }

      EOSLIB_SERIALIZE(accounts, (balance)(issuer_)(deposit))
   };
   typedef multi_index<"accounts"_n, accounts,
      indexed_by<"issuer"_n, const_mem_fun<accounts, uint64_t, &accounts::by_issuer>>
   > accounts_index;

   struct [[eosio::table]] stat {
      asset supply;
      asset max_supply;
      name  issuer;
      uint32_t opts = 0x7; // defatuls to mintable, recallable, freezable

      sio4::binary_extension<asset> amount;
      sio4::binary_extension<uint32_t> duration;

      enum opt {
         mintable = 0,
         recallable,
         freezable,
         pausable,
         paused,
         whitelistable,
         whitelist_on,
         floatable
      };

      bool option(opt n) const { return (opts >> (0 + n)) & 0x1; }
      void option(opt n, bool val) {
         if (val) opts |= 0x1 << n;
         else     opts &= ~(0x1 << n);
      }

      uint64_t primary_key() const { return supply.symbol.code().raw(); }

      EOSLIB_SERIALIZE(stat, (supply)(max_supply)(issuer)(opts)(amount)(duration))
   };
   typedef multi_index<"stat"_n, stat> stat_index;

   struct [[eosio::table]] withdraws {
      asset quantity;
      name issuer;
      time_point_sec scheduled_time;

      inline extended_asset value() const { return {quantity, issuer}; }

      uint64_t primary_key() const { return std::hash<extended_symbol_code>()(extended_symbol_code{quantity.symbol.code(), issuer}); }
      uint64_t by_schedule() const { return static_cast<uint64_t>(scheduled_time.utc_seconds); }

      EOSLIB_SERIALIZE(withdraws, (quantity)(issuer)(scheduled_time))
   };
   typedef multi_index<"withdraws"_n, withdraws,
      indexed_by<"schedule"_n, const_mem_fun<withdraws, uint64_t, &withdraws::by_schedule>>
   > withdraws_index;

   struct [[eosio::table]] allowance {
      name spender;
      asset quantity;
      name issuer;
      std::optional<uint32_t> count;

      inline extended_asset value() const { return {quantity, issuer}; }

      uint64_t primary_key() const {
         std::array<char,24> raw;
         datastream<char*> ds(raw.data(), raw.size());
         ds << spender;
         ds << extended_symbol_code(quantity.symbol.code(), issuer).raw();
         return std::hash<std::array<char,24>>()(raw);
      }

      EOSLIB_SERIALIZE(allowance, (spender)(quantity)(issuer)(count))
   };
   typedef multi_index<"allowance"_n, allowance> allowance_index;

   // METHODS
   extended_asset get_supply(const extended_symbol_code& symbol) {
      stat_index si(_self, symbol.contract.value);
      const auto& it = si.get(symbol.code.raw(), "token not found");
      return {it.supply, it.issuer};
   }

   // ACTION
   [[eosio::action]]
   void mint(extended_asset value, std::vector<option> opts);

   [[eosio::action]]
   void transfer(name from, name to, extended_asset value, std::string memo);

   [[eosio::action]]
   void burn(name owner, extended_asset value, std::string memo);

   [[eosio::action]]
   void setopts(extended_symbol_code symbol, std::vector<option> opts);

   [[eosio::action]]
   void setacntsopts(std::vector<name> accounts, extended_symbol_code symbol, std::vector<option> opts);

   [[eosio::action]]
   void open(name owner, extended_symbol_code symbol, name payer);

   [[eosio::action]]
   void close(name owner, extended_symbol_code symbol);

   [[eosio::action]]
   void deposit(name owner, extended_asset value);

   [[eosio::action]]
   void pushwithdraw(name owner, extended_asset value);

   [[eosio::action]]
   void popwithdraw(name owner, extended_symbol_code symbol);

   [[eosio::action]]
   void clrwithdraws(name owner);
   typedef action_wrapper<"clrwithdraws"_n, &token::clrwithdraws> clear_withdraws;

   [[eosio::action]]
   void approve(name owner, name spender, extended_asset value);

   // dummy actions
   // Remove authorization check after 1.8 upgrade
   // There will be an intrinsic which returns the account where this action is sent
   [[eosio::action]]
   void withdraw(name owner, extended_asset value) { require_auth(_self); }
   typedef action_wrapper<"withdraw"_n, &token::withdraw> withdraw_processed;

   [[eosio::action]]
   void revtwithdraw(name owner, extended_asset value) { require_auth(_self); }
   typedef action_wrapper<"revtwithdraw"_n, &token::revtwithdraw> withdraw_reverted;
};

}
