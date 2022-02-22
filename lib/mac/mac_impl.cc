
#include "mac_impl.h"
#include <mutex>

namespace srsgnb {

class sched_response_adapter final : public sched_cfg_notifier
{
public:
  sched_response_adapter(mac_impl& mac_) : mac_ref(mac_) {}

  void on_ue_config_complete(rnti_t rnti) override { mac_ref.sched_ue_config_response(rnti); }
  void on_ue_delete_response(rnti_t rnti) override { mac_ref.sched_ue_delete_response(rnti); }

  mac_impl& mac_ref;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

mac_impl::mac_impl(mac_config_notifier& cfg_notifier_,
                   mac_ul_sdu_notifier& ul_ccch_notifier_,
                   task_executor&       ul_exec_,
                   span<task_executor*> dl_execs_,
                   task_executor&       ctrl_exec_) :
  logger(srslog::fetch_basic_logger("MAC")),
  sched_notifier(std::make_unique<sched_response_adapter>(*this)),
  dl_worker(*sched_notifier),
  ul_worker(ul_ccch_notifier_),
  ctxt(cfg_notifier_, ul_exec_, dl_execs_, ctrl_exec_, dl_worker, ul_worker),
  ctrl_worker(ctxt)
{}

void mac_impl::push_ul_pdu(mac_rx_data_indication pdu)
{
  ul_worker.push_pdu(pdu.rnti, pdu.lcid, pdu.cell_index, pdu.pdu);
}

void mac_impl::ue_create_request(const mac_ue_create_request_message& msg)
{
  ctrl_worker.ue_create_request(msg);
}

void mac_impl::ue_delete_request(const mac_ue_delete_request_message& msg)
{
  ctrl_worker.ue_delete_request(msg);
}

void mac_impl::sched_ue_config_response(rnti_t rnti)
{
  ctrl_worker.sched_ue_create_response(rnti);
}

void mac_impl::sched_ue_delete_response(rnti_t rnti)
{
  ctrl_worker.sched_ue_delete_response(rnti);
}

void mac_impl::slot_indication(slot_point sl_tx, du_cell_index_t cc)
{
  {
    std::lock_guard<std::mutex> lock(dl_mutex);
    // synchronize sched
  }

  // for each cc, generate MAC DL SDUs
  mac_ue_context* u = ctrl_worker.find_by_rnti(0x4601);
  if (u != nullptr) {
    byte_buffer pdu;

    // TODO: Call MAC SDU builder
  }
}

} // namespace srsgnb
