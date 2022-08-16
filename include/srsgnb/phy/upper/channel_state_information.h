/*
 *
 * Copyright 2013-2022 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#pragma once

namespace srsgnb {

/// Channel State Information from received DM-RS.
struct channel_state_information {
  /// Time alignment measurement in microseconds.
  float time_aligment_us;
  /// Average EPRE in decibels.
  float epre_dB;
  /// Average RSRP in decibels.
  float rsrp_dB;
  /// Average SINR in decibels.
  float sinr_dB;
};

} // namespace srsgnb