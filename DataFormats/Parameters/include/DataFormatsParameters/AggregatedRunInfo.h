// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file GRPECSObject.h
/// \brief Header of the AggregatedRunInfo struct
/// \author ruben.shahoyan@cern.ch sandro.wenzel@cern.ch

#ifndef ALICEO2_DATA_AGGREGATEDRUNINFO_H_
#define ALICEO2_DATA_AGGREGATEDRUNINFO_H_

#include <cstdint>
#include "CCDB/BasicCCDBManager.h"

namespace o2::parameters
{

class GRPECSObject;

/// Composite struct where one may collect important global properties of data "runs"
/// aggregated from various sources (GRPECS, RunInformation CCDB entries, etc.).
/// Also offers the authoritative algorithms to collect these information for easy reuse
/// across various algorithms (anchoredMC, analysis, ...)
struct AggregatedRunInfo {
  int runNumber = 0;       // run number
  int64_t sor = 0;         // best known timestamp for the start of run
  int64_t eor = 0;         // best known timestamp for end of run
  int64_t orbitsPerTF = 0; // number of orbits per TF
  int64_t orbitReset = 0;  // timestamp of orbit reset before run
  int64_t orbitSOR = 0;    // orbit when run starts after orbit reset
  int64_t orbitEOR = 0;    // orbit when run ends after orbit reset

  // we may have pointers to actual data source objects GRPECS, ...
  const o2::parameters::GRPECSObject* grpECS = nullptr; // pointer to GRPECSobject (fetched during struct building)

  // fills and returns AggregatedRunInfo for a given run number.
  static AggregatedRunInfo buildAggregatedRunInfo(o2::ccdb::CCDBManagerInstance& ccdb, int runnumber);
  static AggregatedRunInfo buildAggregatedRunInfo(int runnumber, long sorMS, long eorMS, long orbitResetMUS, const o2::parameters::GRPECSObject* grpecs, const std::vector<Long64_t>* ctfFirstRunOrbitVec);
};

} // namespace o2::parameters

#endif
