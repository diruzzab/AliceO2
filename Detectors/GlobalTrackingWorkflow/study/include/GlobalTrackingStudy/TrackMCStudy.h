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

#ifndef O2_TRACKING_STUDY_H
#define O2_TRACKING_STUDY_H

#include "Framework/DataProcessorSpec.h"
#include "Framework/Task.h"
#include "ReconstructionDataFormats/GlobalTrackID.h"
#include "TPCCalibration/CorrectionMapsLoader.h"

namespace o2::trackstudy
{

/// create a processor spec
o2::framework::DataProcessorSpec getTrackMCStudySpec(o2::dataformats::GlobalTrackID::mask_t srcTracks, o2::dataformats::GlobalTrackID::mask_t srcClus, const o2::tpc::CorrectionMapsLoaderGloOpts& sclOpts);

} // namespace o2::trackstudy

#endif
