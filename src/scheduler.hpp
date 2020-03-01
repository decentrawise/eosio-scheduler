// scheduler of events for EOSIO smart contracts
#pragma once


#include <vector>
#include <functional>

#include <eosio/eosio.hpp>
#include <eosio/system.hpp>


template <typename T>
class scheduler {

  // handler function ref for scheduled tasks
  // typedef bool (& handler_function)(T);
  typedef std::function<bool (T)> handler_function;

  // worker function ref for registered workers
  // typedef bool (& worker_function)();
  typedef std::function<bool ()> worker_function;

public:

  scheduler(eosio::name owner, handler_function handler) : owner(owner), handler(handler), workers() { }
  // scheduler(eosio::name owner) : owner(owner), handler(nullptr), workers() { }

  // void set_handler(handler_function handler) {
  //   this->handler = handler;
  // }

  void attach(worker_function work) {
    workers.push_back(work);
  }

  void schedule(eosio::time_point timestamp, T data) {
    tasks_table tasks(owner, owner.value);

    const auto pk = tasks.available_primary_key();
    tasks.emplace(owner, [&](auto &task) {
      task.id = pk;
      task.timestamp = timestamp;
      task.data = data;
    });
  }

  bool tick() {
    // if (handler == nullptr) return false;   // handler not initialized yet

    tasks_table tasks(owner, owner.value);
    bool done = false;

    // first try to process a scheduled task, if any already due
    auto tasks_itr = tasks.begin();
    // auto timestamp_idx = tasks.get_index<eosio::name("timestamp")>();
    // auto tasks_itr = timestamp_idx.begin();
    if (tasks_itr != tasks.end()) {
      // get the oldest task in queue
      auto const oldest = *tasks_itr;
      // Is it due already?
      eosio::time_point_sec ctime(eosio::current_time_point());
      if (oldest.timestamp >= ctime.utc_seconds) {
        // process the task
        done = handler(oldest.data);
        // remove the processed task from table
        tasks.erase(tasks_itr);
      }
    }
    // if nothing done yet, go through the workers, if any
    if (!done) {
      for (auto & work : workers) {
        if (done = work()) break;
      }
    }
    // return if did anything
    return done;
  }

protected:

  eosio::name owner;
  handler_function handler;
  std::vector<std::reference_wrapper<worker_function>> workers;

  struct [[eosio::table("tasks")]] task_data {
    uint64_t id;
    eosio::time_point_sec timestamp;
    T data;

    uint64_t primary_key() const { return id; }
    uint64_t by_timestamp() const { return timestamp.utc_seconds; }
  };

  typedef eosio::multi_index<
    eosio::name("tasks"), task_data,
    eosio::indexed_by<eosio::name("timestamp"), eosio::const_mem_fun<task_data, uint64_t, &task_data::by_timestamp>>
  > tasks_table;

};
