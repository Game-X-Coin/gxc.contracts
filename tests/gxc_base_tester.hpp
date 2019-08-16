#pragma once
#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/chain/abi_serializer.hpp>
#include <type_traits>
#include <typeindex>

using namespace eosio;
using namespace eosio::chain;
using namespace eosio::testing;
using namespace fc;
using namespace std;
using mvo = mutable_variant_object;

struct contract;

class gxc_base_tester : public tester {
public:
   vector<transaction_trace_ptr> create_accounts(
      vector<account_name> names,
      account_name creator = config::system_account_name,
      bool multisig = false,
      bool include_code = true
   ) {
      vector<transaction_trace_ptr> traces;
      traces.reserve(names.size());
      for (auto n: names) {
         traces.emplace_back(create_account(n, creator, multisig, include_code));
      }
      return traces;
   }

   account_name basename(account_name acc) {
      auto rootname = [&](account_name n) -> account_name {
         auto mask = (uint64_t) -1;
         for (auto i = 0; i < 12; ++i) {
            if (n.value & (0x1FULL << (4 + 5 * (11 - i))))
               continue;
            mask <<= 4 + 5 * (11 - i);
            break;
         }
         return name(n.value & mask);
      };

      const auto& accnt = control->db().find<account_object,by_name>(acc);
      if (accnt != nullptr) {
         return acc;
      } else {
         return rootname(acc);
      }
   }

   fc::variant get_table_row(const account_name& code, const account_name& scope, const account_name& table, uint64_t primary_key, const string& type = "") {
      auto data = get_row_by_account(code, scope, table, primary_key);
      return data.empty() ? fc::variant() : abi_ser[code].binary_to_variant(type.empty() ? table.to_string() : type, data, abi_serializer_max_time);
   }

   action_result push_action(const account_name& code, const account_name& acttype, const account_name& actor, const variant_object& data) {
      string action_type_name = abi_ser[code].get_action_type(acttype);

      action act;
      act.account = code;
      act.name    = acttype;
      act.data    = abi_ser[code].variant_to_binary(action_type_name, data, abi_serializer_max_time);

      return base_tester::push_action(std::move(act), uint64_t(actor));
   }

   void set_code(account_name account, const vector<uint8_t> wasm) try {
      vector<permission_level> authorization = {{account, config::active_name}};
      if (account != config::system_account_name) {
         authorization.push_back({config::system_account_name, config::active_name});
      }

      base_tester::push_action(config::system_account_name, N(setcode),
         authorization,
         mvo()
            ("account", account)
            ("vmtype", 0)
            ("vmversion", 0)
            ("code", bytes(wasm.begin(), wasm.end()))
      );
   } FC_CAPTURE_AND_RETHROW((account))

   void set_abi(account_name account, const char* abi_json) {
      vector<permission_level> authorization = {{account, config::active_name}};
      if (account != config::system_account_name) {
         authorization.push_back({config::system_account_name, config::active_name});
      }

      auto abi = fc::json::from_string(abi_json).template as<abi_def>();
      base_tester::push_action(config::system_account_name, N(setabi),
         authorization,
         mvo()
            ("account", account)
            ("abi", fc::raw::pack(abi))
      );
   }

   map<type_index, shared_ptr<contract>> contracts;
   map<account_name, abi_serializer> abi_ser;
};


