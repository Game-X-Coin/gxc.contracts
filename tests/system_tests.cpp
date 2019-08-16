#include "gxc_tester.hpp"

using gxc_system_tester = gxc_tester<token_contract, system_contract>;

BOOST_AUTO_TEST_SUITE(gxc_system_tests)

BOOST_FIXTURE_TEST_CASE(init_tests, gxc_system_tester) try {
   C(token).mint(EA("1000000000.0000 GXC@gxc"), false);
   produce_blocks(1);

   BOOST_REQUIRE_EQUAL(wasm_assert_msg("unsupported version for init action"),
      C(system).init(3, symbol(4, "GXC"))
   );

   BOOST_REQUIRE_EQUAL(wasm_assert_msg("specified core symbol does not exist (precision mismatch)"),
      C(system).init(0, symbol(2, "GXC"))
   );

   BOOST_REQUIRE_EQUAL(success(),
      C(system).init(0, symbol(4, "GXC"))
   );
   produce_blocks(1);

   BOOST_REQUIRE_EQUAL(wasm_assert_msg("system contract has already been initialized"),
      C(system).init(0, symbol(4, "GXC"))
   );

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(genaccount_test, gxc_system_tester) try {

   create_accounts({ N(gxc.account) });
   authority auth = authority(1, vector<key_weight>{{get_private_key("gxc","active").get_public_key(), 1}}, {}, {});
   produce_blocks(1);

   BOOST_REQUIRE_EQUAL(success(),
      C(system).genaccount(config::system_account_name, N(gamex), auth, auth, "eun2cenick")
   );

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(setprods_test, gxc_system_tester) try {

   vector<account_name> producers = {"proda", "prodb", "prodc", "prodd"};
   create_accounts(producers);
   produce_blocks(1);

   BOOST_REQUIRE_EQUAL(success(),
      C(system).setprods(get_producer_keys(producers))
   );

} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
