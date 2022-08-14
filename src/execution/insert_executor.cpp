//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// insert_executor.cpp
//
// Identification: src/execution/insert_executor.cpp
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include <memory>

#include "execution/executors/insert_executor.h"

namespace bustub {

// InsertExecutor::InsertExecutor(ExecutorContext *exec_ctx, const InsertPlanNode *plan,
//                                std::unique_ptr<AbstractExecutor> &&child_executor)
//     : AbstractExecutor(exec_ctx), plan_(plan), child_executor_(std::move(child_executor)) {
//   table_info_ = exec_ctx_->GetCatalog()->GetTable(plan_->TableOid());
//   index_info_vec_ = exec_ctx_->GetCatalog()->GetTableIndexes(table_info_->name_);
// }

// void InsertExecutor::Init() {
//   if (!plan_->IsRawInsert()) {
//     child_executor_->Init();
//   }
//   next_insert_pos_ = 0;
// }

// bool InsertExecutor::Next([[maybe_unused]] Tuple *tuple, RID *rid) {
//   bool is_inserted = false;
//   if (plan_->IsRawInsert()) {
//     if (next_insert_pos_ == plan_->RawValues().size()) {
//       // nothing to do
//     } else {
//       auto &values = plan_->RawValues();
//       *tuple = Tuple(values[next_insert_pos_++], &table_info_->schema_);
//       is_inserted = table_info_->table_->InsertTuple(*tuple, rid, exec_ctx_->GetTransaction());
//     }
//   } else if (child_executor_->Next(tuple, rid)) {
//     is_inserted = table_info_->table_->InsertTuple(*tuple, rid, exec_ctx_->GetTransaction());
//   }

//   if (is_inserted && !index_info_vec_.empty()) {
//     for (auto index_info : index_info_vec_) {
//       const auto index_key =
//           tuple->KeyFromTuple(table_info_->schema_, index_info->key_schema_, index_info->index_->GetKeyAttrs());
//       index_info->index_->InsertEntry(index_key, *rid, exec_ctx_->GetTransaction());
//     }
//   }
//   return is_inserted;
// }

InsertExecutor::InsertExecutor(ExecutorContext *exec_ctx, const InsertPlanNode *plan,
                               std::unique_ptr<AbstractExecutor> &&child_executor)
    : AbstractExecutor(exec_ctx),
      plan_(plan),
      child_executor_(std::move(child_executor)),
      catalog_(exec_ctx->GetCatalog()),
      table_info_(catalog_->GetTable(plan->TableOid())),
      table_heap_(table_info_->table_.get()) {}

void InsertExecutor::Init() {
  if (!plan_->IsRawInsert()) {
    child_executor_->Init();
  } else {
    iter_ = plan_->RawValues().begin();
  }
}

bool InsertExecutor::Next([[maybe_unused]] Tuple *tuple, RID *rid) {
  std::vector<Tuple> tuples;

  if (!plan_->IsRawInsert()) {
    if (!child_executor_->Next(tuple, rid)) {
      return false;
    }
  } else {
    if (iter_ == plan_->RawValues().end()) {
      return false;
    }
    *tuple = Tuple(*iter_, &table_info_->schema_);
    iter_++;
  }

  if (!table_heap_->InsertTuple(*tuple, rid, exec_ctx_->GetTransaction())) {
    LOG_DEBUG("INSERT FAIL");
    return false;
  }

  Transaction *txn = GetExecutorContext()->GetTransaction();
  LockManager *lock_mgr = GetExecutorContext()->GetLockManager();

  if (txn->IsSharedLocked(*rid)) {
    if (!lock_mgr->LockUpgrade(txn, *rid)) {
      throw TransactionAbortException(txn->GetTransactionId(), AbortReason::DEADLOCK);
    }
  } else {
    if (!lock_mgr->LockExclusive(txn, *rid)) {
      throw TransactionAbortException(txn->GetTransactionId(), AbortReason::DEADLOCK);
    }
  }

  for (const auto &index : catalog_->GetTableIndexes(table_info_->name_)) {
    index->index_->InsertEntry(
        tuple->KeyFromTuple(table_info_->schema_, *index->index_->GetKeySchema(), index->index_->GetKeyAttrs()), *rid,
        exec_ctx_->GetTransaction());
  }

  if (txn->GetIsolationLevel() != IsolationLevel::REPEATABLE_READ) {
    if (!lock_mgr->Unlock(txn, *rid)) {
      throw TransactionAbortException(txn->GetTransactionId(), AbortReason::DEADLOCK);
    }
  }

  return Next(tuple, rid);
}

}  // namespace bustub
