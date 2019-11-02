#pragma once

#include <functional>
#include <eostd/symbol.hpp>
#include <eostd/crypto/xxhash.hpp>

namespace std {

template<>
struct hash<eostd::extended_symbol_code> {
   uint64_t operator()(const eostd::extended_symbol_code& symbol) {
      auto raw = symbol.raw();
      return eostd::xxh64(reinterpret_cast<const char*>(&raw), sizeof(raw));
   }
   uint64_t operator()(eostd::extended_symbol_code&& symbol) {
      auto raw = symbol.raw();
      return eostd::xxh64(reinterpret_cast<const char*>(&raw), sizeof(raw));
   }
};

template<std::size_t N>
struct hash<std::array<char,N>> {
   uint64_t operator()(const std::array<char,N>& raw) {
      return eostd::xxh64(raw.data(), raw.size());
   }
   uint64_t operator()(std::array<char,N>&& raw) {
      return eostd::xxh64(raw.data(), raw.size());
   }
};

template<>
struct hash<std::string> {
   uint64_t operator()(const std::string& s) {
      return eostd::xxh64(s.data(), s.size());
   }
   uint64_t operator()(std::string&& s) {
      return eostd::xxh64(s.data(), s.size());
   }
};

}
