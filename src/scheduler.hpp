// scheduler of events for EOSIO smart contracts
#pragma once


#include <string>
#include <vector>
#include <functional>

#include <libc/bits/stdint.h>

#include <eosio/eosio.hpp>
#include <eosio/system.hpp>
#include <eosio/transaction.hpp>

#include "tasks_table.hpp"


template <typename T>
class scheduler {

  // handler function ref for scheduled tasks
  typedef std::function<bool (T)> handler_function;

  // worker function ref for registered workers
  typedef std::function<bool ()> worker_function;

  // the tasks multi_index table
  typedef eosio::multi_index<
    eosio::name("tasks"), task_data<T>,
    eosio::indexed_by<eosio::name("timestamp"), eosio::const_mem_fun<task_data<T>, uint64_t, &task_data<T>::by_timestamp>>
  > tasks_table;

  // the scheduler owner account
  // this is only used for tasks table initialization and scope
  eosio::name owner;

  // the task handler
  // whenever there is a task due, this handler is executed with the task data
  handler_function handler;

  // the workers attached to the scheduler
  // these run whenever the tick runs and there are no scheduled tasks to be executed
  std::vector<std::reference_wrapper<worker_function>> workers;

protected:

  scheduler(eosio::name owner, handler_function handler) : owner(owner), handler(handler), workers() { }

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
    tasks_table tasks(owner, owner.value);
    bool done = false;

    // first try to process a scheduled task, if any already due
    // use `tasks.template get_index<>()` call to tell compiler that the get_index is a template
    // the compiler doesn't resolve well with just `tasks.get_index<>()` call
    // this was a fun one to figure out... :)
    auto timestamp_idx = tasks.template get_index<eosio::name("timestamp")>();
    auto tasks_itr = timestamp_idx.begin();
    if (tasks_itr != timestamp_idx.end()) {
      // get the oldest task in queue
      auto oldest = *tasks_itr;
      // Is it due already?
      eosio::time_point_sec ctime(eosio::current_time_point());
      if (oldest.timestamp.utc_seconds <= ctime.utc_seconds) {
        // process the task
        done = handler(oldest.data);
        // remove the processed task from table
        timestamp_idx.erase(tasks_itr);
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

};
