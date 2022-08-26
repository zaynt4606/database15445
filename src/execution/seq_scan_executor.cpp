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

SeqScanExecutor::SeqScanExecutor(ExecutorContext *exec_ctx, const SeqScanPlanNode *plan)
    : AbstractExecutor(exec_ctx), plan_(plan), table_iter_(nullptr, RID(), nullptr) {}

// Tuple中许多方法都是针对完整模式（table schema）而言的，对切割后的模式使用很容易发生数组越界
// 两模式中两列是一样的是很难判断的，因为列名可以改变，只能通过原始表的列序号，但其隐藏在ColumnValueExpression类中
// 故只能不同类型使用不同的Evaluate函数
// 这些转换函数的大体流程都一样，即使用输出元祖的模式调用Evaluatexxx方法，输出相应列的值
void SeqScanExecutor::TupleSchemaTranformUseEvaluate(const Tuple *table_tuple, const Schema *table_schema,
                                                     Tuple *dest_tuple, const Schema *dest_schema) {
  auto colums = dest_schema->GetColumns();
  std::vector<Value> dest_value;
  dest_value.reserve(colums.size());

  for (const auto &col : colums) {
    dest_value.emplace_back(col.GetExpr()->Evaluate(table_tuple, table_schema));
  }

  *dest_tuple = Tuple(dest_value, dest_schema);
}

// 通过列名，偏移量判断模式是否相同
auto SeqScanExecutor::SchemaEqual(const Schema *table_schema, const Schema *output_schema) -> bool {
  auto table_colums = table_schema->GetColumns();
  auto output_colums = output_schema->GetColumns();
  if (table_colums.size() != output_colums.size()) {
    return false;
  }

  int col_size = table_colums.size();
  uint32_t offset1;
  uint32_t offset2;
  std::string name1;
  std::string name2;
  for (int i = 0; i < col_size; i++) {
    offset1 = table_colums[i].GetOffset();
    offset2 = output_colums[i].GetOffset();
    name1 = table_colums[i].GetName();
    name2 = output_colums[i].GetName();
    if (name1 != name2 || offset1 != offset2) {
      return false;
    }
  }
  return true;
}
void SeqScanExecutor::Init() {
  auto table_oid = plan_->GetTableOid();
  table_info_ = exec_ctx_->GetCatalog()->GetTable(table_oid);
  table_iter_ = table_info_->table_->Begin(exec_ctx_->GetTransaction());

  auto output_schema = plan_->OutputSchema();
  auto table_schema = table_info_->schema_;
  is_same_schema_ = SchemaEqual(&table_schema, output_schema);

  // 可重复读：给所有元组加上读锁，事务提交后再解锁
  auto transaction = exec_ctx_->GetTransaction();
  auto lockmanager = exec_ctx_->GetLockManager();
  if (transaction->GetIsolationLevel() == IsolationLevel::REPEATABLE_READ) {
    auto iter = table_info_->table_->Begin(exec_ctx_->GetTransaction());
    while (iter != table_info_->table_->End()) {
      lockmanager->LockShared(transaction, iter->GetRid());
      ++iter;
    }
  }
}

auto SeqScanExecutor::Next(Tuple *tuple, RID *rid) -> bool {
  auto predicate = plan_->GetPredicate();
  auto output_schema = plan_->OutputSchema();
  auto table_schema = table_info_->schema_;
  auto transaction = exec_ctx_->GetTransaction();
  auto lockmanager = exec_ctx_->GetLockManager();
  bool res;

  while (table_iter_ != table_info_->table_->End()) {
    // 读已提交：读元组时加上读锁，读完后立即释放
    if (transaction->GetIsolationLevel() == IsolationLevel::READ_COMMITTED) {
      lockmanager->LockShared(transaction, table_iter_->GetRid());
    }

    auto p_tuple = &(*table_iter_);  // 获取指向元组的指针
    res = true;
    if (predicate != nullptr) {
      res = predicate->Evaluate(p_tuple, &table_schema).GetAs<bool>();
    }

    if (res) {
      if (!is_same_schema_) {
        TupleSchemaTranformUseEvaluate(p_tuple, &table_schema, tuple, output_schema);
      } else {
        *tuple = *p_tuple;
      }
      *rid = p_tuple->GetRid();  // 返回行元组的ID
    }
    if (transaction->GetIsolationLevel() == IsolationLevel::READ_COMMITTED) {
      lockmanager->Unlock(transaction, table_iter_->GetRid());
    }
    ++table_iter_;  // 指向下一位置后再返回
    if (res) {
      return true;
    }
  }
  return false;
}

}  // namespace bustub
