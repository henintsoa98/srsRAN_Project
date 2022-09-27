/*
 *
 * Copyright 2013-2022 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "pdcp_tx_test.h"
#include "../../lib/pdcp/pdcp_entity_impl.h"
#include "pdcp_test_vectors.h"
#include "srsgnb/pdcp/pdcp_config.h"
#include "srsgnb/support/test_utils.h"
#include "srsgnb/support/timers.h"
#include <gtest/gtest.h>
#include <queue>

using namespace srsgnb;

/// \brief Test correct creation of PDCP TX  entity
TEST_P(pdcp_tx_test, create_new_entity)
{
  init(GetParam());

  ASSERT_NE(pdcp_tx, nullptr);
}

/// \brief Test correct packing of PDCP data PDU headers
TEST_P(pdcp_tx_test, sn_pack)
{
  init(GetParam());

  auto test_hdr_writer = [this](uint32_t sn) {
    // Generate PDU header
    byte_buffer buf = {};
    pdcp_tx->write_data_pdu_header(buf, sn);
    // Get expected PDU header
    byte_buffer exp_pdu;
    get_expected_pdu(sn, exp_pdu);
    byte_buffer_view expected_hdr = {exp_pdu, 0, pdu_hdr_len};
    ASSERT_EQ(buf.length(), expected_hdr.length());
    ASSERT_EQ(buf, expected_hdr);
  };

  if (config.sn_size == pdcp_sn_size::size12bits) {
    test_hdr_writer(0);
    test_hdr_writer(2048);
    test_hdr_writer(4096);
  } else if (config.sn_size == pdcp_sn_size::size18bits) {
    test_hdr_writer(0);
    test_hdr_writer(131072);
    test_hdr_writer(262144);
  } else {
    FAIL();
  }
}

/// \brief Test correct generation of PDCP PDUs
TEST_P(pdcp_tx_test, pdu_gen)
{
  init(GetParam());

  auto test_pdu_gen = [this](uint32_t tx_next) {
    srsgnb::test_delimit_logger delimiter("TX PDU generation. SN_SIZE={} COUNT={}", sn_size, tx_next);
    // Set state of PDCP entiy
    pdcp_tx_state st = {tx_next};
    pdcp_tx->set_state(st);
    pdcp_tx->set_as_security_config(sec_cfg);
    pdcp_tx->enable_or_disable_security(pdcp_integrity_enabled::enabled, pdcp_ciphering_enabled::enabled);

    // Write SDU
    byte_buffer sdu = {sdu1};
    pdcp_tx->handle_sdu(std::move(sdu));

    // Get generated PDU
    ASSERT_EQ(test_frame.pdu_queue.size(), 1);
    byte_buffer pdu = std::move(test_frame.pdu_queue.front());
    test_frame.pdu_queue.pop();

    // Get expected PDU
    byte_buffer exp_pdu;
    get_expected_pdu(tx_next, exp_pdu);

    ASSERT_EQ(pdu.length(), exp_pdu.length());
    ASSERT_EQ(pdu, exp_pdu);
  };

  if (config.sn_size == pdcp_sn_size::size12bits) {
    test_pdu_gen(0);
    test_pdu_gen(2048);
    test_pdu_gen(4095);
    test_pdu_gen(4096);
  } else if (config.sn_size == pdcp_sn_size::size18bits) {
    test_pdu_gen(0);
    test_pdu_gen(131072);
    test_pdu_gen(262144);
  } else {
    FAIL();
  }
}

/// Test correct start of PDCP discard timers
/// and normal expiry of them
TEST_P(pdcp_tx_test, discard_timer_and_expiry)
{
  init(GetParam());

  auto test_discard_timer_expiry = [this](uint32_t tx_next) {
    // Set state of PDCP entiy
    pdcp_tx_state st = {tx_next};
    pdcp_tx->set_state(st);
    pdcp_tx->set_as_security_config(sec_cfg);
    pdcp_tx->enable_or_disable_security(pdcp_integrity_enabled::enabled, pdcp_ciphering_enabled::enabled);

    // Write first SDU
    {
      byte_buffer sdu = {sdu1};
      pdcp_tx->handle_sdu(std::move(sdu));
      ASSERT_EQ(1, pdcp_tx->nof_discard_timers());
    }
    // Write second SDU
    {
      byte_buffer sdu = {sdu1};
      pdcp_tx->handle_sdu(std::move(sdu));
      ASSERT_EQ(2, pdcp_tx->nof_discard_timers());
    }
    // Let timers expire
    for (int i = 0; i < 10; i++) {
      timers.tick_all();
    }

    // Timers should have expired now.
    ASSERT_EQ(0, pdcp_tx->nof_discard_timers());
  };

  if (config.sn_size == pdcp_sn_size::size12bits) {
    test_discard_timer_expiry(0);
    test_discard_timer_expiry(2047);
    test_discard_timer_expiry(4095);
  } else if (config.sn_size == pdcp_sn_size::size18bits) {
    test_discard_timer_expiry(0);
    test_discard_timer_expiry(131071);
    test_discard_timer_expiry(262143);
  } else {
    FAIL();
  }
}

/// Test correct start of PDCP discard timers
/// and stop from lower layers
TEST_P(pdcp_tx_test, discard_timer_and_stop)
{
  init(GetParam());

  auto test_discard_timer_stop = [this](uint32_t tx_next) {
    // Set state of PDCP entiy
    pdcp_tx_state st = {tx_next};
    pdcp_tx->set_state(st);
    pdcp_tx->set_as_security_config(sec_cfg);
    pdcp_tx->enable_or_disable_security(pdcp_integrity_enabled::enabled, pdcp_ciphering_enabled::enabled);

    // Write first SDU
    {
      byte_buffer sdu = {sdu1};
      pdcp_tx->handle_sdu(std::move(sdu));
      ASSERT_EQ(1, pdcp_tx->nof_discard_timers());
    }
    // Write second SDU
    {
      byte_buffer sdu = {sdu1};
      pdcp_tx->handle_sdu(std::move(sdu));
      ASSERT_EQ(2, pdcp_tx->nof_discard_timers());
    }

    pdcp_tx->stop_discard_timer(tx_next);
    ASSERT_EQ(1, pdcp_tx->nof_discard_timers());
    // Timers should have expired now.
    pdcp_tx->stop_discard_timer(tx_next + 1);
    ASSERT_EQ(0, pdcp_tx->nof_discard_timers());
  };

  if (config.sn_size == pdcp_sn_size::size12bits) {
    test_discard_timer_stop(0);
    test_discard_timer_stop(2048);
    test_discard_timer_stop(4096);
  } else if (config.sn_size == pdcp_sn_size::size18bits) {
    test_discard_timer_stop(0);
    test_discard_timer_stop(131072);
    test_discard_timer_stop(262144);
  } else {
    FAIL();
  }
}

/// Test COUNT wrap-around protection systems
TEST_P(pdcp_tx_test, count_wraparound)
{
  uint32_t       tx_next_notify = 262144;
  uint32_t       tx_next_max    = 262154;
  uint32_t       tx_next_start  = 262143;
  uint32_t       n_sdus         = 20;
  pdcp_max_count max_count{tx_next_notify, tx_next_max};
  init(GetParam(), pdcp_discard_timer::ms10, max_count);

  auto test_max_count = [this, n_sdus](uint32_t tx_next) {
    // Set state of PDCP entiy
    pdcp_tx_state st = {tx_next};
    pdcp_tx->set_state(st);
    pdcp_tx->set_as_security_config(sec_cfg);
    pdcp_tx->enable_or_disable_security(pdcp_integrity_enabled::enabled, pdcp_ciphering_enabled::enabled);

    // Write first SDU
    for (uint32_t i = 0; i < n_sdus; i++) {
      byte_buffer sdu = {sdu1};
      pdcp_tx->handle_sdu(std::move(sdu));
    }
    // check nof max_count reached and max protocol failures.
    ASSERT_EQ(11, test_frame.pdu_queue.size());
    ASSERT_EQ(10, test_frame.nof_max_count_reached);
    ASSERT_EQ(9, test_frame.nof_protocol_failure);
  };

  if (config.sn_size == pdcp_sn_size::size12bits) {
    test_max_count(tx_next_start);
  } else if (config.sn_size == pdcp_sn_size::size18bits) {
    test_max_count(tx_next_start);
  } else {
    FAIL();
  }
}
///////////////////////////////////////////////////////////////////
// Finally, instantiate all testcases for each supported SN size //
///////////////////////////////////////////////////////////////////
std::string test_param_info_to_string(const ::testing::TestParamInfo<pdcp_sn_size>& info)
{
  fmt::memory_buffer buffer;
  fmt::format_to(buffer, "{}bit", to_number(info.param));
  return fmt::to_string(buffer);
}

INSTANTIATE_TEST_SUITE_P(pdcp_tx_test_all_sn_sizes,
                         pdcp_tx_test,
                         ::testing::Values(pdcp_sn_size::size12bits, pdcp_sn_size::size18bits),
                         test_param_info_to_string);

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
