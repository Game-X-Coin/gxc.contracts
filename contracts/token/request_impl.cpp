#include "token.hpp"
#include <eosio/transaction.hpp>

namespace gxc {

void request_impl::refresh_schedule() {
   auto _idx = get_index<"schedule"_n>();
   auto _it = _idx.begin();

   cancel_deferred(owner().value);

   if (_it != _idx.end()) {
      auto timeleft = (_it->scheduled_time - current_time_point()).to_seconds();
      if (timeleft <= 0 ) {
         token::clear_withdraws(code(), {owner(), "active"_n}).send(owner());
      } else {
         transaction out;
         out.actions.emplace_back(permission_level{owner(), active_permission}, code(), "clrwithdraws"_n, owner());
         out.delay_sec = static_cast<uint32_t>(timeleft);
         out.send(owner().value, owner(), true);
      }
   }
}

void request_impl::clear() {
   require_auth(owner());

   auto _idx = get_index<"schedule"_n>();
   auto _it = _idx.begin();

   check(_it != _idx.end(), "withdrawal requests not found");

   for ( ; _it != _idx.end(); _it = _idx.begin()) {
      auto _token = token_impl(code(), _it->issuer, _it->quantity.symbol.code());
      if (_it->scheduled_time > current_time_point()) break;

      _token.get_account(code()).sub_balance(_it->value());

      auto _owner = _token.get_account(owner());
      if (!_owner->option(account_impl::opt::frozen)) {
         _owner.paid_by(owner()).add_balance(_it->value());
         token::withdraw_processed(code(), {code(), active_permission}).send(owner(), _it->value());
      } else {
         _owner.skip_validation().add_deposit(_it->value());
         token::withdraw_reverted(code(), {code(), active_permission}).send(owner(), _it->value());
      }

      _idx.erase(_it);
   }

   refresh_schedule();
}

}
