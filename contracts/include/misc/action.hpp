#pragma once

#include <eosio/action.hpp>
#include <boost/utility/string_view.hpp>

namespace eosio {

name rootname(name n) {
   auto mask = (uint64_t) -1;
   for (auto i = 0; i < 12; ++i) {
      if (n.value & (0x1FULL << (4 + 5 * (11 - i))))
         continue;
      mask <<= 4 + 5 * (11 - i);
      break;
   }
   return name(n.value & mask);
}

name basename(name n) {
   if (is_account(n))
      return n;
   else
      return rootname(n);
}

inline bool has_vauth(name n) {
   return has_auth(basename(n));
}

inline void require_vauth(name n) {
   require_auth(basename(n));
}

bool starts_with(const name& input, const name& test) {
   uint64_t mask = 0xF800000000000000ull;
   auto maxlen = 12;
   auto v = input.value;
   auto c = test.value;

   for (auto i = 0; i < maxlen; ++i, v <<= 5, c<<= 5) {
      if ((v & mask) == (c & mask)) continue;

      if (c & mask) return false;
      else break;
   }

   return true;
}

bool starts_with(const name& input, const boost::string_view& test) {
   return boost::string_view(input.to_string()).starts_with(test);
}

bool has_dot(name input) {
   uint64_t mask = 0xF800000000000000ull;
   auto v = input.value;
   auto len = input.length();
   auto has_dot = false;

   for (auto i = 0; i < len; ++i, v <<= 5) {
      has_dot |= !(v & mask);
   }
   return has_dot;
}

}
