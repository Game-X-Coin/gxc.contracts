#include <contracts/account.hpp>
#include <contracts/system.hpp>

namespace gxc {

void account::authenticate(name name, struct name service, const string& auth_token) {
   check(auth_token.size() == 16, "auth token has invalid length");
   require_auth(name);

   auto expiration = strtoul(auth_token.substr(8).data(), nullptr, 16);
   check(current_time_point().sec_since_epoch() <= expiration, "auth token is expired");
}

void account::connect(name name, struct name service, string auth_token) {
   authenticate(name, service, auth_token);
}

void account::signin(name name, struct name service, string auth_token) {
   authenticate(name, service, auth_token);
}

void account::setnick(name name, string nickname) {
   check(nickname.size() >= 6 && nickname.size() <= 24, "nickname has invalid length");
   check(is_valid_nickname(nickname), "nickname contains invalid character");

   check(has_auth(system::default_account) || has_auth(name), "missing required authority");

   accounts_index acc(_self, _self.value);

   const auto& idx = acc.get_index<"nickname"_n>();
   auto dup = idx.find(string_to_checksum256(nickname));
   check(dup == idx.end(), "nickname already taken");

   auto cb = [&](auto& a) {
      a.name_.value = (a.name_.value & 0xFULL) | name.value;
      a.nickname = nickname;
   };

   auto it = acc.find(name.value);

   if (it == acc.end()) {
      acc.emplace(_self, cb);
   } else {
      check(has_auth(system::default_account), "changing nickname not supported yet");
      acc.modify(it, same_payer, cb);
   }
}

void account::setpartner(name name, bool is_partner) {
   accounts_index acc(_self, _self.value);

   auto it = acc.find(name.value);
   check(it != acc.end(), "not registered account");

   acc.modify(it, same_payer, [&](auto& a) {
      check((a.name_.value & 0x1) != is_partner, "already set given value");
      a.name_.value = (a.name_.value & ~0xFULL) | is_partner;
   });
}

}
