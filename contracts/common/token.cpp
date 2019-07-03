#include <contracts/token.hpp>
#include <misc/action.hpp>

namespace gxc {

void token::mint(extended_asset value, std::vector<option> opts) {
   INLINE_ACTION_WRAPPER(token, mint, token::default_account, (value)(opts));
}

void token::transfer(name from, name to, extended_asset value, std::string memo = "") {
   INLINE_ACTION_WRAPPER(token, transfer, from, (from)(to)(value)(memo));
}

void token::burn(name owner, extended_asset value, std::string memo = "") {
   INLINE_ACTION_WRAPPER(token, burn, owner, (owner)(value)(memo));
}

void token::setopts(extended_symbol_code symbol, std::vector<option> opts) {
   INLINE_ACTION_WRAPPER(token, setopts, eosio::basename(symbol.contract), (symbol)(opts));
}

void token::setacntsopts(std::vector<name> accounts, extended_symbol_code symbol, std::vector<option> opts) {
   INLINE_ACTION_WRAPPER(token, setacntsopts, eosio::basename(symbol.contract), (accounts)(symbol)(opts));
}

void token::open(name owner, extended_symbol_code symbol, name payer) {
   INLINE_ACTION_WRAPPER(token, open, payer, (owner)(symbol)(payer));
}

void token::close(name owner, extended_symbol_code symbol) {
   INLINE_ACTION_WRAPPER(token, close, owner, (owner)(symbol));
}

void token::deposit(name owner, extended_asset value) {
   INLINE_ACTION_WRAPPER(token, deposit, owner, (owner)(value));
}

void token::pushwithdraw(name owner, extended_asset value) {
   INLINE_ACTION_WRAPPER(token, pushwithdraw, owner, (owner)(value));
}

void token::popwithdraw(name owner, extended_symbol_code symbol) {
   INLINE_ACTION_WRAPPER(token, popwithdraw, owner, (owner)(symbol));
}

void token::clrwithdraws(name owner) {
   INLINE_ACTION_WRAPPER(token, clrwithdraws, owner, (owner));
}

void token::approve(name owner, name spender, extended_asset value) {
   INLINE_ACTION_WRAPPER(token, approve, owner, (owner)(spender)(value));
}

}
