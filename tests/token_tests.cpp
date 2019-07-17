#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/chain/abi_serializer.hpp>
#include <contracts.hpp>

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

extended_asset string_to_extended_asset(const string& s) {
   auto at_pos = s.find('@');
   return extended_asset(asset::from_string(s.substr(0, at_pos)), s.substr(at_pos+1));
}

extended_symbol_code string_to_extended_symbol_code(const string& s) {
   auto at_pos = s.find('@');
   return {symbol(0, s.substr(0, at_pos).data()).to_symbol_code(), s.substr(at_pos+1)};
}

#define EA(s) string_to_extended_asset(s)
#define SC(s) string_to_extended_symbol_code(s)

} }

FC_REFLECT(eosio::chain::extended_symbol_code, (code)(contract))

class gxc_token_tester : public tester {
public:

   static constexpr uint64_t default_account_name = N(gxc.token);
   static constexpr uint64_t null_account_name = N(gxc.null);

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

   action_result setpriv(account_name account, bool is_priv, account_name contract = config::system_account_name) {
      action setpriv;
      setpriv.account = contract;
      setpriv.name = N(setpriv);
      setpriv.data.resize(sizeof(name) + sizeof(uint64_t));

      datastream<char*> ds(setpriv.data.data(), setpriv.data.size());
      ds << account.value;
      ds << (uint64_t)is_priv;

      return base_tester::push_action(std::move(setpriv), contract);
   }

