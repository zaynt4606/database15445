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

InsertExecutor::InsertExecutor(ExecutorContext *exec_ctx, const InsertPlanNode *plan,
                               std::unique_ptr<AbstractExecutor> &&child_executor)
    : AbstractExecutor(exec_ctx),
      plan_(plan),
      child_executor_(std::move(child_executor)){
        table_info_ = exec_ctx->GetCatalog()->GetTable(plan_->TableOid()),
        index_info_vec_ = exec_ctx->GetCatalog()->GetTableIndexes(table_info_->name_);
      }

void InsertExecutor::Init() {
  // 判断是否是raw insert, 也就是children_是否是空
  if (!plan_->IsRawInsert()) {
    // 不是直接的raw insert
    child_executor_->Init();
  }
  next_insert_pos_ = 0;
}

bool InsertExecutor::Next([[maybe_unused]] Tuple *tuple, RID *rid) {
  bool inserted = false;       // 记录是否成功insert
  if (plan_->IsRawInsert()) {  // row insert
    // RawValues() -> std::vector<std::vector<Value>>
    if (next_insert_pos_ != plan_->RawValues().size()) {
      auto &values = plan_->RawValues();  // 引用
      // Tuple的构造函数 Tuple(std::vector<Value> values, const Schema *schema);
      *tuple = Tuple(values[next_insert_pos_], &table_info_->schema_);
      next_insert_pos_++;
      inserted = table_info_->table_->InsertTuple(*tuple, rid, exec_ctx_->GetTransaction());
    }
  } else {  // 在childen里面找
    if (child_executor_->Next(tuple, rid)) {
      inserted = table_info_->table_->InsertTuple(*tuple, rid, exec_ctx_->GetTransaction());
    }
  }

  // 
  if (inserted && !index_info_vec_.empty()) {
    for (auto &index_info : index_info_vec_) {
      auto key = tuple->KeyFromTuple(table_info_->schema_, index_info->key_schema_, index_info->index_->GetKeyAttrs());
      index_info->index_->InsertEntry(key, *rid, exec_ctx_->GetTransaction());
    }
  }
  return inserted;
}

}  // namespace bustub
