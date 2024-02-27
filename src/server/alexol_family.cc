#include "server/alexol_family.h"

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

// alex::Alex<uint32_t,uint32_t> alexindex;
alexolInterface<KeyType,PayLoad> alexol_index;

OpResult<uint32_t> OpAdd(const OpArgs& op_args, string_view key, string_view value) {
  KeyType k;
  PayLoad v;
  try{
    std::string str_k(key);
    std::string str_v(value);
    k = std::stoull(str_k);
    v = std::stoull(str_v);
  } catch (const std::exception& e) {
    return OpStatus::WRONG_TYPE;
  }
  alexol_index.put(k,v);
  return OpStatus::OK;
}

OpResult<PayLoad> OpGet(const OpArgs& op_args, string_view key) {
  KeyType k;
  PayLoad v;
  try{
    std::string str_k(key);
    k = std::stoull(str_k);
  } catch (const std::exception& e) {
    return OpStatus::WRONG_TYPE;
  }
  auto it = alexol_index.get(k,v);
  if(it == false)
    return OpStatus::KEY_NOTFOUND;
  return v;
}

OpResult<uint32_t> OpDel(const OpArgs& op_args, string_view key) {
  KeyType k;
  PayLoad v;
  try{
    std::string str_k(key);
    k = std::stoull(str_k);
  } catch (const std::exception& e) {
    return OpStatus::WRONG_TYPE;
  }
  auto res = alexol_index.remove(k);
  return res;
}

}  // namespace

void AlexolFamily::AlexolAdd(CmdArgList args, ConnectionContext* cntx) {
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

void AlexolFamily::AlexolGet(CmdArgList args, ConnectionContext* cntx) {
  string_view key = ArgS(args, 0);

  auto cb = [&](Transaction* t, EngineShard* shard) {
    return OpGet(t->GetOpArgs(shard), key);
  };

  OpResult<PayLoad> result = cntx->transaction->ScheduleSingleHopT(std::move(cb));

  cntx->SendLong(*result);
}

void AlexolFamily::AlexolDel(CmdArgList args, ConnectionContext* cntx) {
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
constexpr uint32_t kAlexolAdd = WRITE | SET | FAST;
constexpr uint32_t kAlexolGet = READ | SET | FAST;
constexpr uint32_t kAlexolDel = WRITE | SET | FAST;
}  // namespace acl

void AlexolFamily::Register(CommandRegistry* registry) {
  registry->StartFamily();
  *registry
      << CI{"ALEXOLADD", CO::WRITE | CO::FAST | CO::DENYOOM, 3, 1, 1, acl::kAlexolAdd}.HFUNC(AlexolAdd)
      << CI{"ALEXOLGET", CO::READONLY | CO::FAST, 2, 1, 1, acl::kAlexolGet}.HFUNC(AlexolGet)
      << CI{"ALEXOLDEL", CO::WRITE | CO::FAST, 2, 1, 1, acl::kAlexolDel}.HFUNC(AlexolDel);
}

}  // namespace dfly
