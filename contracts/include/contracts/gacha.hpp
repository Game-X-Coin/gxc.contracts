#pragma once

#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/crypto.hpp>

#include <sio4/crypto/xxhash.hpp>
#include <sio4/bytes.hpp>
#include <misc/name.hpp>
#include <misc/action.hpp>

namespace gxc {

using namespace eosio;
using namespace std;
using bytes = sio4::bytes;

class [[eosio::contract]] gacha : public contract {
public:
   using contract::contract;

   struct grade {
      asset               reward;
      uint32_t            score;
      optional<uint32_t>  limit;

      EOSLIB_SERIALIZE(grade, (reward)(score)(limit))
   };

   struct [[eosio::table]] scheme {
      name              scheme_name;
      vector<grade>     grades;
      extended_asset    budget;
      time_point_sec    expiration;
      uint8_t           precision;
      uint32_t          deadline_sec;
      asset             out;
      vector<uint32_t>  out_count;
      uint32_t          issued = 0;
      uint32_t          unresolved = 0;

      uint64_t primary_key()const { return scheme_name.value; }

      EOSLIB_SERIALIZE(scheme, (scheme_name)(grades)(budget)(expiration)(precision)(deadline_sec)(out)(out_count)(issued)(unresolved))
   };

   typedef multi_index<"scheme"_n, scheme> scheme_index;

   struct [[eosio::table("gacha")]] _gacha {
      uint64_t        id;
      name            owner;
      extended_name   scheme;
      checksum256     dseedhash;
      checksum256     oseed = {};
      time_point_sec  deadline = time_point_sec::maximum();

      static uint64_t hash(extended_name scheme, const checksum256& dseedhash) {
         std::array<char,2*8+32> data;
         datastream<char*> ds(data.data(), data.size());
         ds << scheme;
         ds << dseedhash.extract_as_byte_array();
         return sio4::xxh64(data.data(), data.size());
      }

      uint64_t primary_key() const { return id; }
      uint64_t by_owner() const { return owner.value; }
      uint64_t by_deadline() const { return static_cast<uint64_t>(deadline.utc_seconds); }

      EOSLIB_SERIALIZE(_gacha, (id)(owner)(scheme)(dseedhash)(oseed)(deadline))
   };

   typedef multi_index<"gacha"_n, _gacha,
              indexed_by<"owner"_n, const_mem_fun<_gacha, uint64_t, &_gacha::by_owner>>,
              indexed_by<"deadline"_n, const_mem_fun<_gacha, uint64_t, &_gacha::by_deadline>>
           > gacha_index;

   [[eosio::action]]
   void close(extended_name scheme);

   [[eosio::action]]
   void open(extended_name scheme, std::vector<grade> &grades, extended_asset budget, time_point_sec expiration, optional<uint8_t> precision, optional<uint32_t> deadline_sec);

   [[eosio::action]]
   void issue(name to, extended_name scheme, checksum256 dseedhash, optional<uint64_t> id);

   [[eosio::action]]
   void setoseed(uint64_t id, checksum256 oseed);

   [[eosio::action]]
   void setdseed(uint64_t id, checksum256 dseed);

   [[eosio::action]]
   void winreward(name owner, uint64_t id, int64_t score, extended_asset value);

   [[eosio::action]]
   void raincheck(name owner, uint64_t id, int64_t score);

   [[eosio::action]]
   void resolve();

   void refresh_schedule();
   void resolve_one(uint64_t id);
   void draw(uint64_t id, optional<checksum256> dseed = nullopt);
};

}
