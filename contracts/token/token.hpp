#pragma once

#include <contracts/token.hpp>
#include <eostd/multi_index_wrapper.hpp>

namespace gxc {

void check_asset_is_valid(asset quantity, bool zeroable = false) {
   check(quantity.symbol.is_valid(), "invalid symbol name `" + quantity.symbol.code().to_string() + "`");
   check(quantity.is_valid(), "invalid quantity");
   if (zeroable)
      check(quantity.amount >= 0, "must not be negative quantity");
   else
      check(quantity.amount > 0, "must be positive quantity");
}

void check_asset_is_valid(extended_asset value, bool zeroable = false) {
   check_asset_is_valid(value.quantity, zeroable);
}

class token_impl;
class account_impl;
class request_impl;

class account_impl: public eostd::multi_index_wrapper<token::accounts_index> {
public:
   using opt = token::accounts::opt;

   account_impl(name code, name scope, uint64_t key, const token_impl& st)
   : multi_index_wrapper(code, scope, key), _st(st)
   , keep_balance(false), skip_valid(false), ram_payer(eosio::same_payer)
   {}

   void check_account_is_valid();
   void setopts(const std::vector<option>& opts);
   void open();
   void close();
   void approve(name spender, extended_asset value);

   inline name owner()const  { return scope(); }

   inline const token_impl& get_token()const { return _st; }

   account_impl& keep() {
      keep_balance = true;
      return *this;
   }

   account_impl& unkeep() {
      keep_balance = false;
      return *this;
   }

   account_impl& paid_by(name payer) {
      ram_payer = payer;
      return *this;
   }

   account_impl& skip_validation() {
      skip_valid = true;
      return *this;
   }

private:
   const token_impl& _st;
   bool  keep_balance;
   bool  skip_valid;
   name  ram_payer;

   void sub_balance(extended_asset value);
   void add_balance(extended_asset value);
   void sub_deposit(extended_asset value);
   void add_deposit(extended_asset value);
   void sub_allowance(name spender, extended_asset value);

   friend class token_impl;
   friend class request_impl;
};

class token_impl: public eostd::multi_index_wrapper<token::stat_index> {
public:
   using opt = token::stat::opt;

   token_impl(name code, name scope, symbol_code symbol)
   : multi_index_wrapper(code, scope, symbol.raw())
   {}

   void mint(extended_asset value, const std::vector<option>& opts);
   void setopts(const std::vector<option>& opts);
   void issue(name to, extended_asset value);
   void retire(name from, extended_asset value);
   void burn(name owner, extended_asset value);
   void transfer(name from, name to, extended_asset value);
   void deposit(name from, extended_asset value);
   void withdraw(name from, extended_asset value);
   void cancel_withdraw(name from, eostd::extended_symbol_code symbol);

   account_impl get_account(name owner) const {
      check(exists(), "token not found");
      return account_impl(code(), owner, std::hash<eostd::extended_symbol_code>()(eostd::extended_symbol_code{_this->supply.symbol.code(), _this->issuer}), *this);
   }

   inline name issuer()const { return scope(); }

private:
   void _setopts(token::stat& s, const std::vector<option>& opts, bool init = false);
};

class request_impl : public eostd::multi_index_wrapper<token::withdraws_index> {
public:
   using multi_index_wrapper::multi_index_wrapper;

   request_impl(name code, name scope, eostd::extended_symbol_code symbol)
   : multi_index_wrapper(code, scope, std::hash<eostd::extended_symbol_code>()(symbol))
   {}

   request_impl(name code, name scope, extended_asset value)
   : multi_index_wrapper(code, scope, std::hash<eostd::extended_symbol_code>()(eostd::extended_symbol_code{value.quantity.symbol.code(), value.contract}))
   {}

   void refresh_schedule();
   void clear();

   inline name owner()const  { return scope(); }
};

}
