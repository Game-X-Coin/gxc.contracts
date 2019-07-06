#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/chain/abi_serializer.hpp>
#include <contracts.hpp>
//#include "eosio.system_tester.hpp"

//#include "Runtime/Runtime.h"

#include <fc/variant_object.hpp>
#include <xxHash/xxhash.h>

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

} }

FC_REFLECT(eosio::chain::extended_symbol_code, (code)(contract))

class gxc_token_tester : public tester {
public:

   static constexpr uint64_t system_account_name = N(eosio);
   static constexpr uint64_t default_account_name = N(gxc.token);
   static constexpr uint64_t null_account_name = N(gxc.null);

   vector<transaction_trace_ptr> create_accounts(
      vector<account_name> names,
      account_name creator = system_account_name,
      bool multisig = false,
      bool include_code = true
   ) {
      vector<transaction_trace_ptr> traces;
      traces.reserve(names.size());
      for (auto n: names) {
         traces.emplace_back(create_account(n, system_account_name, multisig, include_code));
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

      const auto& accnt = control->db().find<account_object,by_name>(default_account_name);
      if (accnt != nullptr) {
         return acc;
      } else {
         return rootname(acc);
      }
   }

   gxc_token_tester() {
      produce_blocks(2);
      create_accounts({ N(conr2d), N(eun2ce), N(ian), default_account_name });
      if (basename(config::system_account_name) != N(gxc)) {
         create_accounts({ null_account_name });
      }
      produce_blocks(2);

      set_code(default_account_name, contracts::token_wasm());
      set_abi(default_account_name, contracts::token_abi().data());
      produce_blocks();

      const auto& accnt = control->db().get<account_object,by_name>(default_account_name);
      abi_def abi;
      BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi), true);
      abi_ser.set_abi(abi, abi_serializer_max_time);
   }

   action_result push_action(const account_name& signer, const action_name& name, const variant_object& data) {
      string action_type_name = abi_ser.get_action_type(name);

      action act;
      act.account = default_account_name;
      act.name = name;
      act.data = abi_ser.variant_to_binary(action_type_name, data, abi_serializer_max_time);

      return base_tester::push_action(std::move(act), uint64_t(signer));
   }

   fc::variant get_stats( const string& symbol_name, account_name contract = system_account_name ) {
      auto symbol_code = symbol::from_string(symbol_name).to_symbol_code().value;
      vector<char> data = get_row_by_account(default_account_name, contract, N(stat), symbol_code);
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant("stat", data, abi_serializer_max_time);
   }

   fc::variant get_account(account_name acc, const string& symbol_name, account_name contract = system_account_name) {
      auto symbol_code = extended_symbol_code{ symbol::from_string(symbol_name).to_symbol_code(), contract };
      BOOST_TEST_MESSAGE("Searching for " << XXH64((const void*)&symbol_code, sizeof(extended_symbol_code), 0));
      vector<char> data = get_row_by_account(default_account_name, acc, N(accounts), XXH64((const void*)&symbol_code, sizeof(extended_symbol_code), 0));
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant("accounts", data, abi_serializer_max_time);
   }

#define PUSH_ACTION_ARGS_ELEM(r, OP, elem) \
   (BOOST_PP_STRINGIZE(elem), elem)

#define PUSH_ACTION_ARGS(args) \
   BOOST_PP_SEQ_FOR_EACH(PUSH_ACTION_ARGS_ELEM, _, args)

#define PUSH_ACTION(actor, args) \
   push_action(actor, name(__func__), mvo() \
      PUSH_ACTION_ARGS(args) \
   )

   action_result mint(extended_asset value, vector<option> opts = {}, bool simple = true) {
      if (simple) {
         opts.emplace_back("recallable", bytes{0});
         opts.emplace_back("freezable", bytes{0});
      }
      return PUSH_ACTION(default_account_name, (value)(opts));
   }

   inline action_result transfer(account_name from, account_name to, extended_asset value, string memo) {
      return transfer(from, to, value, memo, from);
   }

   action_result transfer(account_name from, account_name to, extended_asset value, string memo, account_name actor) {
      return PUSH_ACTION(actor, (from)(to)(value)(memo));
   }

   action_result burn(account_name owner, extended_asset value, string memo, account_name actor) {
      return PUSH_ACTION(actor, (owner)(value)(memo));
   }

   action_result setopts(extended_symbol_code symbol, vector<option> opts) {
      return PUSH_ACTION(basename(symbol.contract), (symbol)(opts));
   }

   action_result setacntsopts(vector<account_name> accounts, extended_symbol_code symbol, vector<option> opts) {
      return PUSH_ACTION(basename(symbol.contract), (accounts)(symbol)(opts));
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

   abi_serializer abi_ser;
};

