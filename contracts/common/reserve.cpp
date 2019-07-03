#include <contracts/reserve.hpp>

namespace gxc {

void reserve::mint(extended_asset derivative, extended_asset underlying, std::vector<option> opts) {
   INLINE_ACTION_WRAPPER(reserve, mint, basename(derivative.contract), (derivative)(underlying)(opts));
}

void reserve::claim(name owner, extended_asset value) {
   INLINE_ACTION_WRAPPER(reserve, claim, owner, (owner)(value));
}

}
