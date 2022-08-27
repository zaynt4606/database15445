//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// lock_manager.cpp
//
// Identification: src/concurrency/lock_manager.cpp
//
// Copyright (c) 2015-2019, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "concurrency/lock_manager.h"

#include <utility>
#include <vector>

namespace bustub {
// 如果中止事务持有锁，则不能擅自解锁，可能一直停留在队列中，如果未持有锁，则需要立即移除
// 杀死请求队列中优先级更小的请求，保证S锁请求前没有比其更年轻的X锁请求，X锁请求前没有比其更年轻的请求
void LockManager::KillRequest(txn_id_t id, const RID &rid, KillType type) {
  Transaction *transaction;
  bool expr;
  auto &request_queue = lock_table_[rid].request_queue_;
  /*
  进行两次遍历，第一次遍历杀死所有未获得锁的低优先级请求并移出队列
  第二次遍历将获得锁的低优先级请求解锁后再中止事务，在解锁函数中已经将该请求移出队列
  之所以进行两次遍历是为了避免本该被杀死的未加锁的请求在前面加锁请求被杀死后获得锁，避免不必要的解锁操作
  所以说最好的遍历方式应该是反向遍历，但反向遍历删除容易产生迭代器失效问题，并且有时请求是在解锁函数中移除，不太好控制，故进行两次遍历
  */
  auto iter = request_queue.begin();
  while (iter != request_queue.end()) {
    // 此时读请求不应该被杀死，对其取反
    expr = !(type == KillType::WRITE_REQUEST && iter->lock_mode_ == LockMode::SHARED);
    transaction = iter->transation_;  // 记录事务指针
    // 将未获得锁的低优先级非中止事务变成中止事务
    if (id < iter->txn_id_ && transaction->GetState() != TransactionState::ABORTED && expr && !iter->granted_) {
      transaction->SetState(TransactionState::ABORTED);
    }
    // 移除未获得锁的中止事务请求
    if (transaction->GetState() == TransactionState::ABORTED && !iter->granted_) {
      iter = request_queue.erase(iter);
    } else {
      ++iter;
    }
  }
  iter = request_queue.begin();
  while (iter != request_queue.end()) {
    // 此时读请求不应该被杀死，对其取反
    expr = !(type == KillType::WRITE_REQUEST && iter->lock_mode_ == LockMode::SHARED);
    transaction = iter->transation_;  // 记录事务指针
    // 对低优先级非中止事务进行操作
    if (id < iter->txn_id_ && transaction->GetState() != TransactionState::ABORTED && expr && iter->granted_) {
      ++iter;
      UnlockImp(transaction, rid);  // 调用解锁函数，同时将请求移出队列
      transaction->SetState(TransactionState::ABORTED);
    } else {
      ++iter;
    }
  }
}

// 唤醒队首后连续的S锁请求
void LockManager::AwakeSharedRequest(const RID &rid) {
  auto &request_queue = lock_table_[rid].request_queue_;
  assert(lock_table_[rid].status_ == RIDStatus::SHARED);
  txn_id_t max_id = MAX_ID;
  // 若存在更新锁请求，则只能唤醒比其优先级更高的S锁请求
  if (lock_table_[rid].upgrading_ != INVALID_TXN_ID) {
    max_id = lock_table_[rid].upgrading_;
  }
  for (auto &req : request_queue) {
    if (req.lock_mode_ == LockMode::EXCLUSIVE) {
      return;
    }
    if (!req.granted_ && req.txn_id_ < max_id) {
      req.granted_ = true;
      lock_table_[rid].share_req_cnt_++;
    }
  }
}

auto LockManager::LockShared(Transaction *txn, const RID &rid) -> bool {
  auto is_shared = txn->IsSharedLocked(rid);
  auto is_exc = txn->IsExclusiveLocked(rid);
  auto transaction_state = txn->GetState();
  auto isolation_level = txn->GetIsolationLevel();
  auto txn_id = txn->GetTransactionId();

  if (is_shared || is_exc) {  // 防止重复加锁
    return true;
  }
  if (transaction_state != TransactionState::GROWING) {  // 判断当前是否为growing阶段
    txn->SetState(TransactionState::ABORTED);
    return false;
  }
  if (isolation_level == IsolationLevel::READ_UNCOMMITTED) {  // 读未提交没有S锁(存在脏读)
    txn->SetState(TransactionState::ABORTED);
    return false;
  }

  std::unique_lock<std::mutex> lock(latch_);
  LockRequest req(txn, txn_id, LockMode::SHARED);

  if (lock_table_.count(rid) == 0) {  // 当前资源未被占用,请求得到保证
    req.granted_ = true;
    lock_table_[rid].request_queue_.emplace_back(req);
    lock_table_[rid].status_ = RIDStatus::SHARED;
    lock_table_[rid].share_req_cnt_ = 1;
  } else {
    lock_table_[rid].request_queue_.emplace_back(req);
    LockRequest &request_ref = lock_table_[rid].request_queue_.back();  // 保留请求引用
    KillRequest(txn_id, rid, KillType::WRITE_REQUEST);                  // 杀死所有低优先级X锁请求
    if (lock_table_[rid].status_ == RIDStatus::SHARED) {                // 唤醒连续的S锁请求
      AwakeSharedRequest(rid);
    }
    lock_table_[rid].cv_.notify_all();  // 唤醒请求，防止中止事务一直等待
    while (txn->GetState() != TransactionState::ABORTED && !(request_ref.granted_)) {  // 事务中止或得到保证
      lock_table_[rid].cv_.wait(lock);
    }
    if (txn->GetState() == TransactionState::ABORTED) {  // 在请求队列中删除未持有中止事务
      return false;
    }
  }
  txn->GetSharedLockSet()->emplace(rid);
  return true;
}

auto LockManager::LockExclusive(Transaction *txn, const RID &rid) -> bool {
  auto is_shared = txn->IsSharedLocked(rid);  // 防止重复加锁
  auto is_exc = txn->IsExclusiveLocked(rid);
  auto transaction_state = txn->GetState();
  auto txn_id = txn->GetTransactionId();

  if (is_exc) {  // 防止重复加锁
    return true;
  }
  if (is_shared) {
    return false;
  }
  if (transaction_state != TransactionState::GROWING) {  // 判断当前是否为growing阶段
    txn->SetState(TransactionState::ABORTED);
    return false;
  }

  std::unique_lock<std::mutex> lock(latch_);
  LockRequest req(txn, txn_id, LockMode::EXCLUSIVE);
  if (lock_table_.count(rid) == 0) {  // 当前资源未被占用
    req.granted_ = true;
    lock_table_[rid].request_queue_.emplace_back(req);
    lock_table_[rid].status_ = RIDStatus::EXCLUSIVE;
  } else {
    // 与LockShared函数一致的步骤
    lock_table_[rid].request_queue_.emplace_back(req);
    LockRequest &request_ref = lock_table_[rid].request_queue_.back();
    KillRequest(txn_id, rid, KillType::ALL_REQUEST);      // 杀死所有低优先级请求
    if (lock_table_[rid].status_ == RIDStatus::SHARED) {  // 唤醒连续的S锁请求
      AwakeSharedRequest(rid);
    }
    lock_table_[rid].cv_.notify_all();  // 唤醒请求，防止中止事务一直等待
    while (txn->GetState() != TransactionState::ABORTED && !(request_ref.granted_)) {  // 事务中止或得到保证
      lock_table_[rid].cv_.wait(lock);
    }
    if (txn->GetState() == TransactionState::ABORTED) {
      return false;
    }
  }

  txn->GetExclusiveLockSet()->emplace(rid);
  return true;
}

auto LockManager::LockUpgrade(Transaction *txn, const RID &rid) -> bool {
  auto transaction_state = txn->GetState();
  auto txn_id = txn->GetTransactionId();

  if (transaction_state != TransactionState::GROWING) {
    txn->SetState(TransactionState::ABORTED);
    return false;
  }
  if (!txn->IsSharedLocked(rid)) {  // 如果自身未持有S锁
    txn->SetState(TransactionState::ABORTED);
    return false;
  }

  std::unique_lock<std::mutex> lock(latch_);
  if (lock_table_[rid].upgrading_ != INVALID_TXN_ID) {  // 已有更新请求
    txn->SetState(TransactionState::ABORTED);
    return false;
  }

  lock_table_[rid].upgrading_ = txn_id;
  KillRequest(txn_id, rid, KillType::ALL_REQUEST);  // 杀死所有低优先级请求
  lock_table_[rid].cv_.notify_all();                // 唤醒请求，防止中止事务一直等待
  while (txn->GetState() != TransactionState::ABORTED &&
         lock_table_[rid].share_req_cnt_ != 1) {  // 未被中止，等待S锁持有者只有自己
    lock_table_[rid].cv_.wait(lock);
  }
  lock_table_[rid].upgrading_ = INVALID_TXN_ID;  // 将更新请求事务id重新置为无效
  if (txn->GetState() == TransactionState::ABORTED) {
    return false;
  }
  auto request_location = lock_table_[rid].request_queue_.begin();
  assert(request_location->txn_id_ == txn_id);  // 队列第一位即该更新请求，此时没有中止事务持有锁
  request_location->lock_mode_ = LockMode::EXCLUSIVE;  // 更改请求模式
  lock_table_[rid].share_req_cnt_ = 0;
  lock_table_[rid].status_ = RIDStatus::EXCLUSIVE;

  txn->GetSharedLockSet()->erase(rid);
  txn->GetExclusiveLockSet()->emplace(rid);
  return true;
}

auto LockManager::Unlock(Transaction *txn, const RID &rid) -> bool {
  std::unique_lock<std::mutex> lock(latch_);
  return UnlockImp(txn, rid);
}
// 实现unlock函数功能，但不加锁，便于KillRequest调用
auto LockManager::UnlockImp(Transaction *txn, const RID &rid) -> bool {
  auto is_shared = txn->IsSharedLocked(rid);
  auto is_exc = txn->IsExclusiveLocked(rid);
  auto state = txn->GetState();
  auto isolation_level = txn->GetIsolationLevel();
  auto txn_id = txn->GetTransactionId();

  if (!is_shared && !is_exc) {  // 未持有锁
    return false;
  }
  // 需提前判断事务当前状态，只在growing时才修改状态为shrinking
  if (isolation_level == IsolationLevel::REPEATABLE_READ && state == TransactionState::GROWING) {
    txn->SetState(TransactionState::SHRINKING);
  }

  // 在请求队列中删除该请求
  for (auto iter = lock_table_[rid].request_queue_.begin(); iter != lock_table_[rid].request_queue_.end(); ++iter) {
    if (iter->txn_id_ == txn_id) {
      lock_table_[rid].request_queue_.erase(iter);
      break;  // 删除后立即返回
    }
  }
  bool need_find_next_req = true;
  bool exist_normal_request = false;
  LockMode next_req_mode;
  if (is_shared) {
    lock_table_[rid].share_req_cnt_--;
    if (lock_table_[rid].share_req_cnt_ != 0) {  // 仍有事务持有该锁，不应该释放
      need_find_next_req = false;
    }
  }
  // 下一个非中止请求，此时没有中止事务的等待请求，故第一个要么是正常请求，要么是中止事务的持有锁的请求，但第二种情况need_find_next_req为false
  auto next_req_iter = lock_table_[rid].request_queue_.begin();
  if (!lock_table_[rid].request_queue_.empty()) {
    exist_normal_request = true;
    next_req_mode = next_req_iter->lock_mode_;
  }

  if (need_find_next_req && exist_normal_request) {  // 给予一些请求保证
    if (next_req_mode == LockMode::SHARED) {
      lock_table_[rid].status_ = RIDStatus::SHARED;
      AwakeSharedRequest(rid);
    } else {
      lock_table_[rid].status_ = RIDStatus::EXCLUSIVE;
      next_req_iter->granted_ = true;
    }
  }
  lock_table_[rid].cv_.notify_all();  // 唤醒请求

  if (need_find_next_req && !exist_normal_request) {  // 请求队列没有请求
    lock_table_.erase(rid);
  }
  if (is_shared) {
    txn->GetSharedLockSet()->erase(rid);
  }
  if (is_exc) {
    txn->GetExclusiveLockSet()->erase(rid);
  }
  return true;
}
}  // namespace bustub
