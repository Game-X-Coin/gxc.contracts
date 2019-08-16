#include "gxc_tester.hpp"

using gxc_token_tester = gxc_tester<token_contract>;

BOOST_AUTO_TEST_SUITE(gxc_token_tests)

BOOST_FIXTURE_TEST_CASE(mint_token_tests, gxc_token_tester) try {

   C(token).mint(EA("1000.000 HOBL@conr2d"));
   REQUIRE_MATCHING_OBJECT(C(token).get_stats("HOBL@conr2d"), mvo()
      ("supply", "0.000 HOBL")
      ("max_supply", "1000.000 HOBL")
      ("issuer", "conr2d")
      ("opts", 1) // mintable
   );
   produce_blocks(1);

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(mint_negative_max_supply, gxc_token_tester) try {

   BOOST_REQUIRE_EQUAL(wasm_assert_msg("must be positive quantity"),
      C(token).mint(EA("-1000.0000 HOBL@conr2d"))
   );

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(mint_existing_token, gxc_token_tester) try {

   C(token).mint(EA("100 HOBL@conr2d"));
   REQUIRE_MATCHING_OBJECT(C(token).get_stats("HOBL@conr2d"), mvo()
      ("supply", "0 HOBL")
      ("max_supply", "100 HOBL")
      ("issuer", "conr2d")
      ("opts", 1) // mintable
   );
   produce_blocks(1);

   C(token).mint(EA("100 HOBL@conr2d"), false);
   REQUIRE_MATCHING_OBJECT(C(token).get_stats("HOBL@conr2d"), mvo()
      ("supply", "0 HOBL")
      ("max_supply", "200 HOBL")
      ("issuer", "conr2d")
      ("opts", 1) // mintable
   );
   produce_blocks(1);

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(mint_non_mintable_token, gxc_token_tester) try {

   C(token).mint(EA("100 HOBL@conr2d"), true, {{"mintable", bytes{0}}});
   REQUIRE_MATCHING_OBJECT(C(token).get_stats("HOBL@conr2d"), mvo()
      ("supply", "0 HOBL")
      ("max_supply", "100 HOBL")
      ("issuer", "conr2d")
      ("opts", 0)
   );
   produce_blocks(1);

   BOOST_REQUIRE_EQUAL(wasm_assert_msg("not allowed additional mint"),
      C(token).mint(EA("100 HOBL@conr2d"))
   );

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(mint_max_supply, gxc_token_tester) try {

   C(token).mint(EA("4611686018427387903 HOBL@conr2d"));
   REQUIRE_MATCHING_OBJECT(C(token).get_stats("HOBL@conr2d"), mvo()
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

   BOOST_CHECK_EXCEPTION(C(token).mint({max, N(conr2d)}), asset_type_exception, [](const asset_type_exception& e) {
      return expect_assert_message(e, "magnitude of asset amount must be less than 2^62");
   });

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(mint_max_decimals, gxc_token_tester) try {

   C(token).mint(EA("1.000000000000000000 HOBL@conr2d"));
   REQUIRE_MATCHING_OBJECT(C(token).get_stats("HOBL@conr2d"), mvo()
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

   BOOST_CHECK_EXCEPTION(C(token).mint({max, N(conr2d)}), asset_type_exception, [](const asset_type_exception& e) {
      return expect_assert_message(e, "magnitude of asset amount must be less than 2^62");
   });

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(issue_simple_tests, gxc_token_tester) try {

   C(token).mint(EA("1000.000 HOBL@conr2d"));
   produce_blocks(1);

   C(token).transfer(config::null_account_name, N(conr2d), EA("500.000 HOBL@conr2d"), "hola");

   REQUIRE_MATCHING_OBJECT(C(token).get_stats("HOBL@conr2d"), mvo()
      ("supply", "500.000 HOBL")
      ("max_supply", "1000.000 HOBL")
      ("issuer", "conr2d")
      ("opts", 1) // mintable
   );

   REQUIRE_MATCHING_OBJECT(C(token).get_account(N(conr2d), "HOBL@conr2d"), mvo()
      ("balance", "500.000 HOBL")
      ("issuer_", "conr2d")
   );

   BOOST_REQUIRE_EQUAL(wasm_assert_msg("quantity exceeds available supply"),
      C(token).transfer(config::null_account_name, N(conr2d), EA("501.000 HOBL@conr2d"), "hola")
   );

   BOOST_REQUIRE_EQUAL(wasm_assert_msg("must be positive quantity"),
      C(token).transfer(config::null_account_name, N(conr2d), EA("-1.000 HOBL@conr2d"), "hola")
   );

   BOOST_REQUIRE_EQUAL(success(),
      C(token).transfer(config::null_account_name, N(conr2d), EA("1.000 HOBL@conr2d"), "hola")
   );

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(retire_simple_tests, gxc_token_tester) try {

   C(token).mint(EA("1000.000 HOBL@conr2d"));
   produce_blocks(1);

   BOOST_REQUIRE_EQUAL(success(),
      C(token).transfer(config::null_account_name, N(conr2d), EA("500.000 HOBL@conr2d"), "hola")
   );

   REQUIRE_MATCHING_OBJECT(C(token).get_stats("HOBL@conr2d"), mvo()
      ("supply", "500.000 HOBL")
      ("max_supply", "1000.000 HOBL")
      ("issuer", "conr2d")
      ("opts", 1) // mintable
   );

   REQUIRE_MATCHING_OBJECT(C(token).get_account(N(conr2d), "HOBL@conr2d"), mvo()
      ("balance", "500.000 HOBL")
      ("issuer_", "conr2d")
   );

   BOOST_REQUIRE_EQUAL(success(),
      C(token).transfer(N(conr2d), config::null_account_name, EA("200.000 HOBL@conr2d"), "hola")
   );
   REQUIRE_MATCHING_OBJECT(C(token).get_stats("HOBL@conr2d"), mvo()
      ("supply", "300.000 HOBL")
      ("max_supply", "1000.000 HOBL")
      ("issuer", "conr2d")
      ("opts", 1) // mintable
   );
   REQUIRE_MATCHING_OBJECT(C(token).get_account(N(conr2d), "HOBL@conr2d"), mvo()
      ("balance", "300.000 HOBL")
      ("issuer_", "conr2d")
   );

   //should fail to retire more than current supply
   BOOST_REQUIRE_EQUAL(wasm_assert_msg("overdrawn balance"),
      C(token).transfer(N(conr2d), config::null_account_name, EA("500.000 HOBL@conr2d"), "hola")
   );

   BOOST_REQUIRE_EQUAL(success(),
      C(token).transfer(N(conr2d), N(eun2ce), EA("200.000 HOBL@conr2d"), "hola")
   );
   //should fail to retire since tokens are not on the issuer's balance
   BOOST_REQUIRE_EQUAL(wasm_assert_msg("overdrawn balance"),
      C(token).transfer(N(conr2d), config::null_account_name, EA("300.000 HOBL@conr2d"), "hola")
   );
   //transfer tokens back
   BOOST_REQUIRE_EQUAL(success(),
      C(token).transfer(N(eun2ce), N(conr2d), EA("200.000 HOBL@conr2d"), "hola")
   );

   BOOST_REQUIRE_EQUAL(success(),
      C(token).transfer(N(conr2d), config::null_account_name, EA("300.000 HOBL@conr2d"), "hola")
   );
   REQUIRE_MATCHING_OBJECT(C(token).get_stats("HOBL@conr2d"), mvo()
      ("supply", "0.000 HOBL")
      ("max_supply", "1000.000 HOBL")
      ("issuer", "conr2d")
      ("opts", 1) // mintable
   );
   BOOST_REQUIRE_EQUAL(true, C(token).get_account(N(conr2d), "HOBL@conr2d").is_null());

   //trying to retire tokens with zero supply
   BOOST_REQUIRE_EQUAL(wasm_assert_msg("cannot pass end iterator to modify"),
      C(token).transfer(N(conr2d), config::null_account_name, EA("1.000 HOBL@conr2d"), "hola")
   );

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(open_tests, gxc_token_tester) try {

   C(token).mint(EA("1000 HOBL@conr2d"));
   BOOST_REQUIRE_EQUAL(true, C(token).get_account(N(conr2d), "HOBL@conr2d").is_null());
   BOOST_REQUIRE_EQUAL(success(),
      C(token).transfer(config::null_account_name, N(conr2d), EA("1000 HOBL@conr2d"), "issue")
   );

   REQUIRE_MATCHING_OBJECT(C(token).get_account(N(conr2d), "HOBL@conr2d"), mvo()
      ("balance", "1000 HOBL")
      ("issuer_", "conr2d")
   );

   BOOST_REQUIRE_EQUAL(true, C(token).get_account(N(eun2ce), "HOBL@conr2d").is_null());

   BOOST_REQUIRE_EQUAL(wasm_assert_msg("owner account does not exist"),
      C(token).open(N(nonexistent), {symbol(SY(0,HOBL)).to_symbol_code(), N(conr2d)}, N(conr2d))
   );
   BOOST_REQUIRE_EQUAL(success(),
      C(token).open(N(eun2ce), {symbol(SY(0,HOBL)).to_symbol_code(), N(conr2d)}, N(conr2d))
   );

   REQUIRE_MATCHING_OBJECT(C(token).get_account(N(eun2ce), "HOBL@conr2d"), mvo()
      ("balance", "0 HOBL")
      ("issuer_", "conr2d")
   );

   BOOST_REQUIRE_EQUAL(success(), C(token).transfer(N(conr2d), N(eun2ce), EA("200 HOBL@conr2d"), "hola"));

   REQUIRE_MATCHING_OBJECT(C(token).get_account(N(eun2ce), "HOBL@conr2d"), mvo()
      ("balance", "200 HOBL")
      ("issuer_", "conr2d")
   );

   BOOST_REQUIRE_EQUAL(wasm_assert_msg("token not found"),
      C(token).open(N(ian), {symbol(SY(0,INVALID)).to_symbol_code(), N(conr2d)}, N(conr2d))
   );

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(close_tests, gxc_token_tester) try {

   C(token).mint(EA("1000 HOBL@conr2d"), true, {{"whitelistable", {1}}, {"whitelist_on", {1}}});

   BOOST_REQUIRE_EQUAL(true, C(token).get_account(N(conr2d), "HOBL@conr2d").is_null());

   BOOST_REQUIRE_EQUAL(success(),
      C(token).transfer(config::null_account_name, N(conr2d), EA("1000 HOBL@conr2d"), "hola")
   );

   // non-whitelisted account of zero balance is deleted without explicit close
   C(token).setacntsopts({N(conr2d)}, SC("HOBL@conr2d"), {{"whitelist", {1}}});

   REQUIRE_MATCHING_OBJECT(C(token).get_account(N(conr2d), "HOBL@conr2d"), mvo()
      ("balance", "1000 HOBL")
      ("issuer_", "conr2d......2")
   );

   BOOST_REQUIRE_EQUAL(success(), C(token).transfer(N(conr2d), N(eun2ce), EA("1000 HOBL@conr2d"), "hola"));

   REQUIRE_MATCHING_OBJECT(C(token).get_account(N(conr2d), "HOBL@conr2d"), mvo()
      ("balance", "0 HOBL")
      ("issuer_", "conr2d......2")
   );

   BOOST_REQUIRE_EQUAL(success(), C(token).close(N(conr2d), {symbol(SY(0,HOBL)).to_symbol_code(), N(conr2d)}));
   BOOST_REQUIRE_EQUAL(true, C(token).get_account(N(conr2d), "HOBL@conr2d").is_null());

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(transfer_simple_tests, gxc_token_tester) try {

   C(token).mint(EA("1000 HOBL@conr2d"));
   produce_blocks(1);

   C(token).transfer(config::null_account_name, N(conr2d), EA("1000 HOBL@conr2d"), "hola");

   C(token).transfer(N(conr2d), N(eun2ce), EA("300 HOBL@conr2d"), "hola");
   REQUIRE_MATCHING_OBJECT(C(token).get_account(N(conr2d), "HOBL@conr2d"), mvo()
      ("balance", "700 HOBL")
      ("issuer_", "conr2d")
   );
   REQUIRE_MATCHING_OBJECT(C(token).get_account(N(eun2ce), "HOBL@conr2d"), mvo()
      ("balance", "300 HOBL")
      ("issuer_", "conr2d")
   );

   BOOST_REQUIRE_EQUAL(wasm_assert_msg("overdrawn balance"),
      C(token).transfer(N(conr2d), N(eun2ce), EA("701 HOBL@conr2d"), "hola")
   );

   BOOST_REQUIRE_EQUAL(wasm_assert_msg("must be positive quantity"),
      C(token).transfer(N(conr2d), N(eun2ce), EA("-1000 HOBL@conr2d"), "hola")
   );

   BOOST_REQUIRE_EQUAL(wasm_assert_msg("missing required authority"),
      C(token).transfer(N(conr2d), N(eun2ce), EA("700 HOBL@conr2d"), "hola", N(eun2ce))
   );

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(transfer_tests , gxc_token_tester) try {

   C(token).mint(EA("1000 ENC@conr2d.com"), false, {{"withdraw_delay_sec", {1, 0, 0, 0, 0, 0, 0, 0}}});
   REQUIRE_MATCHING_OBJECT(C(token).get_stats("ENC@conr2d.com"), mvo()
      ("supply", "0 ENC")
      ("max_supply", "1000 ENC")
      ("issuer", "conr2d.com")
      ("opts", 7) // mintable, recallable, freezable
      ("amount", "0 ENC")
      ("duration", 1)
   );
   produce_blocks(1);

   BOOST_REQUIRE_EQUAL("missing authority of conr2d",
      C(token).transfer(config::null_account_name, N(eun2ce), EA("1000 ENC@conr2d.com"), "hola", N(eun2ce))
   );

   C(token).transfer(config::null_account_name, N(eun2ce), EA("1000 ENC@conr2d.com"), "hola");
   REQUIRE_MATCHING_OBJECT(C(token).get_account(N(eun2ce), "ENC@conr2d.com"), mvo()
      ("balance", "0 ENC")
      ("issuer_", "conr2d.com")
      ("deposit", "1000 ENC")
   );

   // issuer can transfer token from user's deposit to his balance
   C(token).transfer(N(eun2ce), N(conr2d), EA("300 ENC@conr2d.com"), "hola", N(conr2d));
   REQUIRE_MATCHING_OBJECT(C(token).get_account(N(eun2ce), "ENC@conr2d.com"), mvo()
      ("balance", "0 ENC")
      ("issuer_", "conr2d.com")
      ("deposit", "700 ENC")
   );
   REQUIRE_MATCHING_OBJECT(C(token).get_account(N(conr2d), "ENC@conr2d.com"), mvo()
      ("balance", "300 ENC")
      ("issuer_", "conr2d.com")
      ("deposit", "0 ENC")
   );

   BOOST_REQUIRE_EQUAL(wasm_assert_msg("overdrawn balance"),
      C(token).transfer(N(eun2ce), N(ian), EA("100 ENC@conr2d.com"), "hola")
   );

   C(token).pushwithdraw(N(eun2ce), EA("200 ENC@conr2d.com"));
   REQUIRE_MATCHING_OBJECT(C(token).get_account(N(eun2ce), "ENC@conr2d.com"), mvo()
      ("balance", "0 ENC")
      ("issuer_", "conr2d.com")
      ("deposit", "500 ENC")
   );
   produce_blocks(3);

   REQUIRE_MATCHING_OBJECT(C(token).get_account(N(eun2ce), "ENC@conr2d.com"), mvo()
      ("balance", "200 ENC")
      ("issuer_", "conr2d.com")
      ("deposit", "500 ENC")
   );

   C(token).transfer(N(eun2ce), N(ian), EA("100 ENC@conr2d.com"), "hola");
   REQUIRE_MATCHING_OBJECT(C(token).get_account(N(eun2ce), "ENC@conr2d.com"), mvo()
      ("balance", "100 ENC")
      ("issuer_", "conr2d.com")
      ("deposit", "500 ENC")
   );
   REQUIRE_MATCHING_OBJECT(C(token).get_account(N(ian), "ENC@conr2d.com"), mvo()
      ("balance", "100 ENC")
      ("issuer_", "conr2d.com")
      ("deposit", "0 ENC")
   );

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(burn_tests, gxc_token_tester) try {

   C(token).mint(EA("1000 ENC@conr2d.com"), false, {{"withdraw_delay_sec", {1, 0, 0, 0, 0, 0, 0, 0}}});
   C(token).transfer(config::null_account_name, N(conr2d), EA("200 ENC@conr2d.com"), "hola");

   REQUIRE_MATCHING_OBJECT(C(token).get_stats("ENC@conr2d.com"), mvo()
      ("supply", "200 ENC")
      ("max_supply", "1000 ENC")
      ("issuer", "conr2d.com")
      ("opts", 7) //mintable, recallable, freezable
      ("amount", "0 ENC")
      ("duration", 1)
   );
   REQUIRE_MATCHING_OBJECT(C(token).get_account(N(conr2d), "ENC@conr2d.com"), mvo()
      ("balance", "200 ENC")
      ("issuer_", "conr2d.com")
      ("deposit", "0 ENC")
   );
   produce_blocks(1);

   C(token).burn(N(conr2d), EA("100 ENC@conr2d.com"), "hola");
   REQUIRE_MATCHING_OBJECT(C(token).get_account(N(conr2d), "ENC@conr2d.com"), mvo()
      ("balance", "100 ENC")
      ("issuer_", "conr2d.com")
      ("deposit", "0 ENC")
   );
   REQUIRE_MATCHING_OBJECT(C(token).get_stats("ENC@conr2d.com"), mvo()
      ("supply", "100 ENC")
      ("max_supply", "900 ENC")
      ("issuer", "conr2d.com")
      ("opts", 7) // mintable, recallable, freezable
      ("amount", "0 ENC")
      ("duration", 1)
   );
   produce_blocks(1);

   // not allow user (not issuer or token contract) to burn token
   C(token).transfer(config::null_account_name, N(eun2ce), EA("200 ENC@conr2d.com"), "hola");
   REQUIRE_MATCHING_OBJECT(C(token).get_account(N(eun2ce), "ENC@conr2d.com"), mvo()
      ("balance", "0 ENC")
      ("issuer_", "conr2d.com")
      ("deposit", "200 ENC")
   );
   BOOST_REQUIRE_EQUAL(wasm_assert_msg("missing required authority"),
      C(token).burn(N(eun2ce), EA("100 ENC@conr2d.com"), "hola", N(eun2ce))
   );

   C(token).pushwithdraw(N(eun2ce), EA("200 ENC@conr2d.com"));
   produce_blocks(3);
   REQUIRE_MATCHING_OBJECT(C(token).get_account(N(eun2ce), "ENC@conr2d.com"), mvo()
      ("balance", "200 ENC")
      ("issuer_", "conr2d.com")
      ("deposit", "0 ENC")
   );
   BOOST_REQUIRE_EQUAL(wasm_assert_msg("missing required authority"),
      C(token).burn(N(eun2ce), EA("100 ENC@conr2d.com"), "hola", N(eun2ce))
   );

   // token contract can burn the amount it holds
   C(token).transfer(N(eun2ce), C(token).account, EA("100 ENC@conr2d.com"), "hola");
   REQUIRE_MATCHING_OBJECT(C(token).get_account(C(token).account, "ENC@conr2d.com"), mvo()
      ("balance", "100 ENC")
      ("issuer_", "conr2d.com")
      ("deposit", "0 ENC")
   );
   produce_blocks(1);

   C(token).burn(C(token).account, EA("100 ENC@conr2d.com"), "hola");
   BOOST_REQUIRE_EQUAL(true, C(token).get_account(C(token).account, "ENC@conr2d.com").is_null());
   REQUIRE_MATCHING_OBJECT(C(token).get_stats("ENC@conr2d.com"), mvo()
      ("supply", "200 ENC")
      ("max_supply", "800 ENC")
      ("issuer", "conr2d.com")
      ("opts", 7) // mintable, recallable, freezable
      ("amount", "0 ENC")
      ("duration", 1)
   );

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(freezable_token_tests, gxc_token_tester) try {

   C(token).mint(EA("1000 ENC@conr2d.com"), false, {{"withdraw_delay_sec", {1, 0, 0, 0, 0, 0, 0, 0}}});
   produce_blocks(1);

   C(token).transfer(config::null_account_name, N(eun2ce), EA("200 ENC@conr2d.com"), "hola");
   C(token).pushwithdraw(N(eun2ce), EA("100 ENC@conr2d.com"));
   produce_blocks(3);

   C(token).setacntsopts({N(eun2ce)}, SC("ENC@conr2d.com"), {{"frozen", {1}}});
   REQUIRE_MATCHING_OBJECT(C(token).get_account(N(eun2ce), "ENC@conr2d.com"), mvo()
      ("balance", "100 ENC")
      ("issuer_", "conr2d.com..1")
      ("deposit", "100 ENC")
   );
   produce_blocks(1);

   // not possible transfer from frozen account
   BOOST_REQUIRE_EQUAL(wasm_assert_msg("account is frozen"),
      C(token).transfer(N(eun2ce), N(conr2d), EA("100 ENC@conr2d.com"), "hola", N(conr2d))
   );
   BOOST_REQUIRE_EQUAL(wasm_assert_msg("account is frozen"),
      C(token).transfer(N(eun2ce), N(ian), EA("100 ENC@conr2d.com"), "hola")
   );

   C(token).transfer(config::null_account_name, N(conr2d), EA("200 ENC@conr2d.com"), "hola");
   C(token).transfer(N(conr2d), N(ian), EA("100 ENC@conr2d.com"), "hola");
   produce_blocks(1);

   // not possible transfer to frozen account
   BOOST_REQUIRE_EQUAL(wasm_assert_msg("account is frozen"),
      C(token).transfer(N(conr2d), N(eun2ce), EA("100 ENC@conr2d.com"), "hola")
   );
   BOOST_REQUIRE_EQUAL(wasm_assert_msg("account is frozen"),
      C(token).transfer(N(ian), N(eun2ce), EA("100 ENC@conr2d.com"), "hola")
   );

   // not possible transfer with frozen account deposit
   BOOST_REQUIRE_EQUAL(wasm_assert_msg("account is frozen"),
      C(token).transfer(config::null_account_name, N(eun2ce), EA("100 ENC@conr2d.com"), "hola")
   );
   BOOST_REQUIRE_EQUAL(wasm_assert_msg("account is frozen"),
      C(token).transfer(N(eun2ce), config::null_account_name, EA("100 ENC@conr2d.com"), "hola", N(conr2d))
   );

   C(token).setacntsopts({N(eun2ce)}, SC("ENC@conr2d.com"), {{"frozen", {0}}});
   C(token).transfer(config::null_account_name, N(eun2ce), EA("200 ENC@conr2d.com"), "hola");
   REQUIRE_MATCHING_OBJECT(C(token).get_account(N(eun2ce), "ENC@conr2d.com"), mvo()
      ("balance", "100 ENC")
      ("issuer_", "conr2d.com")
      ("deposit", "300 ENC")
   );
   produce_blocks(1);

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(whitelistable_token_tests, gxc_token_tester) try {

   C(token).mint(EA("1000 ENC@conr2d.com"), false, {
      {"withdraw_delay_sec", {1, 0, 0, 0, 0, 0, 0, 0}}, {"whitelistable", {1}}, {"whitelist_on", {1}}
   });
   produce_blocks(1);

   C(token).transfer(config::null_account_name, N(eun2ce), EA("500 ENC@conr2d.com"), "hola");
   REQUIRE_MATCHING_OBJECT(C(token).get_account(N(eun2ce), "ENC@conr2d.com"), mvo()
      ("balance", "0 ENC")
      ("issuer_", "conr2d.com..2")
      ("deposit", "500 ENC")
   );
   produce_blocks(1);

   C(token).pushwithdraw(N(eun2ce), EA("300 ENC@conr2d.com"));
   produce_blocks(3);

   REQUIRE_MATCHING_OBJECT(C(token).get_account(N(eun2ce), "ENC@conr2d.com"), mvo()
      ("balance", "300 ENC")
      ("issuer_", "conr2d.com..2")
      ("deposit", "200 ENC")
   );

   // unwhitelisted account loses control to token
   C(token).setacntsopts({N(eun2ce)}, SC("ENC@conr2d.com"), {{"whitelist", {0}}});
   BOOST_REQUIRE_EQUAL(wasm_assert_msg("not whitelisted account"),
      C(token).pushwithdraw(N(eun2ce), EA("200 ENC@conr2d.com"))
   );

   BOOST_REQUIRE_EQUAL(wasm_assert_msg("not whitelisted account"),
      C(token).transfer(N(eun2ce), N(ian), EA("100 ENC@conr2d.com"), "hola")
   );

   C(token).setacntsopts({N(eun2ce)}, SC("ENC@conr2d.com"), {{"whitelist", {1}}});
   BOOST_REQUIRE_EQUAL(wasm_assert_msg("required to open balance manually"),
      C(token).transfer(N(eun2ce), N(ian), EA("100 ENC@conr2d.com"), "hola")
   );

   C(token).open(N(ian), SC("ENC@conr2d.com"), N(conr2d));
   C(token).transfer(N(eun2ce), N(ian), EA("100 ENC@conr2d.com"), "hola");
   REQUIRE_MATCHING_OBJECT(C(token).get_account(N(ian), "ENC@conr2d.com"), mvo()
      ("balance", "100 ENC")
      ("issuer_", "conr2d.com..2")
      ("deposit", "0 ENC")
   );
   produce_blocks(1);

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(pausable_token_tests, gxc_token_tester) try {

   C(token).mint(EA("1000 ENC@conr2d.com"), false, {
      {"withdraw_delay_sec", {1, 0, 0, 0, 0, 0, 0, 0}}, {"pausable", {1}}, {"paused", {1}}
   });
   produce_blocks(1);

   C(token).transfer(config::null_account_name, N(eun2ce), EA("500 ENC@conr2d.com"), "hola");
   C(token).pushwithdraw(N(eun2ce), EA("500 ENC@conr2d.com"));
   produce_blocks(3);

   BOOST_REQUIRE_EQUAL(wasm_assert_msg("token is paused"),
      C(token).transfer(N(eun2ce), N(ian), EA("100 ENC@conr2d.com"), "hola")
   );

   C(token).setopts(SC("ENC@conr2d.com"), {{"paused", {0}}});
   C(token).transfer(N(eun2ce), N(ian), EA("100 ENC@conr2d.com"), "hola");
   REQUIRE_MATCHING_OBJECT(C(token).get_account(N(eun2ce), "ENC@conr2d.com"), mvo()
      ("balance", "400 ENC")
      ("issuer_", "conr2d.com")
      ("deposit", "0 ENC")
   );
   REQUIRE_MATCHING_OBJECT(C(token).get_account(N(ian), "ENC@conr2d.com"), mvo()
      ("balance", "100 ENC")
      ("issuer_", "conr2d.com")
      ("deposit", "0 ENC")
   );
   produce_blocks(1);

   C(token).setopts(SC("ENC@conr2d.com"), {{"paused", {1}}});
   BOOST_REQUIRE_EQUAL(wasm_assert_msg("token is paused"),
      C(token).transfer(N(eun2ce), N(ian), EA("100 ENC@conr2d.com"), "hola")
   );
   produce_blocks(1);

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(repause_unpausable_tests, gxc_token_tester) try {

   // even unpausable token, it can be paused at its first-mint stage
   C(token).mint(EA("1000 HOBL@conr2d.com"), true, {{"pausable", {0}}, {"paused", {1}}});
   REQUIRE_MATCHING_OBJECT(C(token).get_stats("HOBL@conr2d.com"), mvo()
      ("supply", "0 HOBL")
      ("max_supply", "1000 HOBL")
      ("issuer", "conr2d.com")
      ("opts", 17) // mintable, paused
   );
   produce_blocks(1);

   C(token).transfer(config::null_account_name, N(eun2ce), EA("500 HOBL@conr2d.com"), "hola");
   BOOST_REQUIRE_EQUAL(wasm_assert_msg("token is paused"),
      C(token).transfer(N(eun2ce), N(ian), EA("100 HOBL@conr2d.com"), "hola")
   );

   C(token).setopts(SC("HOBL@conr2d.com"), {{"paused", {0}}});
   C(token).transfer(N(eun2ce), N(ian), EA("100 HOBL@conr2d.com"), "hola");
   REQUIRE_MATCHING_OBJECT(C(token).get_account(N(eun2ce), "HOBL@conr2d.com"), mvo()
      ("balance", "400 HOBL")
      ("issuer_", "conr2d.com")
   );
   REQUIRE_MATCHING_OBJECT(C(token).get_account(N(ian), "HOBL@conr2d.com"), mvo()
      ("balance", "100 HOBL")
      ("issuer_", "conr2d.com")
   );
   produce_blocks(1);

   BOOST_REQUIRE_EQUAL(wasm_assert_msg("not allowed to set paused"),
      C(token).setopts(SC("HOBL@conr2d.com"), {{"paused", {1}}})
   );

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(deposit_tests, gxc_token_tester) try {
   C(token).mint(EA("1000 ENC@conr2d.com"), false, {{"withdraw_delay_sec", {1, 0, 0, 0, 0, 0, 0, 0}}});
   C(token).transfer(config::null_account_name, N(conr2d), EA("300 ENC@conr2d.com"), "hola");
   C(token).transfer(N(conr2d), N(eun2ce), EA("300 ENC@conr2d.com"), "hola");
   C(token).transfer(config::null_account_name, N(eun2ce), EA("200 ENC@conr2d.com"), "hola");
   REQUIRE_MATCHING_OBJECT(C(token).get_account(N(eun2ce), "ENC@conr2d.com"), mvo()
      ("balance", "300 ENC")
      ("issuer_", "conr2d.com")
      ("deposit", "200 ENC")
   );
   produce_blocks(1);

   C(token).deposit(N(eun2ce), EA("100 ENC@conr2d.com"));
   REQUIRE_MATCHING_OBJECT(C(token).get_account(N(eun2ce), "ENC@conr2d.com"), mvo()
      ("balance", "200 ENC")
      ("issuer_", "conr2d.com")
      ("deposit", "300 ENC")
   );
   produce_blocks(1);

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(withdraw_tests, gxc_token_tester) try {
   C(token).mint(EA("1000 ENC@conr2d.com"), false, {{"withdraw_delay_sec", {1, 0, 0, 0, 0, 0, 0, 0}}});
   C(token).transfer(config::null_account_name, N(conr2d), EA("100 ENC@conr2d.com"), "hola");
   C(token).transfer(N(conr2d), N(eun2ce), EA("100 ENC@conr2d.com"), "hola");
   C(token).transfer(config::null_account_name, N(eun2ce), EA("400 ENC@conr2d.com"), "hola");
   REQUIRE_MATCHING_OBJECT(C(token).get_account(N(eun2ce), "ENC@conr2d.com"), mvo()
      ("balance", "100 ENC")
      ("issuer_", "conr2d.com")
      ("deposit", "400 ENC")
   );
   produce_blocks(1);

   C(token).pushwithdraw(N(eun2ce), EA("300 ENC@conr2d.com"));
   REQUIRE_MATCHING_OBJECT(C(token).get_account(N(eun2ce), "ENC@conr2d.com"), mvo()
      ("balance", "100 ENC")
      ("issuer_", "conr2d.com")
      ("deposit", "100 ENC")
   );
   produce_blocks(1);

   C(token).popwithdraw(N(eun2ce), SC("ENC@conr2d.com"));
   REQUIRE_MATCHING_OBJECT(C(token).get_account(N(eun2ce), "ENC@conr2d.com"), mvo()
      ("balance", "100 ENC")
      ("issuer_", "conr2d.com")
      ("deposit", "400 ENC")
   );
   produce_blocks(1);

   C(token).pushwithdraw(N(eun2ce), EA("300 ENC@conr2d.com"));
   produce_blocks(3);
   REQUIRE_MATCHING_OBJECT(C(token).get_account(N(eun2ce), "ENC@conr2d.com"), mvo()
      ("balance", "400 ENC")
      ("issuer_", "conr2d.com")
      ("deposit", "100 ENC")
   );
   produce_blocks(1);

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(approve_tests, gxc_token_tester) try {

   C(token).mint(EA("1000 ENC@conr2d.com"), false, {{"withdraw_delay_sec", {1, 0, 0, 0, 0, 0, 0, 0}}});
   C(token).transfer(config::null_account_name, N(conr2d), EA("500 ENC@conr2d.com"), "hola");
   C(token).approve(N(conr2d), N(eun2ce), EA("400 ENC@conr2d.com"));
   C(token).transfer(N(conr2d), N(eun2ce), EA("200 ENC@conr2d.com"), "hola", N(eun2ce));
   REQUIRE_MATCHING_OBJECT(C(token).get_account(N(eun2ce), "ENC@conr2d.com"), mvo()
      ("balance", "200 ENC")
      ("issuer_", "conr2d.com")
      ("deposit", "0 ENC")
   );
   produce_blocks(1);

   BOOST_REQUIRE_EQUAL(wasm_assert_msg("not possible to transfer more than allowed"),
      C(token).transfer(N(conr2d), N(eun2ce), EA("300 ENC@conr2d.com"), "hola", N(eun2ce))
   );

   C(token).approve(N(conr2d), N(eun2ce), EA("400 ENC@conr2d.com"));
   BOOST_REQUIRE_EQUAL(wasm_assert_msg("overdrawn balance"),
      C(token).transfer(N(conr2d), N(eun2ce), EA("400 ENC@conr2d.com"), "hola", N(eun2ce))
   );

   C(token).transfer(N(conr2d), N(eun2ce), EA("300 ENC@conr2d.com"), "hola");
   REQUIRE_MATCHING_OBJECT(C(token).get_account(N(eun2ce), "ENC@conr2d.com"), mvo()
      ("balance", "500 ENC")
      ("issuer_", "conr2d.com")
      ("deposit", "0 ENC")
   );

   C(token).transfer(config::null_account_name, N(conr2d), EA("200 ENC@conr2d.com"), "hola");
   C(token).transfer(N(conr2d), N(eun2ce), EA("100 ENC@conr2d.com"), "hola", N(eun2ce));
   REQUIRE_MATCHING_OBJECT(C(token).get_account(N(eun2ce), "ENC@conr2d.com"), mvo()
      ("balance", "600 ENC")
      ("issuer_", "conr2d.com")
      ("deposit", "0 ENC")
   );
   produce_blocks(1);

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(floatable_mint_tests, gxc_token_tester) try {
   BOOST_REQUIRE_EQUAL(success(),
      C(token).mint(EA("1000.00 CRD@conr2d.com"), false, {{"floatable", {0}}})
   );

   REQUIRE_MATCHING_OBJECT(C(token).get_stats("CRD@conr2d.com"), mvo()
      ("supply", "0.00 CRD")
      ("max_supply", "1000.00 CRD")
      ("issuer", "conr2d.com")
      ("opts", 7) // mintable, recallable, freezable
      ("amount", "0.00 CRD")
      ("duration", 86400)
   );
   produce_blocks(1);

   BOOST_REQUIRE_EQUAL(success(),
      C(token).mint(EA("1000.00 ENC@conr2d.com"), false, {{"floatable", {1}}})
   );

   REQUIRE_MATCHING_OBJECT(C(token).get_stats("ENC@conr2d.com"), mvo()
      ("supply", "0.00 ENC")
      ("max_supply", "1000.00 ENC")
      ("issuer", "conr2d.com")
      ("opts", 135) // mintable, recallable, freezable, floatable
      ("amount", "0.00 ENC")
      ("duration", 86400)
   );
  produce_blocks(1);

   BOOST_REQUIRE_EQUAL(success(),
      C(token).mint(EA("1000.00 GAB@conr2d.com"), true, {{"floatable", {0}}})
   );

    REQUIRE_MATCHING_OBJECT(C(token).get_stats("GAB@conr2d.com"), mvo()
      ("supply", "0.00 GAB")
      ("max_supply", "1000.00 GAB")
      ("issuer", "conr2d.com")
      ("opts", 1)
   );
  produce_blocks(1);

   BOOST_REQUIRE_EQUAL(wasm_assert_msg("not allowed to set floatable"),
      C(token).mint(EA("1000.00 HOBL@conr2d.com"), true, {{"floatable", {1}}})
   );

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(floatable_deposit_tests, gxc_token_tester) try {
   C(token).mint(EA("1000.00 ENC@conr2d.com"), false, {
      {"withdraw_delay_sec", {1, 0, 0, 0, 0, 0, 0, 0}}, {"floatable", {0}}
   });
   produce_blocks(1);

   C(token).transfer(config::null_account_name, N(eun2ce), EA("500.00 ENC@conr2d.com"), "hola");
   REQUIRE_MATCHING_OBJECT(C(token).get_account(N(eun2ce), "ENC@conr2d.com"), mvo()
      ("balance", "0.00 ENC")
      ("issuer_", "conr2d.com")
      ("deposit", "500.00 ENC")
   );
   produce_blocks(1);

   C(token).pushwithdraw(N(eun2ce), EA("300.00 ENC@conr2d.com"));
   produce_blocks(3);

   REQUIRE_MATCHING_OBJECT(C(token).get_account(N(eun2ce), "ENC@conr2d.com"), mvo()
      ("balance", "300.00 ENC")
      ("issuer_", "conr2d.com")
      ("deposit", "200.00 ENC")
   );
   produce_blocks(1);

   BOOST_REQUIRE_EQUAL(wasm_assert_msg("not available float"),
      C(token).pushwithdraw(N(eun2ce), EA("100.01 ENC@conr2d.com"))
   );

   C(token).mint(EA("1000.00 CRD@conr2d.com"), false, {
      {"withdraw_delay_sec", {1, 0, 0, 0, 0, 0, 0, 0}}, {"floatable", {1}}
   });
   produce_blocks(1);

   C(token).transfer(config::null_account_name, N(eun2ce), EA("500.00 CRD@conr2d.com"), "hola");
   REQUIRE_MATCHING_OBJECT(C(token).get_account(N(eun2ce), "CRD@conr2d.com"), mvo()
      ("balance", "0.00 CRD")
      ("issuer_", "conr2d.com")
      ("deposit", "500.00 CRD")
   );
   produce_blocks(1);

   C(token).pushwithdraw(N(eun2ce), EA("300.00 CRD@conr2d.com"));
   produce_blocks(3);

   REQUIRE_MATCHING_OBJECT(C(token).get_account(N(eun2ce), "CRD@conr2d.com"), mvo()
      ("balance", "300.00 CRD")
      ("issuer_", "conr2d.com")
      ("deposit", "200.00 CRD")
   );
   produce_blocks(1);

   C(token).pushwithdraw(N(eun2ce), EA("100.01 CRD@conr2d.com"));
   produce_blocks(3);

   REQUIRE_MATCHING_OBJECT(C(token).get_account(N(eun2ce), "CRD@conr2d.com"), mvo()
      ("balance", "400.01 CRD")
      ("issuer_", "conr2d.com")
      ("deposit", "99.99 CRD")
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
