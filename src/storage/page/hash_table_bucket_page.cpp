//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// hash_table_bucket_page.cpp
//
// Identification: src/storage/page/hash_table_bucket_page.cpp
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "storage/page/hash_table_bucket_page.h"
#include "common/logger.h"
#include "common/util/hash_util.h"
#include "storage/index/generic_key.h"
#include "storage/index/hash_comparator.h"
#include "storage/table/tmp_tuple.h"

namespace bustub {

//  private:
//   //  For more on BUCKET_ARRAY_SIZE see storage/page/hash_table_page_defs.h
//   // 总共可以存下的bucket_idx数目就是BUCKET_ARRAY_SIZE
//   char occupied_[(BUCKET_ARRAY_SIZE - 1) / 8 + 1];
//   // 0 if tombstone/brand new (never occupied), 1 otherwise.
//   char readable_[(BUCKET_ARRAY_SIZE - 1) / 8 + 1];
//   // #define MappingType std::pair<KeyType, ValueType>
//   MappingType array_[1];

// #define HASH_TABLE_BUCKET_TYPE HashTableBucketPage<KeyType, ValueType, KeyComparator>
/**
 * Scan the bucket and collect values that have the matching key
 *
 * @return true if at least one key matched
 */
// 同样的key可以对应不同的value
template <typename KeyType, typename ValueType, typename KeyComparator>
auto HASH_TABLE_BUCKET_TYPE::GetValue(KeyType key, KeyComparator cmp, std::vector<ValueType> *result) -> bool {
  for (uint32_t i = 0; i < BUCKET_ARRAY_SIZE; i++) {
    // LOG_INFO("judge page %d", i);
    if (!IsReadable(i)) {
      if (!IsOccupied(i)) {
        // LOG_INFO("not exit %d, break", i);
        break;
      }
      continue;
    }
    // 不能放一起判断
    if (cmp(key, KeyAt(i)) == 0) {
      // 这里result是指针所以用-> 如果是迭代器直接用.
      result->push_back(ValueAt(i));
    }
  }
  return !result->empty();
}

// task1
template <typename KeyType, typename ValueType, typename KeyComparator>
auto HASH_TABLE_BUCKET_TYPE::Insert(KeyType key, ValueType value, KeyComparator cmp) -> bool {
  // 满了直接返回
  if (IsFull()) {
    return false;
  }
  // 过程中判断有没有同样的k v 对, 找到相同的k和v不插入
  std::vector<ValueType> res;
  GetValue(key, cmp, &res);
  if (std::find(res.begin(), res.end(), value) != res.end()) {
    return false;
  }

  for (uint32_t i = 0; i < BUCKET_ARRAY_SIZE; i++) {
    if (!IsReadable(i)) {  // 只用判断isreadable remove之后的地方也可以插入
      SetOccupied(i);
      SetReadable(i);
      // #define MappingType std::pair<KeyType, ValueType>
      array_[i] = std::make_pair(key, value);
      return true;
    }
  }
  return false;
}

// task2
template <typename KeyType, typename ValueType, typename KeyComparator>
auto HASH_TABLE_BUCKET_TYPE::Remove(KeyType key, ValueType value, KeyComparator cmp) -> bool {
  for (uint32_t i = 0; i < BUCKET_ARRAY_SIZE; i++) {
    // 删除一个条目之后,IsOccupied仍然显示占据状态,删除只修改readable
    if (IsReadable(i) && cmp(key, KeyAt(i)) == 0 && value == ValueAt(i)) {
      uint32_t mask = 1 << (i % 8);
      // occupied_[i / 8] &= ~mask;  // 设置为0
      readable_[i / 8] &= ~mask;  // mask取反 01111
      return true;
    }
  }
  return false;
}

template <typename KeyType, typename ValueType, typename KeyComparator>
auto HASH_TABLE_BUCKET_TYPE::KeyAt(uint32_t bucket_idx) const -> KeyType {
  return array_[bucket_idx].first;
}

template <typename KeyType, typename ValueType, typename KeyComparator>
auto HASH_TABLE_BUCKET_TYPE::ValueAt(uint32_t bucket_idx) const -> ValueType {
  return array_[bucket_idx].second;
}

template <typename KeyType, typename ValueType, typename KeyComparator>
void HASH_TABLE_BUCKET_TYPE::RemoveAt(uint32_t bucket_idx) {
  uint32_t byte_idx = bucket_idx / 8;
  uint32_t idx = bucket_idx % 8;
  uint32_t mask = 1 << idx;
  readable_[byte_idx] &= ~mask;
}

template <typename KeyType, typename ValueType, typename KeyComparator>
auto HASH_TABLE_BUCKET_TYPE::IsOccupied(uint32_t bucket_idx) const -> bool {
  uint32_t byte_idx = bucket_idx / 8;
  uint32_t idx = bucket_idx % 8;
  return (occupied_[byte_idx] >> idx) & 1;
}

template <typename KeyType, typename ValueType, typename KeyComparator>
void HASH_TABLE_BUCKET_TYPE::SetOccupied(uint32_t bucket_idx) {
  // 就是设置为1
  uint32_t byte_idx = bucket_idx / 8;
  uint32_t idx = bucket_idx % 8;
  uint32_t mask = 1 << idx;  // 1左移idx位取或设置idx位为1
  occupied_[byte_idx] |= mask;
}

template <typename KeyType, typename ValueType, typename KeyComparator>
auto HASH_TABLE_BUCKET_TYPE::IsReadable(uint32_t bucket_idx) const -> bool {
  uint32_t byte_idx = bucket_idx / 8;
  uint32_t idx = bucket_idx % 8;
  // 在byte_idx的第idx位是否为1
  return (readable_[byte_idx] >> idx) & 1;
}

template <typename KeyType, typename ValueType, typename KeyComparator>
void HASH_TABLE_BUCKET_TYPE::SetReadable(uint32_t bucket_idx) {
  uint32_t byte_idx = bucket_idx / 8;
  uint32_t idx = bucket_idx % 8;
  uint32_t mask = 1 << idx;  // 1左移idx位取或设置idx位为1
  readable_[byte_idx] |= mask;
}

template <typename KeyType, typename ValueType, typename KeyComparator>
auto HASH_TABLE_BUCKET_TYPE::IsFull() -> bool {
  // BUCKET_ARRAY_SIZE和occupied 的容量大小不一样, 容量大小比前者小,每个单位occupied的位数和才是前者
  return NumReadable() == BUCKET_ARRAY_SIZE;
}

/**
 * @return the number of readable elements, i.e. current size
 */
template <typename KeyType, typename ValueType, typename KeyComparator>
auto HASH_TABLE_BUCKET_TYPE::NumReadable() -> uint32_t {
  uint32_t num = 0;
  uint32_t bucket_size = GetOccupiedSize();
  // 这里可以优化,每次计算都要调用一遍很浪费时间(倒也还好 都是遍历,判断不影响时间量级,只是时间比例)
  for (uint32_t i = 0; i < bucket_size; i++) {
    // if (IsReadable(i)) {
    //   num++;
    // }
    uint8_t readable = readable_[i];
    while (readable != 0) {
      readable &= readable - 1;
      num++;
    }
  }
  return num;
}

// 自定义的得到occupied_数组容量的函数
template <typename KeyType, typename ValueType, typename KeyComparator>
auto HASH_TABLE_BUCKET_TYPE::GetOccupiedSize() const -> uint32_t {
  return (BUCKET_ARRAY_SIZE - 1) / 8 + 1;
}

template <typename KeyType, typename ValueType, typename KeyComparator>
auto HASH_TABLE_BUCKET_TYPE::IsEmpty() -> bool {
  return NumReadable() == 0;
}

template <typename KeyType, typename ValueType, typename KeyComparator>
void HASH_TABLE_BUCKET_TYPE::PrintBucket() {
  uint32_t size = 0;
  uint32_t taken = 0;
  uint32_t free = 0;
  for (size_t bucket_idx = 0; bucket_idx < BUCKET_ARRAY_SIZE; bucket_idx++) {
    if (!IsOccupied(bucket_idx)) {
      break;
    }

    size++;

    if (IsReadable(bucket_idx)) {
      taken++;
    } else {
      free++;
    }
  }

  LOG_INFO("Bucket Capacity: %lu, Size: %u, Taken: %u, Free: %u", BUCKET_ARRAY_SIZE, size, taken, free);
}

// DO NOT REMOVE ANYTHING BELOW THIS LINE
template class HashTableBucketPage<int, int, IntComparator>;

template class HashTableBucketPage<GenericKey<4>, RID, GenericComparator<4>>;
template class HashTableBucketPage<GenericKey<8>, RID, GenericComparator<8>>;
template class HashTableBucketPage<GenericKey<16>, RID, GenericComparator<16>>;
template class HashTableBucketPage<GenericKey<32>, RID, GenericComparator<32>>;
template class HashTableBucketPage<GenericKey<64>, RID, GenericComparator<64>>;

// template class HashTableBucketPage<hash_t, TmpTuple, HashComparator>;

}  // namespace bustub
