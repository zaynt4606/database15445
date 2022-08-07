//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// hash_table_header_page.cpp
//
// Identification: src/storage/page/hash_table_header_page.cpp
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "storage/page/hash_table_directory_page.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include "common/logger.h"

//  private:
//   page_id_t page_id_;  // 自身页编号
//   lsn_t lsn_;  // 日志序列号
//   uint32_t global_depth_{0};  // 全局位置编码
//   uint8_t local_depths_[DIRECTORY_ARRAY_SIZE];  // 局部位置编码, 表示在槽slot中找到对应的桶所需要的位数(深度)
//   page_id_t bucket_page_ids_[DIRECTORY_ARRAY_SIZE];  // 这是一个数组存储每个bucket_id对应的page_id

namespace bustub {
auto HashTableDirectoryPage::GetPageId() const -> page_id_t { return page_id_; }

void HashTableDirectoryPage::SetPageId(bustub::page_id_t page_id) { page_id_ = page_id; }

auto HashTableDirectoryPage::GetLSN() const -> lsn_t { return lsn_; }

void HashTableDirectoryPage::SetLSN(lsn_t lsn) { lsn_ = lsn; }

auto HashTableDirectoryPage::GetGlobalDepth() -> uint32_t { return global_depth_; }

// 和全局深度相同的mask,其实就是取全局深度,和全局深度相同长度的掩码的意思
auto HashTableDirectoryPage::GetGlobalDepthMask() -> uint32_t {
  uint32_t mask = (1 << global_depth_) - 1;
  return mask;
}

auto HashTableDirectoryPage::GetLocalDepthMask(uint32_t bucket_idx) -> uint32_t {
  uint32_t mask = (1 << GetLocalDepth(bucket_idx)) - 1;
  return mask;
}

void HashTableDirectoryPage::IncrGlobalDepth() { 
  // assert(global_depth_ < MAX_BUCKET_DEPTH);
  // int origin_num = 1 << global_depth_;
  // int new_index = origin_num;
  // int origin_index = 0;
  // // 把之前的index统一放到扩大之后的后面一批
  // for (; origin_index < origin_num; new_index++, origin_index++) {
  //   bucket_page_ids_[new_index] = bucket_page_ids_[origin_index];
  //   local_depths_[new_index] = local_depths_[origin_index];
  // }
  global_depth_++; 
}

void HashTableDirectoryPage::DecrGlobalDepth() { global_depth_--; }

/**
 * Lookup a bucket page using a directory index
 *
 * @param bucket_idx the index in the directory to lookup
 * @return bucket page_id corresponding to bucket_idx
 */
auto HashTableDirectoryPage::GetBucketPageId(uint32_t bucket_idx) -> page_id_t {
  page_id_t page_id = bucket_page_ids_[bucket_idx];
  return page_id;
}

// 给每个bucket_idx设置一个对应的page_id存放在bucket_page_ids_中
void HashTableDirectoryPage::SetBucketPageId(uint32_t bucket_idx, page_id_t bucket_page_id) {
  bucket_page_ids_[bucket_idx] = bucket_page_id;
}

// the current directory size
// 整个directory的大小
auto HashTableDirectoryPage::Size() -> uint32_t { return 1 << global_depth_; }

// true if the directory can be shrunk(收缩)
// 可以shrunk就是整个local_depths_都小于global_depth_,收缩的是global_depth_
auto HashTableDirectoryPage::CanShrink() -> bool {
  for (uint32_t i = 0; i < Size(); i++) {
    uint32_t depth = local_depths_[i];
    // assert(depth <= global_depth_);
    if (depth == global_depth_) {  // local不可能大于global所以只判断相等即可
      return false;
    }
  }
  return true;
}

/**
 * Gets the split image of an index
 *
 * @param bucket_idx the directory index for which to find the split image
 * @return the directory index of the split image
 **/
auto HashTableDirectoryPage::GetSplitImageIndex(uint32_t bucket_idx) -> uint32_t {
  // auto high_bits = GetLocalHighBit(bucket_idx);
  // uint32_t split_image_index =
  //     high_bits | (bucket_idx & static_cast<uint32_t>(std::pow(static_cast<long double>(2),
  //                                                              static_cast<long double>(GetLocalDepth(bucket_idx)))));
  // return split_image_index & GetLocalDepthMask(bucket_idx);

  uint32_t local_depth = local_depths_[bucket_idx];
  // 对应的split_idx就是最高位不同的那一个， 最高位取亦或
  return bucket_idx ^ (1 << (local_depth - 1));
}

/**
 * Gets the local depth of the bucket at bucket_idx
 *
 * @param bucket_idx the bucket index to lookup
 * @return the local depth of the bucket at bucket_idx
 */
auto HashTableDirectoryPage::GetLocalDepth(uint32_t bucket_idx) -> uint32_t { return local_depths_[bucket_idx]; }

void HashTableDirectoryPage::SetLocalDepth(uint32_t bucket_idx, uint8_t local_depth) {
  // assert(local_depth <= global_depth_);
  local_depths_[bucket_idx] = local_depth;
}

void HashTableDirectoryPage::IncrLocalDepth(uint32_t bucket_idx) { 
  local_depths_[bucket_idx]++;
  // assert(local_depths_[bucket_idx] <= global_depth_);
}

void HashTableDirectoryPage::DecrLocalDepth(uint32_t bucket_idx) { local_depths_[bucket_idx]--; }

/**
 * Gets the high bit corresponding to the bucket's local depth.
 * This is not the same as the bucket index itself.  This method
 * is helpful for finding the pair, or "split image", of a bucket.
 *
 * @param bucket_idx bucket index to lookup
 * @return the high bit corresponding to the bucket's local depth
 */
// 得到本地最大的bucket_idx对应的localmask + 1；
// 1001 得到 10000
auto HashTableDirectoryPage::GetLocalHighBit(uint32_t bucket_idx) -> uint32_t {
  auto local_depth = GetLocalDepth(bucket_idx);
  return ((bucket_idx >> (local_depth - 1)) + 1) << (local_depth - 1);
}

/**
 * VerifyIntegrity - Use this for debugging but **DO NOT CHANGE**
 *
 * If you want to make changes to this, make a new function and extend it.
 *
 * Verify the following invariants:
 * (1) All LD <= GD.
 * (2) Each bucket has precisely 2^(GD - LD) pointers pointing to it.
 * (3) The LD is the same at each index with the same bucket_page_id
 */
void HashTableDirectoryPage::VerifyIntegrity() {
  //  build maps of {bucket_page_id : pointer_count} and {bucket_page_id : local_depth}
  std::unordered_map<page_id_t, uint32_t> page_id_to_count = std::unordered_map<page_id_t, uint32_t>();
  std::unordered_map<page_id_t, uint32_t> page_id_to_ld = std::unordered_map<page_id_t, uint32_t>();

  //  verify for each bucket_page_id, pointer
  for (uint32_t curr_idx = 0; curr_idx < Size(); curr_idx++) {
    page_id_t curr_page_id = bucket_page_ids_[curr_idx];
    uint32_t curr_ld = local_depths_[curr_idx];
    assert(curr_ld <= global_depth_);  // 判断条件是不是成立,不成立

    ++page_id_to_count[curr_page_id];

    if (page_id_to_ld.count(curr_page_id) > 0 && curr_ld != page_id_to_ld[curr_page_id]) {
      // 已经存在并且发现ld不同
      uint32_t old_ld = page_id_to_ld[curr_page_id];
      LOG_WARN("Verify Integrity: curr_local_depth: %u, old_local_depth %u, for page_id: %u", curr_ld, old_ld,
               curr_page_id);
      PrintDirectory();
      assert(curr_ld == page_id_to_ld[curr_page_id]);
    } else {
      page_id_to_ld[curr_page_id] = curr_ld;
    }
  }

  auto it = page_id_to_count.begin();

  while (it != page_id_to_count.end()) {
    page_id_t curr_page_id = it->first;
    uint32_t curr_count = it->second;
    uint32_t curr_ld = page_id_to_ld[curr_page_id];
    uint32_t required_count = 0x1 << (global_depth_ - curr_ld);

    if (curr_count != required_count) {
      LOG_WARN("Verify Integrity: curr_count: %u, required_count %u, for page_id: %u", curr_count, required_count,
               curr_page_id);
      PrintDirectory();
      assert(curr_count == required_count);
    }
    it++;
  }
}

void HashTableDirectoryPage::PrintDirectory() {
  LOG_DEBUG("======== DIRECTORY (global_depth_: %u) ========", global_depth_);
  LOG_DEBUG("| bucket_idx | page_id | local_depth |");
  for (uint32_t idx = 0; idx < static_cast<uint32_t>(0x1 << global_depth_); idx++) {
    LOG_DEBUG("|      %u     |     %u     |     %u     |", idx, bucket_page_ids_[idx], local_depths_[idx]);
  }
  LOG_DEBUG("================ END DIRECTORY ================");
}

}  // namespace bustub
