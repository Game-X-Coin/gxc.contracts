#include "token_tester.hpp"

const static name reserve_account_name = N(gxc.reserve);

class gxc_reserve_tester : public gxc_token_tester {
public:

   gxc_reserve_tester() {
      create_accounts({ reserve_account_name, N(gamex) });
      produce_blocks(1);

      _set_code(reserve_account_name, contracts::reserve_wasm());
      _set_abi(reserve_account_name, contracts::reserve_abi().data());

      auto accnt = control->db().get<account_object, by_name>(reserve_account_name);
      abi_def abi;
      BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi), true);
      abi_ser[reserve_account_name].set_abi(abi, abi_serializer_max_time);
   }

   action_result setminamount(extended_asset value) {
      return PUSH_ACTION(reserve_account_name, reserve_account_name, (value));
   }

   action_result prepay(name creator, name name, extended_asset value) {
      return PUSH_ACTION(reserve_account_name, creator, (creator)(name)(value));
   }

};

BOOST_AUTO_TEST_SUITE(gxc_reserve_tests)

BOOST_FIXTURE_TEST_CASE(setminamount_tests, gxc_reserve_tester) try {

   mint(EA("10000.0000 GXC@gxc"), false);
   produce_blocks(1);

/*
   BOOST_REQUIRE_EQUAL(wasm_assert_msg("invalid symbol"),
         setminamount(EA("1.0000 hobl@gxc"))
   );
*/

   BOOST_REQUIRE_EQUAL(wasm_assert_msg("min amount must not be negative"),
         setminamount(EA("-1.0000 GXC@gxc"))
   );

   BOOST_REQUIRE_EQUAL(success(),
         setminamount(EA("1.0000 GXC@gxc"))
   );

   mint(EA("10000.0000 HOBL@nosys"), false);
   BOOST_REQUIRE_EQUAL(wasm_assert_msg("prepaid asset should be system token"),
         setminamount(EA("1.0000 HOBL@nosys"))
   );
   produce_blocks(1);

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(prepay_tests, gxc_reserve_tester) try {

   mint(EA("10000.0000 GXC@gxc"), false);
   setminamount(EA("10.0000 GXC@gxc"));
   transfer(config::null_account_name, N(gxc), EA("100.0000 GXC@gxc"), "hola");
   approve(N(gxc), reserve_account_name, EA("100.0000 GXC@gxc"));
   produce_blocks(1);

   BOOST_REQUIRE_EQUAL(wasm_assert_msg("not enough to prepay"),
         prepay(N(gxc), N(gamex), EA("1.0000 GXC@gxc"))
   );

   BOOST_REQUIRE_EQUAL(success(),
         prepay(N(gxc), N(gamex), EA("100.0000 GXC@gxc"))
   );
   produce_blocks(1);

   REQUIRE_MATCHING_OBJECT(get_table(reserve_account_name, reserve_account_name, N(prepaid), N(gamex)), mvo()
         ("creator", "gxc")
         ("name", "gamex")
         ("value", "100.0000 GXC@gxc")
   );
   produce_blocks(1);

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(reserve_mint_tests, gxc_reserve_tester) try {
   BOOST_TEST_MESSAGE("not implemented yet");
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(reserve_claim_tests, gxc_reserve_tester) try {
   BOOST_TEST_MESSAGE("not implemented yet");
} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
