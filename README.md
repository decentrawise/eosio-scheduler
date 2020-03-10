# eosio-scheduler
Tasks and workers scheduler for EOSIO contracts and tests built with eoslime

## How it works

The only things needed are to plug the scheduler header file and the base class on your contract code, specifying the owner for the scheduler, the handler for tasks and add the workers (using attach function) you want to perform work while idle on tasks. Then your contract will have to generate `scheduler::tick()` calls, either by user iteraction or using some other mechanism like [recurring_action](https://github.com/decentrawise/eosio-recurring-action) and the scheduler will take care of your tasks first and then execute workers by order until some of those had anything to do.

If there are no tasks and the workers didn't have anyting to do either, then `scheduler::tick()` will return false (returns true is something got done) and then it's up to your contract code to decide if the action asserts (to avoid CPU costs for nothing done) or just finishes, check `tick()` action on test contract.

<pre>
<b>#include "../../src/scheduler.hpp"</b>


class [[eosio::contract]] profile: public eosio::contract, <b>scheduler<eosio::name></b> {

  bool task_handler(eosio::name profile) {
    ...

    // return true if anything was done and needs to be kept
    // or false otherwise to cancel the transaction
    return false;
  };

  bool worker_handler() {
    ...

    // return true if this worker did any work, false otherwise so that the
    // scheduler can execute another worker and get something done in this tick
    return false;
  };

public:

  profile(eosio::name receiver, eosio::name code, eosio::datastream<const char*> ds) :
    contract(receiver, code, ds),
    <b>scheduler<eosio::name>(receiver, [&](auto profile) { return task_handler(profile); })</b>
  {
    // attach all workers here
    // they will be called when tick has no tasks to execute
    <b>attach([&](){ return worker_handler(); });</b>
  }

  [[eosio::action]]
  void ...

</pre>

## File tree

- src/ - source directory that contains `scheduler.hpp` main project code file

- contracts/ - test contract directory

- tests/ - tests specs

- scripts/ - helper scripts for running nodeos locally

## Install EOSLime
```bash
$ npm install -g eoslime
```

## Compile the example contract
```bash
$ eoslime compile
```

## Run a local EOSIO node
```bash
$ ./scripts/nodeos.sh
```
**NOTE**: Please customize the script to your local development needs. This might be made
easier in the future with configuration and a better script...

## Run tests
```bash
$ eoslime test
```
