/**
Copyright Łukasz Kucharski 2026

Circular buffer implementation of a single producer - single consumer FIFO
queue.

The implementation ensures correctness for independent pushing and consuming
threads.

The buffer is created by make_unique_for_overwrite to have storage for DataT
objects. It still pays for default-initialization and forbids implementation
of "emplace" which would make sense for more complicated objects. For this
reason it is required that it's possible to move-assign held objects.
Otherwise, a buffer of raw bytes, placement new and reinterpreting would have
to be used.

pop and push indices will grow monotonically, so number of pushes cannot
exceed std::numeric_limits<std::size_t>::max throughout the lifetime of the
object. This is to avoid ambiguity and additional branching when front
catches up to the back.
*/

#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <type_traits>
#include <utility>

template <typename DataT>
class CirularBufferQueue {
  static_assert(std::is_move_assignable_v<DataT>, "Data type must be movable.");

public:
  using ValueT = DataT;

  CirularBufferQueue(std::size_t capacity)
      : cap{capacity},
        queue{std::make_unique_for_overwrite<DataT[]>(capacity)} {};

  /* Pushes new element onto the queue.

  It will move the element if possible and copy otherwise.

  Returns true if succeeded and false otherwise. E.g. if queue is full.
  */
  template <typename... Args>
    requires std::constructible_from<DataT, Args...>
  auto tryPush(Args &&...args) -> bool;

  /* Pops element from the queue into the destination object via move.

  Returns true if succeeded and false otherwise. E.g. when queue is empty.
  */
  auto tryPop(DataT &destination) -> bool;

  /* Returns reference to the front of the queue.

  Popping will invalidate this reference.
  */
  auto peek() const -> const DataT &;

  // Returns current number of elements on the queue.
  auto size() const -> std::size_t;

  // Returns maximum number of elements on the queue.
  auto capacity() const -> std::size_t;

private:
  mutable std::mutex mtx_counter;
  std::atomic_flag writing;
  std::unique_ptr<DataT[]> queue;
  std::size_t front_idx = 0;
  std::size_t back_idx = 0;
  std::size_t cap;
};

template <typename DataT>
auto CirularBufferQueue<DataT>::capacity() const -> std::size_t {
  return cap;
}

template <typename DataT>
auto CirularBufferQueue<DataT>::size() const -> std::size_t {
  return back_idx - front_idx;
}

template <typename DataT>
template <typename... Args>
  requires std::constructible_from<DataT, Args...>
auto CirularBufferQueue<DataT>::tryPush(Args &&...args) -> bool {
  std::unique_lock lock_ctr(mtx_counter);

  if (size() == capacity()) {
    lock_ctr.unlock();
    return false;
  }

  writing.test_and_set(std::memory_order_acquire);
  const auto idx = back_idx++;
  lock_ctr.unlock();

  queue[idx % capacity()] = {std::forward<Args...>(args)...};
  writing.clear(std::memory_order_release);
  writing.notify_one();
  /*
   Also, better "move" doesn't throw or the lock won't get unlocked.
   */
  return true;
}

template <typename DataT>
auto CirularBufferQueue<DataT>::tryPop(DataT &destination) -> bool {
  std::unique_lock lock(mtx_counter);
  if (size() == 0) {
    lock.unlock();
    return false;
  }
  const auto idx = front_idx++;
  lock.unlock();

  if (size() == 0 && writing.test(std::memory_order_acquire)) {
    writing.wait(true);
  }

  destination = std::move(queue[idx % capacity()]);
  return true;
}

template <typename DataT>
auto CirularBufferQueue<DataT>::peek() const -> const DataT & {
  const std::lock_guard lock(mtx_counter);
  return queue[front_idx];
}
// TODO: I'm actually curious if this unlocks before copy if the caller assigns
// to a value. Using a reference in multhithreaded context is dangerous.