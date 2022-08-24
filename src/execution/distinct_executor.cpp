//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// distinct_executor.cpp
//
// Identification: src/execution/distinct_executor.cpp
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "execution/executors/distinct_executor.h"

namespace bustub {

DistinctExecutor::DistinctExecutor(ExecutorContext *exec_ctx, const DistinctPlanNode *plan,
                                   std::unique_ptr<AbstractExecutor> &&child_executor)
    : AbstractExecutor(exec_ctx), plan_(plan), child_executor_(std::move(child_executor)) {}

void DistinctExecutor::Init() {
  child_executor_->Init();
  Tuple child_tuple;
  RID child_rid;
  bool res;
  while (true) {
    res = child_executor_->Next(&child_tuple, &child_rid);
    if (!res) {
      break;
    }
    tuples_.insert(child_tuple);
  }

  tuples_iter_ = tuples_.cbegin();
}

auto DistinctExecutor::Next(Tuple *tuple, RID *rid) -> bool {
  if (tuples_iter_ == tuples_.cend()) {
    return false;
  }

  *tuple = *tuples_iter_;
  *rid = tuple->GetRid();
  ++tuples_iter_;
  return true;
}

}  // namespace bustub
