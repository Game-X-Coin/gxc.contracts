#pragma once

#include <eosio/check.hpp>
#include <sio4/bytes.hpp>

namespace gxc {

struct option {
   std::string name;
   sio4::bytes value;
};

}
