#pragma once

#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/chain/abi_serializer.hpp>
#include <xxHash/xxhash.h>

#include "contracts.hpp"

using namespace eosio;
using namespace eosio::chain;
using namespace eosio::testing;
using namespace fc;
using namespace std;

using mvo = fc::mutable_variant_object;
using option = pair<string,bytes>;
using u128 = eosio::chain::uint128_t;

namespace eosio { namespace chain {

struct extended_symbol_code {
   symbol_code code;
   account_name contract;
};

inline extended_asset string_to_extended_asset(const string& s) {
   auto at_pos = s.find('@');
   return extended_asset(asset::from_string(s.substr(0, at_pos)), s.substr(at_pos+1));
}

inline extended_symbol_code string_to_extended_symbol_code(const string& s) {
   auto at_pos = s.find('@');
   return {symbol(0, s.substr(0, at_pos).data()).to_symbol_code(), s.substr(at_pos+1)};
}

#define EA(s) string_to_extended_asset(s)
#define SC(s) string_to_extended_symbol_code(s)

} }

FC_REFLECT(eosio::chain::extended_symbol_code, (code)(contract))

const static name token_account_name = N(gxc.token);

class gxc_token_tester: public tester {
public:

   vector<transaction_trace_ptr> create_accounts(
      vector<account_name> names,
      account_name creator = config::system_account_name,
      bool multisig = false,
      bool include_code = true
   ) {
      vector<transaction_trace_ptr> traces;
      traces.reserve(names.size());
      for (auto n: names) {
         traces.emplace_back(create_account(n, creator, multisig, include_code));
      }
      return traces;
   }

   account_name basename(account_name acc) {
      auto rootname = [&](account_name n) -> account_name {
         auto mask = (uint64_t) -1;
         for (auto i = 0; i < 12; ++i) {
            if (n.value & (0x1FULL << (4 + 5 * (11 - i))))
               continue;
            mask <<= 4 + 5 * (11 - i);
            break;
         }
         return name(n.value & mask);
      };

      const auto& accnt = control->db().find<account_object,by_name>(acc);
      if (accnt != nullptr) {
         return acc;
      } else {
         return rootname(acc);
      }
   }

   gxc_token_tester() {
      produce_blocks(2);

      create_accounts({ N(conr2d), N(eun2ce), N(ian), token_account_name });
      produce_blocks(2);

      set_code(token_account_name, contracts::token_wasm());
      set_abi(token_account_name, contracts::token_abi().data());
      produce_blocks(1);

      set_code(config::system_account_name, contracts::system_wasm());
      set_abi(config::system_account_name, contracts::system_abi().data());
      produce_blocks(1);

      base_tester::push_action(config::system_account_name, N(setpriv), config::system_account_name, mvo()
         ("account", token_account_name)
         ("is_priv", true)
      );
      produce_blocks(1);

      const auto& accnt = control->db().get<account_object,by_name>(token_account_name);
      abi_def abi;
      BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi), true);
      abi_ser[token_account_name].set_abi(abi, abi_serializer_max_time);
   }

   fc::variant get_table_row(const account_name& code, const account_name& scope, const account_name& table, uint64_t primary_key) {
      auto data = get_row_by_account(code, scope, table, primary_key);
      return data.empty() ? fc::variant() : abi_ser[code].binary_to_variant(table.to_string(), data, abi_serializer_max_time);
   }

   fc::variant get_stats(const string& symbol_name) {
      auto symbol_code = SC(symbol_name);
      return get_table_row(token_account_name, symbol_code.contract, N(stat), symbol_code.code);
   }

   fc::variant get_account(account_name acc, const string& symbol_name) {
      auto symbol_code = SC(symbol_name);
      return get_table_row(token_account_name, acc, N(accounts), XXH64((const void*)&symbol_code, sizeof(extended_symbol_code), 0));
   }

   action_result push_action(const account_name& code, const account_name& acttype, const account_name& actor, const variant_object& data) {
      string action_type_name = abi_ser[code].get_action_type(acttype);

      action act;
      act.account = code;
      act.name    = acttype;
      act.data    = abi_ser[code].variant_to_binary(action_type_name, data, abi_serializer_max_time);

      return base_tester::push_action(std::move(act), uint64_t(actor));
   }

   void _set_code(account_name account, const vector<uint8_t> wasm) try {
      base_tester::push_action(config::system_account_name, N(setcode),
         vector<permission_level>{{account, config::active_name}, {config::system_account_name, config::active_name}},
         mvo()
            ("account", account)
            ("vmtype", 0)
            ("vmversion", 0)
            ("code", bytes(wasm.begin(), wasm.end()))
      );
   } FC_CAPTURE_AND_RETHROW((account))

   void _set_abi(account_name account, const char* abi_json) {
      auto abi = fc::json::from_string(abi_json).template as<abi_def>();
      base_tester::push_action(config::system_account_name, N(setabi),
         vector<permission_level>{{account, config::active_name}, {config::system_account_name, config::active_name}},
         mvo()
            ("account", account)
            ("abi", fc::raw::pack(abi))
      );
   }

