/**
Copyright Łukasz Kucharski 2026

Naive implementation of a single producer - single consumer FIFO queue.
It uses std::queue (with deque) as a backend.

The implementation ensures correctness for independent pushing and consuming
threads. 

*/
#pragma once

#include <cstddef>
#include <queue>
#include <mutex>
#include <type_traits>
#include <utility>

template <typename DataT>
class NaiveQueue {
    static_assert(std::is_move_assignable_v<DataT>, "Data type must be movable.");
    
    public:
        NaiveQueue(std::size_t capacity) : cap{capacity} {};
        auto tryPush(const DataT& element) -> bool;
        auto tryPop(DataT& destination) -> bool;
        auto peek() const -> const DataT&;
        auto size() const -> std::size_t;
        auto capacity() const -> std::size_t;

    private:
        mutable std::mutex mtx;
        std::queue<DataT> queue;
        std::size_t cap;
};

template <typename DataT>
auto NaiveQueue<DataT>::capacity() const -> std::size_t
{
        return cap;
}

template <typename DataT>
auto NaiveQueue<DataT>::size() const -> std::size_t
{
        return queue.size();
}

template <typename DataT>
auto NaiveQueue<DataT>::tryPush(const DataT& element) -> bool
{
    const std::lock_guard lock(mtx);
    if(size() < capacity()) {
        queue.push(element);
        return true;
    }

    return false;
}

template <typename DataT>
auto NaiveQueue<DataT>::tryPop(DataT& destination) -> bool
{
    const std::lock_guard lock(mtx);
    if(size()){
        destination = std::move(queue.front());
        queue.pop();
        return true;
    }
    return false;
}

template <typename DataT>
auto NaiveQueue<DataT>::peek() const -> const DataT&
{
    const std::lock_guard lock(mtx);
    return queue.front();
}
