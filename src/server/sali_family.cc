#include "server/sali_family.h"
#include "server/sali/sali.h"

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

static sali::SALI<SaliKeyType, SaliPayLoad> SaliIndex;

OpStatus convert(string_view from, SaliKeyType& to) {
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

static OpResult<uint32_t> OpAdd(const OpArgs& op_args, string_view key, string_view value) {
    SaliKeyType k;
    SaliPayLoad v;

    CONVERT_CHECK(key, k);
    CONVERT_CHECK(value, v);

    SaliIndex.insert(k,v);

    return OpStatus::OK;
}

static OpResult<SaliPayLoad> OpGet(const OpArgs& op_args, string_view key) {
  SaliKeyType k;
  SaliPayLoad v;

  CONVERT_CHECK(key, k);

  auto it = SaliIndex.at(k,v);

  if (!it) {
    return OpStatus::KEY_NOTFOUND;
  }
    
  return v;
}

static OpResult<uint32_t> OpDel(const OpArgs& op_args, string_view key) {
  SaliKeyType k;
  
  CONVERT_CHECK(key, k);

  auto res = SaliIndex.remove(k);

  return res;
}

}  // namespace

void SaliFamily::SaliAdd(CmdArgList args, ConnectionContext* cntx) {
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

void SaliFamily::SaliGet(CmdArgList args, ConnectionContext* cntx) {
  string_view key = ArgS(args, 0);

  auto cb = [&](Transaction* t, EngineShard* shard) {
    return OpGet(t->GetOpArgs(shard), key);
  };

  OpResult<SaliPayLoad> result = cntx->transaction->ScheduleSingleHopT(std::move(cb));

  cntx->SendLong(*result);
}

void SaliFamily::SaliDel(CmdArgList args, ConnectionContext* cntx) {
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
constexpr uint32_t kSaliAdd = WRITE | SET | FAST;
constexpr uint32_t kSaliGet = READ | SET | FAST;
constexpr uint32_t kSaliDel = WRITE | SET | FAST;
}  // namespace acl

void SaliFamily::Register(CommandRegistry* registry) {
  registry->StartFamily();
  *registry
      << CI{"SALIADD", CO::WRITE | CO::FAST | CO::DENYOOM, 3, 1, 1, acl::kSaliAdd}.HFUNC(SaliAdd)
      << CI{"SALIGET", CO::READONLY | CO::FAST, 2, 1, 1, acl::kSaliGet}.HFUNC(SaliGet)
      << CI{"SALIDEL", CO::WRITE | CO::FAST, 2, 1, 1, acl::kSaliDel}.HFUNC(SaliDel);
}

}  // namespace dfly
