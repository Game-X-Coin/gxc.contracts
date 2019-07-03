#pragma once

#include <eosio/eosio.hpp>
#include <eosio/system.hpp>
#include <utf8/utf8.h>
#include <misc/contract_wrapper.hpp>

namespace gxc {

using namespace eosio;
using std::string;

class [[eosio::contract]] account : public contract_wrapper<account> {
public:
   static constexpr name default_account = "gxc.account"_n;

   using contract_wrapper::contract_wrapper;

   static checksum256 string_to_checksum256(string s) {
      std::array<unsigned char,32> raw;
      memcmp((void*)raw.data(), (const void*)s.data(), s.size());
      return checksum256(raw);
   }

   struct [[eosio::table]] accounts {
      name name_;
      string nickname;

      uint64_t primary_key() const { return name_.value & ~0xFULL; }
      checksum256 by_nickname() const { return string_to_checksum256(nickname); }
      uint64_t by_partner() const { return name_.value & 0x1; }

      EOSLIB_SERIALIZE(accounts, (name_)(nickname))
   };
   typedef multi_index<"accounts"_n, accounts,
      indexed_by<"nickname"_n, const_mem_fun<accounts, eosio::checksum256, &accounts::by_nickname>>,
      indexed_by<"partner"_n, const_mem_fun<accounts, uint64_t, &accounts::by_partner>>
   > accounts_index;

   void authenticate(name name, struct name service, const string& auth_token);

   [[eosio::action]]
   void connect(name name, struct name service, string auth_token);

   [[eosio::action]]
   void signin(name name, struct name service, string auth_token);

   [[eosio::action]]
   void setnick(name name, string nickname);

   [[eosio::action]]
   void setpartner(name name, bool is_partner);

   static bool is_partner(name name) {
      accounts_index acc(default_account, default_account.value);
      auto it = acc.find(name.value);
      return (it != acc.end()) ? it->name_.value & 0x1 : false;
   }

   static bool is_valid_nickname(string nickname) {
      if (nickname.empty()) return false;

      auto it = nickname.begin();
      for (auto cp = utf8::unchecked::next(it) ; cp; cp = utf8::unchecked::next(it))
         if (!is_valid_char(cp)) return false;

      return true;
   }

private:
   constexpr static bool is_valid_char(uint32_t cp) {
      if( (cp >= 'A') && (cp <= 'Z') ) return true;
      if( (cp >= 'a') && (cp <= 'z') ) return true;
      if( (cp >= '0') && (cp <= '9') ) return true;
      if( (cp >= 0xAC00) && (cp <= 0xD7AF) ) return true;
      return false;
   }
};

}
