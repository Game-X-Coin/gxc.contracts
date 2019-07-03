#include <contracts/htlc.hpp>
#include <eosio/system.hpp>
#include <sio4/hex.hpp>

#include "../common/token.cpp"

namespace gxc {

using esc = sio4::extended_symbol_code;

constexpr name vault_account{"gxc.vault"_n};

void htlc::newcontract(name owner, string contract_name, std::variant<name, checksum160> recipient, extended_asset value, checksum256 hashlock, time_point_sec timelock) {
   require_auth(owner);

   htlc_index idx(_self, owner.value);
   check(idx.find(std::hash<std::string>()(contract_name)) == idx.end(), "existing contract name");

   config_index cfg(_self, _self.value);
   auto it = cfg.find(std::hash<esc>()(esc{value.quantity.symbol.code(), value.contract}));

   bool constrained = owner != vault_account && it != cfg.end();

   auto min_amount = (constrained) ? it->min_amount : extended_asset(0, value.get_extended_symbol());
   auto min_duration = (constrained) ? it->min_duration : 0;

   check(value >= min_amount, "specified amount is not enough");
   check(timelock >= current_time_point() + microseconds(static_cast<int64_t>(min_duration * 1000)), "the expiration time should be in the future");

   idx.emplace(owner, [&](auto& l) {
      l.contract_name = contract_name;
      l.recipient = recipient;
      l.value = value;
      l.hashlock = hashlock;
      l.timelock = timelock;
   });

   token _token;
   _token.authorization = {{_self, "active"_n}};
   _token.transfer(owner, _self, value, "htlc created by " + owner.to_string() + (contract_name.size() ? ": " : "") + contract_name);
}

void htlc::withdraw(name owner, string contract_name, checksum256 preimage) {
   htlc_index idx(_self, owner.value);
   const auto& it = idx.get(std::hash<std::string>()(contract_name));
   check(it.timelock >= current_time_point(), "contract is expired");

   // `preimage` works as a key here.
   //require_auth(it.recipient);

   auto data = preimage.extract_as_byte_array();
   auto hash = eosio::sha256(reinterpret_cast<const char*>(data.data()), data.size());
   check(memcmp((const void*)it.hashlock.data(), (const void*)hash.data(), 32) == 0, "invalid preimage");

   token _token;
   if (std::holds_alternative<name>(it.recipient)) {
      _token.transfer(_self, std::get<name>(it.recipient), it.value, "htlc processed from " + owner.to_string() + (contract_name.size() ? ": " : "") + contract_name);
   } else {
      _token.transfer(_self, vault_account, it.value, "htlc processed from " + owner.to_string() + (contract_name.size() ? ": " : "") + contract_name);
   }

   idx.erase(it);
}

void htlc::refund(name owner, string contract_name) {
   require_auth(owner);

   htlc_index idx(_self, owner.value);
   const auto& it = idx.get(std::hash<std::string>()(contract_name));

   check(it.timelock < current_time_point(), "contract not expired");

   token _token;
   if (std::holds_alternative<name>(it.recipient)) {
      _token.transfer(_self, owner, it.value, "htlc refunded from" + std::get<name>(it.recipient).to_string() + (contract_name.size() ? ": " : "") + contract_name);
   } else {
      auto bytes = std::get<checksum160>(it.recipient).extract_as_byte_array();
      auto str_recipient = sio4::to_hex(reinterpret_cast<const char*>(bytes.data()), bytes.size());
      _token.transfer(_self, owner, it.value, "htlc refunded from" + str_recipient + (contract_name.size() ? ": " : "") + contract_name);
   }

   idx.erase(it);
}

void htlc::setconfig(extended_asset min_amount, uint32_t min_duration) {
   require_auth(min_amount.contract);

   check(min_amount.quantity.symbol.is_valid(), "invalid symbol name `" + min_amount.quantity.symbol.code().to_string() + "`");
   check(min_amount.quantity.is_valid(), "invalid quantity");
   check(min_amount.quantity.amount >= 0, "must not be negative quantity");
   
   config_index idx(_self, _self.value);
   auto it = idx.find(std::hash<esc>()(esc{min_amount.quantity.symbol.code(), min_amount.contract}));

   if (it != idx.end()) {
      idx.modify(it, same_payer, [&](auto& c) {
         c.min_amount = min_amount;
         c.min_duration = min_duration;
      });
   } else {
      idx.emplace(min_amount.contract, [&](auto& c) {
         c.min_amount = min_amount;
         c.min_duration = min_duration;
      });
   }
}

}
