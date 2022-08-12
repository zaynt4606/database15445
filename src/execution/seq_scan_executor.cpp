//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// seq_scan_executor.cpp
//
// Identification: src/execution/seq_scan_executor.cpp
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "execution/executors/seq_scan_executor.h"

namespace bustub {

/**
 *exec_ctx       [bfp, log manager, lock manager, catalog, txnmanager]
 *catalog:       [tables、indexes]
 *tables:        [id，table_metadata]
 *table_metadata:[schema(表，索引，外键等等), name, table_(table_heap)(pages组成的链表), id]
 *indexes:       [id,index_info]
 *index_info:    [shema, name, index_, id, table_name, key_size]
 *
 */

/**
 private:
  const SeqScanPlanNode *plan_;
  Schema *schema_;
  TableHeap *table_heap_;
  TableIterator *table_iter_;
 */
SeqScanExecutor::SeqScanExecutor(ExecutorContext *exec_ctx, const SeqScanPlanNode *plan)
    : AbstractExecutor(exec_ctx),
      plan_(plan),
      schema_(&exec_ctx->GetCatalog()->GetTable(plan_->GetTableOid())->schema_),
      table_heap_(exec_ctx->GetCatalog()->GetTable(plan_->GetTableOid())->table_.get()),
      // TableIterator没有默认构造函数，需要手动初始化
      table_iter_(nullptr, RID{}, nullptr),
      table_end_iter_(nullptr, RID{}, nullptr) {}

void SeqScanExecutor::Init() {
  // exec_ctx_是父类abstract_executor的成员
  table_iter_ = table_heap_->Begin(exec_ctx_->GetTransaction());
  table_end_iter_ = table_heap_->End();
}

auto SeqScanExecutor::Next(Tuple *tuple, RID *rid) -> bool {
  for (; table_iter_ != table_end_iter_; table_iter_++) {
    *tuple = *table_iter_;
    *rid = tuple->GetRid();
    // exec_ctx_->GetLockManager()->LockShared(exec_ctx_->GetTransaction(),*rid);
    // @return The predicate to test tuples against;
    // tuples should only be returned if they evaluate to true
    // auto GetPredicate() const -> const AbstractExpression * { return predicate_; }
    if (plan_->GetPredicate() == nullptr) {
      table_iter_++;
      return true;
    } else {
      if (plan_->GetPredicate()->Evaluate(tuple, plan_->OutputSchema()).GetAs<bool>()) {
        // Evaluate返回Value类型的结果，将其转化为bool型判断是否满足表达式的条件
        table_iter_++;
        return true;
      }
    }
  }
  return false;
}

}  // namespace bustub
