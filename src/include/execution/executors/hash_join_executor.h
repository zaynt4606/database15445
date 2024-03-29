//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// hash_join_executor.h
//
// Identification: src/include/execution/executors/hash_join_executor.h
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

#include "common/util/hash_util.h"
#include "execution/executor_context.h"
#include "execution/executors/abstract_executor.h"
#include "execution/expressions/abstract_expression.h"
#include "execution/plans/hash_join_plan.h"
#include "storage/table/tuple.h"

namespace bustub {

struct HashJoinKey {
  // 这里是一个key,不是tuple对应的vector<value>，所以后面的key对应的是二维tuple
  Value key_;

  bool operator==(const HashJoinKey &other) const { return key_.CompareEquals(other.key_) == CmpBool::CmpTrue; }
};
}  // namespace bustub

namespace std {
template <>
struct hash<bustub::HashJoinKey> {
  std::size_t operator()(const bustub::HashJoinKey &key) const { return bustub::HashUtil::HashValue(&key.key_); }
};
}  // namespace std

namespace bustub {
/**
 * HashJoinExecutor executes a nested-loop JOIN on two tables.
 */
class HashJoinExecutor : public AbstractExecutor {
 public:
  /**
   * Construct a new HashJoinExecutor instance.
   * @param exec_ctx The executor context
   * @param plan The HashJoin join plan to be executed
   * @param left_child The child executor that produces tuples for the left side of join
   * @param right_child The child executor that produces tuples for the right side of join
   */
  HashJoinExecutor(ExecutorContext *exec_ctx, const HashJoinPlanNode *plan,
                   std::unique_ptr<AbstractExecutor> &&left_child, std::unique_ptr<AbstractExecutor> &&right_child);

  /** Initialize the join */
  void Init() override;

  /**
   * Yield the next tuple from the join.
   * @param[out] tuple The next tuple produced by the join
   * @param[out] rid The next tuple RID produced by the join
   * @return `true` if a tuple was produced, `false` if there are no more tuples
   */
  auto Next(Tuple *tuple, RID *rid) -> bool override;

  /** @return The output schema for the join */
  auto GetOutputSchema() -> const Schema * override { return plan_->OutputSchema(); };

 private:
  /** The NestedLoopJoin plan node to be executed. */
  const HashJoinPlanNode *plan_;
  /** The left child executor to obtain value from */
  std::unique_ptr<AbstractExecutor> left_child_executor_;
  /** The right child executor to obtain value from */
  std::unique_ptr<AbstractExecutor> right_child_executor_;
  /** The next index to be accessed in outer_table_buffer_ */
  std::size_t next_pos_{0};
  /** Hash table */
  std::unordered_map<HashJoinKey, std::vector<std::vector<Value>>> hash_table_;
  /** Buffer of any value of hash_table_ */
  std::vector<std::vector<Value>> outer_table_buffer_;
};

}  // namespace bustub
