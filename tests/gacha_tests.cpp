#include "account_tester.hpp"

struct grade {
   asset                    reward;
   uint32_t                 score;
   fc::optional<uint32_t>  limit;
};

FC_REFLECT(grade, (reward)(score)(limit))

const static name gacha_account_name = N(gxc.gacha);

class gxc_gacha_tester : public gxc_account_tester {
public:
   gxc_gacha_tester() {
      create_accounts({ gacha_account_name });
      produce_blocks(1);

      _set_code(gacha_account_name, contracts::gacha_wasm());
      _set_abi(gacha_account_name, contracts::gacha_abi().data());
      produce_blocks(1);

      auto accnt = control->db().get<account_object,by_name>(gacha_account_name);
      abi_def abi;
      BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi), true);
      abi_ser[gacha_account_name].set_abi(abi, abi_serializer_max_time);

   }

   void subopen(std::array<name,2> scheme, extended_asset budget) {
      gxc_token_tester::mint(budget);
      gxc_account_tester::setpartner(scheme.at(1));
      gxc_token_tester::transfer(N(gxc.null), scheme.at(1), budget, scheme.at(0).to_string());
      gxc_token_tester::approve(scheme.at(1), gacha_account_name, budget);
      produce_blocks(1);
   }

   action_result open(std::array<name,2> scheme, std::vector<grade> &grades, extended_asset budget, const string& expiration, fc::optional<uint8_t> precision, fc::optional<uint32_t> deadline_sec = fc::optional<uint32_t>()) {
      return PUSH_ACTION(gacha_account_name, scheme.at(1), (scheme)(grades)(budget)(expiration)(precision)(deadline_sec));
   }

   action_result issue(account_name to, std::array<name,2> scheme, const string& dseedhash, fc::optional<uint64_t> id = fc::optional<uint64_t>()) {
      return PUSH_ACTION(gacha_account_name, scheme.at(1), (to)(scheme)(dseedhash)(id));
   }

   action_result setoseed(uint64_t id, const string& oseed, account_name owner) {
      return PUSH_ACTION(gacha_account_name, owner, (id)(oseed));
   }

   action_result setdseed(uint64_t id, const string& dseed) {
      return PUSH_ACTION(gacha_account_name, gacha_account_name, (id)(dseed));
   }

   action_result winreward(account_name owner, uint64_t id, int64_t score, extended_asset value) {
      return PUSH_ACTION(gacha_account_name, gacha_account_name, (owner)(id)(score)(value));
   }

   action_result raincheck(account_name owner, uint64_t id, int64_t score) {
      return PUSH_ACTION(gacha_account_name, gacha_account_name, (owner)(id)(score));
   }
};

BOOST_AUTO_TEST_SUITE(gxc_gacha_tests)

