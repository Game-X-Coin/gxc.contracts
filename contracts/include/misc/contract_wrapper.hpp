#pragma once

#include <eosio/eosio.hpp>
#include <boost/preprocessor/seq/to_tuple.hpp>

#define INLINE_ACTION_WRAPPER(contract, action, actor, args) \
   action_wrapper<name(#action), &contract::action>( \
      get_self(), authorization.empty() ? std::vector<permission_level>{{actor, "active"_n}} : authorization \
   ).send BOOST_PP_SEQ_TO_TUPLE(args)

namespace gxc {

using namespace eosio;

template<class T>
class contract_wrapper: public contract {
public:
   using contract::contract;

   contract_wrapper<T>(name receiver = T::default_account, name first_receiver = {}, datastream<const char*> ds = {nullptr, 0})
   : contract(receiver, first_receiver, ds) {}

   std::vector<permission_level> authorization;
};

}
