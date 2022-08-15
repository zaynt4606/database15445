//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// lru_replacer.cpp
//
// Identification: src/buffer/lru_replacer.cpp
//
// Copyright (c) 2015-2019, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "buffer/lru_replacer.h"

namespace bustub {

LRUReplacer::LRUReplacer(size_t num_pages) {
  capacity_ = num_pages;
  lst_.clear();
  mp_.clear();
  //   size_ = lst_.size();
}

LRUReplacer::~LRUReplacer() = default;
// frame_id_t 是buffer磁盘index
auto LRUReplacer::Victim(frame_id_t *frame_id) -> bool {
  if (lst_.empty()) {
    return false;
  }
  mtx_.lock();
  frame_id_t last = lst_.back();
  mp_.erase(last);
  *frame_id = last;
  lst_.pop_back();
  mtx_.unlock();
  return true;
}

void LRUReplacer::Pin(frame_id_t frame_id) {
  if (lst_.empty()) {
    return;
  }
  if (mp_.count(frame_id) == 0) {
    return;
  }
  mtx_.lock();
  mp_.erase(frame_id);
  lst_.remove(frame_id);
  mtx_.unlock();
}

void LRUReplacer::Unpin(frame_id_t frame_id) {
  // 这里插入一个已经存在的是不用更新排序的
  if (mp_.count(frame_id) != 0) {
    return;
  }
  mtx_.lock();
  // 新加入一个
  mp_[frame_id] = 1;
  lst_.push_front(frame_id);
  // 假如溢出
  if (lst_.size() > capacity_) {
    frame_id_t last = lst_.back();
    mp_.erase(last);
    lst_.pop_back();
  }
  mtx_.unlock();
}

auto LRUReplacer::Size() -> size_t { return lst_.size(); }

}  // namespace bustub
