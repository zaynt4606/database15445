//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// execution_engine.h
//
// Identification: src/include/execution/execution_engine.h
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <vector>

#include "buffer/buffer_pool_manager.h"
#include "catalog/catalog.h"
#include "concurrency/transaction_manager.h"
#include "execution/executor_context.h"
#include "execution/executor_factory.h"
#include "execution/plans/abstract_plan.h"
#include "storage/table/tuple.h"
namespace bustub {

/**
 * The ExecutionEngine class executes query plans.
 * It converts the input query plan to a query executor, 
 * and executes it until all results have been produced. 
 * 将输入的查询计划转化为查询执行器，并执行它直到产生所有结果
 */
class ExecutionEngine {
 public:
  /**
   * Construct a new ExecutionEngine instance.
   * @param bpm The buffer pool manager used by the execution engine
   * @param txn_mgr The transaction manager used by the execution engine
   * @param catalog The catalog used by the execution engine
   */
  ExecutionEngine(BufferPoolManager *bpm, TransactionManager *txn_mgr, Catalog *catalog)
      : bpm_{bpm}, txn_mgr_{txn_mgr}, catalog_{catalog} {}

  DISALLOW_COPY_AND_MOVE(ExecutionEngine);

  /**
   * Execute a query plan.
   * @param plan The query plan to execute，也就是executor对应的planNode，也就是executor的类型
   * @param result_set The set of tuples produced by executing the plan
   * @param txn The transaction context in which the query executes
   * @param exec_ctx The executor context in which the query executes，当前执行的上下文
   *        记录了bfp, log manager, lock manager, catalog，txnmanager，catalog中有Tables和Indexs比较重要 
   * @return `true` if execution of the query plan succeeds, `false` otherwise
   */

  /**
   * learning
   * AbstractPlanNode
   * 是所有PlanNode的父类。对应的有一个枚举类PlanType，表示所有可能的PlanNode类型。
   * AbstractPlanNode只有两个成员变量，一个是output_shcema,
   * 在Next返回tuple（如果需要返回tuple）时可以根据output_schema选择输出tuple的哪几个column（相当于select）。
   * 另一个是vector children_, 里面有所有children的常量指针。
   */
  auto Execute(const AbstractPlanNode *plan, std::vector<Tuple> *result_set, Transaction *txn,
               ExecutorContext *exec_ctx) -> bool {
    // Construct and executor for the plan
    // factory的唯一函数
    // std::unique_ptr<AbstractExecutor>
    auto executor = ExecutorFactory::CreateExecutor(exec_ctx, plan);

    // Prepare the root executor
    // AbstractExecutor代指各种executor，各种executor都有成员函数Init和Next
    executor->Init();

    // Execute the query plan
    // 调用executor的init方法初始化executor，重复执行next方法，
    // next返回true则将结果存入result_set并继续执行next, next返回false 则结束。
    // 所以后面的任务就是实现每个executor的init和next方法
    try {
      Tuple tuple;
      RID rid;
      while (executor->Next(&tuple, &rid)) {
        if (result_set != nullptr) {
          result_set->push_back(tuple);
        }
      }
    } catch (Exception &e) {
      // TODO(student): handle exceptions
    }
    return true;
  }

 private:
  /** The buffer pool manager used during query execution */
  [[maybe_unused]] BufferPoolManager *bpm_;
  /** The transaction manager used during query execution */
  [[maybe_unused]] TransactionManager *txn_mgr_;
  /** The catalog used during query execution */
  [[maybe_unused]] Catalog *catalog_;
};

}  // namespace bustub
