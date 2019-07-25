#ifndef QMCPLUSPLUS_STATE_H
#define QMCPLUSPLUS_STATE_H

//#include "QMCDrivers/QMCDriver.h"
//#include "QMCDrivers/CloneManager.h"
#include "Particle/MCWalkerConfiguration.h"
#include "QMCDrivers/QMCUpdateBase.h"

namespace qmcplusplus
{

class MCWalkerConfiguration;
  
class StateVMC
{
  public:

  typedef MCWalkerConfiguration::Walker_t Walker_t;
  typedef MCWalkerConfiguration::WalkerList_t WalkerList_t;
  //typedef std::vector< Walker_t* > WalkerList_t;

  MCWalkerConfiguration W;
  std::vector< QMCUpdateBase* > Movers;
  std::vector<int> wPerNode;

  StateVMC(const MCWalkerConfiguration &W, std::vector< int > &wPerNode){
    this->W = MCWalkerConfiguration(W);
    this->W.WalkerList = cloneWalkerList(W.WalkerList);
    this->wPerNode = wPerNode;
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

  StateVMC* clone(void){
    StateVMC *clonedState = new StateVMC(this->W, this->wPerNode);

    return clonedState;
  };

};

}
#endif
