#include <contracts/token.hpp>
#include <eosio/check.hpp>

#include "token_impl.cpp"
#include "account_impl.cpp"
#include "request_impl.cpp"

namespace gxc {

constexpr name null_account{"gxc.null"_n};

void token::mint(extended_asset value, std::vector<option> opts) {
   token_impl(_self, value.contract, value.quantity.symbol.code()).mint(value, opts);
}

void token::transfer(name from, name to, extended_asset value, std::string memo) {
   check(memo.size() <= 256, "memo has more than 256 bytes");
   check(from != to, "cannot transfer to self");
   check(is_account(to), "`to` account does not exist");

   auto _token = token_impl(_self, value.contract, value.quantity.symbol.code());

   if (from == null_account) {
      _token.issue(to, value);
   } else if (to == null_account) {
      _token.retire(from, value);
   } else {
      _token.transfer(from, to, value);
   }
}

void token::burn(name owner, extended_asset value, std::string memo) {
   check(memo.size() <= 256, "memo has more than 256 bytes");
   token_impl(_self, value.contract, value.quantity.symbol.code()).burn(owner, value);
}

void token::setopts(extended_symbol_code symbol, std::vector<option> opts) {
   token_impl(_self, symbol.contract, symbol.code).setopts(opts);
}

void token::setacntsopts(std::vector<name> accounts, extended_symbol_code symbol, std::vector<option> opts) {
   auto _token = token_impl(_self, symbol.contract, symbol.code);
   for (auto& account: accounts) {
      _token.get_account(account).setopts(opts);
   }
}

void token::open(name owner, extended_symbol_code symbol, name payer) {
   token_impl(_self, symbol.contract, symbol.code).get_account(owner).paid_by(payer).open();
}

void token::close(name owner, extended_symbol_code symbol) {
   token_impl(_self, symbol.contract, symbol.code).get_account(owner).close();
}

void token::deposit(name owner, extended_asset value) {
   token_impl(_self, value.contract, value.quantity.symbol.code()).deposit(owner, value);
}

void token::pushwithdraw(name owner, extended_asset value) {
   token_impl(_self, value.contract, value.quantity.symbol.code()).withdraw(owner, value);
}

void token::popwithdraw(name owner, extended_symbol_code symbol) {
   token_impl(_self, symbol.contract, symbol.code).cancel_withdraw(owner, symbol);
}

void token::clrwithdraws(name owner) {
   request_impl(_self, owner).clear();
}

void token::approve(name owner, name spender, extended_asset value) {
   token_impl(_self, value.contract, value.quantity.symbol.code()).get_account(owner).approve(spender, value);
}

}
