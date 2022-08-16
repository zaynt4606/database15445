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
// lab1
InsertExecutor::InsertExecutor(ExecutorContext *exec_ctx, const InsertPlanNode *plan,
                               std::unique_ptr<AbstractExecutor> &&child_executor)
    : AbstractExecutor(exec_ctx), plan_(plan), child_executor_(std::move(child_executor)) {
  table_info_ = exec_ctx_->GetCatalog()->GetTable(plan_->TableOid());
  index_info_array_ = exec_ctx_->GetCatalog()->GetTableIndexes(table_info_->name_);
}

void InsertExecutor::Init() {
  if (!plan_->IsRawInsert()) {
    child_executor_->Init();
  }
  next_insert_pos_ = 0;
}

bool InsertExecutor::Next([[maybe_unused]] Tuple *tuple, RID *rid) {
  bool is_inserted = false;
  if (plan_->IsRawInsert()) {
    if (next_insert_pos_ == plan_->RawValues().size()) {
      // nothing to do
    } else {
      auto &values = plan_->RawValues();
      *tuple = Tuple(values[next_insert_pos_++], &table_info_->schema_);
      is_inserted = table_info_->table_->InsertTuple(*tuple, rid, exec_ctx_->GetTransaction());
    }
  } else if (child_executor_->Next(tuple, rid)) {
    is_inserted = table_info_->table_->InsertTuple(*tuple, rid, exec_ctx_->GetTransaction());
  }

  if (is_inserted && !index_info_array_.empty()) {
    for (auto index_info : index_info_array_) {
      const auto index_key =
          tuple->KeyFromTuple(table_info_->schema_, index_info->key_schema_, index_info->index_->GetKeyAttrs());
      index_info->index_->InsertEntry(index_key, *rid, exec_ctx_->GetTransaction());
    }
  }
  return is_inserted;
}

// // lab2
// InsertExecutor::InsertExecutor(ExecutorContext *exec_ctx, const InsertPlanNode *plan,
//                                std::unique_ptr<AbstractExecutor> &&child_executor)
//     : AbstractExecutor(exec_ctx),
//       plan_(plan),
//       child_executor_(std::move(child_executor)),
//       catalog_(exec_ctx->GetCatalog()),
//       table_info_(catalog_->GetTable(plan->TableOid())),
//       table_heap_(table_info_->table_.get()) {}

// void InsertExecutor::Init() {
//   if (!plan_->IsRawInsert()) {
//     child_executor_->Init();
//   } else {
//     iter_ = plan_->RawValues().begin();
//   }
// }

// bool InsertExecutor::Next([[maybe_unused]] Tuple *tuple, RID *rid) {
//   std::vector<Tuple> tuples;

//   if (!plan_->IsRawInsert()) {
//     if (!child_executor_->Next(tuple, rid)) {
//       return false;
//     }
//   } else {
//     if (iter_ == plan_->RawValues().end()) {
//       return false;
//     }
//     *tuple = Tuple(*iter_, &table_info_->schema_);
//     iter_++;
//   }

//   if (!table_heap_->InsertTuple(*tuple, rid, exec_ctx_->GetTransaction())) {
//     LOG_DEBUG("INSERT FAIL");
//     return false;
//   }

//   Transaction *txn = GetExecutorContext()->GetTransaction();
//   LockManager *lock_mgr = GetExecutorContext()->GetLockManager();

//   if (txn->IsSharedLocked(*rid)) {
//     if (!lock_mgr->LockUpgrade(txn, *rid)) {
//       throw TransactionAbortException(txn->GetTransactionId(), AbortReason::DEADLOCK);
//     }
//   } else {
//     if (!lock_mgr->LockExclusive(txn, *rid)) {
//       throw TransactionAbortException(txn->GetTransactionId(), AbortReason::DEADLOCK);
//     }
//   }

//   for (const auto &index : catalog_->GetTableIndexes(table_info_->name_)) {
//     index->index_->InsertEntry(
//         tuple->KeyFromTuple(table_info_->schema_, *index->index_->GetKeySchema(), index->index_->GetKeyAttrs()),
//         *rid, exec_ctx_->GetTransaction());
//   }

//   if (txn->GetIsolationLevel() != IsolationLevel::REPEATABLE_READ) {
//     if (!lock_mgr->Unlock(txn, *rid)) {
//       throw TransactionAbortException(txn->GetTransactionId(), AbortReason::DEADLOCK);
//     }
//   }

//   return Next(tuple, rid);
// }

// // lab3
// InsertExecutor::InsertExecutor(ExecutorContext *exec_ctx, const InsertPlanNode *plan,
//                                std::unique_ptr<AbstractExecutor> &&child_executor)
//     : AbstractExecutor(exec_ctx), plan_(plan), child_executor_(std::move(child_executor)) {
//   is_raw_insert_ = plan_->IsRawInsert();
// }

// void InsertExecutor::Init() {
//   if (!is_raw_insert_) {  // 初始化子计划或者数组迭代器
//     child_executor_->Init();
//   } else {
//     values_iter_ = plan_->RawValues().cbegin();
//   }

//   auto table_oid = plan_->TableOid();
//   table_info_ = exec_ctx_->GetCatalog()->GetTable(table_oid);
//   index_info_ = exec_ctx_->GetCatalog()->GetTableIndexes(table_info_->name_);
// }

// bool InsertExecutor::Next([[maybe_unused]] Tuple *tuple, RID *rid) {
//   auto transaction = exec_ctx_->GetTransaction();
//   auto lockmanager = exec_ctx_->GetLockManager();
//   auto table_oid = plan_->TableOid();
//   auto catalog = exec_ctx_->GetCatalog();

//   auto table_schema = table_info_->schema_;
//   Tuple insert_tuple;
//   RID insert_rid;  // 插入表后才被赋值
//   bool res;

//   if (!is_raw_insert_) {
//     res = child_executor_->Next(&insert_tuple, &insert_rid);
//   } else {
//     if (values_iter_ == plan_->RawValues().cend()) {
//       res = false;
//     } else {
//       insert_tuple = Tuple(*values_iter_, &table_schema);  // 合成元组
//       ++values_iter_;                                      // 移向下一位置
//       res = true;
//     }
//   }

//   if (res) {
//     table_info_->table_->InsertTuple(insert_tuple, &insert_rid, transaction);  // insert_rid此时才被赋值
//     lockmanager->LockExclusive(transaction, insert_rid);                       // 加上写锁
//     Tuple key_tuple;
//     for (auto info : index_info_) {  // 更新索引
//       key_tuple = insert_tuple.KeyFromTuple(table_schema, info->key_schema_, info->index_->GetKeyAttrs());
//       info->index_->InsertEntry(key_tuple, insert_rid, transaction);
//       transaction->AppendIndexWriteRecord(IndexWriteRecord{insert_rid, table_oid, WType::INSERT, key_tuple,
//       key_tuple,
//                                                            info->index_oid_, catalog});  // 维护IndexWriteSet
//     }
//   }

//   return res;
// }

}  // namespace bustub
