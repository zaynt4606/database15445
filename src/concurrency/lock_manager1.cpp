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

#include "concurrency/lock_manager1.h"

#include <utility>
#include <vector>

namespace bustub {

// 加读锁
bool LockManager::LockShared(Transaction *txn, const RID &rid) {
  std::unique_lock<std::mutex> lock(latch_);

  id_2_txn_.emplace(txn->GetTransactionId(), txn);
  // READ_UNCOMMITTED只有在需要时上写锁
  if (txn->GetIsolationLevel() == IsolationLevel::READ_UNCOMMITTED) {
    txn->SetState(TransactionState::ABORTED);
    throw TransactionAbortException(txn->GetTransactionId(), AbortReason::LOCKSHARED_ON_READ_UNCOMMITTED);
    return false;
  }
  if (!LockPrepare(txn, rid)) {
    return false;
  }

  LockRequestQueue *request_queue = &lock_table_.find(rid)->second;
  request_queue->request_queue_.emplace_back(txn->GetTransactionId(), LockMode::SHARED);

  // 如果正在进行写操作
  if (request_queue->is_writing_) {
    DeadlockPrevent(txn, request_queue);                       // 预防死锁 task2
    request_queue->cv_.wait(lock, [request_queue]() -> bool {  // 循环等待
      return !request_queue->is_writing_;                      // 直到写操作结束
    });
  }

  txn->GetSharedLockSet()->emplace(rid);
  request_queue->sharing_count_++;
  auto iter = GetIterator(&request_queue->request_queue_, txn->GetTransactionId());
  iter->granted_ = true;

  return true;
}

// 加写锁
bool LockManager::LockExclusive(Transaction *txn, const RID &rid) {
  std::unique_lock<std::mutex> lock(latch_);

  id_2_txn_.emplace(txn->GetTransactionId(), txn);

  if (!LockPrepare(txn, rid)) {
    return false;
  }

  LockRequestQueue *request_queue = &lock_table_.find(rid)->second;
  request_queue->request_queue_.emplace_back(txn->GetTransactionId(), LockMode::EXCLUSIVE);

  if (request_queue->is_writing_ || request_queue->sharing_count_ > 0) {  // 加写锁不能有读锁和写锁
    DeadlockPrevent(txn, request_queue);
    request_queue->cv_.wait(
        lock, [request_queue]() -> bool { return !request_queue->is_writing_ && request_queue->sharing_count_ == 0; });
  }

  txn->GetExclusiveLockSet()->emplace(rid);
  request_queue->is_writing_ = true;
  auto iter = GetIterator(&request_queue->request_queue_, txn->GetTransactionId());
  iter->granted_ = true;

  return true;
}

/**
升级锁
将读锁升级为写锁。由于加写锁需要保证当前没有读锁，那么如果队列中有两个更新锁的请求，就会互相等待对方解读锁
因此，为了判断是否出现这种情况，在队列中维护标志变量upgrading_，不允许队列中出现两个更新锁的请求。
将队列中的读锁请求改为写锁，循环判断是否满足加更新写锁条件（当前没有写锁，且只有唯一一个该读锁）。
 */
bool LockManager::LockUpgrade(Transaction *txn, const RID &rid) {
  std::unique_lock<std::mutex> lock(latch_);

  if (txn->GetState() == TransactionState::SHRINKING) {
    txn->SetState(TransactionState::ABORTED);
    throw TransactionAbortException(txn->GetTransactionId(), AbortReason::LOCK_ON_SHRINKING);
    return false;
  }

  LockRequestQueue *request_queue = &lock_table_.find(rid)->second;

  if (request_queue->upgrading_) {  // 队列中出现过更新锁，直接aborted这个更新锁
    txn->SetState(TransactionState::ABORTED);
    throw TransactionAbortException(txn->GetTransactionId(), AbortReason::UPGRADE_CONFLICT);
    return false;
  }

  // 升级读锁，把读锁改成写锁， 不能放在等待后面，这里要改掉读锁，就等着改
  txn->GetSharedLockSet()->erase(rid);
  request_queue->sharing_count_--;
  auto iter = GetIterator(&request_queue->request_queue_, txn->GetTransactionId());
  iter->lock_mode_ = LockMode::EXCLUSIVE;
  iter->granted_ = false;

  // aborted要求是后面才return false 但是一开始就直接false也行
  if (txn->GetState() == TransactionState::ABORTED) {
    throw TransactionAbortException(txn->GetTransactionId(), AbortReason::DEADLOCK);
    return false;
  }

  // 只有没有读锁的时候才能升级
  if (request_queue->is_writing_ || request_queue->sharing_count_ > 0) {
    DeadlockPrevent(txn, request_queue);
    request_queue->upgrading_ = true;  // 等待升级
    request_queue->cv_.wait(
        lock, [request_queue]() -> bool { return !request_queue->is_writing_ && request_queue->sharing_count_ == 0; });
  }

  // 等前面的读锁结束才能升级
  txn->GetExclusiveLockSet()->emplace(rid);
  request_queue->upgrading_ = false;  // 升级结束
  request_queue->is_writing_ = true;
  iter = GetIterator(&request_queue->request_queue_, txn->GetTransactionId());
  iter->granted_ = true;

  return true;
}

/**
 * @brief
在请求队列和LockSet中删除对应的请求(不存在锁请求就return false), 并设置事务状态为Shrinking
notify_all()通知其他请求可以尝试加锁了, 不是notify_one()而是all的原因是,
可能可以通知很多想要加S锁的请求, 这样他们就都能拿到锁了
！！只有Growing时才设置txn的状态为Shrinking, 因为总不能Aborted的时候解个锁解成Shrinking
！！当lock_mode是S锁时, 只有RR才设置Shrinking,
    因为RC是可以重复的加S锁/解S锁的, 这样他才可以在一个事务周期中读到不同的数据
 */
bool LockManager::Unlock(Transaction *txn, const RID &rid) {
  std::unique_lock<std::mutex> lock(latch_);

  LockRequestQueue *request_queue = &lock_table_.find(rid)->second;

  txn->GetSharedLockSet()->erase(rid);
  txn->GetExclusiveLockSet()->erase(rid);

  auto iter = GetIterator(&request_queue->request_queue_, txn->GetTransactionId());
  LockMode mode = iter->lock_mode_;
  request_queue->request_queue_.erase(iter);

  // 只有growing的时候才设置成shrinking，aborted就不能设了
  if (!(mode == LockMode::SHARED && txn->GetIsolationLevel() == IsolationLevel::READ_COMMITTED) &&
      txn->GetState() == TransactionState::GROWING) {
    txn->SetState(TransactionState::SHRINKING);
  }

  if (mode == LockMode::SHARED) {  // 读锁
                                   // 都通知其他了，不一定要没有读锁才通知
                                   // if (--request_queue->sharing_count_ == 0)
    request_queue->cv_.notify_all();

  } else {  // 有写锁 先解写锁再通知
    request_queue->is_writing_ = false;
    request_queue->cv_.notify_all();
  }
  return true;
}

/** 上锁前的准备工作
检查一下事务是否已经是Shrinking状态, 如果已经Shrinking了, 这时上锁是不合法行为, 要将事务回滚
检查一下事务是否已经是Abort状态, 如果是, 直接return false即可
如果是RU隔离界别来上S锁, 那我们同样应该回滚事务,
    这是因为RU的实现方式就是无锁读, 这样就能读到脏数据了, 如果上了S锁, RU将无法读到脏数据
检查是否已经上过同级或更高级的锁了, 如果已经上过了, 我们直接return true
 */
bool LockManager::LockPrepare(Transaction *txn, const RID &rid) {
  if (txn->GetState() == TransactionState::SHRINKING) {
    txn->SetState(TransactionState::ABORTED);  // 回滚？
    // debug
    throw TransactionAbortException(txn->GetTransactionId(), AbortReason::LOCK_ON_SHRINKING);
    return false;
  }

  if (txn->GetState() == TransactionState::ABORTED) {
    // debug
    throw TransactionAbortException(txn->GetTransactionId(), AbortReason::DEADLOCK);
    return false;
  }
  // mutex和condition_variable不能被复制或移动，所以要用emplave 和 piecewise_construct
  if (lock_table_.find(rid) == lock_table_.end()) {
    lock_table_.emplace(std::piecewise_construct, std::forward_as_tuple(rid), std::forward_as_tuple());
  }
  return true;
}

// 得到id为所要的LockRequest的迭代器, 没找到就返回给的list的end迭代器
std::list<LockManager::LockRequest>::iterator LockManager::GetIterator(std::list<LockRequest> *request_queue,
                                                                       txn_id_t txn_id) {
  for (auto iter = request_queue->begin(); iter != request_queue->end(); ++iter) {
    if (iter->txn_id_ == txn_id) {
      return iter;
    }
  }
  return request_queue->end();
}

// 预防死锁
void LockManager::DeadlockPrevent(Transaction *txn, LockRequestQueue *request_queue) {
  // 遍历std::list<LockRequest> request_queue_;
  for (const auto &request : request_queue->request_queue_) {
    // 冲突了去掉这个
    if (request.granted_ && request.txn_id_ > txn->GetTransactionId()) {
      id_2_txn_[request.txn_id_]->SetState(TransactionState::ABORTED);
      // 调整去掉的时候的标记
      if (request.lock_mode_ == LockMode::SHARED) {
        request_queue->sharing_count_--;
      } else {
        request_queue->is_writing_ = false;
      }
    }
  }
}
}  // namespace bustub
