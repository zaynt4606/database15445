//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// hash_join_executor.cpp
//
// Identification: src/execution/hash_join_executor.cpp
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "execution/executors/hash_join_executor.h"
#include "execution/expressions/abstract_expression.h"
#include "execution/expressions/column_value_expression.h"

namespace bustub {

HashJoinExecutor::HashJoinExecutor(ExecutorContext *exec_ctx, const HashJoinPlanNode *plan,
                                   std::unique_ptr<AbstractExecutor> &&left_child,
                                   std::unique_ptr<AbstractExecutor> &&right_child)
    : AbstractExecutor(exec_ctx),
      plan_(plan),
      left_child_executor_(std::move(left_child)),
      right_child_executor_(std::move(right_child)) {
  next_pos_ = 0;
  outer_table_buffer_.clear();
}

void HashJoinExecutor::Init() {
  left_child_executor_->Init();
  right_child_executor_->Init();
  Tuple left_tuple;
  RID left_rid;
  // 构造hash_table_
  while (left_child_executor_->Next(&left_tuple, &left_rid)) {
    // 构造Hash_Join_Key
    Value value = plan_->LeftJoinKeyExpression()->Evaluate(&left_tuple, left_child_executor_->GetOutputSchema());
    HashJoinKey key{value};
    // 得到HashJoinKey对应的Tuple
    auto left_col_count = plan_->GetLeftPlan()->OutputSchema()->GetColumnCount();
    std::vector<Value> values;
    values.reserve(left_col_count);
    for (uint32_t i = 0; i < left_col_count; i++) {
      values.push_back(left_tuple.GetValue(plan_->GetLeftPlan()->OutputSchema(), i));
    }
    // 插入hash_table_
    if (hash_table_.count(key) > 0) {
      hash_table_[key].push_back(std::move(values));
    } else {
      hash_table_.insert({key, {values}});
    }
  }
}

bool HashJoinExecutor::Next(Tuple *tuple, RID *rid) {
  bool is_find = false;
  // 连接到最后
  if (next_pos_ >= outer_table_buffer_.size()) {
    while (right_child_executor_->Next(tuple, rid)) {
      Value value = plan_->RightJoinKeyExpression()->Evaluate(tuple, right_child_executor_->GetOutputSchema());
      auto iter = hash_table_.find(HashJoinKey{value});
      // 内层找到
      if (iter != hash_table_.end()) {
        outer_table_buffer_ = iter->second;
        next_pos_ = 0;
        is_find = true;
        break;
      }
    }
    if (!is_find) {
      return false;
    }
  }

  std::vector<Value> values;
  // 得到key对应的tuple的vector<value>
  values.reserve(plan_->OutputSchema()->GetColumnCount());
  for (const auto &column : plan_->OutputSchema()->GetColumns()) {
    auto column_expr = reinterpret_cast<const ColumnValueExpression *>(column.GetExpr());
    // Tuple在哪里拿
    if (column_expr->GetTupleIdx() == 0) {
      values.push_back(outer_table_buffer_[next_pos_][column_expr->GetColIdx()]);
    } else {
      values.push_back(tuple->GetValue(right_child_executor_->GetOutputSchema(), column_expr->GetColIdx()));
    }
  }

  *tuple = Tuple(values, plan_->OutputSchema());
  next_pos_++;
  return true;
}

}  // namespace bustub
