#ifndef QMCPLUSPLUS_STATE_H
#define QMCPLUSPLUS_STATE_H

#include "QMCDrivers/QMCDriver.h"
#include "QMCDrivers/CloneManager.h"

namespace qmcplusplus
{
  
class State
{
  public:

  MCWalkerConfiguration W;

  State(const MCWalkerConfiguration &W){
    this->W = MCWalkerConfiguration(W);
  };

};

}
#endif
