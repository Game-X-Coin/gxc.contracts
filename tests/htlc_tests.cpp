#include "token_tester.hpp"

const static name htlc_account_name = N(gxc.htlc);
const static name vault_account_name = N(gxc.vault);

class gxc_htlc_tester : public gxc_token_tester {
public:

   gxc_htlc_tester() {
      create_accounts({ htlc_account_name, vault_account_name });
      produce_blocks(1);

      _set_code(htlc_account_name, contracts::htlc_wasm());
      _set_abi(htlc_account_name, contracts::htlc_abi().data());
      produce_blocks(1);

      set_authority(config::system_account_name, N(token), authority({htlc_account_name, config::eosio_code_name}), config::owner_name,
         {{config::system_account_name, config::owner_name}},
         {get_private_key(config::system_account_name, "active")}
      );

      base_tester::push_action(config::system_account_name, N(linkauth), config::system_account_name, mvo()
         ("account", name(config::system_account_name))
         ("code", token_account_name)
         ("type", name("transfer"))
         ("requirement", name("token"))
      );

      auto accnt = control->db().get<account_object,by_name>(htlc_account_name);
      abi_def abi;
      BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi), true);
      abi_ser[htlc_account_name].set_abi(abi, abi_serializer_max_time);

      mint(EA("1000000000.0000 GXC@gxc"));
      produce_blocks(2);
   }

   fc::variant get_htlc(const account_name& owner, const string& contract_name) {
      vector<char> data = get_row_by_account(htlc_account_name, owner, N(htlc), XXH64(contract_name.data(), contract_name.size(), 0));
      return data.empty() ? fc::variant() : abi_ser[htlc_account_name].binary_to_variant("lock_contract", data, abi_serializer_max_time);
   }

   action_result newcontract(account_name owner, const string& contract_name, std::array<string,2> recipient, extended_asset value, const string& hashlock, const string& timelock) {
      return PUSH_ACTION(htlc_account_name, owner, (owner)(contract_name)(recipient)(value)(hashlock)(timelock));
   }

   action_result withdraw(account_name owner, const string& contract_name, key256_t preimage) {
      return PUSH_ACTION(htlc_account_name, owner, (owner)(contract_name)(preimage));
   }

   action_result refund(account_name owner, const string& contract_name) {
      return PUSH_ACTION(htlc_account_name, owner, (owner)(contract_name));
   }

   action_result setconfig(extended_asset min_amount, uint32_t min_duration) {
      return PUSH_ACTION(htlc_account_name, min_amount.contract, (min_amount)(min_duration));
   }
};

BOOST_AUTO_TEST_SUITE(gxc_htlc_tests)

BOOST_FIXTURE_TEST_CASE(newcontract_tests, gxc_htlc_tester) try {
   string contract_name = "0x30aad1038d0e503e85c3d4b5b5c7130f548a5e82b3b3cbf2b1ec33925128a89c";
   std::array<string,2> recipient = {"name", "conr2d"};
   string hashlock = "0b671e154e4bff76508922c6e68a3b331a00dcd94e7e97ada243735df42c5360";
   string timelock = "2021-01-01T00:00:00";

   newcontract(vault_account_name, contract_name, recipient, EA("1000.0000 GXC@gxc"), hashlock, timelock);
   REQUIRE_MATCHING_OBJECT(get_htlc(vault_account_name, contract_name), mvo()
      ("owner", "gxc.vault")
      ("contract_name", contract_name)
      ("recipient", recipient)
      ("value", "1000.0000 GXC@gxc")
      ("hashlock", hashlock)
      ("timelock", timelock)
   );
   produce_blocks(1);

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(withdraw_tests, gxc_htlc_tester) try {
   BOOST_TEST_MESSAGE("not implemented yet");
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(refund_tests, gxc_htlc_tester) try {
   BOOST_TEST_MESSAGE("not implemented yet");
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(setconfig_tests, gxc_htlc_tester) try {
   BOOST_TEST_MESSAGE("not implemented yet");
} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
