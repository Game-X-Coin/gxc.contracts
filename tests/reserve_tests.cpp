#include "account_tester.hpp"

const static name reserve_account_name = N(gxc.reserve);

class gxc_reserve_tester : public gxc_account_tester {
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

   transaction_trace_ptr create_account_with_mulsign(account_name a, account_name creator, const vector<permission_level>& auths) {
      signed_transaction trx;
      set_transaction_headers(trx);

      trx.actions.emplace_back(auths,
                               newaccount{
                                 .creator = creator,
                                 .name = a,
                                 .owner = authority(get_public_key(a, "owner")),
                                 .active = authority(get_public_key(a, "active"))
                               });

      set_transaction_headers(trx);
      for(const auto& auth : auths) {
      trx.sign(get_private_key( auth.actor, auth.permission.to_string()), control->get_chain_id() );
      }
      return push_transaction( trx );
   }

   action_result setminamount(extended_asset value) {
      return PUSH_ACTION(reserve_account_name, reserve_account_name, (value));
   }

   action_result mint(extended_asset derivative, extended_asset underlying, std::vector<option> opts ={}) {
      return PUSH_ACTION(reserve_account_name, basename(derivative.contract), (derivative)(underlying)(opts));
   }

   action_result prepay(account_name creator, account_name name, extended_asset value) {
      return PUSH_ACTION(reserve_account_name, creator, (creator)(name)(value));
   }

   action_result claim(account_name owner, extended_asset value) {
      return PUSH_ACTION(reserve_account_name, owner, (owner)(value));
   }

};

BOOST_AUTO_TEST_SUITE(gxc_reserve_tests)

