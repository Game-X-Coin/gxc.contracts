#include <contracts/bancor.hpp>
#include "../common/token.cpp"

namespace gxc {

using connector = bancor::connector;
using converted = bancor::connector::converted;

converted connector::convert_to_smart(const extended_asset& from, const extended_symbol& to, bool reverse) {
   const double S = token().get_supply(extended_symbol_code{to.get_symbol().code(), to.get_contract()}).quantity.amount; 
   const double C = balance.amount;
   const double dC = from.quantity.amount;

   double dS = S * (std::pow(1. + dC / C, weight) - 1.);
   if (dS < 0) dS = 0;

   auto conversion_rate = ((int64_t)dS) / dS;
   auto delta = asset{ from.quantity.amount - int64_t(from.quantity.amount * (1 - conversion_rate)), from.quantity.symbol };

   if (!reverse) balance += delta;
   else balance -= delta;

   return { {int64_t(dS), to}, delta, conversion_rate };
}

converted connector::convert_from_smart(const extended_asset& from, const extended_symbol& to, bool reverse) {
   const double C = balance.amount;
   const double S = token().get_supply(extended_symbol_code{from.quantity.symbol.code(), from.contract}).quantity.amount; 
   const double dS = -from.quantity.amount;

   double dC = C * (std::pow(1. + dS / S, double(1) / weight) - 1.);
   if (dC > 0) dC = 0;

   auto delta = asset{ int64_t(-dC), balance.symbol };

   if (!reverse) balance -= delta;
   else balance += delta;

   return { {int64_t(-dC), to}, delta, ((int64_t)-dC) / (-dC) };
}

}