   gxc_token_tester() {
      produce_blocks(2);

      create_accounts({ N(conr2d), N(eun2ce), N(ian), default_account_name });
      produce_blocks(2);

      set_code(default_account_name, contracts::token_wasm());
      set_abi(default_account_name, contracts::token_abi().data());
      produce_blocks(1);

      set_code(config::system_account_name, contracts::system_wasm());
      set_abi(config::system_account_name, contracts::system_abi().data());
      produce_blocks(1);

      setpriv(default_account_name, true);
      produce_blocks(1);

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

   fc::variant get_stats( const string& symbol_name ) {
      auto symbol_code = SC(symbol_name);
      vector<char> data = get_row_by_account(default_account_name, symbol_code.contract, N(stat), symbol_code.code);
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant("stat", data, abi_serializer_max_time);
   }

   fc::variant get_account(account_name acc, const string& symbol_name) {
      auto symbol_code = SC(symbol_name);
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

   action_result mint(extended_asset value, bool simple = true, vector<option> opts = {}) {
      if (simple) {
         opts.emplace_back("recallable", bytes{0});
         opts.emplace_back("freezable", bytes{0});
      }
      return PUSH_ACTION(default_account_name, (value)(opts));
   }

   inline action_result transfer(account_name from, account_name to, extended_asset value, string memo) {
      return transfer(from, to, value, memo, (from != null_account_name) ? from : basename(value.contract));
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

   mint(EA("1000.000 HOBL@conr2d"));
   REQUIRE_MATCHING_OBJECT(get_stats("HOBL@conr2d"), mvo()
      ("supply", "0.000 HOBL")
      ("max_supply", "1000.000 HOBL")
      ("issuer", "conr2d")
      ("opts", 1) // mintable
   );
   produce_blocks(1);

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(mint_negative_max_supply, gxc_token_tester) try {

   BOOST_REQUIRE_EQUAL(wasm_assert_msg("must be positive quantity"),
      mint(EA("-1000.0000 HOBL@conr2d"))
   );

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(mint_existing_token, gxc_token_tester) try {

   mint(EA("100 HOBL@conr2d"));
   REQUIRE_MATCHING_OBJECT(get_stats("HOBL@conr2d"), mvo()
      ("supply", "0 HOBL")
      ("max_supply", "100 HOBL")
      ("issuer", "conr2d")
      ("opts", 1) // mintable
   );
   produce_blocks(1);

   mint(EA("100 HOBL@conr2d"), false);
   REQUIRE_MATCHING_OBJECT(get_stats("HOBL@conr2d"), mvo()
      ("supply", "0 HOBL")
      ("max_supply", "200 HOBL")
      ("issuer", "conr2d")
      ("opts", 1) // mintable
   );
   produce_blocks(1);

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(mint_non_mintable_token, gxc_token_tester) try {

   mint(EA("100 HOBL@conr2d"), true, {{"mintable", bytes{0}}});
   REQUIRE_MATCHING_OBJECT(get_stats("HOBL@conr2d"), mvo()
      ("supply", "0 HOBL")
      ("max_supply", "100 HOBL")
      ("issuer", "conr2d")
      ("opts", 0)
   );
   produce_blocks(1);

   BOOST_REQUIRE_EQUAL(wasm_assert_msg("not allowed additional mint"),
      mint(EA("100 HOBL@conr2d"))
   );

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(mint_max_supply, gxc_token_tester) try {

   mint(EA("4611686018427387903 HOBL@conr2d"));
   REQUIRE_MATCHING_OBJECT(get_stats("HOBL@conr2d"), mvo()
      ("supply", "0 HOBL")
      ("max_supply", "4611686018427387903 HOBL")
      ("issuer", "conr2d")
      ("opts", 1) // mintable
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

   mint(EA("1.000000000000000000 HOBL@conr2d"));
   REQUIRE_MATCHING_OBJECT(get_stats("HOBL@conr2d"), mvo()
      ("supply", "0.000000000000000000 HOBL")
      ("max_supply", "1.000000000000000000 HOBL")
      ("issuer", "conr2d")
      ("opts", 1) // mintable
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

BOOST_FIXTURE_TEST_CASE(issue_simple_tests, gxc_token_tester) try {

   mint(EA("1000.000 HOBL@conr2d"));
   produce_blocks(1);

   transfer(null_account_name, N(conr2d), EA("500.000 HOBL@conr2d"), "hola");

   REQUIRE_MATCHING_OBJECT(get_stats("HOBL@conr2d"), mvo()
      ("supply", "500.000 HOBL")
      ("max_supply", "1000.000 HOBL")
      ("issuer", "conr2d")
      ("opts", 1) // mintable
   );

   REQUIRE_MATCHING_OBJECT(get_account(N(conr2d), "HOBL@conr2d"), mvo()
      ("balance", "500.000 HOBL")
      ("issuer_", "conr2d")
   );

   BOOST_REQUIRE_EQUAL(wasm_assert_msg("quantity exceeds available supply"),
      transfer(null_account_name, N(conr2d), EA("501.000 HOBL@conr2d"), "hola")
   );

   BOOST_REQUIRE_EQUAL(wasm_assert_msg("must be positive quantity"),
      transfer(null_account_name, N(conr2d), EA("-1.000 HOBL@conr2d"), "hola")
   );

   BOOST_REQUIRE_EQUAL(success(),
      transfer(null_account_name, N(conr2d), EA("1.000 HOBL@conr2d"), "hola")
   );

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(retire_simple_tests, gxc_token_tester) try {

   mint(EA("1000.000 HOBL@conr2d"));
   produce_blocks(1);

   BOOST_REQUIRE_EQUAL(success(),
      transfer(null_account_name, N(conr2d), EA("500.000 HOBL@conr2d"), "hola")
   );

   REQUIRE_MATCHING_OBJECT(get_stats("HOBL@conr2d"), mvo()
      ("supply", "500.000 HOBL")
      ("max_supply", "1000.000 HOBL")
      ("issuer", "conr2d")
      ("opts", 1) // mintable
   );

   REQUIRE_MATCHING_OBJECT(get_account(N(conr2d), "HOBL@conr2d"), mvo()
      ("balance", "500.000 HOBL")
      ("issuer_", "conr2d")
   );

   BOOST_REQUIRE_EQUAL(success(),
      transfer(N(conr2d), null_account_name, EA("200.000 HOBL@conr2d"), "hola")
   );
   REQUIRE_MATCHING_OBJECT(get_stats("HOBL@conr2d"), mvo()
      ("supply", "300.000 HOBL")
      ("max_supply", "1000.000 HOBL")
      ("issuer", "conr2d")
      ("opts", 1) // mintable
   );
   REQUIRE_MATCHING_OBJECT(get_account(N(conr2d), "HOBL@conr2d"), mvo()
      ("balance", "300.000 HOBL")
      ("issuer_", "conr2d")
   );

   //should fail to retire more than current supply
   BOOST_REQUIRE_EQUAL(wasm_assert_msg("overdrawn balance"),
      transfer(N(conr2d), null_account_name, EA("500.000 HOBL@conr2d"), "hola")
   );

   BOOST_REQUIRE_EQUAL(success(),
      transfer(N(conr2d), N(eun2ce), EA("200.000 HOBL@conr2d"), "hola")
   );
   //should fail to retire since tokens are not on the issuer's balance
   BOOST_REQUIRE_EQUAL(wasm_assert_msg("overdrawn balance"),
      transfer(N(conr2d), null_account_name, EA("300.000 HOBL@conr2d"), "hola")
   );
   //transfer tokens back
   BOOST_REQUIRE_EQUAL(success(),
      transfer(N(eun2ce), N(conr2d), EA("200.000 HOBL@conr2d"), "hola")
   );

   BOOST_REQUIRE_EQUAL(success(),
      transfer(N(conr2d), null_account_name, EA("300.000 HOBL@conr2d"), "hola")
   );
   REQUIRE_MATCHING_OBJECT(get_stats("HOBL@conr2d"), mvo()
      ("supply", "0.000 HOBL")
      ("max_supply", "1000.000 HOBL")
      ("issuer", "conr2d")
      ("opts", 1) // mintable
   );
   BOOST_REQUIRE_EQUAL(true, get_account(N(conr2d), "HOBL@conr2d").is_null());

   //trying to retire tokens with zero supply
   BOOST_REQUIRE_EQUAL(wasm_assert_msg("cannot pass end iterator to modify"),
      transfer(N(conr2d), null_account_name, EA("1.000 HOBL@conr2d"), "hola")
   );

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(open_tests, gxc_token_tester) try {

   mint(EA("1000 HOBL@conr2d"));
   BOOST_REQUIRE_EQUAL(true, get_account(N(conr2d), "HOBL@conr2d").is_null());
   BOOST_REQUIRE_EQUAL(success(),
      transfer(null_account_name, N(conr2d), EA("1000 HOBL@conr2d"), "issue")
   );

   REQUIRE_MATCHING_OBJECT(get_account(N(conr2d), "HOBL@conr2d"), mvo()
      ("balance", "1000 HOBL")
      ("issuer_", "conr2d")
   );

   BOOST_REQUIRE_EQUAL(true, get_account(N(eun2ce), "HOBL@conr2d").is_null());

   BOOST_REQUIRE_EQUAL(wasm_assert_msg("owner account does not exist"),
      open(N(nonexistent), {symbol(SY(0,HOBL)).to_symbol_code(), N(conr2d)}, N(conr2d))
   );
   BOOST_REQUIRE_EQUAL(success(),
      open(N(eun2ce), {symbol(SY(0,HOBL)).to_symbol_code(), N(conr2d)}, N(conr2d))
   );

   REQUIRE_MATCHING_OBJECT(get_account(N(eun2ce), "HOBL@conr2d"), mvo()
      ("balance", "0 HOBL")
      ("issuer_", "conr2d")
   );

   BOOST_REQUIRE_EQUAL(success(), transfer(N(conr2d), N(eun2ce), EA("200 HOBL@conr2d"), "hola"));

   REQUIRE_MATCHING_OBJECT(get_account(N(eun2ce), "HOBL@conr2d"), mvo()
      ("balance", "200 HOBL")
      ("issuer_", "conr2d")
   );

   BOOST_REQUIRE_EQUAL(wasm_assert_msg("token not found"),
      open(N(ian), {symbol(SY(0,INVALID)).to_symbol_code(), N(conr2d)}, N(conr2d))
   );

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(close_tests, gxc_token_tester) try {

   mint(EA("1000 HOBL@conr2d"), true, {{"whitelistable", {1}}, {"whitelist_on", {1}}});

   BOOST_REQUIRE_EQUAL(true, get_account(N(conr2d), "HOBL@conr2d").is_null());

   BOOST_REQUIRE_EQUAL(success(),
      transfer(null_account_name, N(conr2d), EA("1000 HOBL@conr2d"), "hola")
   );

   // non-whitelisted account of zero balance is deleted without explicit close
   setacntsopts({N(conr2d)}, SC("HOBL@conr2d"), {{"whitelist", {1}}});

   REQUIRE_MATCHING_OBJECT(get_account(N(conr2d), "HOBL@conr2d"), mvo()
      ("balance", "1000 HOBL")
      ("issuer_", "conr2d......2")
   );

   BOOST_REQUIRE_EQUAL(success(), transfer(N(conr2d), N(eun2ce), EA("1000 HOBL@conr2d"), "hola"));

   REQUIRE_MATCHING_OBJECT(get_account(N(conr2d), "HOBL@conr2d"), mvo()
      ("balance", "0 HOBL")
      ("issuer_", "conr2d......2")
   );

   BOOST_REQUIRE_EQUAL(success(), close(N(conr2d), {symbol(SY(0,HOBL)).to_symbol_code(), N(conr2d)}));
   BOOST_REQUIRE_EQUAL(true, get_account(N(conr2d), "HOBL@conr2d").is_null());

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(transfer_simple_tests, gxc_token_tester) try {

   mint(EA("1000 HOBL@conr2d"));
   produce_blocks(1);

   transfer(null_account_name, N(conr2d), EA("1000 HOBL@conr2d"), "hola");

   transfer(N(conr2d), N(eun2ce), EA("300 HOBL@conr2d"), "hola");
   REQUIRE_MATCHING_OBJECT(get_account(N(conr2d), "HOBL@conr2d"), mvo()
      ("balance", "700 HOBL")
      ("issuer_", "conr2d")
   );
   REQUIRE_MATCHING_OBJECT(get_account(N(eun2ce), "HOBL@conr2d"), mvo()
      ("balance", "300 HOBL")
      ("issuer_", "conr2d")
   );

   BOOST_REQUIRE_EQUAL(wasm_assert_msg("overdrawn balance"),
      transfer(N(conr2d), N(eun2ce), EA("701 HOBL@conr2d"), "hola")
   );

   BOOST_REQUIRE_EQUAL(wasm_assert_msg("must be positive quantity"),
      transfer(N(conr2d), N(eun2ce), EA("-1000 HOBL@conr2d"), "hola")
   );

   BOOST_REQUIRE_EQUAL(wasm_assert_msg("missing required authority"),
      transfer(N(conr2d), N(eun2ce), EA("700 HOBL@conr2d"), "hola", N(eun2ce))
   );

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(transfer_tests , gxc_token_tester) try {

   mint(EA("1000 ENC@conr2d.com"), false, {{"withdraw_delay_sec", {1, 0, 0, 0, 0, 0, 0, 0}}});
   REQUIRE_MATCHING_OBJECT(get_stats("ENC@conr2d.com"), mvo()
      ("supply", "0 ENC")
      ("max_supply", "1000 ENC")
      ("issuer", "conr2d.com")
      ("opts", 7) // mintable, recallable, freezable
      ("amount", "0 ENC")
      ("duration", 1)
   );
   produce_blocks(1);

   BOOST_REQUIRE_EQUAL("missing authority of conr2d",
      transfer(null_account_name, N(eun2ce), EA("1000 ENC@conr2d.com"), "hola", N(eun2ce))
   );

   transfer(null_account_name, N(eun2ce), EA("1000 ENC@conr2d.com"), "hola");
   REQUIRE_MATCHING_OBJECT(get_account(N(eun2ce), "ENC@conr2d.com"), mvo()
      ("balance", "0 ENC")
      ("issuer_", "conr2d.com")
      ("deposit", "1000 ENC")
   );

   // issuer can transfer token from user's deposit to his balance
   transfer(N(eun2ce), N(conr2d), EA("300 ENC@conr2d.com"), "hola", N(conr2d));
   REQUIRE_MATCHING_OBJECT(get_account(N(eun2ce), "ENC@conr2d.com"), mvo()
      ("balance", "0 ENC")
      ("issuer_", "conr2d.com")
      ("deposit", "700 ENC")
   );
   REQUIRE_MATCHING_OBJECT(get_account(N(conr2d), "ENC@conr2d.com"), mvo()
      ("balance", "300 ENC")
      ("issuer_", "conr2d.com")
      ("deposit", "0 ENC")
   );

   BOOST_REQUIRE_EQUAL(wasm_assert_msg("overdrawn balance"),
      transfer(N(eun2ce), N(ian), EA("100 ENC@conr2d.com"), "hola")
   );

   pushwithdraw(N(eun2ce), EA("200 ENC@conr2d.com"));
   REQUIRE_MATCHING_OBJECT(get_account(N(eun2ce), "ENC@conr2d.com"), mvo()
      ("balance", "0 ENC")
      ("issuer_", "conr2d.com")
      ("deposit", "500 ENC")
   );
   produce_blocks(3);

   REQUIRE_MATCHING_OBJECT(get_account(N(eun2ce), "ENC@conr2d.com"), mvo()
      ("balance", "200 ENC")
      ("issuer_", "conr2d.com")
      ("deposit", "500 ENC")
   );

   transfer(N(eun2ce), N(ian), EA("100 ENC@conr2d.com"), "hola");
   REQUIRE_MATCHING_OBJECT(get_account(N(eun2ce), "ENC@conr2d.com"), mvo()
      ("balance", "100 ENC")
      ("issuer_", "conr2d.com")
      ("deposit", "500 ENC")
   );
   REQUIRE_MATCHING_OBJECT(get_account(N(ian), "ENC@conr2d.com"), mvo()
      ("balance", "100 ENC")
      ("issuer_", "conr2d.com")
      ("deposit", "0 ENC")
   );

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(burn_tests, gxc_token_tester) try {

   mint(EA("1000 ENC@conr2d.com"), false, {{"withdraw_delay_sec", {1, 0, 0, 0, 0, 0, 0, 0}}});
   transfer(null_account_name, N(conr2d), EA("200 ENC@conr2d.com"), "hola");

   REQUIRE_MATCHING_OBJECT(get_stats("ENC@conr2d.com"), mvo()
      ("supply", "200 ENC")
      ("max_supply", "1000 ENC")
      ("issuer", "conr2d.com")
      ("opts", 7) //mintable, recallable, freezable
      ("amount", "0 ENC")
      ("duration", 1)
   );
   REQUIRE_MATCHING_OBJECT(get_account(N(conr2d), "ENC@conr2d.com"), mvo()
      ("balance", "200 ENC")
      ("issuer_", "conr2d.com")
      ("deposit", "0 ENC")
   );
   produce_blocks(1);

   burn(N(conr2d), EA("100 ENC@conr2d.com"), "hola");
   REQUIRE_MATCHING_OBJECT(get_account(N(conr2d), "ENC@conr2d.com"), mvo()
      ("balance", "100 ENC")
      ("issuer_", "conr2d.com")
      ("deposit", "0 ENC")
   );
   REQUIRE_MATCHING_OBJECT(get_stats("ENC@conr2d.com"), mvo()
      ("supply", "100 ENC")
      ("max_supply", "900 ENC")
      ("issuer", "conr2d.com")
      ("opts", 7) // mintable, recallable, freezable
      ("amount", "0 ENC")
      ("duration", 1)
   );
   produce_blocks(1);

   // not allow user (not issuer or token contract) to burn token
   transfer(null_account_name, N(eun2ce), EA("200 ENC@conr2d.com"), "hola");
   REQUIRE_MATCHING_OBJECT(get_account(N(eun2ce), "ENC@conr2d.com"), mvo()
      ("balance", "0 ENC")
      ("issuer_", "conr2d.com")
      ("deposit", "200 ENC")
   );
   BOOST_REQUIRE_EQUAL(wasm_assert_msg("missing required authority"),
      burn(N(eun2ce), EA("100 ENC@conr2d.com"), "hola", N(eun2ce))
   );

   pushwithdraw(N(eun2ce), EA("200 ENC@conr2d.com"));
   produce_blocks(3);
   REQUIRE_MATCHING_OBJECT(get_account(N(eun2ce), "ENC@conr2d.com"), mvo()
      ("balance", "200 ENC")
      ("issuer_", "conr2d.com")
      ("deposit", "0 ENC")
   );
   BOOST_REQUIRE_EQUAL(wasm_assert_msg("missing required authority"),
      burn(N(eun2ce), EA("100 ENC@conr2d.com"), "hola", N(eun2ce))
   );

   // token contract can burn the amount it holds
   transfer(N(eun2ce), default_account_name, EA("100 ENC@conr2d.com"), "hola");
   REQUIRE_MATCHING_OBJECT(get_account(default_account_name, "ENC@conr2d.com"), mvo()
      ("balance", "100 ENC")
      ("issuer_", "conr2d.com")
      ("deposit", "0 ENC")
   );
   produce_blocks(1);

   burn(default_account_name, EA("100 ENC@conr2d.com"), "hola");
   BOOST_REQUIRE_EQUAL(true, get_account(default_account_name, "ENC@conr2d.com").is_null());
   REQUIRE_MATCHING_OBJECT(get_stats("ENC@conr2d.com"), mvo()
      ("supply", "200 ENC")
      ("max_supply", "800 ENC")
      ("issuer", "conr2d.com")
      ("opts", 7) // mintable, recallable, freezable
      ("amount", "0 ENC")
      ("duration", 1)
   );

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(freezable_token_tests, gxc_token_tester) try {

   mint(EA("1000 ENC@conr2d.com"), false, {{"withdraw_delay_sec", {1, 0, 0, 0, 0, 0, 0, 0}}});
   produce_blocks(1);

   transfer(null_account_name, N(eun2ce), EA("200 ENC@conr2d.com"), "hola");
   pushwithdraw(N(eun2ce), EA("100 ENC@conr2d.com"));
   produce_blocks(3);

   setacntsopts({N(eun2ce)}, SC("ENC@conr2d.com"), {{"frozen", {1}}});
   REQUIRE_MATCHING_OBJECT(get_account(N(eun2ce), "ENC@conr2d.com"), mvo()
      ("balance", "100 ENC")
      ("issuer_", "conr2d.com..1")
      ("deposit", "100 ENC")
   );
   produce_blocks(1);

   // not possible transfer from frozen account
   BOOST_REQUIRE_EQUAL(wasm_assert_msg("account is frozen"),
      transfer(N(eun2ce), N(conr2d), EA("100 ENC@conr2d.com"), "hola", N(conr2d))
   );
   BOOST_REQUIRE_EQUAL(wasm_assert_msg("account is frozen"),
      transfer(N(eun2ce), N(ian), EA("100 ENC@conr2d.com"), "hola")
   );

   transfer(null_account_name, N(conr2d), EA("200 ENC@conr2d.com"), "hola");
   transfer(N(conr2d), N(ian), EA("100 ENC@conr2d.com"), "hola");
   produce_blocks(1);

   // not possible transfer to frozen account
   BOOST_REQUIRE_EQUAL(wasm_assert_msg("account is frozen"),
      transfer(N(conr2d), N(eun2ce), EA("100 ENC@conr2d.com"), "hola")
   );
   BOOST_REQUIRE_EQUAL(wasm_assert_msg("account is frozen"),
      transfer(N(ian), N(eun2ce), EA("100 ENC@conr2d.com"), "hola")
   );

   // not possible transfer with frozen account deposit
   BOOST_REQUIRE_EQUAL(wasm_assert_msg("account is frozen"),
      transfer(null_account_name, N(eun2ce), EA("100 ENC@conr2d.com"), "hola")
   );
   BOOST_REQUIRE_EQUAL(wasm_assert_msg("account is frozen"),
      transfer(N(eun2ce), null_account_name, EA("100 ENC@conr2d.com"), "hola", N(conr2d))
   );

   setacntsopts({N(eun2ce)}, SC("ENC@conr2d.com"), {{"frozen", {0}}});
   transfer(null_account_name, N(eun2ce), EA("200 ENC@conr2d.com"), "hola");
   REQUIRE_MATCHING_OBJECT(get_account(N(eun2ce), "ENC@conr2d.com"), mvo()
      ("balance", "100 ENC")
      ("issuer_", "conr2d.com")
      ("deposit", "300 ENC")
   );
   produce_blocks(1);

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(whitelistable_token_tests, gxc_token_tester) try {

   mint(EA("1000 ENC@conr2d.com"), false, {
      {"withdraw_delay_sec", {1, 0, 0, 0, 0, 0, 0, 0}}, {"whitelistable", {1}}, {"whitelist_on", {1}}
   });
   produce_blocks(1);

   transfer(null_account_name, N(eun2ce), EA("500 ENC@conr2d.com"), "hola");
   REQUIRE_MATCHING_OBJECT(get_account(N(eun2ce), "ENC@conr2d.com"), mvo()
      ("balance", "0 ENC")
      ("issuer_", "conr2d.com..2")
      ("deposit", "500 ENC")
   );
   produce_blocks(1);

   pushwithdraw(N(eun2ce), EA("300 ENC@conr2d.com"));
   produce_blocks(3);

   REQUIRE_MATCHING_OBJECT(get_account(N(eun2ce), "ENC@conr2d.com"), mvo()
      ("balance", "300 ENC")
      ("issuer_", "conr2d.com..2")
      ("deposit", "200 ENC")
   );

   // unwhitelisted account loses control to token
   setacntsopts({N(eun2ce)}, SC("ENC@conr2d.com"), {{"whitelist", {0}}});
   BOOST_REQUIRE_EQUAL(wasm_assert_msg("not whitelisted account"),
      pushwithdraw(N(eun2ce), EA("200 ENC@conr2d.com"))
   );

   BOOST_REQUIRE_EQUAL(wasm_assert_msg("not whitelisted account"),
      transfer(N(eun2ce), N(ian), EA("100 ENC@conr2d.com"), "hola")
   );

   setacntsopts({N(eun2ce)}, SC("ENC@conr2d.com"), {{"whitelist", {1}}});
   BOOST_REQUIRE_EQUAL(wasm_assert_msg("required to open balance manually"),
      transfer(N(eun2ce), N(ian), EA("100 ENC@conr2d.com"), "hola")
   );

   open(N(ian), SC("ENC@conr2d.com"), N(conr2d));
   transfer(N(eun2ce), N(ian), EA("100 ENC@conr2d.com"), "hola");
   REQUIRE_MATCHING_OBJECT(get_account(N(ian), "ENC@conr2d.com"), mvo()
      ("balance", "100 ENC")
      ("issuer_", "conr2d.com..2")
      ("deposit", "0 ENC")
   );
   produce_blocks(1);

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(pausable_token_tests, gxc_token_tester) try {

   mint(EA("1000 ENC@conr2d.com"), false, {
      {"withdraw_delay_sec", {1, 0, 0, 0, 0, 0, 0, 0}}, {"pausable", {1}}, {"paused", {1}}
   });
   produce_blocks(1);

   transfer(null_account_name, N(eun2ce), EA("500 ENC@conr2d.com"), "hola");
   pushwithdraw(N(eun2ce), EA("500 ENC@conr2d.com"));
   produce_blocks(3);

   BOOST_REQUIRE_EQUAL(wasm_assert_msg("token is paused"),
      transfer(N(eun2ce), N(ian), EA("100 ENC@conr2d.com"), "hola")
   );

   setopts(SC("ENC@conr2d.com"), {{"paused", {0}}});
   transfer(N(eun2ce), N(ian), EA("100 ENC@conr2d.com"), "hola");
   REQUIRE_MATCHING_OBJECT(get_account(N(eun2ce), "ENC@conr2d.com"), mvo()
      ("balance", "400 ENC")
      ("issuer_", "conr2d.com")
      ("deposit", "0 ENC")
   );
   REQUIRE_MATCHING_OBJECT(get_account(N(ian), "ENC@conr2d.com"), mvo()
      ("balance", "100 ENC")
      ("issuer_", "conr2d.com")
      ("deposit", "0 ENC")
   );
   produce_blocks(1);

   setopts(SC("ENC@conr2d.com"), {{"paused", {1}}});
   BOOST_REQUIRE_EQUAL(wasm_assert_msg("token is paused"),
      transfer(N(eun2ce), N(ian), EA("100 ENC@conr2d.com"), "hola")
   );
   produce_blocks(1);

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(repause_unpausable_tests, gxc_token_tester) try {

   // even unpausable token, it can be paused at its first-mint stage
   mint(EA("1000 HOBL@conr2d.com"), true, {{"pausable", {0}}, {"paused", {1}}});
   REQUIRE_MATCHING_OBJECT(get_stats("HOBL@conr2d.com"), mvo()
      ("supply", "0 HOBL")
      ("max_supply", "1000 HOBL")
      ("issuer", "conr2d.com")
      ("opts", 33) // mintable, paused
   );
   produce_blocks(1);

   transfer(null_account_name, N(eun2ce), EA("500 HOBL@conr2d.com"), "hola");
   BOOST_REQUIRE_EQUAL(wasm_assert_msg("token is paused"),
      transfer(N(eun2ce), N(ian), EA("100 HOBL@conr2d.com"), "hola")
   );

   setopts(SC("HOBL@conr2d.com"), {{"paused", {0}}});
   transfer(N(eun2ce), N(ian), EA("100 HOBL@conr2d.com"), "hola");
   REQUIRE_MATCHING_OBJECT(get_account(N(eun2ce), "HOBL@conr2d.com"), mvo()
      ("balance", "400 HOBL")
      ("issuer_", "conr2d.com")
   );
   REQUIRE_MATCHING_OBJECT(get_account(N(ian), "HOBL@conr2d.com"), mvo()
      ("balance", "100 HOBL")
      ("issuer_", "conr2d.com")
   );
   produce_blocks(1);

   BOOST_REQUIRE_EQUAL(wasm_assert_msg("not allowed to set paused"),
      setopts(SC("HOBL@conr2d.com"), {{"paused", {1}}})
   );

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(deposit_tests, gxc_token_tester) try {
   mint(EA("1000 ENC@conr2d.com"), false, {{"withdraw_delay_sec", {1, 0, 0, 0, 0, 0, 0, 0}}});
   transfer(null_account_name, N(conr2d), EA("300 ENC@conr2d.com"), "hola");
   transfer(N(conr2d), N(eun2ce), EA("300 ENC@conr2d.com"), "hola");
   transfer(null_account_name, N(eun2ce), EA("200 ENC@conr2d.com"), "hola");
   REQUIRE_MATCHING_OBJECT(get_account(N(eun2ce), "ENC@conr2d.com"), mvo()
      ("balance", "300 ENC")
      ("issuer_", "conr2d.com")
      ("deposit", "200 ENC")
   );
   produce_blocks(1);

   deposit(N(eun2ce), EA("100 ENC@conr2d.com"));
   REQUIRE_MATCHING_OBJECT(get_account(N(eun2ce), "ENC@conr2d.com"), mvo()
      ("balance", "200 ENC")
      ("issuer_", "conr2d.com")
      ("deposit", "300 ENC")
   );
   produce_blocks(1);

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(withdraw_tests, gxc_token_tester) try {
   mint(EA("1000 ENC@conr2d.com"), false, {{"withdraw_delay_sec", {1, 0, 0, 0, 0, 0, 0, 0}}});
   transfer(null_account_name, N(conr2d), EA("100 ENC@conr2d.com"), "hola");
   transfer(N(conr2d), N(eun2ce), EA("100 ENC@conr2d.com"), "hola");
   transfer(null_account_name, N(eun2ce), EA("400 ENC@conr2d.com"), "hola");
   REQUIRE_MATCHING_OBJECT(get_account(N(eun2ce), "ENC@conr2d.com"), mvo()
      ("balance", "100 ENC")
      ("issuer_", "conr2d.com")
      ("deposit", "400 ENC")
   );
   produce_blocks(1);

   pushwithdraw(N(eun2ce), EA("300 ENC@conr2d.com"));
   REQUIRE_MATCHING_OBJECT(get_account(N(eun2ce), "ENC@conr2d.com"), mvo()
      ("balance", "100 ENC")
      ("issuer_", "conr2d.com")
      ("deposit", "100 ENC")
   );
   produce_blocks(1);

   popwithdraw(N(eun2ce), SC("ENC@conr2d.com"));
   REQUIRE_MATCHING_OBJECT(get_account(N(eun2ce), "ENC@conr2d.com"), mvo()
      ("balance", "100 ENC")
      ("issuer_", "conr2d.com")
      ("deposit", "400 ENC")
   );
   produce_blocks(1);

   pushwithdraw(N(eun2ce), EA("300 ENC@conr2d.com"));
   produce_blocks(3);
   REQUIRE_MATCHING_OBJECT(get_account(N(eun2ce), "ENC@conr2d.com"), mvo()
      ("balance", "400 ENC")
      ("issuer_", "conr2d.com")
      ("deposit", "100 ENC")
   );
   produce_blocks(1);

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(approve_tests, gxc_token_tester) try {

   mint(EA("1000 ENC@conr2d.com"), false, {{"withdraw_delay_sec", {1, 0, 0, 0, 0, 0, 0, 0}}});
   transfer(null_account_name, N(conr2d), EA("500 ENC@conr2d.com"), "hola");
   approve(N(conr2d), N(eun2ce), EA("400 ENC@conr2d.com"));
   transfer(N(conr2d), N(eun2ce), EA("200 ENC@conr2d.com"), "hola", N(eun2ce));
   REQUIRE_MATCHING_OBJECT(get_account(N(eun2ce), "ENC@conr2d.com"), mvo()
      ("balance", "200 ENC")
      ("issuer_", "conr2d.com")
      ("deposit", "0 ENC")
   );
   produce_blocks(1);

   BOOST_REQUIRE_EQUAL(wasm_assert_msg("not possible to transfer more than allowed"),
      transfer(N(conr2d), N(eun2ce), EA("300 ENC@conr2d.com"), "hola", N(eun2ce))
   );

   approve(N(conr2d), N(eun2ce), EA("400 ENC@conr2d.com"));
   BOOST_REQUIRE_EQUAL(wasm_assert_msg("overdrawn balance"),
      transfer(N(conr2d), N(eun2ce), EA("400 ENC@conr2d.com"), "hola", N(eun2ce))
   );

   transfer(N(conr2d), N(eun2ce), EA("300 ENC@conr2d.com"), "hola");
   REQUIRE_MATCHING_OBJECT(get_account(N(eun2ce), "ENC@conr2d.com"), mvo()
      ("balance", "500 ENC")
      ("issuer_", "conr2d.com")
      ("deposit", "0 ENC")
   );

   transfer(null_account_name, N(conr2d), EA("200 ENC@conr2d.com"), "hola");
   transfer(N(conr2d), N(eun2ce), EA("100 ENC@conr2d.com"), "hola", N(eun2ce));
   REQUIRE_MATCHING_OBJECT(get_account(N(eun2ce), "ENC@conr2d.com"), mvo()
      ("balance", "600 ENC")
      ("issuer_", "conr2d.com")
      ("deposit", "0 ENC")
   );
   produce_blocks(1);

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(token_options_tests, gxc_token_tester) try {
   BOOST_TEST_MESSAGE("not implemented yet");
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(accounts_options_tests, gxc_token_tester) try {
   BOOST_TEST_MESSAGE("not implemented yet");
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(transfer_from_deposit_tests, gxc_token_tester) try {
   BOOST_TEST_MESSAGE("not implemented yet");
} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
