#include "gmock/gmock.h"
#include <algorithm>
#include <gtest/gtest.h>
#include <random>
#include <thread>
#include <vector>

#include "../circular_buffer.hpp"
#include "../naive.hpp"

const std::size_t int_vector_size = 128 * 1024u;

using DataT = int;

template <typename QueueT> class QueueTest : public testing::Test {
protected:
  QueueTest()
      : q0_{int_vector_size}, q1_{1}, q2_{2}, q4_{int_vector_size >> 1} {
    q1_.tryPush(1);
    q2_.tryPush(2);
    q2_.tryPush(1337);
    q2_.tryPush(42);
  }

  QueueT q0_;
  QueueT q1_;
  QueueT q2_;
  QueueT q4_;

  void reset_with_random_vals(std::vector<DataT> &vect) {
    static std::random_device rnd_dev;
    static std::default_random_engine rnd_eng{rnd_dev()};
    std::ranges::generate(vect, [] { return rnd_eng(); });
  }
};

using QueueTypes =
    ::testing::Types<NaiveQueue<DataT>, CirularBufferQueue<DataT>>;
TYPED_TEST_SUITE(QueueTest, QueueTypes);

TYPED_TEST(QueueTest, IsEmptyInitially) { EXPECT_EQ(this->q0_.size(), 0); }

TYPED_TEST(QueueTest, SizeIsCorrect) {
  EXPECT_EQ(this->q0_.size(), 0);
  EXPECT_EQ(this->q1_.size(), 1);
  EXPECT_EQ(this->q2_.size(), 2);
}

TYPED_TEST(QueueTest, PeekSeesFirstElement) {
  EXPECT_EQ(this->q2_.peek(), 2);
  EXPECT_EQ(this->q2_.peek(), 2);
}

TYPED_TEST(QueueTest, TryPushOverCapacityIsIdempotent) {
  const auto initial_size = this->q1_.size();
  EXPECT_FALSE(this->q1_.tryPush(15));
  EXPECT_FALSE(this->q1_.tryPush(16));
  EXPECT_FALSE(this->q1_.tryPush(17));
  EXPECT_EQ(initial_size, this->q1_.size());
}

TYPED_TEST(QueueTest, TryPopEmpty) {
  DataT e;
  ASSERT_FALSE(this->q0_.tryPop(e));
  EXPECT_EQ(0, this->q0_.size());
}

TYPED_TEST(QueueTest, PopReturnsElementRemovedFromFront) {
  DataT val;
  ASSERT_TRUE(this->q2_.tryPop(val));
  EXPECT_EQ(val, 2);
  EXPECT_EQ(this->q2_.size(), 1);
  ASSERT_TRUE(this->q2_.tryPop(val));
  EXPECT_EQ(val, 1337);
  EXPECT_EQ(this->q2_.size(), 0);
}

TYPED_TEST(QueueTest, SingleThreadLargeRandomSet) {
  std::vector<DataT> expected(int_vector_size);
  std::vector<DataT> result;
  result.reserve(int_vector_size);
  this->reset_with_random_vals(expected);

  for (auto &e : expected) {
    ASSERT_NO_THROW(this->q0_.tryPush(e));
  }
  EXPECT_EQ(this->q0_.size(), int_vector_size);

  DataT e;
  while (this->q0_.size()) {
    while (result.size() < int_vector_size) {
      this->q0_.tryPop(e);
      ASSERT_NO_THROW(result.emplace_back(e));
    }
  }

  EXPECT_EQ(result, expected);
}

/**
Tests whether concurrent pushing and popping from the queue yields original
vector.

WARNING: False positives might happen. Beware that in case of multithreaded
tests the result here might be volatile, depending on the correctness of
syncronization between threads.
*/
TYPED_TEST(QueueTest, TwoThreadLargeRandomSet) {
  std::vector<DataT> expected(int_vector_size);
  std::vector<DataT> result;
  result.reserve(int_vector_size);
  this->reset_with_random_vals(expected);

  std::thread producer([&] {
    for (auto &e : expected) {
      ASSERT_NO_THROW(this->q0_.tryPush(e));
    }
  });

  std::thread consumer([&] {
    DataT e;
    while (result.size() < int_vector_size) {
      // Need to assume that consumer thread caught up to producer.
      if (this->q0_.tryPop(e)) {
        ASSERT_NO_THROW(result.emplace_back(e));
      }
    }
  });

  producer.join();
  consumer.join();

  EXPECT_EQ(result.size(), expected.size());
  EXPECT_THAT(result, ::testing::ContainerEq(expected));
}