BOOST_FIXTURE_TEST_CASE(open_tests, gxc_gacha_tester) try {

   std::array<name, 2> schm = {N(gacha1), N(eun2ce)};
   vector<grade> gd = {{.reward = asset::from_string("5000.00 ENC"), .score = 1, .limit = 0}};
   string expiration = "2020-07-28T12:43:00";

   subopen(schm, EA("5000.00 ENC@eun2ce"));
   BOOST_REQUIRE_EQUAL(success(),
         open(schm, gd, EA("5000.00 ENC@eun2ce"), expiration, 1)
   );

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(issue_tests, gxc_gacha_tester) try {

   vector<grade> gd = {{.reward = asset::from_string("5000.00 ENC"), .score = 1, .limit = 0}};
   string expiration = "2018-01-01T00:00:10";
   string dseedhash = "4855c552bbdd69cef5faee9bcc16a2f6a6b9d5f04560fa6a4045a5195c1f4f75";

   BOOST_REQUIRE_EQUAL(wasm_assert_msg("scheme not found"),
         issue(N(conr2d), {N(gacha123), N(eun2ce)}, dseedhash)
   );

   subopen({N(gacha2), N(eun2ce)}, EA("5000.00 ENC@eun2ce"));
   open({N(gacha2), N(eun2ce)}, gd, EA("5000.00 ENC@eun2ce"), expiration, 1);
   produce_blocks(1);

   BOOST_REQUIRE_EQUAL(wasm_assert_msg("scheme expired"),
         issue(N(conr2d), {N(gacha2), N(eun2ce)}, dseedhash)
   );

   expiration = "2022-01-01T00:00:10";
   subopen({N(gacha3), N(conr2d)}, EA("5000.00 CRD@conr2d"));
   open({N(gacha3), N(conr2d)}, gd, EA("5000.00 CRD@conr2d"), expiration, 1);
   produce_blocks(1);

   BOOST_REQUIRE_EQUAL(success(),
         issue(N(eun2ce), {N(gacha3), N(conr2d)}, dseedhash)
   );

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(setoseed_tests, gxc_gacha_tester) try {

   vector<grade> gd = {{.reward = asset::from_string("5000.00 ENC"), .score = 1, .limit = 0}};
   string expiration = "2022-01-01T00:00:10";
   string dseedhash = "4855c552bbdd69cef5faee9bcc16a2f6a6b9d5f04560fa6a4045a5195c1f4f75";
   string oseed = "4c519413ac98e5ead1c3b412e5d053ba0d57245e7689e6fcbe6dd9b81aa88dd7";

   subopen({N(gachatest), N(eun2ce)}, EA("5000.00 ENC@eun2ce"));
   open({N(gachatest), N(eun2ce)}, gd, EA("5000.00 ENC@eun2ce"), expiration, 1);
   issue(N(conr2d), {N(gachatest), N(eun2ce)}, dseedhash, 1);
   produce_blocks(1);

   BOOST_REQUIRE_EQUAL(success(),
         setoseed(1, oseed, N(conr2d))
   );
   produce_blocks(1);

   string scmstr = "{\"scheme_name\": \"gachatest\", \"grades\": [{\"reward\": \"5000.00 ENC\", \"score\": 1, \"limit\": 0}], \"budget\": \"5000.00 ENC@eun2ce\", \"expiration\": \"2022-01-01T00:00:10\", \"precision\": 1, \"deadline_sec\": 604800, \"out\": \"0.00 ENC\", \"out_count\": [0], \"issued\": 1, \"unresolved\": 1}";
   string gchstr = "{\"id\": 1, \"owner\": \"conr2d\", \"scheme\": {\"name\": \"gachatest\", \"contract\": \"eun2ce\"}, \"dseedhash\": \"4855c552bbdd69cef5faee9bcc16a2f6a6b9d5f04560fa6a4045a5195c1f4f75\", \"oseed\": \"4c519413ac98e5ead1c3b412e5d053ba0d57245e7689e6fcbe6dd9b81aa88dd7\", \"deadline\": \"2020-01-08T00:00:11\"}";

   BOOST_REQUIRE_EQUAL(scmstr,
         std::to_string(get_table_row(gacha_account_name, N(eun2ce), N(scheme), N(gachatest)))
   );

   BOOST_REQUIRE_EQUAL(gchstr,
         std::to_string(get_table_row(gacha_account_name, gacha_account_name, N(gacha), 1, "_gacha"))
   );

   oseed = "4c519413ac98e5ead1c3b412e5d053ba0d57245e7689e6fcbe6dd9b81aa88dd6";
   BOOST_REQUIRE_EQUAL(wasm_assert_msg("oseed is already set"),
         setoseed(1, oseed, N(conr2d))
   );

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(setdseed_tests, gxc_gacha_tester) try {

   vector<grade> gd = {{.reward = asset::from_string("5000.00 ENC"), .score = 1, .limit = 0}};
   string expiration = "2022-01-01T00:00:10";
   string dseedhash = "4855c552bbdd69cef5faee9bcc16a2f6a6b9d5f04560fa6a4045a5195c1f4f75";
   string oseed = "1ba0668f40b6fdd0a2b553a8e5dfe57ee977506532ba437f31f1b72abc199dbe";

   subopen({N(gachatest), N(eun2ce)}, EA("5000.00 ENC@eun2ce"));
   open({N(gachatest), N(eun2ce)}, gd, EA("5000.00 ENC@eun2ce"), expiration, 1);
   issue(N(conr2d), {N(gachatest), N(eun2ce)}, dseedhash, 1);
   setoseed(1, oseed, N(conr2d));
   produce_blocks(1);

   BOOST_REQUIRE_EQUAL(success(),
      setdseed(1, "4c519413ac98e5ead1c3b412e5d053ba0d57245e7689e6fcbe6dd9b81aa88dd7")
   );

} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