BOOST_AUTO_TEST_SUITE(gxc_token_tests)

BOOST_FIXTURE_TEST_CASE(mint_token_tests, gxc_token_tester) try {

   mint({asset::from_string("1000.000 HOBL"), N(conr2d)});
   REQUIRE_MATCHING_OBJECT(get_stats("3,HOBL", N(conr2d)), mvo()
      ("supply", "0.000 HOBL")
      ("max_supply", "1000.000 HOBL")
      ("issuer", "conr2d")
      ("opts", 1)
   );
   produce_blocks(1);

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(mint_negative_max_supply, gxc_token_tester) try {

   BOOST_REQUIRE_EQUAL(wasm_assert_msg("must be positive quantity"),
      mint({asset::from_string("-1000.000 HOBL"), N(conr2d)})
   );

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(mint_existing_token, gxc_token_tester) try {

   mint({asset::from_string("100 HOBL"), N(conr2d)});
   REQUIRE_MATCHING_OBJECT(get_stats("0,HOBL", N(conr2d)), mvo()
      ("supply", "0 HOBL")
      ("max_supply", "100 HOBL")
      ("issuer", "conr2d")
      ("opts", 1)
   );
   produce_blocks(1);

   mint({asset::from_string("100 HOBL"), N(conr2d)}, {}, false);
   REQUIRE_MATCHING_OBJECT(get_stats("0,HOBL", N(conr2d)), mvo()
      ("supply", "0 HOBL")
      ("max_supply", "200 HOBL")
      ("issuer", "conr2d")
      ("opts", 1)
   );
   produce_blocks(1);

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(mint_non_mintable_token, gxc_token_tester) try {

   mint({asset::from_string("100 HOBL"), N(conr2d)}, {{"mintable", bytes{0}}});
   REQUIRE_MATCHING_OBJECT(get_stats("0,HOBL", N(conr2d)), mvo()
      ("supply", "0 HOBL")
      ("max_supply", "100 HOBL")
      ("issuer", "conr2d")
      ("opts", 0)
   );
   produce_blocks(1);

   BOOST_REQUIRE_EQUAL(wasm_assert_msg("not allowed additional mint"),
      mint({asset::from_string("100 HOBL"), N(conr2d)})
   );

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(mint_max_supply, gxc_token_tester) try {

   mint({asset::from_string("4611686018427387903 HOBL"), N(conr2d)});
   REQUIRE_MATCHING_OBJECT(get_stats("0,HOBL", N(conr2d)), mvo()
      ("supply", "0 HOBL")
      ("max_supply", "4611686018427387903 HOBL")
      ("issuer", "conr2d")
      ("opts", 1)
   );
   produce_blocks(1);

   asset max(10, symbol(SY(0, ENC)));
   share_type amount = 4611686018427387904;
   static_assert(sizeof(share_type) <= sizeof(asset), "asset changed so test is no longer valid");
   static_assert(std::is_trivially_copyable<asset>::value, "asset is not trivially copyable");
   memcpy(&max, &amount, sizeof(share_type)); // hack in an invalid amount

   BOOST_CHECK_EXCEPTION(mint({max, N(conr2d)}), asset_type_exception, [](const asset_type_exception& e) {
      return expect_assert_message(e, "magnitude of asset amount must be less than 2^62");
   });

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(mint_max_decimals, gxc_token_tester) try {

   mint({asset::from_string("1.000000000000000000 HOBL"), N(conr2d)});
   REQUIRE_MATCHING_OBJECT(get_stats("0,HOBL", N(conr2d)), mvo()
      ("supply", "0.000000000000000000 HOBL")
      ("max_supply", "1.000000000000000000 HOBL")
      ("issuer", "conr2d")
      ("opts", 1)
   );
   produce_blocks(1);


   asset max(10, symbol(SY(0,ENC)));
   // 1.0000 0000 0000 0000 000 => 0x8ac7230489e80000L
   share_type amount = 0x8ac7230489e80000L;
   static_assert(sizeof(share_type) <= sizeof(asset), "asset changed so test is no longer valid");
   static_assert(std::is_trivially_copyable<asset>::value, "asset is not trivially copyable");
   memcpy(&max, &amount, sizeof(share_type)); // hack in an invalid amount

   BOOST_CHECK_EXCEPTION(mint({max, N(conr2d)}), asset_type_exception, [](const asset_type_exception& e) {
      return expect_assert_message(e, "magnitude of asset amount must be less than 2^62");
   });

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(issue_tests, gxc_token_tester) try {

   mint({asset::from_string("1000.000 HOBL"), N(conr2d)});
   produce_blocks(1);

   transfer(null_account_name, N(conr2d), {asset::from_string("500.000 HOBL"), N(conr2d)}, "hola", N(conr2d));

   REQUIRE_MATCHING_OBJECT(get_stats("3,HOBL", N(conr2d)), mvo()
      ("supply", "500.000 HOBL")
      ("max_supply", "1000.000 HOBL")
      ("issuer", "conr2d")
      ("opts", 1)
   );

   REQUIRE_MATCHING_OBJECT(get_account(N(conr2d), "3,HOBL", N(conr2d)), mvo()
      ("balance", "500.000 HOBL")
      ("issuer_", "conr2d")
   );

   BOOST_REQUIRE_EQUAL(wasm_assert_msg("quantity exceeds available supply"),
      transfer(null_account_name, N(conr2d), {asset::from_string("501.000 HOBL"), N(conr2d)}, "hola", N(conr2d))
   );

   BOOST_REQUIRE_EQUAL(wasm_assert_msg("must be positive quantity"),
      transfer(null_account_name, N(conr2d), {asset::from_string("-1.000 HOBL"), N(conr2d)}, "hola", N(conr2d))
   );

   BOOST_REQUIRE_EQUAL(success(),
      transfer(null_account_name, N(conr2d), {asset::from_string("1.000 HOBL"), N(conr2d)}, "hola", N(conr2d))
   );

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(retire_tests, gxc_token_tester) try {

   mint({asset::from_string("1000.000 HOBL"), N(conr2d)});
   produce_blocks(1);

   BOOST_REQUIRE_EQUAL(success(),
      transfer(null_account_name, N(conr2d), {asset::from_string("500.000 HOBL"), N(conr2d)}, "hola", N(conr2d))
   );

   REQUIRE_MATCHING_OBJECT(get_stats("3,HOBL", N(conr2d)), mvo()
      ("supply", "500.000 HOBL")
      ("max_supply", "1000.000 HOBL")
      ("issuer", "conr2d")
      ("opts", 1)
   );

   REQUIRE_MATCHING_OBJECT(get_account(N(conr2d), "3,HOBL", N(conr2d)), mvo()
      ("balance", "500.000 HOBL")
      ("issuer_", "conr2d")
   );

   BOOST_REQUIRE_EQUAL(success(),
      transfer(N(conr2d), null_account_name, {asset::from_string("200.000 HOBL"), N(conr2d)}, "hola")
   );
   REQUIRE_MATCHING_OBJECT(get_stats("3,HOBL", N(conr2d)), mvo()
      ("supply", "300.000 HOBL")
      ("max_supply", "1000.000 HOBL")
      ("issuer", "conr2d")
      ("opts", 1)
   );
   REQUIRE_MATCHING_OBJECT(get_account(N(conr2d), "3,HOBL", N(conr2d)), mvo()
      ("balance", "300.000 HOBL")
      ("issuer_", "conr2d")
   );

   //should fail to retire more than current supply
   BOOST_REQUIRE_EQUAL(wasm_assert_msg("overdrawn balance"),
      transfer(N(conr2d), null_account_name, {asset::from_string("500.000 HOBL"), N(conr2d)}, "hola")
   );

   BOOST_REQUIRE_EQUAL(success(),
      transfer(N(conr2d), N(eun2ce), {asset::from_string("200.000 HOBL"), N(conr2d)}, "hola")
   );
   //should fail to retire since tokens are not on the issuer's balance
   BOOST_REQUIRE_EQUAL(wasm_assert_msg("overdrawn balance"),
      transfer(N(conr2d), null_account_name, {asset::from_string("300.000 HOBL"), N(conr2d)}, "hola")
   );
   //transfer tokens back
   BOOST_REQUIRE_EQUAL(success(),
      transfer(N(eun2ce), N(conr2d), {asset::from_string("200.000 HOBL"), N(conr2d)}, "hola")
   );

   BOOST_REQUIRE_EQUAL(success(),
      transfer(N(conr2d), null_account_name, {asset::from_string("300.000 HOBL"), N(conr2d)}, "hola")
   );
   REQUIRE_MATCHING_OBJECT(get_stats("3,HOBL", N(conr2d)), mvo()
      ("supply", "0.000 HOBL")
      ("max_supply", "1000.000 HOBL")
      ("issuer", "conr2d")
      ("opts", 1)
   );
   REQUIRE_MATCHING_OBJECT(get_account(N(conr2d), "3,HOBL", N(conr2d)), mvo()
      ("balance", "0.000 HOBL")
      ("issuer_", "conr2d")
   );

   //trying to retire tokens with zero supply
   BOOST_REQUIRE_EQUAL(wasm_assert_msg("overdrawn balance"),
      transfer(N(conr2d), null_account_name, {asset::from_string("1.000 HOBL"), N(conr2d)}, "hola")
   );

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(open_tests, gxc_token_tester) try {

   mint({asset::from_string("1000 HOBL"), N(conr2d)});
   BOOST_REQUIRE_EQUAL(true, get_account(N(conr2d), "0,HOBL", N(conr2d)).is_null());
   BOOST_REQUIRE_EQUAL(success(),
      transfer(null_account_name, N(conr2d), {asset::from_string("1000 HOBL"), N(conr2d)}, "issue", N(conr2d))
   );

   REQUIRE_MATCHING_OBJECT(get_account(N(conr2d), "0,HOBL", N(conr2d)), mvo()
      ("balance", "1000 HOBL")
      ("issuer_", "conr2d")
   );

   BOOST_REQUIRE_EQUAL(true, get_account(N(eun2ce), "0,HOBL", N(conr2d)).is_null());

   BOOST_REQUIRE_EQUAL(wasm_assert_msg("owner account does not exist"),
      open(N(nonexistent), {symbol(SY(0,HOBL)).to_symbol_code(), N(conr2d)}, N(conr2d))
   );
   BOOST_REQUIRE_EQUAL(success(),
      open(N(eun2ce), {symbol(SY(0,HOBL)).to_symbol_code(), N(conr2d)}, N(conr2d))
   );

   REQUIRE_MATCHING_OBJECT(get_account(N(eun2ce), "0,HOBL", N(conr2d)), mvo()
      ("balance", "0 HOBL")
      ("issuer_", "conr2d")
   );

   BOOST_REQUIRE_EQUAL(success(), transfer(N(conr2d), N(eun2ce), {asset::from_string("200 HOBL"), N(conr2d)}, "hola"));

   REQUIRE_MATCHING_OBJECT(get_account(N(eun2ce), "0,HOBL", N(conr2d)), mvo()
      ("balance", "200 HOBL")
      ("issuer_", "conr2d")
   );

   BOOST_REQUIRE_EQUAL(wasm_assert_msg("token not found"),
      open(N(ian), {symbol(SY(0,INVALID)).to_symbol_code(), N(conr2d)}, N(conr2d))
   );

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(close_tests, gxc_token_tester) try {

   mint({asset::from_string("1000 HOBL"), N(conr2d)}, {{"whitelistable", {1}}, {"whitelist_on", {1}}});

   BOOST_REQUIRE_EQUAL(true, get_account(N(conr2d), "0,HOBL", N(conr2d)).is_null());

   BOOST_REQUIRE_EQUAL(success(),
      transfer(null_account_name, N(conr2d), {asset::from_string("1000 HOBL"), N(conr2d)}, "hola", N(conr2d))
   );

   // non-whitelisted account of zero balance is deleted without explicit close
   setacntsopts({N(conr2d)}, {symbol(SY(0,HOBL)).to_symbol_code(), N(conr2d)}, {{"whitelist", {1}}});

   REQUIRE_MATCHING_OBJECT(get_account(N(conr2d), "0,HOBL", N(conr2d)), mvo()
      ("balance", "1000 HOBL")
      ("issuer_", "conr2d......2")
   );

   BOOST_REQUIRE_EQUAL(success(), transfer(N(conr2d), N(eun2ce), {asset::from_string("1000 HOBL"), N(conr2d)}, "hola"));

   REQUIRE_MATCHING_OBJECT(get_account(N(conr2d), "0,HOBL", N(conr2d)), mvo()
      ("balance", "0 HOBL")
      ("issuer_", "conr2d......2")
   );

   BOOST_REQUIRE_EQUAL(success(), close(N(conr2d), {symbol(SY(0,HOBL)).to_symbol_code(), N(conr2d)}));
   BOOST_REQUIRE_EQUAL(true, get_account(N(conr2d), "0,HOBL", N(conr2d)).is_null());

} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
