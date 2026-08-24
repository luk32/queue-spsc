#include <algorithm>
#include <gtest/gtest.h>
#include <random>
#include <thread>
#include <vector>

#include "../naive.hpp"

const std::size_t int_vector_size = 1024*1024u;

class NaiveQueueTest : public testing::Test {
  protected:
    using T = int;

    NaiveQueueTest() : 
        q0_{int_vector_size},
        q1_{1},
        q2_{2}
    {
      q1_.tryPush(1);
      q2_.tryPush(2);
      q2_.tryPush(3);
    }

    NaiveQueue<T> q0_;
    NaiveQueue<T> q1_;
    NaiveQueue<T> q2_;
    
    void reset_with_random_vals(std::vector<T>& vect);
};

TEST_F(NaiveQueueTest, IsEmptyInitially) {
  EXPECT_EQ(q0_.size(), 0);
}

TEST_F(NaiveQueueTest, SizeIsCorrect) {
  EXPECT_EQ(q1_.size(), 1);
}

TEST_F(NaiveQueueTest, PeekSeesFirstElement) {
  EXPECT_EQ(q2_.peek(), 2);
  EXPECT_EQ(q2_.peek(), 2);
}

TEST_F(NaiveQueueTest, TryPushOverCapacity) {
  EXPECT_FALSE(q1_.tryPush(15));
}

TEST_F(NaiveQueueTest, TryPopEmpty) {
    T e;
    ASSERT_FALSE(q0_.tryPop(e));
}

TEST_F(NaiveQueueTest, PopReturnsElementRemovedFromFront) {
  T val;
  ASSERT_TRUE(q2_.tryPop(val));
  EXPECT_EQ(val, 2);
  EXPECT_EQ(q2_.size(), 1);
  ASSERT_TRUE(q2_.tryPop(val));
  EXPECT_EQ(val, 3);
  EXPECT_EQ(q2_.size(), 0);
}


void NaiveQueueTest::reset_with_random_vals(std::vector<T>& vect) {
    static std::random_device rnd_dev;
    static std::default_random_engine rnd_eng{rnd_dev()};
    std::ranges::generate(vect, []{return rnd_eng();});
}

TEST_F(NaiveQueueTest, SingleThreadLargeRandomSet) {
    std::vector<T> expected(int_vector_size);
    std::vector<T> result;
    result.reserve(int_vector_size);
    reset_with_random_vals(expected);

    for (const auto& e: expected) {
        ASSERT_NO_THROW(q0_.tryPush(e));
    }
    EXPECT_EQ(q0_.size(), int_vector_size);


    T e;
    while(q0_.size()){
        while(result.size() < int_vector_size){
            q0_.tryPop(e);
            ASSERT_NO_THROW(result.emplace_back(e));
        }
    }

    EXPECT_EQ(result, expected);
}

TEST_F(NaiveQueueTest, TwoThreadLargeRandomSet) {
    std::vector<T> expected(int_vector_size);
    std::vector<T> result;
    result.reserve(int_vector_size);
    reset_with_random_vals(expected);

    std::thread producer([&]{
        for (const auto& e: expected) {
            ASSERT_NO_THROW(q0_.tryPush(e));
        }
    });

    std::thread consumer([&]{
        T e;
        while(result.size() < int_vector_size){
            // Need to assume that consumer thread caught up to producer.
            if(q0_.tryPop(e)) {
                ASSERT_NO_THROW(result.emplace_back(e));
            }
        }
    });

    producer.join();
    consumer.join();

    EXPECT_EQ(result.size(), expected.size());
    EXPECT_EQ(result, expected);
}

