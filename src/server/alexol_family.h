// Copyright 2022, DragonflyDB authors.  All rights reserved.
// See LICENSE for licensing terms.
//

#pragma once

#include <variant>

#include "facade/op_status.h"
#include "server/common.h"
#include "server/alexol/api.h"

namespace dfly {

class ConnectionContext;
class CommandRegistry;

using KeyType = uint64_t;
using PayLoad = uint64_t;

class AlexolFamily {
 public:
  static void Register(CommandRegistry* registry);
  //static alex::Alex<uint32_t,uint32_t> alexindex;

 private:
   static void AlexolAdd(CmdArgList args, ConnectionContext* cntx);
   static void AlexolGet(CmdArgList args, ConnectionContext* cntx);
   static void AlexolDel(CmdArgList args, ConnectionContext* cntx);
};

extern alexolInterface<KeyType,PayLoad> alexol_index;

}  // namespace dfly
