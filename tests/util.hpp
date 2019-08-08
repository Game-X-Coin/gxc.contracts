#pragma once
#include <eosio/chain/asset.hpp>

namespace eosio { namespace chain {

struct extended_symbol_code {
   symbol_code code;
   account_name contract;
};

inline extended_asset string_to_extended_asset(const string& s) {
   auto at_pos = s.find('@');
   return extended_asset(asset::from_string(s.substr(0, at_pos)), s.substr(at_pos+1));
}

inline extended_symbol_code string_to_extended_symbol_code(const string& s) {
   auto at_pos = s.find('@');
   return {symbol(0, s.substr(0, at_pos).data()).to_symbol_code(), s.substr(at_pos+1)};
}
#define EA(s) string_to_extended_asset(s)
#define SC(s) string_to_extended_symbol_code(s)
} }

FC_REFLECT(eosio::chain::extended_symbol_code, (code)(contract))

namespace std {

inline std::string to_string(const fc::variant& v) {
   if( v.is_object() ) {
      std::string ret = "{";
      auto vo = v.get_object();
      for( auto it: vo ) {
         if( ret.back() != '{' )
            ret.append(", ");
         ret.append("\"" + it.key() + "\": ");
         if( it.value().is_string() )
            ret.append("\"");
         try {
            ret.append(it.value().as_string());
         } catch( const fc::bad_cast_exception& e ) {
            ret.append(to_string(it.value()));
         } FC_CAPTURE_AND_RETHROW( (it.value()) );
         if( it.value().is_string() )
            ret.append("\"");
      }
      return ret.append("}");
   } else if( v.is_array() ) {
      std::string ret = "[";
      auto vo = v.get_array();
      for( auto it: vo ) {
         if( ret.back() != '[' )
            ret.append(", ");
         if( it.is_string() )
            ret.append("\"");
         try {
            ret.append(it.as_string());
         } catch( const fc::bad_cast_exception& e ) {
            ret.append(to_string(it));
         } FC_CAPTURE_AND_RETHROW( (it) );
         if( it.is_string() )
            ret.append("\"");
      }
      return ret.append("]");
   } else {
      return v.as_string();
   }
}

}
