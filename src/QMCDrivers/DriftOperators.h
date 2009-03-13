//////////////////////////////////////////////////////////////////
// (c) Copyright 2005- by Jeongnim Kim
//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
//   Jeongnim Kim
//   National Center for Supercomputing Applications &
//   Materials Computation Center
//   University of Illinois, Urbana-Champaign
//   Urbana, IL 61801
//   e-mail: jnkim@ncsa.uiuc.edu
//   Tel:    217-244-6319 (NCSA) 217-333-3324 (MCC)
//
// Supported by 
//   National Center for Supercomputing Applications, UIUC
//   Materials Computation Center, UIUC
//////////////////////////////////////////////////////////////////
// -*- C++ -*-
#ifndef QMCPLUSPLUS_QMCDRIFTOPERATORS_H
#define QMCPLUSPLUS_QMCDRIFTOPERATORS_H
#include "ParticleBase/ParticleAttribOps.h"
#include "ParticleBase/RandomSeqGenerator.h"
#include "QMCWaveFunctions/OrbitalTraits.h"
namespace qmcplusplus {

  template<class T, unsigned D>
  inline T getDriftScale(T tau, const ParticleAttrib<TinyVector<T,D> >& ga) 
  {
    T vsq=Dot(ga,ga);
    return (vsq<numeric_limits<T>::epsilon())? tau:((-1.0+std::sqrt(1.0+2.0*tau*vsq))/vsq);
  }

  template<class T, unsigned D>
  inline T getDriftScale(T tau, const ParticleAttrib<TinyVector<complex<T>,D> >& ga) 
  {
    T vsq=Dot(ga,ga);
    return (vsq<numeric_limits<T>::epsilon())? tau:((-1.0+std::sqrt(1.0+2.0*tau*vsq))/vsq);
  }

  template<class T, unsigned D>
  inline T getDriftScale(T tau, const TinyVector<T,D>& qf)
  {
    T vsq=dot(qf,qf);
    return (vsq<numeric_limits<T>::epsilon())? tau:((-1.0+std::sqrt(1.0+2.0*tau*vsq))/vsq);
  }

  template<class T, unsigned D>
  inline T getDriftScale(T tau, const TinyVector<complex<T>,D>& qf)
  {
    T vsq=OTCDot<T,T,D>::apply(qf,qf);
    return (vsq<numeric_limits<T>::epsilon())? tau:((-1.0+std::sqrt(1.0+2.0*tau*vsq))/vsq);
  }

  /** evaluate \f$\gamma\$ for \f$ \bar V= \gamma V\f$
   *
   * Using eq. 34 of JCP 99, 2865 (1993)
   * \f$ \bar v(i)=\gamma_i \frac{-1+\sqrt{1+2*a*v^2*\tau}{av^2\tau} v(i)\f$
   */
  template<class T, unsigned D>
  inline T getNodeCorrectionP(T tau, const ParticleAttrib<TinyVector<T,D> >& ga, T a=1) 
  {
    T norm=0.0, norm_scaled=0.0;
    for(int i=0; i<ga.size(); ++i)
    {
      T vsq=dot(ga[i],ga[i]);
      T x=a*vsq*tau;
      T scale= (vsq<numeric_limits<T>::epsilon())? 1.0:((-1.0+std::sqrt(1.0+2.0*x))/x);
      norm_scaled+=vsq*scale*scale;
      norm+=vsq;
    }
    return std::sqrt(norm_scaled/norm);
  }

  /** evaluate \f$\gamma\$ for \f$ \bar V= \gamma V\f$
   *
   * Using eq. 33 of JCP 99, 2865 (1993)
   * \f$ \bar v(i)=\gamma_i \frac{-1+\sqrt{1+2*a*v^2*\tau}{av^2\tau} v(i)\f$
   * Scale velocity for each particle
   */
  template<class T, unsigned D>
  inline T getNodeCorrectionW(T tau, const ParticleAttrib<TinyVector<T,D> >& ga) 
  {
    T vsq=Dot(ga,ga);
    T x=tau*vsq;
    return (vsq<numeric_limits<T>::epsilon())? 1.0:((-1.0+std::sqrt(1.0+2.0*x))/x);
  }

    
  /** evaluate drift using the scaling function by JCP93
   * @param tau timestep
   * @param qf quantum force
   * @param drift drift
   */
  template<class T, unsigned D>
  inline void setScaledDriftPbyP(T tau, 
      const ParticleAttrib<TinyVector<T,D> >& qf,
      ParticleAttrib<TinyVector<T,D> >& drift) 
  {
    for(int iat=0; iat<qf.size(); ++iat)
    {
      T vsq=dot(qf[iat],qf[iat]);
      T sc=(vsq<numeric_limits<T>::epsilon())? tau:((-1.0+std::sqrt(1.0+2.0*tau*vsq))/vsq);
      drift[iat]=sc*qf[iat];
    }
  }
  
