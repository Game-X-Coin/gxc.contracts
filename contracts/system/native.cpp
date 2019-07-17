#include <contracts/system.hpp>
#include <eosio/crypto.hpp>

namespace gxc {

void system::onblock(ignore<block_header> header) {
   require_auth(_self);

   block_timestamp timestamp;
   _ds >> timestamp;

   _gstate.last_block_num = timestamp;
}

void system::newaccount(name creator, name name, ignore<authority> owner, ignore<authority> active) {
   require_auth(_self);

   if (creator != _self) {
      check(name.length() >= 6, "a name shorter than 6 is reserved");
      check(!has_dot(name), "user account name cannot contain dot");

      user_resources_table userres(_self, name.value);

      userres.emplace(name, [&](auto& res) {
         res.owner = name;
         res.net_weight = asset(0, get_core_symbol());
         res.cpu_weight = asset(0, get_core_symbol());
      });

      eosio::set_resource_limits(name, 0 + ram_gift_bytes, 0, 0);
   }
}

void system::setabi(name account, const std::vector<char>& abi) {
   require_auth(_self);

   eosio::multi_index<"abihash"_n, abi_hash> table(_self, _self.value);

   auto itr = table.find(account.value);
   if (itr == table.end()) {
      table.emplace(account, [&](auto& row) {
         row.owner = account;
         auto hash = eosio::sha256(const_cast<char*>(abi.data()), abi.size()).extract_as_byte_array();
         std::copy(hash.begin(), hash.end(), row.hash.begin());
      });
   } else {
      table.modify(itr, same_payer, [&](auto& row) {
         auto hash = eosio::sha256(const_cast<char*>(abi.data()), abi.size()).extract_as_byte_array();
         std::copy(hash.begin(), hash.end(), row.hash.begin());
      });
   }
}

void system::setcode(name account, uint8_t vmtype, uint8_t vmversion, const std::vector<char>& code) {
   require_auth(_self);
}

}
