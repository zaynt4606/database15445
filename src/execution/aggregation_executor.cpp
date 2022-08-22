//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// aggregation_executor.cpp
//
// Identification: src/execution/aggregation_executor.cpp
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//
#include <memory>
#include <vector>

#include "execution/executors/aggregation_executor.h"

namespace bustub {

AggregationExecutor::AggregationExecutor(ExecutorContext *exec_ctx, const AggregationPlanNode *plan,
                                         std::unique_ptr<AbstractExecutor> &&child)
    : AbstractExecutor(exec_ctx),
    plan_(plan),
    child_(std::move(child)),
    aht_(SimpleAggregationHashTable(plan_->GetAggregates(), plan_->GetAggregateTypes())),
    aht_iterator_(aht_.Begin()) {}

void AggregationExecutor::Init() {
    child_->Init();
    Tuple tuple;
    RID rid;
    while (child_->Next(&tuple, &rid)) {
        aht_.InsertCombine(MakeAggregateKey(&tuple), MakeAggregateValue(&tuple));
    }
    aht_iterator_ = aht_.Begin();
}

auto AggregationExecutor::Next(Tuple *tuple, RID *rid) -> bool {
    if (aht_iterator_ == aht_.End()) {
        return false;
    }
    const AggregateKey &agg_key = aht_iterator_.Key();
    const AggregateValue &agg_value = aht_iterator_.Val();
    ++aht_iterator_;  // 重载的运算符只有前++
    if (plan_->GetHaving() == nullptr ||
        plan_->GetHaving()->EvaluateAggregate(agg_key.group_bys_, agg_value.aggregates_).GetAs<bool>()) {
            std::vector<Value> ret;
            for (const auto& col : plan_->OutputSchema()->GetColumns()) {
                ret.emplace_back(col.GetExpr()->EvaluateAggregate(agg_key.group_bys_, agg_value.aggregates_));
            }
            *tuple = Tuple(ret, plan_->OutputSchema());
            return true;
    }
    return Next(tuple, rid);
}

auto AggregationExecutor::GetChildExecutor() const -> const AbstractExecutor * { return child_.get(); }

}  // namespace bustub
