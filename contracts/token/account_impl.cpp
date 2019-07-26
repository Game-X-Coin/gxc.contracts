#include "token.hpp"
#include <cmath>

namespace gxc {

void account_impl::check_account_is_valid() {
   if (skip_valid) {
      skip_valid = false;
   } else if (code() != owner()) {
      check(!_this->option(opt::frozen), "account is frozen");
      check(!_st->option(token_impl::opt::whitelist_on) || _this->option(opt::whitelist), "not whitelisted account");
   }
}

void account_impl::setopts(const std::vector<option>& opts) {
   check(opts.size(), "no changes on options");
   require_vauth(_st.issuer());

   modify(ram_payer, [&](auto& a) {
      for (auto o: opts) {
         if (o.first == "frozen") {
            check(_st->option(token_impl::opt::freezable), "not configured to freeze account");
            auto value = unpack<bool>(o.second);
            check(a.option(opt::frozen) != value, "option already has given value");
            a.option(opt::frozen, value);
         } else if (o.first == "whitelist") {
            check(_st->option(token_impl::opt::whitelistable), "not configured to whitelist account");
            auto value = unpack<bool>(o.second);
            check(a.option(opt::whitelist) != value, "option already has given value");
            a.option(opt::whitelist, value);
         } else {
            check(false, "unknown option `" + o.first + "`");
         }
      }
   });
}

void account_impl::open() {
   check(is_account(owner()), "owner account does not exist");

   if (!exists()) {
      emplace(ram_payer, [&](auto& a) {
         a.balance.symbol = _st->supply.symbol;
         a.issuer(_st->issuer);
         if (_st->option(token_impl::opt::recallable)) {
            a.deposit.emplace(asset(0, _st->supply.symbol));
         }
         if (_st->option(token_impl::opt::whitelist_on)) {
            a.option(opt::whitelist, has_vauth(_st->issuer));
         }
      });
   }
}

void account_impl::close() {
   require_auth(owner());
   check(exists(), "account balance doesn't exist");
   check(!_this->balance.amount && (!_this->deposit || !_this->deposit->amount), "cannot close non-zero balance");
   erase();
}

void account_impl::approve(name spender, extended_asset value) {
   check_asset_is_valid(value, true);
   require_auth(owner());

   token::allowance_index _allowed(code(), owner().value);

   auto it = _allowed.find(token::allowance{spender, value.quantity, value.contract}.primary_key());
   if (it == _allowed.end()) {
      // no existing allowance, but try approving `0` amount (erase allowance)
      check(value.quantity.amount > 0, "allowance not found");

      _allowed.emplace(owner(), [&](auto& a) {
         a.spender  = spender;
         a.quantity = value.quantity;
         a.issuer   = value.contract;
      });
   } else if (value.quantity.amount > 0) {
      _allowed.modify(it, owner(), [&](auto& a) {
         a.quantity = value.quantity;
      });
   } else {
      _allowed.erase(it);
   }
}

void account_impl::sub_balance(extended_asset value) {
   check_account_is_valid();
   check(_this->balance.amount >= value.quantity.amount, "overdrawn balance");

   if (!_this->option(opt::frozen) && (!_this->option(opt::whitelist) || code() == owner()) && !keep_balance &&
       _this->balance.amount == value.quantity.amount && (!_this->deposit || _this->deposit->amount == 0))
   {
      erase();
   } else {
      modify(ram_payer, [&](auto& a) {
         a.balance -= value.quantity;
      });
   }
}

void account_impl::add_balance(extended_asset value) {
   if (!exists()) {
      bool whitelist = false;
      check(!_st->option(token_impl::opt::whitelist_on) ||
            (whitelist = (code() == owner()) || has_vauth(value.contract)), "required to open balance manually");
      emplace(ram_payer, [&](auto& a) {
         a.balance = value.quantity;
         if (_st->option(token_impl::opt::recallable))
            a.deposit.emplace(asset(0, value.quantity.symbol));
         a.issuer(value.contract);
         a.option(opt::whitelist, whitelist);
      });
   } else {
      check_account_is_valid();
      modify(ram_payer, [&](auto& a) {
         a.balance += value.quantity;
      });
   }
}

void account_impl::sub_deposit(extended_asset value) {
   check_account_is_valid();
   check(_this->deposit->amount >= value.quantity.amount, "overdrawn deposit");
   check(_st->option(token_impl::opt::floatable) ||
         value.quantity.amount % static_cast<int64_t>(std::pow(10, value.quantity.symbol.precision())) == 0, "not available float");

   if (!_this->option(opt::frozen) && (!_this->option(opt::whitelist) || code() == owner()) && !keep_balance &&
       _this->deposit->amount == value.quantity.amount && _this->balance.amount == 0)
   {
      erase();
   } else {
      modify(ram_payer, [&](auto& a) {
         a.deposit.emplace(*a.deposit - value.quantity);
      });
   }
}

void account_impl::add_deposit(extended_asset value) {
   check(_st->option(token_impl::opt::floatable) ||
      value.quantity.amount % static_cast<int64_t>(std::pow(10, value.quantity.symbol.precision())) == 0, "not available float");

   if (!exists()) {
      bool whitelist = false;
      check(!_st->option(token_impl::opt::whitelist_on) ||
            (whitelist = (code() == owner()) || has_vauth(value.contract)), "required to open deposit manually");
      emplace(ram_payer, [&](auto& a) {
         a.balance = asset(0, value.quantity.symbol);
         a.deposit.emplace(value.quantity);
         a.issuer(value.contract);
         a.option(opt::whitelist, whitelist);
      });
   } else {
      check_account_is_valid();
      modify(ram_payer, [&](auto& a) {
         a.deposit.emplace(*a.deposit + value.quantity);
      });
   }
}

void account_impl::sub_allowance(name spender, extended_asset value) {
   token::allowance_index _allowed(code(), owner().value);

   const auto& it = _allowed.get(token::allowance{spender, value.quantity, value.contract}.primary_key());

   if (it.quantity > value.quantity) {
      _allowed.modify(it, owner(), [&](auto& a) {
         a.quantity -= value.quantity;
      });
   } else if (it.quantity == value.quantity) {
      _allowed.erase(it);
   } else {
      check(false, "not possible to transfer more than allowed");
   }
}

}
