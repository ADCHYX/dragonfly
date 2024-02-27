// Copyright 2022, DragonflyDB authors.  All rights reserved.
// See LICENSE for licensing terms.
//

#pragma once

#include <variant>

#include "facade/op_status.h"
#include "server/common.h"
#include "server/finedex/finedex_api.h"

namespace dfly {

class ConnectionContext;
class CommandRegistry;

using KeyType = uint64_t;
using PayLoad = uint64_t;

class FinedexFamily {
 public:
  static void Register(CommandRegistry* registry);

 private:
   static void FinedexAdd(CmdArgList args, ConnectionContext* cntx);
   static void FinedexGet(CmdArgList args, ConnectionContext* cntx);
   static void FinedexDel(CmdArgList args, ConnectionContext* cntx);
};

extern finedexInterface<KeyType,PayLoad> finedex_index;

}  // namespace dfly
