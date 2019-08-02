#ifndef QMCPLUSPLUS_STATE_H
#define QMCPLUSPLUS_STATE_H

//#include "QMCDrivers/QMCDriver.h"
//#include "QMCDrivers/CloneManager.h"
#include "Particle/MCWalkerConfiguration.h"
#include "QMCDrivers/QMCUpdateBase.h"
#include "QMCDrivers/VMC/VMCUpdatePbyP.h"
#include "QMCDrivers/VMC/VMCUpdateAll.h"
#include "OhmmsApp/RandomNumberControl.h"

namespace qmcplusplus
{

class MCWalkerConfiguration;
  
class StateVMC
{
  public:
  /** enumeration coupled with QMCMode */
  enum
  {
    QMC_UPDATE_MODE,
    QMC_MULTIPLE,
    QMC_OPTIMIZE,
    QMC_WARMUP
  };

  typedef MCWalkerConfiguration::Walker_t Walker_t;
  typedef MCWalkerConfiguration::WalkerList_t WalkerList_t;
  //typedef std::vector< Walker_t* > WalkerList_t;

  MCWalkerConfiguration W;
  TrialWaveFunction *Psi;
  QMCHamiltonian *H;
  TraceManager* Traces;
  EstimatorManagerBase* Estimators;
  std::vector<QMCUpdateBase*> Movers;
  std::vector<int> wPerNode;
  std::vector<EstimatorManagerBase*> estimatorClones;
  std::vector<TraceManager*> traceClones;
  std::vector<MCWalkerConfiguration*> wClones;
  std::vector<TrialWaveFunction*> psiClones;
  std::vector<QMCHamiltonian*> hClones;
  std::vector<RandomGenerator_t*> Rng;
  int NumThreads;
  int nSubSteps;
  std::bitset<4> QMCDriverMode;

  StateVMC(int NumThreads, int nSubSteps, const MCWalkerConfiguration &W, std::vector< int > &wPerNode, EstimatorManagerBase *Estimators, TrialWaveFunction *Psi, QMCHamiltonian *H, TraceManager *Traces, std::bitset<4> &QMCDriverMode){
    this->NumThreads = NumThreads;
    this->W = MCWalkerConfiguration(W);
    this->W.WalkerList = cloneWalkerList(W.WalkerList);
    this->wPerNode = wPerNode;
    this->Psi = Psi->makeClone(this->W);
    this->H = H->makeClone(this->W, *(this->Psi));
    this->Traces = Traces->makeClone();
    this->Traces->master_copy  = true; // ED DANGEROUS: in theory only the master copy should call makeClone() (see src/Estimators/TraceManager.h)
    this->Estimators = new EstimatorManagerBase(*Estimators);
    this->QMCDriverMode = QMCDriverMode;

    // Create wClones, psiClones, hClones
    this->wClones.resize(this->NumThreads);
    this->psiClones.resize(this->NumThreads);
    this->hClones.resize(this->NumThreads);

    this->wClones[0] = &(this->W);
    this->psiClones[0] = this->Psi;
    this->hClones[0] = this->H;
    #pragma omp parallel
    {
      const int ip = omp_get_thread_num();
      if (ip > 0){
        #if defined(USE_PARTCILESET_CLONE)
        this->wClones[ip] = dynamic_cast<MCWalkerConfiguration*>(this->W.get_clone(ip));
        #else
        this->wClones[ip] = new MCWalkerConfiguration(this->W);
        #endif
        this->psiClones[ip] = this->Psi->makeClone(*(this->wClones[ip]));
        this->hClones[ip]   = this->H->makeClone(*(this->wClones[ip]), *(this->psiClones[ip]));
      }
    }

    // Create estimators, traces, and random number generators
    this->Movers.resize(this->NumThreads, 0);
    this->estimatorClones.resize(this->NumThreads, 0);
    this->traceClones.resize(this->NumThreads, 0);
    this->Rng.resize(this->NumThreads, 0);
    #pragma omp parallel for
    for (int ip = 0; ip < this->NumThreads; ++ip){
      this->estimatorClones[ip] = new EstimatorManagerBase(*(this->Estimators));
      this->estimatorClones[ip]->resetTargetParticleSet(*(this->wClones[ip]));
      this->estimatorClones[ip]->setCollectionMode(false);
      #if !defined(REMOVE_TRACEMANAGER)
      this->traceClones[ip] = this->Traces->makeClone();
      #endif
      #ifdef USE_FAKE_RNG
      this->Rng[ip] = new FakeRandom();
      #else
      this->Rng[ip] = new RandomGenerator_t(*(RandomNumberControl::Children[ip])); // ED TODO: check copy constructors of RandomGenerator_t (see src/OhmmsApp/RandomNumberControl.h)
      this->hClones[ip]->setRandomGenerator(this->Rng[ip]);
      #endif
      if (QMCDriverMode[QMC_UPDATE_MODE])
      {
        this->Movers[ip] = new VMCUpdatePbyP(*(this->wClones[ip]), *(this->psiClones[ip]), *(this->hClones[ip]), *(this->Rng[ip]));
      }
      else
      {
        this->Movers[ip] = new VMCUpdateAll(*(this->wClones[ip]), *(this->psiClones[ip]), *(this->hClones[ip]), *(this->Rng[ip]));
      }
      this->Movers[ip]->nSubSteps = nSubSteps;

    }

  };

  ~StateVMC(){
    for (auto i = 0; i < this->W.WalkerList.size(); ++i){
      //delete this->W.WalkerList[i];
    }
    for (int ip = 0; ip < this->NumThreads; ++ip){
      delete this->estimatorClones[ip];
      delete this->traceClones[ip];
      //delete this->wClones[ip];
      delete this->psiClones[ip];
      delete this->hClones[ip];
      delete this->Rng[ip];
      delete this->Movers[ip];
    }
    delete this->Traces;
    delete this->Estimators;
    //delete this->Psi;
    //delete this->H;
  };

  WalkerList_t cloneWalkerList(const WalkerList_t &WalkerList){
    WalkerList_t WalkerListLocal;
    for (auto i = 0; i < WalkerList.size(); ++i){
      WalkerListLocal.push_back(new Walker_t(*(WalkerList[i])));
    }

    return WalkerListLocal;
  };

  StateVMC* clone(void){
    StateVMC *clonedState = new StateVMC(this->NumThreads, this->nSubSteps, this->W, this->wPerNode, this->Estimators, this->Psi, this->H, this->Traces, this->QMCDriverMode);

    return clonedState;
  };

};

}
#endif
