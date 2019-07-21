#include "token_tester.hpp"

const static name accounts_name = N(gxc.account);

class gxc_account_tester : public gxc_token_tester {
public:

   gxc_account_tester() {
      create_accounts({ accounts_name });
      produce_blocks(1);

      _set_code(accounts_name, contracts::account_wasm());
      _set_abi(accounts_name, contracts::account_abi().data());
      produce_blocks(1);

      auto accnt = control->db().get<account_object,by_name>(accounts_name);
      abi_def abi;
      BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi), true);
      abi_ser[accounts_name].set_abi(abi, abi_serializer_max_time);
   }

   action_result setnick(account_name name, string nickname) {
      return PUSH_ACTION(accounts_name, name, (name)(nickname));
   }

   action_result setpartner(account_name name, bool is_partner = true) {
      return PUSH_ACTION(accounts_name, name, (name)(is_partner));
   }

   action_result connect(name name, struct name service, string auth_token) {
      return PUSH_ACTION(accounts_name, name, (name)(service)(auth_token));
   }

   action_result signin(name name, struct name service, string auth_token) {
      return PUSH_ACTION(accounts_name, name, (name)(service)(auth_token));
   }

};

BOOST_AUTO_TEST_SUITE(gxc_account_tests)

BOOST_FIXTURE_TEST_CASE(setnick_tests, gxc_account_tester) try {

   BOOST_REQUIRE_EQUAL(wasm_assert_msg("nickname has invalid length"),
      setnick(N(conr2d), "short")
   );

   BOOST_REQUIRE_EQUAL(wasm_assert_msg("nickname has invalid length"),
      setnick(N(conr2d), "12341234123412341234123412341234")
   );

   BOOST_REQUIRE_EQUAL(wasm_assert_msg("nickname contains invalid character"),
      setnick(N(conr2d), "c@nrad")
   );

   setnick(N(eun2ce), "eun2cenick");
   REQUIRE_MATCHING_OBJECT(get_table(accounts_name, accounts_name, N(accounts), N(eun2ce)), mvo()
      ("name_", "eun2ce")
      ("nickname", "eun2cenick")
   );
   produce_blocks(1);

   BOOST_REQUIRE_EQUAL(wasm_assert_msg("nickname already taken"),
      setnick(N(conr2d), "eun2cenick")
   );

   BOOST_REQUIRE_EQUAL(wasm_assert_msg("changing nickname not supported yet"),
      setnick(N(eun2ce), "changenick")
   );

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(setpartner_tests, gxc_account_tester) try {

   setnick(N(eun2ce), "eun2cepartner");
   produce_blocks(1);

   setpartner(N(eun2ce));
   produce_blocks(1);

   REQUIRE_MATCHING_OBJECT(get_table(accounts_name, accounts_name, N(accounts), N(eun2ce)), mvo()
      ("name_", "eun2ce......1")
      ("nickname", "eun2cepartner")
   );

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(connect_tests, gxc_account_tester) try {

   setnick(N(eun2ce), "eun2cenick");
   produce_blocks(1);

   setnick(N(conr2d), "conr2dservice");
   setpartner(N(conr2d));
   produce_blocks(2);

   BOOST_REQUIRE_EQUAL(wasm_assert_msg("auth token has invalid length"),
      connect(N(eun2ce), N(conr2d), "hCLfi5J55d311c")
   );

   BOOST_REQUIRE_EQUAL(wasm_assert_msg("auth token is expired"),
      connect(N(eun2ce), N(conr2d), "edEaVd_S6d0lle12")
   );

   BOOST_REQUIRE_EQUAL(success(),
      connect(N(eun2ce), N(conr2d), "h0togj7PFd344d48")
   );

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(signin_tests, gxc_account_tester) try {

   setnick(N(eun2ce), "eun2cenick");
   produce_blocks(1);

   setnick(N(conr2d), "conr2dservice");
   setpartner(N(conr2d));
   produce_blocks(3);

   BOOST_REQUIRE_EQUAL(wasm_assert_msg("auth token has invalid length"),
      signin(N(eun2ce), N(conr2d), "hCLfi5J55d311c")
   );

   BOOST_REQUIRE_EQUAL(wasm_assert_msg("auth token is expired"),
      signin(N(eun2ce), N(conr2d), "edEaVd_S6d0lle12")
   );

   BOOST_REQUIRE_EQUAL(success(),
      signin(N(eun2ce), N(conr2d), "h0togj7PFd344d48")
   );

} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
