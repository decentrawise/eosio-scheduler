#include <eosio/eosio.hpp>
#include <eosio/transaction.hpp>
#include "../../src/scheduler.hpp"


class [[eosio::contract]] profile: public eosio::contract, scheduler<eosio::name> {

  bool task_handler(eosio::name profile) {
    // change user count to 100 if 0 or return false otherwise
    profiles_table profiles(_self, _self.value);
    auto iterator = profiles.find(profile.value);
    eosio::check(iterator != profiles.end(), "User doesn't have a profile");
    if (iterator->count < 100) {
      profiles.modify(iterator, eosio::same_payer, [&](auto& row) {
        row.count = 100;
      });
      return true;
    }

    // return true if anything was done and needs to be kept
    // or false otherwise to cancel the transaction
    return false;
  };

  bool worker_handler() {
    // check user
    profiles_table profiles(_self, _self.value);
    auto iterator = profiles.begin();
    if (iterator->count < 100) {
      profiles.modify(iterator, eosio::same_payer, [&](auto& row) {
        row.count += 1;
      });
      return true;
    }

    // return true if this worker did any work, false otherwise so that the
    // scheduler can execute another worker and get something done in this tick
    return false;
  };

public:

  profile(eosio::name receiver, eosio::name code, eosio::datastream<const char*> ds) :
    contract(receiver, code, ds),
    scheduler<eosio::name>(receiver, [&](auto profile) { return task_handler(profile); })
  {
    // attach all workers here
    // they will be called when tick has no tasks to execute
    attach([&](){ return worker_handler(); });
  }

  [[eosio::action]]
  void update(eosio::name user, std::string nickname, std::string avatar,
              std::string website, std::string locale, std::string metadata) {
    require_auth(user);

    profiles_table profiles(_self, _self.value);
    auto iterator = profiles.find(user.value);
    if (iterator == profiles.end()) {
      // Create new
      profiles.emplace(user, [&](auto& row) {
        row.user = user;
        row.nickname = nickname;
        row.avatar = avatar;
        row.website = website;
        row.locale = locale;
        row.metadata = metadata;
      });
    } else {
      // Update existing
      profiles.modify(iterator, user, [&](auto& row) {
        row.user = user;
        row.nickname = nickname;
        row.avatar = avatar;
        row.website = website;
        row.locale = locale;
        row.metadata = metadata;
      });
    }
  }

  [[eosio::action]]
  void remove(eosio::name user) {
    require_auth(user);

    profiles_table profiles(_self, _self.value);
    auto iterator = profiles.find(user.value);
    eosio::check(iterator != profiles.end(), "User doesn't have a profile");
    profiles.erase(iterator);
  }

  [[eosio::action]]
  void tick() {
    // if the scheduler tick function returns true, it means that something has been done, so the state needs to be maintained,
    // otherwise, the transaction can simply be cancelled to avoid CPU costs for nothing done
    eosio::check(scheduler::tick(), "Nothing done");
  }

  [[eosio::action]]
  void schedule(eosio::name user) {
    require_auth(user);

    // check user
    profiles_table profiles(_self, _self.value);
    auto iterator = profiles.find(user.value);
    eosio::check(iterator != profiles.end(), "User doesn't have a profile");

    // calc 10 secods from now and schedule
    eosio::time_point_sec ntime(eosio::current_time_point());
    ntime += eosio::seconds(10);
    scheduler::schedule(ntime, user);
  }

protected:

  struct [[eosio::table]] profile_entry {
    eosio::name user;
    std::string nickname;
    std::string avatar;
    std::string website;
    std::string locale;
    std::string metadata;
    uint64_t count = 0;

    uint64_t primary_key() const { return user.value; }
    uint64_t by_count() const { return count; }
  };
  typedef eosio::multi_index<eosio::name("profiles"), profile_entry> profiles_table;

};
