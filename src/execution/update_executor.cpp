//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// update_executor.cpp
//
// Identification: src/execution/update_executor.cpp
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//
#include <memory>

#include "execution/executors/update_executor.h"

namespace bustub {

// UpdateExecutor::UpdateExecutor(ExecutorContext *exec_ctx, const UpdatePlanNode *plan,
//                                std::unique_ptr<AbstractExecutor> &&child_executor)
//     : AbstractExecutor(exec_ctx),
//       plan_(plan),
//       table_info_(exec_ctx->GetCatalog()->GetTable(plan->TableOid())),
//       child_executor_(std::move(child_executor)) {}

// void UpdateExecutor::Init() {
//   // exec_ctx_ 是abstract_executor的成员，上面的构造函数用exec_ctx来构造了AbstractExecutor
//   index_info_vec_ = exec_ctx_->GetCatalog()->GetTableIndexes(table_info_->name_);
//   child_executor_->Init();
// }

// auto UpdateExecutor::Next([[maybe_unused]] Tuple *tuple, RID *rid) -> bool {
//   // auto transaction = exec_ctx_->GetTransaction();
//   // auto lockmanager = exec_ctx_->GetLockManager();
//   Tuple child_tuple;
//   RID child_rid;
//   Tuple update_tuple;
//   RID update_rid;
//   bool res = child_executor_->Next(&child_tuple, &child_rid);
//   if (res) {
//     update_tuple = GenerateUpdatedTuple(child_tuple);
//     update_rid = update_tuple.GetRid();

//     // if (transaction->GetIsolationLevel() == IsolationLevel::REPEATABLE_READ) {
//     //   lockmanager->LockUpgrade(transaction, child_rid);  // 之前查询获取了读锁，现在需要将锁升级
//     // } else {
//     //   lockmanager->LockExclusive(transaction, child_rid);  // 加上写锁
//     // }

//     // 用TableHeap执行表修改, 参数需要new tuple, rid of old tuple
//     table_info_->table_->UpdateTuple(update_tuple, child_rid, exec_ctx_->GetTransaction());

//     // 修改所有索引
//     for (auto &index_info : index_info_vec_) {
//       // index删掉旧的，插入新的, rid都是child_executor输出tuple的rid
//       auto old_key_tuple =
//           child_tuple.KeyFromTuple(table_info_->schema_, index_info->key_schema_, index_info->index_->GetKeyAttrs());
//       auto new_key_tuple =
//           update_tuple.KeyFromTuple(table_info_->schema_, index_info->key_schema_,
//           index_info->index_->GetKeyAttrs());
//       index_info->index_->DeleteEntry(old_key_tuple, child_rid, exec_ctx_->GetTransaction());
//       index_info->index_->InsertEntry(new_key_tuple, child_rid, exec_ctx_->GetTransaction());
//     }
//   }
//   return res;
// }

UpdateExecutor::UpdateExecutor(ExecutorContext *exec_ctx, const UpdatePlanNode *plan,
                               std::unique_ptr<AbstractExecutor> &&child_executor)
    : AbstractExecutor(exec_ctx), plan_(plan), child_executor_(std::move(child_executor)) {}

void UpdateExecutor::Init() {
  child_executor_->Init();
  auto table_oid = plan_->TableOid();
  table_info_ = exec_ctx_->GetCatalog()->GetTable(table_oid);
  index_info_ = exec_ctx_->GetCatalog()->GetTableIndexes(table_info_->name_);
}

auto UpdateExecutor::Next([[maybe_unused]] Tuple *tuple, RID *rid) -> bool {
  auto transaction = exec_ctx_->GetTransaction();
  // auto lockmanager = exec_ctx_->GetLockManager();
  auto table_oid = plan_->TableOid();
  auto catalog = exec_ctx_->GetCatalog();

  auto table_schema = table_info_->schema_;
  Tuple child_tuple;  // 原来的元组
  RID child_rid;
  Tuple update_tuple;  // 更新后的元组
  RID update_rid;
  bool res = child_executor_->Next(&child_tuple, &child_rid);

  if (res) {
    update_tuple = GenerateUpdatedTuple(child_tuple);
    update_rid = update_tuple.GetRid();

    // if (transaction->GetIsolationLevel() == IsolationLevel::REPEATABLE_READ) {
    //   lockmanager->LockUpgrade(transaction, child_rid);  // 之前查询获取了读锁，现在需要将锁升级
    // } else {
    //   lockmanager->LockExclusive(transaction, child_rid);  // 加上写锁
    // }

    table_info_->table_->UpdateTuple(update_tuple, child_rid, transaction);  // 传入old rid
    Tuple old_key_tuple;
    Tuple new_key_tuple;
    for (auto info : index_info_) {  // 更新索引，RID都为子执行器输出元组的RID
      old_key_tuple = child_tuple.KeyFromTuple(table_schema, info->key_schema_, info->index_->GetKeyAttrs());
      new_key_tuple = update_tuple.KeyFromTuple(table_schema, info->key_schema_, info->index_->GetKeyAttrs());
      info->index_->DeleteEntry(old_key_tuple, child_rid, transaction);
      info->index_->InsertEntry(new_key_tuple, child_rid, transaction);
      // 维护IndexWriteSet
      // transaction->AppendIndexWriteRecord(IndexWriteRecord{child_rid, table_oid, WType::UPDATE, new_key_tuple,
      //                                                      old_key_tuple, info->index_oid_, catalog});
      transaction->AppendIndexWriteRecord(
          IndexWriteRecord{child_rid, table_oid, WType::UPDATE, update_tuple, child_tuple, info->index_oid_, catalog});
    }
  }
  return res;
}

auto UpdateExecutor::GenerateUpdatedTuple(const Tuple &src_tuple) -> Tuple {
  const auto &update_attrs = plan_->GetUpdateAttr();
  Schema schema = table_info_->schema_;
  uint32_t col_count = schema.GetColumnCount();
  std::vector<Value> values;
  for (uint32_t idx = 0; idx < col_count; idx++) {
    if (update_attrs.find(idx) == update_attrs.cend()) {
      values.emplace_back(src_tuple.GetValue(&schema, idx));
    } else {
      const UpdateInfo info = update_attrs.at(idx);
      Value val = src_tuple.GetValue(&schema, idx);
      switch (info.type_) {
        case UpdateType::Add:
          values.emplace_back(val.Add(ValueFactory::GetIntegerValue(info.update_val_)));
          break;
        case UpdateType::Set:
          values.emplace_back(ValueFactory::GetIntegerValue(info.update_val_));
          break;
      }
    }
  }
  return Tuple{values, &schema};
}

}  // namespace bustub
