#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

#include "GlobalSequencer.hpp"
#include "OrderBook.hpp"
#include "ThreadPool.hpp"

namespace FastLittleMarket {

class Exchange {
 private:
  /*
  TODO: replace std::unordered_map<...> for pair
    stable_vector<Instrument>
    std::unordered_map<InstrumentId, Instrument*>

  template <class T, size_t ChunkSize>
  struct stable_vector {
    static_assert(ChunkSize % 2 == 0, "ChunkSize need to be a multiplier of 2");

    reference operator[](size_type i) {
      return (*mChunks[i / ChunkSize])[i % ChunkSize];
    }

    ...

    using Chunk = boost::container::static_vector<T, ChunkSize>;
    std::vector<std::unique_ptr<Chunk>> mChunks;
  }

  Reference: https://youtu.be/8uAW5FQtcvE?si=51X6YsEVReDvTg0H&t=1357
  */
  std::array<std::unordered_map<std::string, OrderBook>, NUM_SHARDS> shards_;
  ThreadPool thread_pool_;

  // std::array<GlobalSequencer, NUM_SHARDS> sequencers_;

 public:
  static Exchange& getInstance();

  size_t getShard(const std::string& symbol) const;
  void newOrder(const std::string& symbol, Order order);
  void cancelOrder(const std::string& symbol, uint64_t orderId);

  /*
  TODO: allow for async read of orders, even within same shard, using 'seqlock'
  type structure or itself

  General structure/idea for 'seqlock' is:

  template <class T> class SeqLock
  {
    std::atomic<unit32_t> mVersion;
    T mdata;
  };

  template <class T>
  void SeqLock<T>::Store(const T& value)
  {
    mVersion += 1;
    std::memcopy(mData, &value, sizeof(T));
    mversion += 1;
  }

  template <class T>
  bool SeqLock<T>::Load(T& value)
  {
    const uint32_t version = mVersion.load();
    if (version & 1 != 0)
      return false;

    std::memcopy(&value, mData, sizeof(T));

    return version == mVersion;
  }

  Reference: https://youtu.be/8uAW5FQtcvE?si=lTHT9ta6A-PATWBH&t=2058
  */
  std::optional<Order> getOrder(const std::string& symbol,
                                uint64_t orderId) const;
};

}  // namespace FastLittleMarket