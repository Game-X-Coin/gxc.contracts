#include "token_tester.hpp"

class gxc_system_tester : public gxc_token_tester {
public:

   gxc_system_tester() {
      const auto& accnt = control->db().get<account_object,by_name>(config::system_account_name);
      abi_def abi;
      BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi), true);
      abi_ser[config::system_account_name].set_abi(abi, abi_serializer_max_time);
      produce_blocks(1);
   }

   action_result init(unsigned_int version, symbol core) {
      return PUSH_ACTION(config::system_account_name, config::system_account_name, (version)(core));
   }

   action_result genaccount(account_name creator, account_name name, authority owner, authority active, std::string nickname) {
      return PUSH_ACTION(config::system_account_name, creator, (creator)(name)(owner)(active)(nickname));
   }

   action_result setprods(vector<producer_key> schedule) {
      return PUSH_ACTION(config::system_account_name, config::system_account_name, (schedule));
   }
};

BOOST_AUTO_TEST_SUITE(gxc_system_tests)

BOOST_FIXTURE_TEST_CASE(init_tests, gxc_system_tester) try {

   gxc_token_tester::mint(EA("1000000000.0000 GXC@gxc"), false);
   produce_blocks(1);

   BOOST_REQUIRE_EQUAL(wasm_assert_msg("unsupported version for init action"),
      init(3, symbol(4, "GXC"))
   );

   BOOST_REQUIRE_EQUAL(wasm_assert_msg("specified core symbol does not exist (precision mismatch)"),
      init(0, symbol(2, "GXC"))
   );

   BOOST_REQUIRE_EQUAL(success(),
      init(0, symbol(4, "GXC"))
   );
   produce_blocks(1);

   BOOST_REQUIRE_EQUAL(wasm_assert_msg("system contract has already been initialized"),
      init(0, symbol(4, "GXC"))
   );

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(genaccount_test, gxc_system_tester) try {

   create_accounts({ N(gxc.account) });
   authority auth = authority(1, vector<key_weight>{{get_private_key("gxc","active").get_public_key(), 1}}, {}, {});
   produce_blocks(1);

   BOOST_REQUIRE_EQUAL(success(),
      genaccount(config::system_account_name, N(gamex), auth, auth, "eun2cenick")
   );

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(setprods_test, gxc_system_tester) try {

   vector<account_name> producers = {"proda", "prodb", "prodc", "prodd"};
   create_accounts(producers);
   produce_blocks(1);

   BOOST_REQUIRE_EQUAL(success(),
      setprods(get_producer_keys(producers))
   );

} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
