#include <contracts/reserve.hpp>
#include <contracts/system.hpp>
#include <contracts/account.hpp>
#include <cmath>

#include "../common/token.cpp"

namespace gxc {

void reserve::mint(extended_asset derivative, extended_asset underlying, std::vector<option> opts) {
   require_vauth(derivative.contract);
   check(account::is_partner(basename(derivative.contract)), "only partner account can use reserve");

   std::vector<std::string> valid_opts = {
      "withdraw_min_amount",
      "withdraw_delay_sec"
   };
   auto option_is_valid = [&](const std::string& key) -> bool {
      return std::find(valid_opts.begin(), valid_opts.end(), key) != valid_opts.end();
   };

   for (auto o : opts) {
      check(option_is_valid(o.first), "not allowed to set option `" + o.first + "`");
   }

   check(underlying.contract == system::default_account, "underlying asset should be system token");

   reserves_index rsv(_self, derivative.contract.value);
   auto it = rsv.find(derivative.quantity.symbol.code().raw());

   check(it == rsv.end(), "additional issuance not supported yet");
   rsv.emplace(_self, [&](auto& r) {
      r.derivative = derivative;
      r.underlying = underlying.quantity;
      r.rate = underlying.quantity.amount * std::pow(10, derivative.quantity.symbol.precision())
             / double(derivative.quantity.amount) / std::pow(10, underlying.quantity.symbol.precision());
   });

   // deposit underlying asset to reserve
   prepaid_index ppd(_self, _self.value);
   auto pit = ppd.find(basename(derivative.contract).value);

   auto leftover = underlying;

   if (pit != ppd.end()) {
      if (leftover >= pit->value) {
         leftover -= pit->value;
         ppd.erase(pit);
      } else {
         ppd.modify(pit, same_payer, [&](auto& p) {
            p.value -= leftover;
            leftover.quantity.amount = 0;
         });
      }
   }

   token _token;
   _token.authorization = {{_self, "active"_n}};

   if (leftover.quantity.amount > 0) {
      _token.transfer(basename(derivative.contract), _self, leftover, "deposit in reserve");
   }

   // create derivative token
   _token.authorization.clear();
   _token.mint(derivative, opts);
}

void reserve::claim(name owner, extended_asset value) {
   check(value.quantity.amount > 0, "invalid quantity");
   require_auth(owner);

   reserves_index rsv(_self, value.contract.value);
   const auto& it = rsv.get(value.quantity.symbol.code().raw(), "underlying asset not found");

   double dC = value.quantity.amount * it.rate / pow(10, value.quantity.symbol.precision() - it.underlying.symbol.precision());
   if (dC < 0) dC = 0;

   auto claimed_asset = asset(int64_t(dC), it.underlying.symbol);

   token _token;
   _token.authorization = {{_self, "active"_n}};
   _token.transfer(owner, _self, value, "claim reserve");
   _token.transfer(_self, token::default_account, value, "claim reserve");
   _token.transfer(_self, owner, extended_asset{claimed_asset, system::default_account}, "claim reserve");

   _token.authorization = {{token::default_account, "active"_n}};
   _token.burn(token::default_account, value, "claim reserve");

   rsv.modify(it, same_payer, [&](auto& r) {
      r.derivative -= value;
      r.underlying -= claimed_asset;
   });
}

void reserve::setminamount(extended_asset value) {
   require_auth(_self);

   check(value.quantity.amount >= 0, "min amount must not be negative");
   check(value.quantity.symbol.is_valid(), "invalid symbol");
   check(value.contract == system::default_account, "prepaid asset should be system token");

   configuration cfg(_self, _self.value);
   cfg.set({value}, _self);
}

void reserve::prepay(name creator, name name, extended_asset value) {
   require_auth(creator);

   configuration cfg(_self, _self.value);
   check(value >= cfg.get().amount, "not enough to prepay");

   prepaid_index ppd(_self, _self.value);
   ppd.emplace(_self, [&](auto& p) {
      p.creator = creator;
      p.name = name;
      p.value = value;
   });

   auto _token = token();
   _token.authorization = {{_self, "active"_n}};
   _token.transfer(creator, _self, value, "prepaid for partner account creation");
}

}
