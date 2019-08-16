#pragma once
#include "gxc_base_tester.hpp"
#include "contracts.hpp"
#include "util.hpp"
#include "../contracts/libsio4/lib/xxHash/xxhash.h"

using action_result = base_tester::action_result;
using option = pair<string,bytes>;

#define PUSH_ACTION_ARGS_ELEM(r, OP, elem) \
   (BOOST_PP_STRINGIZE(elem), elem)

#define PUSH_ACTION_ARGS(args) \
   BOOST_PP_SEQ_FOR_EACH(PUSH_ACTION_ARGS_ELEM, _, args)

#define PUSH_ACTION(actor, args) \
   chain.push_action(account, eosio::chain::name(__func__), actor, mvo() \
      PUSH_ACTION_ARGS(args) \
   )

/**
 * abstract contract
 */
struct contract {
   contract(gxc_base_tester& c): chain(c) {}

   virtual ~contract() {}
   virtual void setup() = 0;

   abi_serializer get_abi_serializer() try {
      const auto& accnt = chain.control->db().get<account_object,by_name>(account);
      abi_def abi;
      BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi), true);
      return {abi, base_tester::abi_serializer_max_time};
   } FC_CAPTURE_AND_RETHROW((account))

   gxc_base_tester& chain;
   eosio::chain::account_name account;
};

/**
 * token contract
 */
struct token_contract : public contract {
   token_contract(gxc_base_tester& c): contract(c) {
      account = N(gxc.token);
   };

   void setup() override {
      chain.produce_blocks(2);

      chain.create_accounts({ N(conr2d), N(eun2ce), N(ian), account });
      chain.produce_blocks(2);

      chain.set_code(account, contracts::token_wasm());
      chain.set_abi(account, contracts::token_abi().data());
      chain.produce_blocks(1);

      static_cast<base_tester&>(chain).push_action(config::system_account_name, N(setpriv), config::system_account_name, mvo()
         ("account", account)
         ("is_priv", true)
      );
      chain.produce_blocks(1);
   }

   fc::variant get_stats(const string& symbol_name) {
      auto symbol_code = SC(symbol_name);
      return chain.get_table_row(account, symbol_code.contract, N(stat), symbol_code.code);
   }

   fc::variant get_account(account_name acc, const string& symbol_name) {
      auto symbol_code = SC(symbol_name);
      return chain.get_table_row(account, acc, N(accounts), XXH64((const void*)&symbol_code, sizeof(extended_symbol_code), 0));
   }

   action_result mint(extended_asset value, bool simple = true, vector<option> opts = {}) {
      if (simple) {
         opts.emplace_back("recallable", bytes{0});
         opts.emplace_back("freezable", bytes{0});
      }
      return PUSH_ACTION(account, (value)(opts));
   }

   inline action_result transfer(account_name from, account_name to, extended_asset value, string memo) {
      return transfer(from, to, value, memo, (from != config::null_account_name) ? from : chain.basename(value.contract));
   }

   action_result transfer(account_name from, account_name to, extended_asset value, string memo, account_name actor) {
      return PUSH_ACTION(actor, (from)(to)(value)(memo));
   }

   inline action_result burn(account_name owner, extended_asset value, string memo) {
      return burn(owner, value, memo, owner);
   }

   action_result burn(account_name owner, extended_asset value, string memo, account_name actor) {
      return PUSH_ACTION(actor, (owner)(value)(memo));
   }

   action_result setopts(extended_symbol_code symbol, vector<option> opts) {
      return PUSH_ACTION(chain.basename(symbol.contract), (symbol)(opts));
   }

   action_result setacntsopts(vector<account_name> accounts, extended_symbol_code symbol, vector<option> opts) {
      return PUSH_ACTION(chain.basename(symbol.contract), (accounts)(symbol)(opts));
   }

   action_result open(account_name owner, extended_symbol_code symbol, account_name payer) {
      return PUSH_ACTION(payer, (owner)(symbol)(payer));
   }

   action_result close(account_name owner, extended_symbol_code symbol) {
      return PUSH_ACTION(owner, (owner)(symbol));
   }

   action_result deposit(account_name owner, extended_asset value) {
      return PUSH_ACTION(owner, (owner)(value));
   }

   action_result pushwithdraw(account_name owner, extended_asset value) {
      return PUSH_ACTION(owner, (owner)(value));
   }

   action_result popwithdraw(account_name owner, extended_symbol_code symbol) {
      return PUSH_ACTION(owner, (owner)(symbol));
   }

   action_result clrwithdraws(account_name owner) {
      return PUSH_ACTION(owner, (owner));
   }

   action_result approve(account_name owner, account_name spender, extended_asset value) {
      return PUSH_ACTION(owner, (owner)(spender)(value));
   }
};

/**
 * system contract
 */
struct system_contract : public contract {
   system_contract(gxc_base_tester& c): contract(c) {
      account = config::system_account_name;
   };

   void setup() override {
      BOOST_TEST_MESSAGE("gxc != " + account.to_string());
      chain.set_code(account, contracts::system_wasm());
      chain.set_abi(account, contracts::system_abi().data());
      chain.produce_blocks(1);
   }

   action_result init(unsigned_int version, symbol core) {
      return PUSH_ACTION(config::system_account_name, (version)(core));
   }

   action_result genaccount(account_name creator, account_name name, authority owner, authority active, std::string nickname) {
      return PUSH_ACTION(creator, (creator)(name)(owner)(active)(nickname));
   }

   action_result setprods(vector<producer_key> schedule) {
      return PUSH_ACTION(config::system_account_name, (schedule));
   }
};
