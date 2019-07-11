#pragma once

#include <functional>
#include <sio4/symbol.hpp>
#include <sio4/crypto/xxhash.hpp>

namespace std {

template<>
struct hash<sio4::extended_symbol_code> {
   uint64_t operator()(const sio4::extended_symbol_code& symbol) {
      auto raw = symbol.raw();
      return sio4::xxh64(reinterpret_cast<const char*>(&raw), sizeof(raw));
   }
   uint64_t operator()(sio4::extended_symbol_code&& symbol) {
      auto raw = symbol.raw();
      return sio4::xxh64(reinterpret_cast<const char*>(&raw), sizeof(raw));
   }
};

template<std::size_t N>
struct hash<std::array<char,N>> {
   uint64_t operator()(const std::array<char,N>& raw) {
      return sio4::xxh64(raw.data(), raw.size());
   }
   uint64_t operator()(std::array<char,N>&& raw) {
      return sio4::xxh64(raw.data(), raw.size());
   }
};

template<>
struct hash<std::string> {
   uint64_t operator()(const std::string& s) {
      return sio4::xxh64(s.data(), s.size());
   }
   uint64_t operator()(std::string&& s) {
      return sio4::xxh64(s.data(), s.size());
   }
};

}
