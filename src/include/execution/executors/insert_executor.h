//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// insert_executor.h
//
// Identification: src/include/execution/executors/insert_executor.h
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <memory>
#include <utility>

#include "execution/executor_context.h"
#include "execution/executors/abstract_executor.h"
#include "execution/plans/insert_plan.h"
#include "storage/table/tuple.h"

namespace bustub {

/**
 * InsertExecutor executes an insert on a table.
 *
 * Unlike UPDATE and DELETE, inserted values may either be
 * embedded in the plan itself or be pulled from a child executor.
 */
class InsertExecutor : public AbstractExecutor {
 public:
  /**
   * Construct a new InsertExecutor instance.
   * @param exec_ctx The executor context
   * @param plan The insert plan to be executed
   * @param child_executor The child executor from which inserted tuples are pulled (may be `nullptr`)
   */
  InsertExecutor(ExecutorContext *exec_ctx, const InsertPlanNode *plan,
                 std::unique_ptr<AbstractExecutor> &&child_executor);

  /** Initialize the insert */
  void Init() override;

  /**
   * Yield the next tuple from the insert.
   * @param[out] tuple The next tuple produced by the insert
   * @param[out] rid The next tuple RID produced by the insert
   * @return `true` if a tuple was produced, `false` if there are no more tuples
   *
   * NOTE: InsertExecutor::Next() does not use the `tuple` out-parameter.
   * NOTE: InsertExecutor::Next() does not use the `rid` out-parameter.
   */
  auto Next([[maybe_unused]] Tuple *tuple, RID *rid) -> bool override;

  auto Insert(Tuple &tuple, RID *rid) -> bool;

  /** @return The output schema for the insert */
  auto GetOutputSchema() -> const Schema * override { return plan_->OutputSchema(); };

 private:
  /** The insert plan node to be executed*/
  const InsertPlanNode *plan_;

  // lab1
  /** Metadata identifying the table that should be updated */
  TableInfo *table_info_{Catalog::NULL_TABLE_INFO};
  /** All indexes associated with the table being inserted */
  std::vector<IndexInfo *> index_info_array_;
  /** The child executor to obtain value from */
  std::unique_ptr<AbstractExecutor> child_executor_;
  /** The index of the next value to be inserted */
  size_t next_insert_pos_{0};

  // // code2
  // std::unique_ptr<AbstractExecutor> child_executor_;
  // Catalog *catalog_;
  // TableInfo *table_info_;
  // TableHeap *table_heap_;
  // std::vector<std::vector<Value>>::const_iterator iter_;

  // // code3
  // std::unique_ptr<AbstractExecutor> child_executor_;
  // bool is_raw_insert_;
  // TableInfo *table_info_;
  // std::vector<IndexInfo *> index_info_;
  // std::vector<std::vector<bustub::Value>>::const_iterator values_iter_;
};

}  // namespace bustub
