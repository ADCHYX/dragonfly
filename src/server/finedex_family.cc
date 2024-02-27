#include "server/finedex_family.h"

extern "C" {
#include "redis/object.h"
}
#include "base/logging.h"
#include "server/acl/acl_commands_def.h"
#include "server/command_registry.h"
#include "server/conn_context.h"
#include "server/engine_shard_set.h"
#include "server/transaction.h"

namespace dfly {

using namespace std;
using namespace facade;

namespace {

OpStatus convert(string_view from, KeyType& to) {
    try {
        to = std::stoull(std::string{from});

        return OpStatus::OK;
    } catch (const std::invalid_argument&) {
        return OpStatus::WRONG_TYPE;
    } catch (const std::out_of_range&) {
        return OpStatus::OUT_OF_RANGE;
    }
}

#define CONVERT_CHECK(from, to) do {                                    \
    if (auto status = convert((from), (to)); status != OpStatus::OK) {  \
        return status;                                                  \
    }                                                                   \
} while (0)

OpResult<uint32_t> OpAdd(const OpArgs& op_args, string_view key, string_view value) {
    KeyType k;
    PayLoad v;
    CONVERT_CHECK(key, k);
    CONVERT_CHECK(value, v);

    finedex_index.put(k,v);
    return OpStatus::OK;
}

OpResult<PayLoad> OpGet(const OpArgs& op_args, string_view key) {
  KeyType k;
  PayLoad v;
  CONVERT_CHECK(key, k);

  auto it = finedex_index.get(k,v);
  if (it == false) {
    return OpStatus::KEY_NOTFOUND;
  }
  return v;
}

OpResult<uint32_t> OpDel(const OpArgs& op_args, string_view key) {
  KeyType k;
  CONVERT_CHECK(key, k);

  auto res = finedex_index.remove(k);
  return res;
}

}  // namespace

void FinedexFamily::FinedexAdd(CmdArgList args, ConnectionContext* cntx) {
  string_view key = ArgS(args, 0);
  string_view value = ArgS(args, 1);

  auto cb = [&](Transaction* t, EngineShard* shard) {
    return OpAdd(t->GetOpArgs(shard), key, value);
  };

  OpResult<uint32_t> result = cntx->transaction->ScheduleSingleHopT(std::move(cb));

  if (result) {
    return cntx->SendLong(1);
  }
  cntx->SendError(result.status());
}

void FinedexFamily::FinedexGet(CmdArgList args, ConnectionContext* cntx) {
  string_view key = ArgS(args, 0);

  auto cb = [&](Transaction* t, EngineShard* shard) {
    return OpGet(t->GetOpArgs(shard), key);
  };

  OpResult<PayLoad> result = cntx->transaction->ScheduleSingleHopT(std::move(cb));

  cntx->SendLong(*result);
}

void FinedexFamily::FinedexDel(CmdArgList args, ConnectionContext* cntx) {
  string_view key = ArgS(args, 0);

  auto cb = [&](Transaction* t, EngineShard* shard) {
    return OpDel(t->GetOpArgs(shard), key);
  };

  OpResult<uint32_t> result = cntx->transaction->ScheduleSingleHopT(std::move(cb));

  if (result || result.status() == OpStatus::KEY_NOTFOUND) {
    cntx->SendLong(*result);
  } else {
    cntx->SendError(result.status());
  }
}

using CI = CommandId;

#define HFUNC(x) SetHandler(&x)

namespace acl {
constexpr uint32_t kFinedexAdd = WRITE | SET | FAST;
constexpr uint32_t kFinedexGet = READ | SET | FAST;
constexpr uint32_t kFinedexDel = WRITE | SET | FAST;
}  // namespace acl

void FinedexFamily::Register(CommandRegistry* registry) {
  registry->StartFamily();
  *registry
      << CI{"FINEDEXADD", CO::WRITE | CO::FAST | CO::DENYOOM, 3, 1, 1, acl::kFinedexAdd}.HFUNC(FinedexAdd)
      << CI{"FINEDEXGET", CO::READONLY | CO::FAST, 2, 1, 1, acl::kFinedexGet}.HFUNC(FinedexGet)
      << CI{"FINEDEXDEL", CO::WRITE | CO::FAST, 2, 1, 1, acl::kFinedexDel}.HFUNC(FinedexDel);
  // std::cout<<"finedex register"<<std::endl;
}

finedexInterface<KeyType,PayLoad> finedex_index;

}  // namespace dfly
