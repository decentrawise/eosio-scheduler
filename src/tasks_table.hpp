// tasks table for scheduler
#pragma once


#include <eosio/eosio.hpp>

template <typename T>
struct [[eosio::table("tasks")]] task_data {
  uint64_t id;
  eosio::time_point_sec timestamp;
  T data;

  uint64_t primary_key() const { return id; }
  uint64_t by_timestamp() const { return timestamp.utc_seconds; }
};

/*
  typedef eosio::multi_index<
    eosio::name("tasks"), task_data<T>,
    eosio::indexed_by<eosio::name("timestamp"), eosio::const_mem_fun<task_data<T>, uint64_t, &task_data<T>::by_timestamp>>
  > tasks_table;
*/
