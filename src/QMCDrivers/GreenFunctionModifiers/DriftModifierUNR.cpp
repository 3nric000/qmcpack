//////////////////////////////////////////////////////////////////////////////////////
// This file is distributed under the University of Illinois/NCSA Open Source License.
// See LICENSE file in top directory for details.
//
// Copyright (c) 2019 QMCPACK developers.
//
// File developed by: Ye Luo, yeluo@anl.gov, Argonne National Laboratory
//
// File created by: Ye Luo, yeluo@anl.gov, Argonne National Laboratory
//////////////////////////////////////////////////////////////////////////////////////


#include "QMCDrivers/GreenFunctionModifiers/DriftModifierUNR.h"
#include "OhmmsData/ParameterSet.h"

namespace qmcplusplus
{
void DriftModifierUNR::getDrift(RealType tau, const GradType& qf, PosType& drift) const
{
  // convert the complex WF gradient to real and temporarily store in drift.
  convert(qf, drift);
  RealType vsq = dot(drift, drift);
  RealType sc  = (vsq < std::numeric_limits<RealType>::epsilon())
      ? tau
      : ((-1.0 + std::sqrt(1.0 + 2.0 * a * tau * vsq)) / (a * vsq));
  //Apply the umrigar scaled drift.
  drift *= sc;
}

bool DriftModifierUNR::parseXML(xmlNodePtr cur)
{
  ParameterSet m_param;
  m_param.add(a, "drift_UNR_a", "double");
  m_param.put(cur);
  app_log() << "  Set drift_modifier UNR parameter a = " << a << std::endl;
  return true;
}

} // namespace qmcplusplus
