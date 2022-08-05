//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// parallel_buffer_pool_manager.cpp
//
// Identification: src/buffer/buffer_pool_manager.cpp
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "buffer/parallel_buffer_pool_manager.h"

namespace bustub {

ParallelBufferPoolManager::ParallelBufferPoolManager(size_t num_instances, size_t pool_size, DiskManager *disk_manager,
                                                     LogManager *log_manager)
    : buffer_pools_{num_instances}, pool_size_(pool_size) {
  // Allocate and create individual BufferPoolManagerInstances
  num_instances_ = num_instances;
  for (size_t index = 0; index < num_instances; index++) {
    buffer_pools_[index] = new BufferPoolManagerInstance(pool_size, num_instances, index, disk_manager, log_manager);
  }
}

// Update constructor to destruct all BufferPoolManagerInstances and deallocate any associated memory
ParallelBufferPoolManager::~ParallelBufferPoolManager() {
  for (auto &bp : buffer_pools_) {
    delete bp;
  }
}

auto ParallelBufferPoolManager::GetPoolSize() -> size_t {
  std::lock_guard<std::mutex> lock(latch_);
  // Get size of all BufferPoolManagerInstances
  // 求的是总大小
  return pool_size_ * num_instances_;
}

auto ParallelBufferPoolManager::GetBufferPoolManager(page_id_t page_id) -> BufferPoolManager * {
  // std::lock_guard<std::mutex> lock(latch_);  // 这里不能加锁
  /**
   * @param page_id id of page
   * @return pointer to the BufferPoolManager responsible for handling given page id
   */
  // 后面都会用到这个, 返回page_id对应的BufferPoolManager,自己定义的,直接用vector映射当做哈希
  // Get BufferPoolManager responsible for handling given page id. You can use this method in your other methods.
  return buffer_pools_[page_id % num_instances_];
}

auto ParallelBufferPoolManager::FetchPgImp(page_id_t page_id) -> Page * {
  std::lock_guard<std::mutex> lock(latch_);
  /**
   * Fetch the requested page from the buffer pool.
   * @param page_id id of page to be fetched
   * @return the requested page
   */
  // Fetch page for page_id from responsible BufferPoolManagerInstance
  return GetBufferPoolManager(page_id)->FetchPage(page_id);
}

auto ParallelBufferPoolManager::UnpinPgImp(page_id_t page_id, bool is_dirty) -> bool {
  std::lock_guard<std::mutex> lock(latch_);
  /**
   * Unpin the target page from the buffer pool.
   * @param page_id id of page to be unpinned
   * @param is_dirty true if the page should be marked as dirty, false otherwise
   * @return false if the page pin count is <= 0 before this call, true otherwise
   */
  // Unpin page_id from responsible BufferPoolManagerInstance
  return GetBufferPoolManager(page_id)->UnpinPage(page_id, is_dirty);
}

auto ParallelBufferPoolManager::FlushPgImp(page_id_t page_id) -> bool {
  /**
   * Flushes the target page to disk.
   * @param page_id id of page to be flushed, cannot be INVALID_PAGE_ID
   * @return false if the page could not be found in the page table, true otherwise
   */
  // Flush page_id from responsible BufferPoolManagerInstance
  // 前一个page_id是为了得到该page_id所在的BufferPoolManager,后一个page_id是Flush的参数
  return GetBufferPoolManager(page_id)->FlushPage(page_id);
}

auto ParallelBufferPoolManager::NewPgImp(page_id_t *page_id) -> Page * {
  std::lock_guard<std::mutex> lock(latch_);
  /**
   * Creates a new page in the buffer pool.
   * @param[out] page_id id of created page
   * @return nullptr if no new pages could be created, otherwise pointer to new page
   */
  // create new page. We will request page allocation in a round robin manner from the underlying
  // BufferPoolManagerInstances
  // 1.   From a starting index of the BPMIs, call NewPageImpl until either 1) success and return 2) looped around to
  // starting index and return nullptr
  // 2.   Bump the starting index (mod number of instances) to start search at a different BPMI each time this function
  // is called
  size_t bp_index = next_instance_;
  Page *page;
  do {
    page = buffer_pools_[bp_index]->NewPage(page_id);
    if (page != nullptr) {
      break;
    }
    bp_index = (bp_index + 1) % buffer_pools_.size();
  } while (bp_index != next_instance_);
  next_instance_ = (next_instance_ + 1) % num_instances_;
  return page;
}

auto ParallelBufferPoolManager::DeletePgImp(page_id_t page_id) -> bool {
  std::lock_guard<std::mutex> lock(latch_);
  /**
   * Deletes a page from the buffer pool.
   * @param page_id id of page to be deleted
   * @return false if the page exists but could not be deleted, true if the page didn't exist or deletion succeeded
   */
  // Delete page_id from responsible BufferPoolManagerInstance
  return GetBufferPoolManager(page_id)->DeletePage(page_id);
}

void ParallelBufferPoolManager::FlushAllPgsImp() {
  // std::lock_guard<std::mutex> lock(latch_);
  /**
   * Flushes all the pages in the buffer pool to disk.
   */
  // flush all pages from all BufferPoolManagerInstances
  for (auto &bp : buffer_pools_) {
    bp->FlushAllPages();
  }
}

}  // namespace bustub
