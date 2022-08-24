//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// delete_executor.cpp
//
// Identification: src/execution/delete_executor.cpp
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include <memory>

#include "execution/executors/delete_executor.h"

namespace bustub {

// DeleteExecutor::DeleteExecutor(ExecutorContext *exec_ctx, const DeletePlanNode *plan,
//                                std::unique_ptr<AbstractExecutor> &&child_executor)
//     : AbstractExecutor(exec_ctx),
//       plan_(plan),
//       child_executor_(std::move(child_executor)) {}

// void DeleteExecutor::Init() {
//     table_info_ = exec_ctx_->GetCatalog()->GetTable(plan_->TableOid());
//     index_info_vec_ = exec_ctx_->GetCatalog()->GetTableIndexes(table_info_->name_);
//     child_executor_->Init();
// }

// bool DeleteExecutor::Next([[maybe_unused]] Tuple *tuple, RID *rid) {
//   bool is_deleted = false;
//   if (child_executor_->Next(tuple, rid)) {
//     if (table_info_->table_->MarkDelete(*rid, exec_ctx_->GetTransaction())) {
//       is_deleted = true;
//     }
//   }
//   if (is_deleted && !index_info_vec_.empty()) {
//     for (auto index_info : index_info_vec_) {
//       const auto index_key =
//           tuple->KeyFromTuple(table_info_->schema_,
//                               index_info->key_schema_,
//                               index_info->index_->GetKeyAttrs());
//       index_info->index_->DeleteEntry(index_key, *rid, exec_ctx_->GetTransaction());
//     }
//   }
//   return is_deleted;
// }
DeleteExecutor::DeleteExecutor(ExecutorContext *exec_ctx, const DeletePlanNode *plan,
                               std::unique_ptr<AbstractExecutor> &&child_executor)
    : AbstractExecutor(exec_ctx), plan_(plan), child_executor_(std::move(child_executor)) {}

void DeleteExecutor::Init() {
  child_executor_->Init();
  auto table_oid = plan_->TableOid();
  table_info_ = exec_ctx_->GetCatalog()->GetTable(table_oid);
  index_info_ = exec_ctx_->GetCatalog()->GetTableIndexes(table_info_->name_);
}

auto DeleteExecutor::Next([[maybe_unused]] Tuple *tuple, RID *rid) -> bool {
  auto transaction = exec_ctx_->GetTransaction();
  auto lockmanager = exec_ctx_->GetLockManager();
  // auto table_oid = plan_->TableOid();
  // auto catalog = exec_ctx_->GetCatalog();

  auto table_schema = table_info_->schema_;
  Tuple child_tuple;
  RID child_rid;
  bool res = child_executor_->Next(&child_tuple, &child_rid);

  if (res) {
    if (transaction->GetIsolationLevel() == IsolationLevel::REPEATABLE_READ) {
      lockmanager->LockUpgrade(transaction, child_rid);  // 之前查询获取了读锁，现在需要将锁升级
    } else {
      lockmanager->LockExclusive(transaction, child_rid);  // 未获取读锁
    }

    bool delete_res = table_info_->table_->MarkDelete(child_rid, transaction);
    if (!delete_res) {  // 抛出异常，需要进行相应处理
      throw Exception("delete failed");
    }

    Tuple key_tuple;
    for (auto info : index_info_) {  // 更新索引
      key_tuple = child_tuple.KeyFromTuple(table_schema, info->key_schema_, info->index_->GetKeyAttrs());
      info->index_->DeleteEntry(key_tuple, child_rid, transaction);
      // transaction->AppendIndexWriteRecord(IndexWriteRecord{child_rid, table_oid, WType::DELETE, child_tuple,
      //                                                      child_tuple, info->index_oid_,
      //                                                      catalog});  // 维护IndexWriteSet
    }
  }
  return res;
}

}  // namespace bustub
