// Copyright 2022, DragonflyDB authors.  All rights reserved.
// See LICENSE for licensing terms.
//

#pragma once

#include "facade/op_status.h"
#include "server/common.h"

namespace dfly {

class ConnectionContext;
class CommandRegistry;

using SaliKeyType = uint64_t;
using SaliPayLoad = uint64_t;

class SaliFamily {
 public:
  static void Register(CommandRegistry* registry);

 private:
   static void SaliAdd(CmdArgList args, ConnectionContext* cntx);
   static void SaliGet(CmdArgList args, ConnectionContext* cntx);
   static void SaliDel(CmdArgList args, ConnectionContext* cntx);
};


}  // namespace dfly
