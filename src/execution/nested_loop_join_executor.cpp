//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// nested_loop_join_executor.cpp
//
// Identification: src/execution/nested_loop_join_executor.cpp
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "execution/executors/nested_loop_join_executor.h"
#include "execution/expressions/column_value_expression.h"
#include "execution/expressions/comparison_expression.h"
#include "execution/expressions/constant_value_expression.h"

namespace bustub {

NestedLoopJoinExecutor::NestedLoopJoinExecutor(ExecutorContext *exec_ctx, const NestedLoopJoinPlanNode *plan,
                                               std::unique_ptr<AbstractExecutor> &&left_executor,
                                               std::unique_ptr<AbstractExecutor> &&right_executor)
    : AbstractExecutor(exec_ctx),
      plan_(plan),
      left_child_executor_(std::move(left_executor)),
      right_child_executor_(std::move(right_executor)) {}

void NestedLoopJoinExecutor::Init() {
  left_child_executor_->Init();
  right_child_executor_->Init();
  // left先执行一次，
  is_left_selected_ = left_child_executor_->Next(&left_tuple_, &left_rid_);
}

bool NestedLoopJoinExecutor::Next(Tuple *tuple, RID *rid) {
  // left要是空，直接结束
  if (!is_left_selected_) {
    return false;
  }
  Tuple right_tuple;
  RID right_rid;
  while (true) {
    // right没有到尾就下一个
    while (!right_child_executor_->Next(&right_tuple, &right_rid)) {
      // left到达了末尾，结束执行
      if (!left_child_executor_->Next(&left_tuple_, &left_rid_)) {
        return false;
      }
      // right的Init里面有left会下一个
      right_child_executor_->Init();
    }
    auto predicate = plan_->Predicate();
    auto value_res = predicate
                         ->EvaluateJoin(&left_tuple_, left_child_executor_->GetOutputSchema(), &right_tuple,
                                        right_child_executor_->GetOutputSchema())
                         .GetAs<bool>();
    if (value_res) {
      std::vector<Value> values;
      values.reserve(plan_->OutputSchema()->GetColumnCount());
      for (const auto &column : plan_->OutputSchema()->GetColumns()) {
        values.emplace_back(column.GetExpr()->EvaluateJoin(&left_tuple_, left_child_executor_->GetOutputSchema(),
                                                           &right_tuple, right_child_executor_->GetOutputSchema()));
      }
      *tuple = Tuple(values, plan_->OutputSchema());
      *rid = left_tuple_.GetRid();
      return true;
    }
  }
  return false;
}

}  // namespace bustub