  template<class T, unsigned D>
      inline T setScaledDriftPbyPandNodeCorr(T tau, 
                                     const ParticleAttrib<TinyVector<T,D> >& qf,
                                     ParticleAttrib<TinyVector<T,D> >& drift) 
      {
        T norm=0.0, norm_scaled=0.0, tau2=tau*tau;
        for(int iat=0; iat<qf.size(); ++iat)
        {
          T vsq=dot(qf[iat],qf[iat]);
          T sc=(vsq<numeric_limits<T>::epsilon())? tau:((-1.0+std::sqrt(1.0+2.0*tau*vsq))/vsq);
          norm_scaled+=vsq*sc*sc;
          norm+=vsq*tau2;
          drift[iat]=sc*qf[iat];
        }
        return std::sqrt(norm_scaled/norm);
      }

  template<class T, unsigned D>
      inline T setScaledDriftPbyPandNodeCorr(T tau, 
                                     const ParticleAttrib<TinyVector<complex<T>,D> >& qf,
                                     ParticleAttrib<TinyVector<T,D> >& drift) 
      {
        T norm=0.0, norm_scaled=0.0, tau2=tau*tau;
        for(int iat=0; iat<qf.size(); ++iat)
        {
          T vsq=real(dot(qf[iat],qf[iat]));
          T sc=(vsq<numeric_limits<T>::epsilon())? tau:((-1.0+std::sqrt(1.0+2.0*tau*vsq))/vsq);
          norm_scaled+=vsq*sc*sc;
          norm+=vsq*tau2;
          drift[iat]=sc*qf[iat];
        }
        return std::sqrt(norm_scaled/norm);
      }

  template<class T, unsigned D>
      inline T setLargestScaledDriftPbyP(T tau,
                                          const ParticleAttrib<TinyVector<T,D> >& qf,
                                          ParticleAttrib<TinyVector<T,D> >& drift)
    {
      T maxSC=tau;
      for(int iat=0; iat<qf.size(); ++iat)
      {
        T vsq=dot(qf[iat],qf[iat]);
        T sc=(vsq<numeric_limits<T>::epsilon())? tau:((-1.0+std::sqrt(1.0+2.0*tau*vsq))/vsq);
        maxSC=(sc<maxSC? sc:maxSC);
      }
      for(int iat=0; iat<qf.size(); ++iat)
      {
        drift[iat]=maxSC*qf[iat];
      }
      return maxSC;
    }

      
      
  /** da = scaled(tau)*ga
   * @param tau time step
   * @param qf real quantum forces
   * @param drift drift
   */
  template<class T, unsigned D>
  inline void setScaledDrift(T tau, 
      const ParticleAttrib<TinyVector<T,D> >& qf,
      ParticleAttrib<TinyVector<T,D> >& drift) {
    T s = getDriftScale(tau,qf);
    PAOps<T,D>::scale(s,qf,drift);
  }

  /** da = scaled(tau)*ga
   * @param tau time step
   * @param qf real quantum forces
   * @param drift drift
   */
  template<class T, unsigned D>
  inline void setScaledDrift(T tau, 
      ParticleAttrib<TinyVector<T,D> >& drift) {
    T s = getDriftScale(tau,drift);
    drift *=s;
  }


  /** da = scaled(tau)*ga
   * @param tau time step
   * @param qf complex quantum forces
   * @param drift drift
   */
  template<class T, unsigned D>
  inline void setScaledDrift(T tau, 
      const ParticleAttrib<TinyVector<complex<T>,D> >& qf,
      ParticleAttrib<TinyVector<T,D> >& drift) {
    T s = getDriftScale(tau,qf);
    PAOps<T,D>::scale(s,qf,drift);
  }

  /** da = scaled(tau)*ga
   * @param tau time step
   * @param qf complex quantum forces
   * @param drift drift
   */
  template<class T, unsigned D>
  inline void setScaledDrift(T tau, 
      const ParticleAttrib<TinyVector<complex<T>,D> >& qf,
      ParticleAttrib<TinyVector<complex<T>,D> >& drift) {
    T s = getDriftScale(tau,qf);
    ///INCOMPLETE implementation
    //PAOps<T,D>::scale(s,qf,drift);
  }

  template<class T, unsigned D>
  inline void assignDrift(T s, 
      const ParticleAttrib<TinyVector<T,D> >& ga,
      ParticleAttrib<TinyVector<T,D> >& da) {
    PAOps<T,D>::scale(s,ga,da);
  }

  template<class T, unsigned D>
  inline void assignDrift(T s, 
      const ParticleAttrib<TinyVector<complex<T>,D> >& ga,
      ParticleAttrib<TinyVector<T,D> >& da) {
    PAOps<T,D>::scale(s,ga,da);
  }

}
#endif
/***************************************************************************
 * $RCSfile$   $Author$
 * $Revision$   $Date$
 * $Id$ 
 ***************************************************************************/
