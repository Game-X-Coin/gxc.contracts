#include "account_tester.hpp"
#include "util.hpp"

struct extended_name {
   name name;
   struct name contract;
};

FC_REFLECT(extended_name, (name)(contract))

struct grade {
   asset                    reward;
   uint32_t                 score;
   fc::optional<uint32_t>   limit;
};

namespace fc {
   inline void to_variant(const grade& var, fc::variant& vo) {
      vo = fc::mutable_variant_object()("reward", var.reward)("score", var.score)("limit", var.limit ? *var.limit : fc::variant());
   }
}
FC_REFLECT(grade, (reward)(score)(limit))

std::ostream& operator<<( std::ostream& osm, const fc::mutable_variant_object& v ) {
   return osm << variant_object(v);
}

namespace boost { namespace test_tools { namespace tt_detail {
   template<>
   struct print_log_value<fc::mutable_variant_object> {
      void operator()( std::ostream& osm, const fc::mutable_variant_object& v )
      {
         ::operator<<(osm, v);
      }
   };
} } }

bool operator == (const variant_object& a, const variant_object& b) {
   return std::equal(a.begin(), a.end(), b.begin(), [](const auto& l, const auto& r) -> bool {
      if(l.key() != r.key()) return false;
      if(l.value().is_object() || r.value().is_object()) {
         return l.value().get_object() == r.value().get_object();
      } else if((l.value().is_array() && l.value().get_array()[0].is_object()) ||
                (r.value().is_array() && r.value().get_array()[0].is_object())) {
         auto la = l.value().get_array();
         auto ra = r.value().get_array();
         return std::equal(la.begin(), la.end(), ra.begin(), [](const auto& lhs, const auto& rhs) -> bool {
            return lhs.get_object() == rhs.get_object();
         });
      } else if (l.value().is_null() && r.value().is_null()) {
         return true;
      }
      return l == r;
   });
}

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

   void subopen(extended_name scheme, extended_asset budget) {
      gxc_token_tester::mint(budget);
      gxc_account_tester::setpartner(scheme.contract);
      gxc_token_tester::transfer(N(gxc.null), scheme.contract, budget, scheme.name.to_string());
      gxc_token_tester::approve(scheme.contract, gacha_account_name, budget);
      produce_blocks(1);
   }

   action_result open(extended_name scheme, std::vector<grade> &grades, extended_asset budget, const string& expiration, fc::optional<uint8_t> precision, fc::optional<uint32_t> deadline_sec = fc::optional<uint32_t>()) {
      return PUSH_ACTION(gacha_account_name, scheme.contract, (scheme)(grades)(budget)(expiration)(precision)(deadline_sec));
   }

   action_result issue(account_name to, extended_name scheme, const string& dseedhash, fc::optional<uint64_t> id = fc::optional<uint64_t>()) {
      return PUSH_ACTION(gacha_account_name, scheme.contract, (to)(scheme)(dseedhash)(id));
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

   extended_name schm = {.name = N(gacha1), .contract = N(eun2ce)};
   vector<grade> gd = {{.reward = asset::from_string("5000.00 ENC"), .score = 1, .limit = fc::optional<uint32_t>()}};
   string expiration = "2020-07-28T12:43:00";

   subopen(schm, EA("5000.00 ENC@eun2ce"));
   BOOST_REQUIRE_EQUAL(success(),
      open(schm, gd, EA("5000.00 ENC@eun2ce"), expiration, 1)
   );
   produce_blocks(1);

   BOOST_REQUIRE_EQUAL(true, get_table_row(gacha_account_name, N(eun2ce), N(scheme), N(gacha1)).get_object() == variant_object(mvo()
      ("scheme_name", "gacha1")
      ("grades", gd)
      ("budget", "5000.00 ENC@eun2ce")
      ("expiration", "2020-07-28T12:43:00")
      ("precision", 1)
      ("deadline_sec", 604800)
      ("out", "0.00 ENC")
      ("out_count", vector<uint32_t>{0})
      ("issued", 0)
      ("unresolved", 0))
   );

   REQUIRE_MATCHING_OBJECT(get_account(gacha_account_name, "ENC@eun2ce"), mvo()
      ("balance", "5000.00 ENC")
      ("issuer_", "eun2ce")
   );

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(issue_tests, gxc_gacha_tester) try {

   extended_name schm = {.name = N(gacha2), .contract = N(eun2ce)};
   vector<grade> gd = {{.reward = asset::from_string("5000.00 ENC"), .score = 1, .limit = fc::optional<uint32_t>()}};
   string expiration = "2018-01-01T00:00:10";
   string dseedhash = "4855c552bbdd69cef5faee9bcc16a2f6a6b9d5f04560fa6a4045a5195c1f4f75";

   BOOST_REQUIRE_EQUAL(wasm_assert_msg("scheme not found"),
      issue(N(conr2d), {.name = N(gacha123), .contract = N(eun2ce)}, dseedhash)
   );

   subopen(schm, EA("5000.00 ENC@eun2ce"));
   open(schm, gd, EA("5000.00 ENC@eun2ce"), expiration, 1);
   produce_blocks(1);

   BOOST_REQUIRE_EQUAL(wasm_assert_msg("scheme expired"),
      issue(N(conr2d), schm, dseedhash)
   );

   extended_name schm2 = {.name = N(gacha3), .contract = N(conr2d)};
   expiration = "2022-01-01T00:00:10";
   subopen(schm2, EA("5000.00 CRD@conr2d"));
   open(schm2, gd, EA("5000.00 CRD@conr2d"), expiration, 1);
   produce_blocks(1);


   BOOST_REQUIRE_EQUAL(success(),
      issue(N(eun2ce), schm2, dseedhash)
   );
   produce_blocks(1);

   BOOST_REQUIRE_EQUAL(true, get_table_row(gacha_account_name, N(conr2d), N(scheme), N(gacha3)).get_object() == variant_object(mvo()
      ("scheme_name", "gacha3")
      ("grades", gd)
      ("budget", "5000.00 CRD@conr2d")
      ("expiration", "2022-01-01T00:00:10")
      ("precision", 1)
      ("deadline_sec", 604800)
      ("out", "0.00 CRD")
      ("out_count", vector<uint32_t>{0})
      ("issued", 1)
      ("unresolved", 0))
   );

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(setoseed_tests, gxc_gacha_tester) try {

   extended_name schm = {.name = N(gachatest), .contract = N(eun2ce)};
   vector<grade> gd = {{.reward = asset::from_string("5000.00 ENC"), .score = 1, .limit = fc::optional<uint32_t>()}};
   string expiration = "2022-01-01T00:00:10";
   string dseedhash = "4855c552bbdd69cef5faee9bcc16a2f6a6b9d5f04560fa6a4045a5195c1f4f75";
   string oseed = "4c519413ac98e5ead1c3b412e5d053ba0d57245e7689e6fcbe6dd9b81aa88dd7";

   subopen(schm, EA("5000.00 ENC@eun2ce"));
   open(schm, gd, EA("5000.00 ENC@eun2ce"), expiration, 1);
   issue(N(conr2d), schm, dseedhash, 1);
   produce_blocks(1);

   BOOST_REQUIRE_EQUAL(success(),
      setoseed(1, oseed, N(conr2d))
   );
   produce_blocks(1);

   BOOST_REQUIRE_EQUAL(true, get_table_row(gacha_account_name, N(eun2ce), N(scheme), N(gachatest)).get_object() == variant_object(mvo()
      ("scheme_name", "gachatest")
      ("grades", gd)
      ("budget", "5000.00 ENC@eun2ce")
      ("expiration", "2022-01-01T00:00:10")
      ("precision", 1)
      ("deadline_sec", 604800)
      ("out", "0.00 ENC")
      ("out_count", vector<uint32_t>{0})
      ("issued", 1)
      ("unresolved", 1))
   );

   BOOST_REQUIRE_EQUAL(true, get_table_row(gacha_account_name, gacha_account_name, N(gacha), 1, "_gacha").get_object() == variant_object(mvo()
      ("id", 1)
      ("owner", "conr2d")
      ("scheme", schm)
      ("dseedhash", dseedhash)
      ("oseed", oseed)
      ("deadline", "2020-01-08T00:00:11"))
   );

   oseed = "4c519413ac98e5ead1c3b412e5d053ba0d57245e7689e6fcbe6dd9b81aa88dd6";
   BOOST_REQUIRE_EQUAL(wasm_assert_msg("oseed is already set"),
      setoseed(1, oseed, N(conr2d))
   );

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(setdseed_tests, gxc_gacha_tester) try {

   extended_name schm = {.name = N(gachatest), .contract = N(eun2ce)};
   vector<grade> gd = {{.reward = asset::from_string("1000.00 ENC"), .score= 2, .limit = fc::optional<uint32_t>()},{.reward = asset::from_string("3000.00 ENC"), .score = 1, .limit = fc::optional<uint32_t>()}};
   string expiration = "2022-01-01T00:00:10";
   string dseedhash = "4855c552bbdd69cef5faee9bcc16a2f6a6b9d5f04560fa6a4045a5195c1f4f75";
   string oseed = "1ba0668f40b6fdd0a2b553a8e5dfe57ee977506532ba437f31f1b72abc199dbe";

   subopen(schm, EA("5000.00 ENC@eun2ce"));
   open(schm, gd, EA("5000.00 ENC@eun2ce"), expiration, 1);
   issue(N(conr2d), schm, dseedhash, 1);
   setoseed(1, oseed, N(conr2d));
   produce_blocks(1);

   BOOST_REQUIRE_EQUAL(success(),
      setdseed(1, "4c519413ac98e5ead1c3b412e5d053ba0d57245e7689e6fcbe6dd9b81aa88dd7")
   );
   produce_blocks(1);

   BOOST_REQUIRE_EQUAL(true, get_table_row(gacha_account_name, N(eun2ce), N(scheme), N(gachatest)).get_object() == variant_object(mvo()
      ("scheme_name", "gachatest")
      ("grades", gd)
      ("budget", "5000.00 ENC@eun2ce")
      ("expiration", "2022-01-01T00:00:10")
      ("precision", 1)
      ("deadline_sec", 604800)
      ("out", "1000.00 ENC")
      ("out_count", vector<uint32_t>{1, 0})
      ("issued", 1)
      ("unresolved", 0))
   );

} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
