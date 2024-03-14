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

using FinedexKeyType = uint64_t;
using FinedexPayLoad = uint64_t;

class FinedexFamily {
 public:
  static void Register(CommandRegistry* registry);

 private:
   static void FinedexAdd(CmdArgList args, ConnectionContext* cntx);
   static void FinedexGet(CmdArgList args, ConnectionContext* cntx);
   static void FinedexDel(CmdArgList args, ConnectionContext* cntx);
};

extern finedexInterface<FinedexKeyType, FinedexPayLoad> finedex_index;

}  // namespace dfly
