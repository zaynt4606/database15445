//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// extendible_hash_table.cpp
//
// Identification: src/container/hash/extendible_hash_table.cpp
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include <cmath>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "common/exception.h"
#include "common/logger.h"
#include "common/rid.h"
#include "container/hash/extendible_hash_table.h"

namespace bustub {

/*
 * // member variables
 * page_id_t directory_page_id_;
 * BufferPoolManager *buffer_pool_manager_;
 * KeyComparator comparator_;
 *
 * // Readers includes inserts and removes, writers are splits and merges
 * ReaderWriterLatch table_latch_;
 * HashFunction<KeyType> hash_fn_;
 */
template <typename KeyType, typename ValueType, typename KeyComparator>
HASH_TABLE_TYPE::ExtendibleHashTable(const std::string &name, BufferPoolManager *buffer_pool_manager,
                                     const KeyComparator &comparator, HashFunction<KeyType> hash_fn)
    : buffer_pool_manager_(buffer_pool_manager), comparator_(comparator), hash_fn_(std::move(hash_fn)) {
  // dir_page是一个page
  auto dir_page = buffer_pool_manager_->NewPage(&directory_page_id_);
  // 把dir_page转化为一个HashTableDirectoryPage
  auto dir_page_data = reinterpret_cast<HashTableDirectoryPage *>(dir_page->GetData());

  // 初始化两个bucket
  page_id_t bucket_page_0;
  page_id_t bucket_page_1;
  buffer_pool_manager_->NewPage(&bucket_page_0);
  buffer_pool_manager_->NewPage(&bucket_page_1);

  // 对应的page_id设置桶的id并设置ld
  dir_page_data->SetBucketPageId(0, bucket_page_0);
  dir_page_data->SetLocalDepth(0, 1);
  dir_page_data->SetBucketPageId(1, bucket_page_1);
  dir_page_data->SetLocalDepth(1, 1);

  // 更新
  dir_page_data->IncrGlobalDepth();
  dir_page_data->SetPageId(directory_page_id_);

  // UnpinPage
  buffer_pool_manager_->UnpinPage(directory_page_id_, true);  // 需要更新
  buffer_pool_manager_->UnpinPage(bucket_page_0, false);
  buffer_pool_manager_->UnpinPage(bucket_page_1, false);
}

/*****************************************************************************
 * HELPERS
 *****************************************************************************/
/**
 * Hash - simple helper to downcast MurmurHash's 64-bit hash to 32-bit
 * for extendible hashing.
 *
 * @param key the key to hash
 * @return the downcasted 32-bit hash
 */
template <typename KeyType, typename ValueType, typename KeyComparator>
auto HASH_TABLE_TYPE::Hash(KeyType key) -> uint32_t {
  return static_cast<uint32_t>(hash_fn_.GetHash(key));
}

template <typename KeyType, typename ValueType, typename KeyComparator>
auto HASH_TABLE_TYPE::Pow(uint32_t base, uint32_t power) -> uint32_t {
  return static_cast<uint32_t>(std::pow(static_cast<long double>(base), static_cast<long double>(power)));
}

/**
 * 头文件定义DirectoryIndex = Hash(key) & GLOBAL_DEPTH_MASK
 *
 * @param key the key to use for lookup
 * @param dir_page to use for lookup of global depth
 * @return the directory index 也就是bucket_id
 */
template <typename KeyType, typename ValueType, typename KeyComparator>
inline auto HASH_TABLE_TYPE::KeyToDirectoryIndex(KeyType key, HashTableDirectoryPage *dir_page) -> uint32_t {
  return Hash(key) & dir_page->GetGlobalDepthMask();
}

/**
 * Get the bucket page_id corresponding to a key.
 *
 * @param key the key for lookup
 * @param dir_page a pointer to the hash table's directory page
 * @return the bucket page_id corresponding to the input key
 * 根据key和dir_page_data得到对应的bucket_page_id
 */
template <typename KeyType, typename ValueType, typename KeyComparator>
inline auto HASH_TABLE_TYPE::KeyToPageId(KeyType key, HashTableDirectoryPage *dir_page) -> uint32_t {
  return dir_page->GetBucketPageId(KeyToDirectoryIndex(key, dir_page));
}

/**
 * Fetches the directory page from the buffer pool manager.
 *
 * @return a pointer to the directory page
 * 引入一个指向当前HashTableDirectoryPage的指针, 用dir_page_data指代
 */
template <typename KeyType, typename ValueType, typename KeyComparator>
auto HASH_TABLE_TYPE::FetchDirectoryPage() -> HashTableDirectoryPage * {
  // FetchPage返回的是一个page
  auto tem_page = buffer_pool_manager_->FetchPage(directory_page_id_);
  return reinterpret_cast<HashTableDirectoryPage *>(tem_page->GetData());
}

/**
 * Fetches the a bucket page from the buffer pool manager using the bucket's page_id.
 *
 * @param bucket_page_id the page_id to fetch
 * @return a pointer to a bucket page
 * 通过bucket_page_id得到对应的HashTableBucketPage指针,用hash_bucket_page指代
 */
// #define HASH_TABLE_BUCKET_TYPE HashTableBucketPage<KeyType, ValueType, KeyComparator>
template <typename KeyType, typename ValueType, typename KeyComparator>
auto HASH_TABLE_TYPE::FetchBucketPage(page_id_t bucket_page_id) -> HASH_TABLE_BUCKET_TYPE * {
  auto bucket_page = buffer_pool_manager_->FetchPage(bucket_page_id);
  return reinterpret_cast<HASH_TABLE_BUCKET_TYPE *>(bucket_page->GetData());
}

/*****************************************************************************
 * SEARCH
 *****************************************************************************/
/**
 * Performs a point query on the hash table.
 *
 * @param transaction the current transaction 当前事物
 * @param key the key to look up
 * @param[out] result the value(s) associated with a given key
 * @return the value(s) associated with the given key
 */
template <typename KeyType, typename ValueType, typename KeyComparator>
auto HASH_TABLE_TYPE::GetValue(Transaction *transaction, const KeyType &key, std::vector<ValueType> *result) -> bool {
  table_latch_.RLock();

  auto dir_page = FetchDirectoryPage();
  auto bucket_page_id = KeyToPageId(key, dir_page);
  auto hash_bucket_page = FetchBucketPage(bucket_page_id);
  // #define HASH_TABLE_BUCKET_TYPE HashTableBucketPage<KeyType, ValueType, KeyComparator>
  // auto GetValue(KeyType key, KeyComparator cmp, std::vector<ValueType> *result) -> bool;
  //  * @return true if at least one key matched
  auto success = hash_bucket_page->GetValue(key, comparator_, result);

  // 加读锁
  auto bucket_page = buffer_pool_manager_->FetchPage(bucket_page_id);
  bucket_page->RLatch();
  // 取消对该页的引用
  buffer_pool_manager_->UnpinPage(bucket_page_id, false);
  buffer_pool_manager_->UnpinPage(directory_page_id_, false);
  bucket_page->RUnlatch();
  table_latch_.RUnlock();

  return success;
}

/*****************************************************************************
 * INSERTION
 *****************************************************************************/
/**
 * Inserts a key-value pair into the hash table.
 *
 * @param transaction the current transaction
 * @param key the key to create
 * @param value the value to be associated with the key
 * @return true if insert succeeded, false otherwise
 */
template <typename KeyType, typename ValueType, typename KeyComparator>
auto HASH_TABLE_TYPE::Insert(Transaction *transaction, const KeyType &key, const ValueType &value) -> bool {
  table_latch_.RLock();
  auto dir_page = FetchDirectoryPage();
  auto bucket_page_id = KeyToPageId(key, dir_page);
  auto hash_bucket_page = FetchBucketPage(bucket_page_id);
  auto bucket_page = buffer_pool_manager_->FetchPage(bucket_page_id);
  bucket_page->WLatch();
  // std::cout << "start insert" << std::endl;
  if (hash_bucket_page->IsFull()) {
    // 没有走进来
    // bucket满了需要split再insert
    buffer_pool_manager_->UnpinPage(bucket_page_id, false);
    buffer_pool_manager_->UnpinPage(directory_page_id_, false);
    bucket_page->WUnlatch();
    table_latch_.RUnlock();
    // std::cout << "using split" << std::endl;
    return SplitInsert(transaction, key, value);
  }
  // 没有满可以直接Insert
  auto success = hash_bucket_page->Insert(key, value, comparator_);
  buffer_pool_manager_->UnpinPage(bucket_page_id, success);

  buffer_pool_manager_->UnpinPage(directory_page_id_, false);
  // std::cout << "finish insert" << std::endl;
  bucket_page->WUnlatch();
  table_latch_.RUnlock();
  // if (success == false) {
  //   std::cout << "false insert key " << key << std::endl;
  //   uint32_t num = hash_bucket_page->NumReadable();
  //   std::cout << " hash_bucket_page->NumReadable() = " << num << std::endl;
  //   std::cout << " hash_bucket_page->IsFull() " << hash_bucket_page->IsFull() << std::endl;
  //   std::cout << " full size " << BUCKET_ARRAY_SIZE << std::endl;
  // }
  return success;
}

/**
 * Performs insertion with an optional bucket splitting.
 *
 * @param transaction a pointer to the current transaction
 * @param key the key to insert
 * @param value the value to insert
 * @return whether or not the insertion was successful
 */
template <typename KeyType, typename ValueType, typename KeyComparator>
auto HASH_TABLE_TYPE::SplitInsert(Transaction *transaction, const KeyType &key, const ValueType &value) -> bool {
  std::cout << "split inset key = " << key << std::endl;
  table_latch_.WLock();
  auto dir_page = FetchDirectoryPage();
  auto success = false;   // 返回值,是否插入成功
  auto inserted = false;  // 判断是否在bucket没有满的时候执行insert操作
  auto growed = false;

  while (!inserted) {
    // 没有分裂出可以执行insert的bucket就一直分裂
    auto old_global_depth = dir_page->GetGlobalDepth();
    auto bucket_page_id = KeyToPageId(key, dir_page);
    auto bucket_page = buffer_pool_manager_->FetchPage(bucket_page_id);
    auto hash_bucket_page = FetchBucketPage(bucket_page_id);
    auto bucket_idx = KeyToDirectoryIndex(key, dir_page);

    bucket_page->WLatch();

    if (hash_bucket_page->IsFull()) {
      // 判断是否需要扩充directory
      if (dir_page->GetGlobalDepth() == dir_page->GetLocalDepth(bucket_idx)) {
        dir_page->IncrGlobalDepth();
        growed = true;
      }

      // 找到要分裂的bucket并更新
      dir_page->IncrLocalDepth(bucket_idx);
      auto split_bucket_idx = dir_page->GetSplitImageIndex(bucket_idx);  // 得到bucket_idx分裂的新split_bucket_idx
      page_id_t split_page_id;                                           // 用来新分配一个page
      auto new_page = buffer_pool_manager_->NewPage(&split_page_id);     // 新分配一个page并给split_page_id赋值
      auto split_page = reinterpret_cast<HASH_TABLE_BUCKET_TYPE *>(new_page->GetData());
      // 给新的bucket_page的id和ld对应起来
      dir_page->SetBucketPageId(split_bucket_idx, split_page_id);
      dir_page->SetLocalDepth(split_bucket_idx,
                              dir_page->GetLocalDepth(bucket_idx));  // 新的ld和之前的bucket_idx的ld一样

      // 对原来bucket_idx内的每个page重新分配
      uint32_t all_num_readable = hash_bucket_page->NumReadable();  // 满的这个bucket总共的存储数
      uint32_t tem_num_readable = 0;
      while (tem_num_readable < all_num_readable) {
        if (hash_bucket_page->IsReadable(tem_num_readable)) {
          auto tem_key = hash_bucket_page->KeyAt(tem_num_readable);
          uint32_t new_bucket_idx = Hash(tem_key) & (Pow(2, dir_page->GetLocalDepth(bucket_idx)) - 1);
          if ((new_bucket_idx ^ split_bucket_idx) == 0) {
            // 相同为0, 不同为1
            auto tem_value = hash_bucket_page->ValueAt(tem_num_readable);
            split_page->Insert(tem_key, tem_value, comparator_);
            hash_bucket_page->RemoveAt(tem_num_readable);
          }
          tem_num_readable++;
        }
      }
      buffer_pool_manager_->UnpinPage(split_page_id, true, nullptr);
      // redirect the rest of the buckets.
      //! for more info, see VerifyIntegrity().
      for (uint32_t i = Pow(2, old_global_depth); i < dir_page->Size(); i++) {
        if (i == split_bucket_idx) {
          continue;
        }
        uint32_t redirect_bucket_idx = i & (Pow(2, old_global_depth) - 1);
        dir_page->SetBucketPageId(i, dir_page->GetBucketPageId(redirect_bucket_idx));
        dir_page->SetLocalDepth(i, dir_page->GetLocalDepth(redirect_bucket_idx));
      }

    } else {
      success = hash_bucket_page->Insert(key, value, comparator_);
      inserted = true;
    }
    // 定义在循环内, 结束该循环的时候得unpin
    buffer_pool_manager_->UnpinPage(bucket_page_id, true, nullptr);
    bucket_page->WUnlatch();
  }
  buffer_pool_manager_->UnpinPage(directory_page_id_, growed, nullptr);
  table_latch_.WUnlock();
  return success;
}

/*****************************************************************************
 * REMOVE
 *****************************************************************************/
/**
 * Deletes the associated value for the given key.
 * 删除对应的kv对, 涉及到merge
 * @param transaction the current transaction
 * @param key the key to delete
 * @param value the value to delete
 * @return true if remove succeeded, false otherwise
 */
template <typename KeyType, typename ValueType, typename KeyComparator>
auto HASH_TABLE_TYPE::Remove(Transaction *transaction, const KeyType &key, const ValueType &value) -> bool {
  table_latch_.RLock();
  auto dir_page = FetchDirectoryPage();
  auto bucket_page_id = KeyToPageId(key, dir_page);
  auto hash_bucket_page = FetchBucketPage(bucket_page_id);
  auto bucket_page = buffer_pool_manager_->FetchPage(bucket_page_id);
  bucket_page->WLatch();
  auto success = hash_bucket_page->Remove(key, value, comparator_);

  if (success && hash_bucket_page->IsEmpty()) {
    // 需要merge
    buffer_pool_manager_->UnpinPage(bucket_page_id, success, nullptr);
    buffer_pool_manager_->UnpinPage(directory_page_id_, false, nullptr);
    bucket_page->WUnlatch();
    table_latch_.RUnlock();
    // std::cout << "start merging " << std::endl;
    Merge(transaction, key, value);
    // std::cout << "end mergind " << std::endl;
    // 这里结束了得返回
    return success;
  }
  // 不用merge
  buffer_pool_manager_->UnpinPage(bucket_page_id, success, nullptr);
  buffer_pool_manager_->UnpinPage(directory_page_id_, false, nullptr);
  bucket_page->WUnlatch();
  table_latch_.RUnlock();
  return success;
}

/*****************************************************************************
 * MERGE
 *****************************************************************************/
template <typename KeyType, typename ValueType, typename KeyComparator>
void HASH_TABLE_TYPE::Merge(Transaction *transaction, const KeyType &key, const ValueType &value) {
  table_latch_.WLock();

  auto dir_page = FetchDirectoryPage();
  uint32_t bucket_idx = 0;
  while (bucket_idx < dir_page->Size()) {
    // dir_page->Size() 每次收缩都会变化
    auto bucket_page_id = dir_page->GetBucketPageId(bucket_idx);
    auto hash_bucket_page = FetchBucketPage(bucket_page_id);
    auto bucket_page = buffer_pool_manager_->FetchPage(bucket_page_id);

    bucket_page->RLatch();

    auto old_local_depth = dir_page->GetLocalDepth(bucket_idx);
    if (old_local_depth > 1 && hash_bucket_page->IsEmpty()) {
      // i对应的bucket的ld大于1 并且为空 需要merge
      // 找到bucket_idx 对应的之前被split的split_bucket_idx来合并
      auto split_bucket_idx = dir_page->GetSplitImageIndex(bucket_idx);
      // 两个bucket的ld相同就可以(正常情况下是相同的)
      if (dir_page->GetLocalDepth(split_bucket_idx) == old_local_depth) {
        dir_page->DecrLocalDepth(bucket_idx);
        dir_page->DecrLocalDepth(split_bucket_idx);
        dir_page->SetBucketPageId(bucket_idx,
                                  dir_page->GetBucketPageId(split_bucket_idx));  // 把bucket_idx指向split也就是合并
        auto new_bucket_page_id = dir_page->GetBucketPageId(bucket_idx);         // 也就是上面后一项

        // 这两个bucket_page合并之后, 所有之前在内的bucket_idx都要重新指向新的new_bucket_page_id
        for (uint32_t j = 0; j < dir_page->Size(); j++) {
          if (j == bucket_idx || j == split_bucket_idx) {
            continue;
          }
          auto tem_bucket_page_id = dir_page->GetBucketPageId(j);
          if (tem_bucket_page_id == bucket_page_id || tem_bucket_page_id == new_bucket_page_id) {
            dir_page->SetLocalDepth(tem_bucket_page_id, dir_page->GetLocalDepth(split_bucket_idx));
            dir_page->SetBucketPageId(j, new_bucket_page_id);
          }
        }
      }
      // 合并之后判断一下gd是否可以shrink
      if (dir_page->CanShrink()) {
        dir_page->DecrGlobalDepth();
      }
    }
    bucket_page->RUnlatch();
    buffer_pool_manager_->UnpinPage(bucket_page_id, false, nullptr);
    // 别忘了还在while循环里
    bucket_idx++;
  }

  buffer_pool_manager_->UnpinPage(directory_page_id_, true, nullptr);
  table_latch_.WUnlock();
}

/*****************************************************************************
 * GETGLOBALDEPTH - DO NOT TOUCH
 *****************************************************************************/
template <typename KeyType, typename ValueType, typename KeyComparator>
auto HASH_TABLE_TYPE::GetGlobalDepth() -> uint32_t {
  table_latch_.RLock();
  HashTableDirectoryPage *dir_page = FetchDirectoryPage();
  uint32_t global_depth = dir_page->GetGlobalDepth();
  assert(buffer_pool_manager_->UnpinPage(directory_page_id_, false, nullptr));
  table_latch_.RUnlock();
  return global_depth;
}

/*****************************************************************************
 * VERIFY INTEGRITY - DO NOT TOUCH
 *****************************************************************************/
template <typename KeyType, typename ValueType, typename KeyComparator>
void HASH_TABLE_TYPE::VerifyIntegrity() {
  table_latch_.RLock();
  HashTableDirectoryPage *dir_page = FetchDirectoryPage();
  dir_page->VerifyIntegrity();
  assert(buffer_pool_manager_->UnpinPage(directory_page_id_, false, nullptr));
  table_latch_.RUnlock();
}

/*****************************************************************************
 * TEMPLATE DEFINITIONS - DO NOT TOUCH
 *****************************************************************************/
template class ExtendibleHashTable<int, int, IntComparator>;

template class ExtendibleHashTable<GenericKey<4>, RID, GenericComparator<4>>;
template class ExtendibleHashTable<GenericKey<8>, RID, GenericComparator<8>>;
template class ExtendibleHashTable<GenericKey<16>, RID, GenericComparator<16>>;
template class ExtendibleHashTable<GenericKey<32>, RID, GenericComparator<32>>;
template class ExtendibleHashTable<GenericKey<64>, RID, GenericComparator<64>>;

}  // namespace bustub
