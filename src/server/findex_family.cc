#include "server/findex_family.h"

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

finedexInterface<uint64_t,uint64_t> findex;

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
  // uint32_t k,v;
  // try{
  //   std::string str_k(key);
  //   std::string str_v(value);
  //   k = std::stoul(str_k);
  //   v = std::stoul(str_v);
  // } catch (const std::exception& e) {
  //   return OpStatus::WRONG_TYPE;
  // }
  // findex.put(k,v);
  // return OpStatus::OK;

    KeyType k;
    PayLoad v;

    CONVERT_CHECK(key, k);
    CONVERT_CHECK(value, v);

    findex.put(k,v);

    return OpStatus::OK;
}

OpResult<PayLoad> OpGet(const OpArgs& op_args, string_view key) {
  // uint64_t k,v;
  // try{
  //   std::string str_k(key);
  //   k = std::stoul(str_k);
  // } catch (const std::exception& e) {
  //   return OpStatus::WRONG_TYPE;
  // }
  // auto it = findex.get(k,v);
  // if(it == false)
  //   return OpStatus::KEY_NOTFOUND;
  // return v;

  KeyType k;
  PayLoad v;

  CONVERT_CHECK(key, k);

  auto it = findex.get(k,v);

  if (it == false) {
    return OpStatus::KEY_NOTFOUND;
  }
    
  return v;
}

OpResult<uint32_t> OpDel(const OpArgs& op_args, string_view key) {
  // uint64_t k,v;
  // try{
  //   std::string str_k(key);
  //   k = std::stoul(str_k);
  // } catch (const std::exception& e) {
  //   return OpStatus::WRONG_TYPE;
  // }
  // auto res = findex.remove(k);
  // return res;

  KeyType k;
  
  CONVERT_CHECK(key, k);

  auto res = findex.remove(k);

  return res;
}

}  // namespace

void FindexFamily::FindexAdd(CmdArgList args, ConnectionContext* cntx) {
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

void FindexFamily::FindexGet(CmdArgList args, ConnectionContext* cntx) {
  string_view key = ArgS(args, 0);

  auto cb = [&](Transaction* t, EngineShard* shard) {
    return OpGet(t->GetOpArgs(shard), key);
  };

  OpResult<PayLoad> result = cntx->transaction->ScheduleSingleHopT(std::move(cb));

  cntx->SendLong(*result);
}

void FindexFamily::FindexDel(CmdArgList args, ConnectionContext* cntx) {
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
constexpr uint32_t kFindexAdd = WRITE | SET | FAST;
constexpr uint32_t kFindexGet = READ | SET | FAST;
constexpr uint32_t kFindexDel = WRITE | SET | FAST;
}  // namespace acl

void FindexFamily::Register(CommandRegistry* registry) {
  registry->StartFamily();
  *registry
      << CI{"FindexADD", CO::WRITE | CO::FAST | CO::DENYOOM, 3, 1, 1, acl::kFindexAdd}.HFUNC(FindexAdd)
      << CI{"FindexGET", CO::READONLY | CO::FAST, 2, 1, 1, acl::kFindexGet}.HFUNC(FindexGet)
      << CI{"FindexDEL", CO::WRITE | CO::FAST, 2, 1, 1, acl::kFindexDel}.HFUNC(FindexDel);
}

}  // namespace dfly
