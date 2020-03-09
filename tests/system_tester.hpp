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
