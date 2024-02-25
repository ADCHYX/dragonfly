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

using KeyType = uint64_t;
using PayLoad = uint64_t;

class FindexFamily {
 public:
  static void Register(CommandRegistry* registry);

 private:
   static void FindexAdd(CmdArgList args, ConnectionContext* cntx);
   static void FindexGet(CmdArgList args, ConnectionContext* cntx);
   static void FindexDel(CmdArgList args, ConnectionContext* cntx);
};

}  // namespace dfly
