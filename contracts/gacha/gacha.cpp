#include <contracts/gacha.hpp>
#include <contracts/account.hpp>
#include <eosio/transaction.hpp>

#include <sio4/crypto/drbg.hpp>

#include "../common/token.cpp"

namespace gxc {

bytes gacha::mixseed(const checksum256& dseed, const checksum256& oseed) const {
   bytes raw;
   raw.resize(64);

   datastream<char*> ds(reinterpret_cast<char*>(raw.data()), raw.size());
   ds << dseed.extract_as_byte_array();
   ds << oseed.extract_as_byte_array();

   return raw;
}

void gacha::close(extended_name scheme) {
   require_vauth(scheme.contract);

   scheme_index schm(_self, scheme.contract.value);
   const auto& it = schm.get(scheme.name.value);

   check(it.expiration < current_time_point(), "not expired gacha cannot be closed");
   check(it.unresolved == 0, "unresolved gacha remains");

   token().transfer(_self, basename(scheme.contract), it.budget - extended_asset{it.out, it.budget.contract}, "close gacha scheme");

   schm.erase(it);
}

void gacha::open(extended_name scheme, std::vector<grade> &grades, extended_asset budget, time_point_sec expiration, optional<uint8_t> precision, optional<uint32_t> deadline_sec) {
   require_vauth(scheme.contract);
   check(account::is_partner(basename(scheme.contract)), "only partner account can create scheme");

   scheme_index schm(_self, scheme.contract.value);
   check(schm.find(scheme.name.value) == schm.end(), "existing scheme name");

   const grade* prev = nullptr;
   for (const auto& it: grades) {
      check(!prev || it.score < prev->score, "grades should be sorted in descending order by score");
      prev = &it;
   }

   schm.emplace(_self, [&](auto& s) {
      s.scheme_name = scheme.name;
      s.grades = grades;
      s.budget = budget;
      s.expiration = expiration;
      check(!precision || *precision <= 4, "precision cannot exceed 4 bytes");
      s.precision = (precision) ? *precision : 1;
      s.deadline_sec = (deadline_sec) ? *deadline_sec : 60 * 60 * 24 * 7;
      s.out.symbol = budget.quantity.symbol;
      s.out_count.resize(grades.size());
   });

   auto _token = token();
   _token.authorization = {{_self, "active"_n}};
   _token.transfer(basename(scheme.contract), _self, budget, "open gacha scheme");
}

void gacha::issue(name to, extended_name scheme, checksum256 dseedhash, optional<uint64_t> id) {
   require_vauth(scheme.contract);

   scheme_index schm(_self, scheme.contract.value);
   auto sit = schm.find(scheme.name.value);

   check(sit != schm.end(), "scheme not found");
   check(sit->expiration > current_time_point(), "scheme expired");
   check(sit->out <= sit->budget.quantity, "budget exhausted");

   gacha_index gch(_self, _self.value);

   auto gacha_id = (id) ? *id : _gacha::hash(scheme, dseedhash);
   auto git = gch.find(gacha_id);
   check(git == gch.end(), "existing gacha");

   gch.emplace(_self, [&](auto& c) {
      c.id = gacha_id;
      c.owner = to;
      c.scheme = scheme;
      c.dseedhash = dseedhash;
   });

   schm.modify(sit, same_payer, [&](auto& s) {
      s.issued++;
   });
}

void gacha::setoseed(uint64_t id, checksum256 oseed) {
   char zerobytes[32] = { 0, };

   gacha_index gch(_self, _self.value);
   const auto& git = gch.get(id);

   require_auth(git.owner);
   check(memcmp(git.oseed.data(), &zerobytes[0], 32) == 0, "oseed is already set");

   scheme_index schm(_self, git.scheme.contract.value);
   const auto& sit = schm.get(git.scheme.name.value);

   gch.modify(git, same_payer, [&](auto& c) {
      c.oseed = oseed;
      c.deadline = current_time_point() + seconds(sit.deadline_sec);
   });

   schm.modify(sit, same_payer, [&](auto& s) {
      s.unresolved++;
   });

   refresh_schedule();
}

void gacha::setdseed(uint64_t id, checksum256 dseed) {
   draw(id, dseed);

   refresh_schedule();
}

void gacha::refresh_schedule() {
   gacha_index gch(_self, _self.value);
   auto deadline = gch.get_index<"deadline"_n>();
   auto it = deadline.begin();

   cancel_deferred(_self.value);

   if (it != deadline.end()) {
      auto timeleft = (it->deadline - current_time_point()).to_seconds();
      if (timeleft <= 0) {
         action_wrapper<"resolve"_n, &gacha::resolve>(_self, {_self, "active"_n}).send();
      } else {
         transaction out;
         out.actions.emplace_back(permission_level{_self, "active"_n}, _self, "resolve"_n, std::make_tuple());
         out.delay_sec = static_cast<uint32_t>(timeleft);
         out.send(_self.value, _self, true);
      }
   }
}

void gacha::resolve() {
   require_auth(_self);

   gacha_index gch(_self, _self.value);
   auto deadline = gch.get_index<"deadline"_n>();

   auto it = deadline.begin();

   for ( ; it != deadline.end(); it = deadline.begin()) {
      const auto& git = *it;
      if (git.deadline > current_time_point()) break;

      resolve_one(git.id);
   }

   refresh_schedule();
}

void gacha::resolve_one(uint64_t id) {
   draw(id);
}

void gacha::draw(uint64_t id, optional<checksum256> dseed) {
   gacha_index gch(_self, _self.value);
   const auto& git = gch.get(id);

   check(has_auth(_self) || (dseed &&  has_vauth(git.scheme.contract)), "missing required authority");

   scheme_index schm(_self, git.scheme.contract.value);
   const auto& sit = schm.get(git.scheme.name.value);

   uint32_t grade = 0;
   int64_t score = 0;

   if (dseed) {
      char zerobytes[32] = { 0, };
      check(memcmp(git.oseed.data(), &zerobytes[0], 32), "oseed is not set");

      auto data = dseed->extract_as_byte_array();
      auto hash = eosio::sha256(reinterpret_cast<const char*>(data.data()), data.size());
      check(memcmp((const void*)git.dseedhash.data(), (const void*)hash.data(), 32) == 0, "hash mismatch");

      sio4::byte result[4];
      auto seed = mixseed(*dseed, git.oseed);
      sio4::hash_drbg drbg(seed.data(), seed.size());
      drbg.generate_block(&result[0], sizeof(result));

      memcpy((void*)&score, (const void*)result, sit.precision);

      for (auto it: sit.grades) {
         if (score >= it.score && (!it.limit || sit.out_count[grade] < *(it.limit)))
            break;
         grade++;
      }
   } else {
      score = -1;
   }

   if (grade >= sit.grades.size()) {
      action_wrapper<"raincheck"_n, &gacha::raincheck>(_self, {_self, "active"_n}).send(git.owner, git.id, score);
   } else {
      auto reward = extended_asset{sit.grades[grade].reward, sit.budget.contract};

      action_wrapper<"winreward"_n, &gacha::winreward>(_self, {_self, "active"_n}).send(git.owner, git.id, score, reward);
      token().transfer(_self, git.owner, reward, "");

      schm.modify(sit, same_payer, [&](auto& s) {
         s.out += sit.grades[grade].reward;
         s.out_count[grade]++;
         check(s.out <= s.budget.quantity, "budget exceeded");
         s.unresolved--;
      });
   }

   gch.erase(git);
}

void gacha::winreward(name owner, uint64_t id, int64_t score, extended_asset value) {
   require_auth(_self);
}

void gacha::raincheck(name owner, uint64_t id, int64_t score) {
   require_auth(_self);
}

}
