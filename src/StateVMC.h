#ifndef QMCPLUSPLUS_STATE_H
#define QMCPLUSPLUS_STATE_H

//#include "QMCDrivers/QMCDriver.h"
//#include "QMCDrivers/CloneManager.h"
#include "Particle/MCWalkerConfiguration.h"

namespace qmcplusplus
{

class MCWalkerConfiguration;
  
class StateVMC
{
  public:

  typedef MCWalkerConfiguration::Walker_t Walker_t;
  typedef MCWalkerConfiguration::WalkerList_t WalkerList_t;
  //typedef std::vector<Walker_t*> WalkerList_t;

  MCWalkerConfiguration W;

  StateVMC(const MCWalkerConfiguration &W){
    this->W = MCWalkerConfiguration(W);
    this->W.WalkerList = cloneWalkerList(W.WalkerList);
  };

  ~StateVMC(){
    for (auto i = 0; i < this->W.WalkerList.size(); ++i){
      //delete this->W.WalkerList[i];
    }
  };

  WalkerList_t cloneWalkerList(const WalkerList_t &WalkerList){
    WalkerList_t WalkerListLocal;
    for (auto i = 0; i < WalkerList.size(); ++i){
      WalkerListLocal.push_back(new Walker_t(*(WalkerList[i])));
    }

    return WalkerListLocal;
  };

};

}
#endif
