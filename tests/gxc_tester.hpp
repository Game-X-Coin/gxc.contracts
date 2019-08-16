#pragma once
#include "gxc_base_tester.hpp"
#include "contract.hpp"

#define C(X) static_cast<X ## _contract&>(*contracts[typeid(X ## _contract)])

template<typename... Contracts>
class gxc_tester : public gxc_base_tester {
public:
   template<typename = enable_if_t<(is_convertible_v<Contracts*, contract*> && ...)>>
   gxc_tester() {
      contracts = { {typeid(Contracts), make_shared<Contracts>(static_cast<gxc_base_tester&>(*this))}... };
      for (auto con: contracts) {
         con.second->setup();
         abi_ser[con.second->account] = con.second->get_abi_serializer();
      }
   }
};
