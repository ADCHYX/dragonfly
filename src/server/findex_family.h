// Copyright 2022, DragonflyDB authors.  All rights reserved.
// See LICENSE for licensing terms.
//

#pragma once

#include <variant>

#include "facade/op_status.h"
#include "server/common.h"
#include "server/findex/findex_api.h"

namespace dfly {

class ConnectionContext;
class CommandRegistry;

class FindexFamily {
 public:
  static void Register(CommandRegistry* registry);

 private:
   static void FindexlAdd(CmdArgList args, ConnectionContext* cntx);
   static void FindexGet(CmdArgList args, ConnectionContext* cntx);
   static void FindexDel(CmdArgList args, ConnectionContext* cntx);
};

}  // namespace dfly