BOOST_FIXTURE_TEST_CASE(setminamount_tests, gxc_reserve_tester) try {

   gxc_token_tester::mint(EA("10000.0000 GXC@gxc"), false);
   produce_blocks(1);

   BOOST_REQUIRE_EQUAL(wasm_assert_msg("min amount must not be negative"),
         setminamount(EA("-1.0000 GXC@gxc"))
   );

   BOOST_REQUIRE_EQUAL(success(),
         setminamount(EA("1.0000 GXC@gxc"))
   );
   produce_blocks(1);

   REQUIRE_MATCHING_OBJECT(get_table_row(reserve_account_name, reserve_account_name, N(config), N(config)), mvo()
         ("amount", "1.0000 GXC@gxc")
   );

   gxc_token_tester::mint(EA("10000.0000 HOBL@nosys"), false);
   produce_blocks(1);

   BOOST_REQUIRE_EQUAL(wasm_assert_msg("prepaid asset should be system token"),
         setminamount(EA("1.0000 HOBL@nosys"))
   );

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(reserve_mint_tests, gxc_reserve_tester) try {

   set_authority(token_account_name, "active",
      authority(1, vector<key_weight>{{get_private_key("gxc.token", "active").get_public_key(), 1}}, vector<permission_level_weight>{{{N(gxc.reserve), config::eosio_code_name}, 1}, {{N(gxc.token), config::eosio_code_name}, 1}}, {}),
      config::owner_name,
      {{token_account_name, config::active_name}},
      {get_private_key(token_account_name, "active")}
   );

   set_authority(reserve_account_name, "active",
      authority(1, vector<key_weight>{{get_private_key("gxc.reserve", "active").get_public_key(), 1}}, vector<permission_level_weight>{{{N(gxc.reserve), config::eosio_code_name}, 1}}, {}),
      config::owner_name,
      {{reserve_account_name, config::active_name}},
      {get_private_key(reserve_account_name, "active")}
   );
   produce_blocks(1);

   gxc_token_tester::mint(EA("1000000.0000 HOBL@gxc"));
   setminamount(EA("10.0000 HOBL@gxc"));
   produce_blocks(1);

   BOOST_REQUIRE_EQUAL(wasm_assert_msg("only partner account can use reserve"),
      mint(EA("10000.00 GXCP@gamex"), EA("100000.0000 HOBL@gxc"))
   );

   setpartner(N(gamex));
   transfer(config::null_account_name, N(gamex), EA("100000.0000 HOBL@gxc"), "hola");
   approve(N(gamex), N(gxc.reserve), EA("100000.0000 HOBL@gxc"));
   produce_blocks(1);

   BOOST_REQUIRE_EQUAL(wasm_assert_msg("not allowed to set option `mintable`"),
      mint(EA("10000.00 GXCP@gamex"), EA("100000.0000 HOBL@gxc"), {{"mintable", {false}}})
   );

   BOOST_REQUIRE_EQUAL(wasm_assert_msg("underlying asset should be system token"),
      mint(EA("10000.00 GXCP@gamex"), EA("100000.0000 HOBL@conr2d"))
   );

   BOOST_REQUIRE_EQUAL(success(),
      mint(EA("10000.00 GXCP@gamex"), EA("100000.0000 HOBL@gxc"))
   );
   produce_blocks(1);

   REQUIRE_MATCHING_OBJECT(get_table_row(reserve_account_name, N(gamex), N(reserves), symbol(0, "GXCP").to_symbol_code().value), mvo()
      ("derivative", "10000.00 GXCP@gamex")
      ("underlying", "100000.0000 HOBL")
      ("rate", "10.00000000000000000")
   );

   REQUIRE_MATCHING_OBJECT(get_stats("GXCP@gamex"), mvo()
      ("supply", "0.00 GXCP")
      ("max_supply", "10000.00 GXCP")
      ("issuer", "gamex")
      ("opts", 7)
      ("amount", "0.00 GXCP")
      ("duration", 86400)
   );

   BOOST_REQUIRE_EQUAL(wasm_assert_msg("additional issuance not supported yet"),
      mint(EA("1000.00 GXCP@gamex"), EA("100000.0000 HOBL@gxc"))
   );

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(prepay_tests, gxc_reserve_tester) try {

   create_accounts({ N(gamexcoin) });

   set_authority(token_account_name, "active",
      authority(1, vector<key_weight>{{get_private_key("gxc.token", "active").get_public_key(), 1}}, vector<permission_level_weight>{{{N(gxc.reserve), config::eosio_code_name}, 1}, {{N(gxc.token), config::eosio_code_name}, 1}}, {}),
      config::owner_name,
      {{token_account_name, config::active_name}},
      {get_private_key(token_account_name, "active")}
   );

   set_authority(reserve_account_name, "active",
      authority(1, vector<key_weight>{{get_private_key("gxc.reserve", "active").get_public_key(), 1}}, vector<permission_level_weight>{{{N(gxc.reserve), config::eosio_code_name}, 1}}, {}),
      config::owner_name,
      {{reserve_account_name, config::active_name}},
      {get_private_key(reserve_account_name, "active")}
   );
   produce_blocks(1);

   gxc_token_tester::mint(EA("1000.0000 GXC@gxc"), false);
   init(0, symbol(4,"GXC"));
   setminamount(EA("10.0000 GXC@gxc"));
   produce_blocks(1);

   transfer(config::null_account_name, N(gxc), EA("500.0000 GXC@gxc"), "hola");
   transfer(N(gxc), N(gamexcoin), EA("300.0000 GXC@gxc"), "hola");

   BOOST_REQUIRE_EQUAL(success(),
         approve(N(gamexcoin), reserve_account_name, EA("100.0000 GXC@gxc"))
   );
   produce_blocks(1);

   BOOST_REQUIRE_EQUAL(wasm_assert_msg("not enough to prepay"),
      prepay(N(gamexcoin), N(gamco), EA("1.0000 GXC@gxc"))
   );

   BOOST_REQUIRE_EQUAL(success(),
      prepay(N(gamexcoin), N(gamco), EA("100.0000 GXC@gxc"))
   );
   produce_blocks(1);

   REQUIRE_MATCHING_OBJECT(get_table_row(reserve_account_name, reserve_account_name, N(prepaid), N(gamco)), mvo()
      ("creator", "gamexcoin")
      ("name", "gamco")
      ("value", "100.0000 GXC@gxc")
   );

   create_account_with_mulsign(N(gamco), N(gamexcoin), vector<permission_level>{{N(gamexcoin), config::active_name}, {config::system_account_name, config::active_name}});
   produce_blocks(1);

   BOOST_REQUIRE_EQUAL(success(),
      mint(EA("10.00 GXCP@gamco"), EA("100.0000 GXC@gxc"))
   );
   produce_blocks(1);

   REQUIRE_MATCHING_OBJECT(get_table_row(reserve_account_name, N(gamco), N(reserves), symbol(0, "GXCP").to_symbol_code().value), mvo()
      ("derivative", "10.00 GXCP@gamco")
      ("underlying", "100.0000 GXC")
      ("rate", "10.00000000000000000")
   );

   BOOST_REQUIRE_EQUAL(success(),
      transfer(config::null_account_name, N(gamco), EA("10.00 GXCP@gamco"), "hola", N(gamco))
   );
   produce_blocks(1);

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(reserve_claim_tests, gxc_reserve_tester) try {

   set_authority(token_account_name, "active",
      authority(1, vector<key_weight>{{get_private_key("gxc.token", "active").get_public_key(), 1}}, vector<permission_level_weight>{{{N(gxc.reserve), config::eosio_code_name}, 1}, {{N(gxc.token), config::eosio_code_name}, 1}}, {}),
      config::owner_name,
      {{token_account_name, config::active_name}},
      {get_private_key(token_account_name, "active")}
   );

   set_authority(reserve_account_name, "active",
      authority(1, vector<key_weight>{{get_private_key("gxc.reserve", "active").get_public_key(), 1}}, vector<permission_level_weight>{{{N(gxc.reserve), config::eosio_code_name}, 1}}, {}),
      config::owner_name,
      {{reserve_account_name, config::active_name}},
      {get_private_key(reserve_account_name, "active")}
   );
   produce_blocks(1);

   gxc_token_tester::mint(EA("1000000.0000 HOBL@gxc"));
   setminamount(EA("10.0000 HOBL@gxc"));
   produce_blocks(1);

   setpartner(N(gamex));
   transfer(config::null_account_name, N(gamex), EA("100000.0000 HOBL@gxc"), "hola");
   approve(N(gamex), N(gxc.reserve), EA("100000.0000 HOBL@gxc"));
   mint(EA("10000.00 GXCP@gamex"), EA("100000.0000 HOBL@gxc"));
   produce_blocks(1);

   transfer(config::null_account_name, N(gamex), EA("1000.00 GXCP@gamex"), "hola", N(gamex));
   approve(N(gamex), N(gxc.reserve), EA("100.00 GXCP@gamex"));
   produce_blocks(1);

   BOOST_REQUIRE_EQUAL(success(),
      claim(N(gamex), EA("100.00 GXCP@gamex"))
   );
   produce_blocks(1);

   REQUIRE_MATCHING_OBJECT(get_account(N(gamex), "GXCP@gamex"), mvo()
      ("balance", "900.00 GXCP")
      ("issuer_", "gamex")
      ("deposit", "0.00 GXCP")
   );

   REQUIRE_MATCHING_OBJECT(get_stats("GXCP@gamex"), mvo()
      ("supply", "900.00 GXCP")
      ("max_supply", "9900.00 GXCP")
      ("issuer", "gamex")
      ("opts", 7)
      ("amount", "0.00 GXCP")
      ("duration", 86400)
   );

   REQUIRE_MATCHING_OBJECT(get_table_row(reserve_account_name, N(gamex), N(reserves), symbol(0, "GXCP").to_symbol_code().value), mvo()
      ("derivative", "9900.00 GXCP@gamex")
      ("underlying", "99000.0000 HOBL")
      ("rate", "10.00000000000000000")
   );

} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
