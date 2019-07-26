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

   action_result setnick(account_name _name, string nickname) {
      return PUSH_ACTION(accounts_name, _name, (_name)(nickname));
   }

   action_result setpartner(account_name _name, bool is_partner = true) {
      return PUSH_ACTION(accounts_name, accounts_name, (_name)(is_partner));
   }

   action_result connect(account_name _name, name service, string auth_token) {
      return PUSH_ACTION(accounts_name, _name, (_name)(service)(auth_token));
   }

   action_result signin(account_name _name, name service, string auth_token) {
      return PUSH_ACTION(accounts_name, _name, (_name)(service)(auth_token));
   }

};

