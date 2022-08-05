//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// hash_table_test.cpp
//
// Identification: test/container/hash_table_test.cpp
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include <thread>  // NOLINT
#include <vector>

#include "buffer/buffer_pool_manager_instance.h"
#include "common/logger.h"
#include "container/hash/extendible_hash_table.h"
#include "gtest/gtest.h"
#include "murmur3/MurmurHash3.h"

namespace bustub {

// NOLINTNEXTLINE

// NOLINTNEXTLINE
TEST(HashTableTest, SampleTest) {
  auto *disk_manager = new DiskManager("test.db");
  auto *bpm = new BufferPoolManagerInstance(50, disk_manager);
  ExtendibleHashTable<int, int, IntComparator> ht("blah", bpm, IntComparator(), HashFunction<int>());
  std::cout << "[----------] set new hash_table" << std::endl;

  // insert a few values
  for (int i = 0; i < 50; i++) {
    ht.Insert(nullptr, i, i);
    std::vector<int> res;
    ht.GetValue(nullptr, i, &res);
    EXPECT_EQ(1, res.size()) << "Failed to insert " << i << std::endl;
    EXPECT_EQ(i, res[0]);
  }
  std::cout << "[----------] insert value" << std::endl;
  ht.VerifyIntegrity();

  // check if the inserted values are all there
  for (int i = 0; i < 5; i++) {
    std::vector<int> res;
    ht.GetValue(nullptr, i, &res);
    EXPECT_EQ(1, res.size()) << "Failed to keep " << i << std::endl;
    EXPECT_EQ(i, res[0]);
  }
  std::cout << "[----------] check inserted value" << std::endl;
  ht.VerifyIntegrity();

  // insert one more value for each key
  for (int i = 0; i < 5; i++) {
    if (i == 0) {
      // duplicate values for the same key are not allowed
      EXPECT_FALSE(ht.Insert(nullptr, i, 2 * i));
    } else {
      EXPECT_TRUE(ht.Insert(nullptr, i, 2 * i));
    }
    ht.Insert(nullptr, i, 2 * i);
    std::vector<int> res;
    ht.GetValue(nullptr, i, &res);
    if (i == 0) {
      // duplicate values for the same key are not allowed
      EXPECT_EQ(1, res.size());
      EXPECT_EQ(i, res[0]);
    } else {
      EXPECT_EQ(2, res.size());
      if (res[0] == i) {
        EXPECT_EQ(2 * i, res[1]);
      } else {
        EXPECT_EQ(2 * i, res[0]);
        EXPECT_EQ(i, res[1]);
      }
    }
  }

  std::cout << "[----------] insert one more value for each key" << std::endl;

  ht.VerifyIntegrity();

  // look for a key that does not exist
  std::vector<int> res;
  ht.GetValue(nullptr, 100, &res);
  EXPECT_EQ(0, res.size());

  std::cout << "[----------] look for a key that does not exist" << std::endl;

  // delete some values
  for (int i = 0; i < 5; i++) {
    EXPECT_TRUE(ht.Remove(nullptr, i, i));
    std::vector<int> res;
    ht.GetValue(nullptr, i, &res);
    if (i == 0) {
      // (0, 0) is the only pair with key 0
      EXPECT_EQ(0, res.size());
    } else {
      EXPECT_EQ(1, res.size());
      EXPECT_EQ(2 * i, res[0]);
    }
  }
  std::cout << "[----------] delete some values" << std::endl;
  ht.VerifyIntegrity();

  // print all the key and value
  for (int i = 0; i < 5; i++) {
    std::vector<int> res;
    ht.GetValue(nullptr, i, &res);
    if (i == 0) {
      // std::cout << "i == 0 GetValue is " << ht.GetValue(nullptr, i, &res) << std::endl;
      EXPECT_FALSE(ht.GetValue(nullptr, i, &res));
      continue;
    }
    // std::cout << "res.size() = " << res.size() << std::endl;
    // std::cout << "res[0] = " << res[0] << std::endl;
    EXPECT_EQ(1, res.size());
    EXPECT_EQ(2 * i, res[0]);
  }
  std::cout << "[----------] test exit key and value" << std::endl;
  ht.VerifyIntegrity();

  // Remove测试不通过, 卡在循环
  // delete all values
  std::cout << "[----------] test delete" << std::endl;
  for (int i = 0; i < 5; i++) {
    // std::vector<int> res;
    // ht.GetValue(nullptr, i, &res);
    if (i == 0) {
      // (0, 0) has been deleted
      // std::cout << "k v exit " << ht.GetValue(nullptr, i, &res) << std::endl;
      // std::cout << "remove i is " << ht.Remove(nullptr, i, 2 * i) << std::endl;
      EXPECT_FALSE(ht.Remove(nullptr, i, 2 * i));
    } else {
      // std::cout << "removing value " << i << std::endl;
      // assert(ht.Remove(nullptr, i, 2 * i));
      // i == 3 的时候需要merge
      EXPECT_TRUE(ht.Remove(nullptr, i, 2 * i));
    }
  }

  ht.VerifyIntegrity();

  disk_manager->ShutDown();
  remove("test.db");
  delete disk_manager;
  delete bpm;

  std::cout << "[----------] all test ends" << std::endl;
}

}  // namespace bustub
