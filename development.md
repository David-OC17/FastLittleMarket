# Development

Some characteristics of the system are:
* Scale (real exchange may handle 3 million messages per second, thousands of participants, several million live orders, and 10, 000 symbols)
* Fairness (for example, use multicast for send out information and add re-transmitors servers (they record messages seen and if some message is missing they can ask each other or the ME to keep a consistent state))
* Reliability
* Durability
* Robustness to bad clients

## Compilation

### All checks (test, ASAN)

```bash
rm -rf build

cmake -B build \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_C_COMPILER=gcc-13 \
  -DCMAKE_CXX_COMPILER=g++-13 \
  -DBUILD_TESTS=ON \
  -DENABLE_ASAN=ON \
  -G Ninja

cmake --build build --parallel
```

### Coverage build

```bash
rm -rf build

cmake -B build \
  -DCMAKE_BUILD_TYPE=Coverage \
  -DCMAKE_C_COMPILER=gcc-13 \
  -DCMAKE_CXX_COMPILER=g++-13 \
  -DBUILD_TESTS=ON \
  -G Ninja

cmake --build build --parallel

cd build

gcovr -r .. \
  --gcov-executable gcov-13 \
  --filter ../core \
  --exclude ../test \
  --exclude '/usr/include/.*' \
  --exclude '.*external.*' \
  --gcov-ignore-parse-errors=negative_hits.warn \
  --html --html-details -o ../coverage/coverage.html
```

### benchmarking

```bash
sudo cpupower frequency-set --governor performance
```

```bash
rm -rf build

cmake -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER=gcc-13 \
  -DCMAKE_CXX_COMPILER=g++-13 \
  -DBUILD_BENCHMARKS=ON \
  -G Ninja
cmake --build build --target benchmarks --parallel

./build/benchmarks
```

One line statistics of the benchmark.

```bash
echo -1 | sudo tee /proc/sys/kernel/perf_event_paranoid

perf stat ./build/benchmarks
```

Create metadata for Hotspot to consume.

```bash
perf record -g --call-graph dwarf ./build/benchmarks
```

### clang-tidy

```bash
script -q -c "clang-tidy --checks='-*,readability-identifier-naming' \
  --header-filter='(core|test|common)/.*' \
  core/*.cpp core/*.hpp test/*.cpp common/*.hpp \                        
  -- -std=c++20 \
  -I./external/quill/include \
  -I./external/quill/include/quill/ \
  -I./external/concurrentqueue \
  -I./common \
  -I./core 2>&1" output.txt
```

## Current steps

0. Complete ME (logging --> timestamps)
1. TCP server with Boost.Asio — get raw bytes flowing
2. FIX parser — tag splitter, checksum, MsgType dispatch
3. FIX session state machine — logon, heartbeat, sequence numbers
4. Gateway inbound — NewOrderSingle and OrderCancelRequest → your types
5. Gateway outbound — ExecutionReport builder
6. Risk gate — simple checks before hitting the engine
7. Market data feed — UDP multicast of top-of-book after each match

## Major elements

![Exchange diagram](img/exchange_diagram.png)

1. OrderBook (NASDAQ style)
* [X] Basic OrderBook object
* [X] Handle adding orders
* [X] Handle remove orders
* [X] Handle modify orders
* [X] Handle query book (top)
* [X] Migrate to 32 bit packed struct

2. Support multiple symbols
_Core architecture_
* Each symbol has its own order book
* Fixed symbols, initially support 100
* Sharding of symbols (order books), initially 4 shards (25 symbols per shard)

_Concurrency model_
* Initially support 10 agents
* Real-time required --> how to give guarantees of latency?
* [X] Single-threaded (sim) → mutex free task queue via moodycamel:: ConcurrentQueue
* [X] Multi-threaded → Lock-free queues? Sharding?
* [X] Agent-parallel → one thread per shard, split symbols/OrderBooks on shards

_Order routing_
* [X] newOrder(symbol, order) → Find book → Add
* [X] removeOrder(symbol, order)
* [X] querySymbol(symbol)
* Handle incoming orders via lock-free queues per symbol (lock needed to write to queue?)

* [ ] registerClient()

![Sharding diagram](img/sharding_diagram.png)

* [ ] (Optional) Implement CPU affinity to sharding ME thread
* [ ] Increase sharding ME thread priority

* Check if cancel/replace is its own operation and implement it if so

3. FIX encoder - decoder
* Inspired by [cpp_fix_codec](https://github.com/robaho/cpp_fixed)

3. Append only logger
* [ ] Log events from ME
* [ ] Log events from port/routing
* Implement to happen async from each process (no wait) --> write lock required?
* [ ] Ensure consistency (ops are complete and in order up to current) --> operation number

2. API for participants --> ports with TCP connections

3. "Cancel fairy" (for cancellations in the future)
* [ ] Cancel reject (in case a cancel cannot go through the ME) (cancel rejects are required because the protocol specifies that any change in state, in either side, has to be acknowledged)

4. Trade reporter flow (logging in this case?)

5. Market data flow

6. Basic client

7. Advanced reporting
  + Private stream (per client)
    - Full order lifecycle
    - Includes rejects
  + Public market data
    - Trades
    - Book updates (aggregated)

## Upgrades

* Create TCP/IP ports for users to connect through and trade (ports may be the ones to find some ID of an order, in case a cancel or modify is sent, making the rest of the process cheaper for the cancel fairy and ME)
* Implement communication between internal exchange modules via UDP? is it the fastest
* Allow for Immediate-Of-Cancel and Good-Till-Cancelled orders
* Add an "auction fairy" which find out via some optimization which of the overlapping incoming orders find the price that maximizes the shares traded, sends result to matching engine
* Implement security for connection to the "outside"
* Implement robust logging to allow for state-machine-replication (of the entire system) --> can later allow for incredible reliability, as we can bring up full blocks/servers if they fail via replication of their deduced state at some point in time
* Implement a passive ME (watches ME output) to replicate the primary ME and take over if ME fails

* Check FieldMap size! currently too big --> migrate to template meta programming in the future

## Development notes

* Need for speed: one-in-flight --> latency determines throughput
* Avoid copies and allocates as much possible in the critical path

**Locking (the Jane Street way)**
* Every message has a unique topic
* And a per-topic sequence number
* Contributor tries to grab next sequence number (if fails, has to change state and retry with the next number)

**Testing**
* Unit testing
* Fuzzing
* State machine replication
* Latency testing
* Chaos engineering

## Resources

* (Design a limit order book - Jordan has no life)[https://www.youtube.com/watch?v=nmYx6tQxtSs]
* (How to build an exchange - Jane Street)[https://www.youtube.com/watch?v=b1e4t2k2KJY]

## Extra learning notes

| Criteria     | Use struct                  | Use class                 |
| ------------ | --------------------------- | ------------------------- |
| Purpose      | Plain data (ID, price, vol) | Behavior (logic, vtables) |
| sizeof()     | Fixed/predictable           | Variable OK               |
| Copy         | Frequent (queues)           | Infrequent                |
| Polymorphism | Never                       | Virtual methods           |
| Cache        | Hot path                    | Cold path                 |
| STL          | Value type                  | Policy/functor            |