#define PUSH_ACTION_ARGS_ELEM(r, OP, elem) \
   (BOOST_PP_STRINGIZE(elem), elem)

#define PUSH_ACTION_ARGS(args) \
   BOOST_PP_SEQ_FOR_EACH(PUSH_ACTION_ARGS_ELEM, _, args)

#define PUSH_ACTION(contract, actor, args) \
   push_action(contract, name(__func__), actor, mvo() \
      PUSH_ACTION_ARGS(args) \
   )

   action_result mint(extended_asset value, bool simple = true, vector<option> opts = {}) {
      if (simple) {
         opts.emplace_back("recallable", bytes{0});
         opts.emplace_back("freezable", bytes{0});
      }
      return PUSH_ACTION(token_account_name, token_account_name, (value)(opts));
   }

   inline action_result transfer(account_name from, account_name to, extended_asset value, string memo) {
      return transfer(from, to, value, memo, (from != config::null_account_name) ? from : basename(value.contract));
   }

   action_result transfer(account_name from, account_name to, extended_asset value, string memo, account_name actor) {
      return PUSH_ACTION(token_account_name, actor, (from)(to)(value)(memo));
   }

   inline action_result burn(account_name owner, extended_asset value, string memo) {
      return burn(owner, value, memo, owner);
   }

   action_result burn(account_name owner, extended_asset value, string memo, account_name actor) {
      return PUSH_ACTION(token_account_name, actor, (owner)(value)(memo));
   }

   action_result setopts(extended_symbol_code symbol, vector<option> opts) {
      return PUSH_ACTION(token_account_name, basename(symbol.contract), (symbol)(opts));
   }

   action_result setacntsopts(vector<account_name> accounts, extended_symbol_code symbol, vector<option> opts) {
      return PUSH_ACTION(token_account_name, basename(symbol.contract), (accounts)(symbol)(opts));
   }

   action_result open(account_name owner, extended_symbol_code symbol, account_name payer) {
      return PUSH_ACTION(token_account_name, payer, (owner)(symbol)(payer));
   }

   action_result close(account_name owner, extended_symbol_code symbol) {
      return PUSH_ACTION(token_account_name, owner, (owner)(symbol));
   }

   action_result deposit(account_name owner, extended_asset value) {
      return PUSH_ACTION(token_account_name, owner, (owner)(value));
   }

   action_result pushwithdraw(account_name owner, extended_asset value) {
      return PUSH_ACTION(token_account_name, owner, (owner)(value));
   }

   action_result popwithdraw(account_name owner, extended_symbol_code symbol) {
      return PUSH_ACTION(token_account_name, owner, (owner)(symbol));
   }

   action_result clrwithdraws(account_name owner) {
      return PUSH_ACTION(token_account_name, owner, (owner));
   }

   action_result approve(account_name owner, account_name spender, extended_asset value) {
      return PUSH_ACTION(token_account_name, owner, (owner)(spender)(value));
   }

   map<account_name, abi_serializer> abi_ser;
};
