#include "server/alex_family.h"

extern "C" {
#include "redis/object.h"
}
#include "base/logging.h"
#include "server/acl/acl_commands_def.h"
#include "server/command_registry.h"
#include "server/conn_context.h"
#include "server/engine_shard_set.h"
#include "server/transaction.h"

#include <optional>
#include <stdexcept>

namespace dfly {

using namespace std;
using namespace facade;
using namespace alex;

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

    alex_index.insert(k,v);

    return OpStatus::OK;
}

OpResult<PayLoad> OpGet(const OpArgs& op_args, string_view key) {
  KeyType k;

  CONVERT_CHECK(key, k);

  auto it = alex_index.find(k);

  if (it == alex_index.end()) {
    return OpStatus::KEY_NOTFOUND;
  }
    
  return it.payload();
}

OpResult<uint32_t> OpDel(const OpArgs& op_args, string_view key) {
  KeyType k;
  
  CONVERT_CHECK(key, k);

  auto res = alex_index.erase(k);

  return res;
}

}  // namespace

void AlexFamily::AlexAdd(CmdArgList args, ConnectionContext* cntx) {
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

void AlexFamily::AlexGet(CmdArgList args, ConnectionContext* cntx) {
  string_view key = ArgS(args, 0);

  auto cb = [&](Transaction* t, EngineShard* shard) {
    return OpGet(t->GetOpArgs(shard), key);
  };

  OpResult<PayLoad> result = cntx->transaction->ScheduleSingleHopT(std::move(cb));

  cntx->SendLong(*result);
}

void AlexFamily::AlexDel(CmdArgList args, ConnectionContext* cntx) {
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
constexpr uint32_t kAlexAdd = WRITE | SET | FAST;
constexpr uint32_t kAlexGet = READ | SET | FAST;
constexpr uint32_t kAlexDel = WRITE | SET | FAST;
}  // namespace acl

void AlexFamily::Register(CommandRegistry* registry) {
  registry->StartFamily();
  *registry
      << CI{"ALEXADD", CO::WRITE | CO::FAST | CO::DENYOOM, 3, 1, 1, acl::kAlexAdd}.HFUNC(AlexAdd)
      << CI{"ALEXGET", CO::READONLY | CO::FAST, 2, 1, 1, acl::kAlexGet}.HFUNC(AlexGet)
      << CI{"ALEXDEL", CO::WRITE | CO::FAST, 2, 1, 1, acl::kAlexDel}.HFUNC(AlexDel);
    // std::cout<<"alex register"<<std::endl;
}

alex::Alex<KeyType, PayLoad> alex_index;

}  // namespace dfly
