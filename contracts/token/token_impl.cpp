#include "token.hpp"
#include <contracts/system.hpp>
#include <contracts/account.hpp>
#include <misc/action.hpp>
#include <eosio/system.hpp>

using namespace eosio;

namespace gxc {

constexpr name active_permission{"active"_n};

void token_impl::_setopts(token::stat& s, const std::vector<option>& opts, bool init) {
   for (auto o: opts) {
      if (o.first == "paused") {
         s.option(opt::paused, unpack<bool>(o.second));
      } else if (o.first == "whitelist_on") {
         s.option(opt::whitelist_on, unpack<bool>(o.second));
      } else {
         // Below options can be configured only when creating token.
         check(init, "not allowed to change the option `" + o.first + "`");

         if (o.first == "mintable") {
            s.option(opt::mintable, unpack<bool>(o.second));
         } else if (o.first == "recallable") {
            s.option(opt::recallable, unpack<bool>(o.second));
         } else if (o.first == "freezable") {
            s.option(opt::freezable, unpack<bool>(o.second));
         } else if (o.first == "floatable") {
            s.option(opt::floatable, unpack<bool>(o.second));
         } else if (o.first == "pausable") {
            s.option(opt::pausable, unpack<bool>(o.second));
         } else if (o.first == "whitelistable") {
            s.option(opt::whitelistable, unpack<bool>(o.second));
         } else if (o.first == "withdraw_min_amount") {
            auto value = unpack<int64_t>(o.second);
            check(value >= 0, "withdraw_min_amount should be positive");
            s.amount.emplace(asset(value, s.supply.symbol));
         } else if (o.first == "withdraw_delay_sec") {
            auto value = unpack<uint64_t>(o.second);
            s.duration.emplace(static_cast<uint32_t>(value));
         } else {
            check(false, "unknown option `" + o.first + "`");
         }
      }
   }

   if (s.option(opt::recallable)) {
      if (!s.amount) s.amount.emplace(asset(0, s.supply.symbol));
      if (!s.duration) s.duration.emplace(24 * 60 * 60);
   }

   check((!s.amount && !s.duration) || (s.option(opt::recallable)), "non-recallable token can't have withdraw options");
   check(!s.option(opt::floatable) || s.option(opt::recallable), "not allowed to set floatable");
   check(!s.option(opt::paused) || (init || s.option(opt::pausable)), "not allowed to set paused");
   check(!s.option(opt::whitelist_on) || s.option(opt::whitelistable), "not allowed to set whitelist");
}

void token_impl::mint(extended_asset value, const std::vector<option>& opts) {
   require_auth(code());
   check_asset_is_valid(value);

   bool init = !exists();

   if (init) {
      emplace(code(), [&](auto& s) {
         s.supply.symbol = value.quantity.symbol;
         s.max_supply = value.quantity;
         s.issuer = value.contract;
         _setopts(s, opts, init);
      });
   } else {
      check(_this->option(opt::mintable), "not allowed additional mint");
      modify(same_payer, [&](auto& s) {
         s.max_supply += value.quantity;
         _setopts(s, opts, init);
      });
   }
}

void token_impl::setopts(const std::vector<option>& opts) {
   check(opts.size(), "no changes on options");
   require_vauth(issuer());
   modify(same_payer, [&](auto& s) {
      _setopts(s, opts);
   });
}

void token_impl::issue(name to, extended_asset value) {
   require_vauth(value.contract);
   check_asset_is_valid(value);

   //TODO: check game account
   check(value.quantity.symbol == _this->supply.symbol, "symbol precision mismatch");
   check(value.quantity.amount <= _this->max_supply.amount - _this->supply.amount, "quantity exceeds available supply");

   modify(same_payer, [&](auto& s) {
      s.supply += value.quantity;
   });

   name payer = (value.contract == system::default_account || account::is_partner(basename(value.contract))) ? code() : basename(value.contract);

   auto _to = get_account(to);

   if (_this->option(opt::recallable) && (to != basename(value.contract))) {
      _to.paid_by(code()).add_deposit(value);
   } else {
      _to.paid_by(payer).add_balance(value);
   }
}

void token_impl::retire(name from, extended_asset value) {
   check_asset_is_valid(value);

   //TODO: check game account
   check(value.quantity.symbol == _this->supply.symbol, "symbol precision mismatch");

   bool is_recall = false;

   if (!has_auth(from)) {
      check(_this->option(opt::recallable) && has_vauth(value.contract), "missing required authority");
      is_recall = true;
   }

   modify(same_payer, [&](auto& s) {
      s.supply -= value.quantity;
   });

   auto _to = get_account(from);

   if (!is_recall) {
      _to.sub_balance(value);
   } else {
      _to.paid_by(code()).sub_deposit(value);
   }
}

void token_impl::burn(name owner, extended_asset value) {
   check((owner == basename(value.contract) && has_vauth(value.contract)) ||
         (owner == code() && has_auth(code())), "missing required authority");
   check_asset_is_valid(value);

   //TODO: check game account
   check(value.quantity.symbol == _this->supply.symbol, "symbol precision mismatch");

   modify(same_payer, [&](auto& s) {
      s.supply -= value.quantity;
      s.max_supply -= value.quantity;
   });

   get_account(owner).sub_balance(value);
}

void token_impl::transfer(name from, name to, extended_asset value) {
   check(from != to, "cannot transfer to self");
   check(is_account(to), "`to` account does not exist");

   check_asset_is_valid(value);
   check(!_this->option(opt::paused)
         || from == basename(value.contract) || to == basename(value.contract)
         || from == code() || to == code(), "token is paused");

   bool is_recall = false;
   bool is_allowed = false;

   if (!has_auth(from)) {
      if (_this->option(opt::recallable) && has_vauth(value.contract)) {
         is_recall = true;
      } else if (has_auth(to)) {
         token::allowance_index _allowed(code(), from.value);
         auto it = _allowed.find(token::allowance{to, value.quantity, value.contract}.primary_key());
         is_allowed = (it != _allowed.end());
      }
      check(is_recall || is_allowed, "missing required authority");
   }

   // subtract asset from `from`
   auto _from = get_account(from);

   if (is_allowed) {
      _from.sub_allowance(to, value);
   }

   if (!is_recall) {
      _from.sub_balance(value);
   } else {
      // normal case, transfer from's deposit
      if (*_from->deposit >= value.quantity) {
         _from.paid_by(code()).sub_deposit(value);
      } else {
         // exceptional case, cached amount is not enough
         // so withdrawal request is partially cancelled
         auto leftover = value.quantity - *_from->deposit;
         auto _req = request_impl(code(), from, value);
         check(_req, "overdrawn deposit, but no withdrawal request");
         check(_req->quantity >= leftover, "overdrawn deposit, but not enough withdrawal requested amount");

         if (_req->quantity > leftover) {
            _req.modify(same_payer, [&](auto& rq) {
               rq.quantity -= leftover;
            });
         } else {
            _req.erase();
            _req.refresh_schedule();
         }
         get_account(code()).sub_balance(extended_asset(leftover, value.contract));
         _from.paid_by(code()).sub_deposit(extended_asset(*_from->deposit,  value.contract));

         token::withdraw_reverted(code(), {code(), active_permission}).send(from, extended_asset(leftover, value.contract));
      }
   }

   name payer;

   // case is_recall   : in-game transfer
   // case has_auth(to): approved transfer
   // case else        : all other cases
   if (is_recall) payer = code();
   else if (has_auth(to)) payer = to;
   else payer = from;

   // add asset to `to`
   get_account(to).paid_by(payer).add_balance(value);
}

void token_impl::deposit(name from, extended_asset value) {
   check_asset_is_valid(value);
   check(_this->option(opt::recallable), "not supported token");

   auto _from = get_account(from);

   _from.sub_balance(value);
   _from.paid_by(from).add_deposit(value);
}

void token_impl::withdraw(name from, extended_asset value) {
   check_asset_is_valid(value);
   require_auth(from);

   check(_this->option(opt::recallable), "not supported token");
   check(value.quantity >= *_this->amount, "withdraw amount is too small");

   auto _req = request_impl(code(), from, value);

   if (_req) {
      _req.modify(same_payer, [&](auto& rq) {
         rq.scheduled_time = current_time_point() + seconds(*_this->duration);
         rq.quantity += value.quantity;
      });
   } else {
      _req.emplace(from, [&](auto& rq) {
         rq.scheduled_time = current_time_point() + seconds(*_this->duration);
         rq.quantity       = value.quantity;
         rq.issuer         = value.contract;
      });
   }

   get_account(from).keep().sub_deposit(value);
   get_account(code()).paid_by(code()).add_balance(value);

   _req.refresh_schedule();
}

void token_impl::cancel_withdraw(name from, sio4::extended_symbol_code symbol) {
   require_auth(from);

   auto _req = request_impl(code(), from, symbol);
   check(_req, "withdrawal request not found");

   auto value = extended_asset(_req->quantity, _req->issuer);
   get_account(code()).sub_balance(value);
   get_account(from).paid_by(from).add_deposit(value);

   token::withdraw_reverted(code(), {code(), active_permission}).send(from, value);

   _req.erase();
}

}
