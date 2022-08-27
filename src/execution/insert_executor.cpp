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

#include "execution/executors/insert_executor.h"
#include "concurrency/transaction.h"

namespace bustub {

InsertExecutor::InsertExecutor(ExecutorContext *exec_ctx, const InsertPlanNode *plan,
                               std::unique_ptr<AbstractExecutor> &&child_executor)
    : AbstractExecutor(exec_ctx), plan_(plan), child_executor_(std::move(child_executor)) {
  is_raw_insert_ = plan_->IsRawInsert();
}

void InsertExecutor::Init() {
  if (!is_raw_insert_) {  // 初始化子计划或者数组迭代器
    child_executor_->Init();
  } else {
    values_iter_ = plan_->RawValues().cbegin();
  }

  auto table_oid = plan_->TableOid();
  table_info_ = exec_ctx_->GetCatalog()->GetTable(table_oid);
  index_info_ = exec_ctx_->GetCatalog()->GetTableIndexes(table_info_->name_);
}

auto InsertExecutor::Next([[maybe_unused]] Tuple *tuple, RID *rid) -> bool {
  auto transaction = exec_ctx_->GetTransaction();
  auto lockmanager = exec_ctx_->GetLockManager();
  auto table_oid = plan_->TableOid();
  auto catalog = exec_ctx_->GetCatalog();

  auto table_schema = table_info_->schema_;
  Tuple insert_tuple;
  RID insert_rid;  // 插入表后才被赋值
  bool res;

  if (!is_raw_insert_) {
    res = child_executor_->Next(&insert_tuple, &insert_rid);
  } else {
    if (values_iter_ == plan_->RawValues().cend()) {
      res = false;
    } else {
      insert_tuple = Tuple(*values_iter_, &table_schema);  // 合成元组
      ++values_iter_;                                      // 移向下一位置
      res = true;
    }
  }

  if (res) {
    table_info_->table_->InsertTuple(insert_tuple, &insert_rid, transaction);  // insert_rid此时才被赋值
    lockmanager->LockExclusive(transaction, insert_rid);                       // 加上写锁
    Tuple key_tuple;
    for (auto info : index_info_) {  // 更新索引
      key_tuple = insert_tuple.KeyFromTuple(table_schema, info->key_schema_, info->index_->GetKeyAttrs());
      info->index_->InsertEntry(key_tuple, insert_rid, transaction);
      // 维护IndexWriteSet
      transaction->AppendIndexWriteRecord(IndexWriteRecord{insert_rid, table_oid, WType::INSERT, insert_tuple,
                                                           insert_tuple, info->index_oid_, catalog});
    }
  }

  return res;
}

}  // namespace bustub
