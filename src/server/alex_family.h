// Copyright 2022, DragonflyDB authors.  All rights reserved.
// See LICENSE for licensing terms.
//

#pragma once

#include <variant>

#include "facade/op_status.h"
#include "server/common.h"
#include "server/alex/alex.h"

namespace dfly {

class ConnectionContext;
class CommandRegistry;

using KeyType = uint64_t;
using PayLoad = uint64_t;

class AlexFamily {
 public:
  static void Register(CommandRegistry* registry);

 private:
   static void AlexAdd(CmdArgList args, ConnectionContext* cntx);
   static void AlexGet(CmdArgList args, ConnectionContext* cntx);
   static void AlexDel(CmdArgList args, ConnectionContext* cntx);
};

extern alex::Alex<KeyType,PayLoad> alex_index;

}  // namespace dfly
