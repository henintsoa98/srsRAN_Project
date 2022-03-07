
#ifndef SRSGNB_UE_H
#define SRSGNB_UE_H

#include "srsgnb/adt/circular_map.h"
#include "srsgnb/mac/sched_interface.h"
#include "srsgnb/ran/du_types.h"
#include "srsgnb/ran/rnti.h"

namespace srsgnb {

class ue_carrier
{
public:
  unsigned active_bwp_id() const { return 0; }
  bool     is_active() const { return true; }
};

class ue
{
public:
  ue(const add_ue_configuration_request& req) {}

  void slot_indication(slot_point sl_tx) {}

  ue_carrier* find_cc(du_cell_index_t cell_index)
  {
    srsran_assert(cell_index < MAX_CELLS, "Invalid cell_index={}", cell_index);
    return cells[cell_index].get();
  }

  bool has_pending_txs() const { return true; }

private:
  static const size_t MAX_CELLS = 4;

  std::array<std::unique_ptr<ue_carrier>, MAX_CELLS> cells;
};

using ue_map_t = circular_map<rnti_t, std::unique_ptr<ue>, MAX_NOF_UES>;

} // namespace srsgnb

#endif // SRSGNB_UE_H