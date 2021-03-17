/**
 *
 * \section COPYRIGHT
 *
 * Copyright 2013-2020 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#ifndef SRSLTE_SCHED_CARRIER_H
#define SRSLTE_SCHED_CARRIER_H

#include "sched.h"
#include "schedulers/sched_base.h"
#include "srslte/srslog/srslog.h"

namespace srsenb {

class bc_sched;
class ra_sched;

class sched::carrier_sched
{
public:
  explicit carrier_sched(rrc_interface_mac*                              rrc_,
                         std::map<uint16_t, std::unique_ptr<sched_ue> >* ue_db_,
                         uint32_t                                        enb_cc_idx_,
                         sched_result_ringbuffer*                        sched_results_);
  ~carrier_sched();
  void                   reset();
  void                   carrier_cfg(const sched_cell_params_t& sched_params_);
  void                   set_dl_tti_mask(uint8_t* tti_mask, uint32_t nof_sfs);
  const cc_sched_result& generate_tti_result(srslte::tti_point tti_rx);
  int                    dl_rach_info(dl_sched_rar_info_t rar_info);

  // getters
  const ra_sched* get_ra_sched() const { return ra_sched_ptr.get(); }
  //! Get a subframe result for a given tti
  const sf_sched_result* get_sf_result(tti_point tti_rx) const;

private:
  //! Compute DL scheduler result for given TTI
  void alloc_dl_users(sf_sched* tti_result);
  //! Compute UL scheduler result for given TTI
  int alloc_ul_users(sf_sched* tti_sched);
  //! Get sf_sched for a given TTI
  sf_sched* get_sf_sched(srslte::tti_point tti_rx);

  // args
  const sched_cell_params_t*                      cc_cfg = nullptr;
  srslog::basic_logger&                           logger;
  rrc_interface_mac*                              rrc   = nullptr;
  std::map<uint16_t, std::unique_ptr<sched_ue> >* ue_db = nullptr;
  const uint32_t                                  enb_cc_idx;

  // Subframe scheduling logic
  srslte::circular_array<sf_sched, TTIMOD_SZ> sf_scheds;

  // scheduling results
  sched_result_ringbuffer* prev_sched_results;

  std::vector<uint8_t> sf_dl_mask; ///< Some TTIs may be forbidden for DL sched due to MBMS

  std::unique_ptr<bc_sched>   bc_sched_ptr;
  std::unique_ptr<ra_sched>   ra_sched_ptr;
  std::unique_ptr<sched_base> sched_algo;
};

//! Broadcast (SIB + paging) scheduler
class bc_sched
{
public:
  explicit bc_sched(const sched_cell_params_t& cfg_, rrc_interface_mac* rrc_);
  void dl_sched(sf_sched* tti_sched);
  void reset();

private:
  struct sched_sib_t {
    bool      is_in_window = false;
    tti_point window_start{};
    uint32_t  n_tx = 0;
  };

  void update_si_windows(sf_sched* tti_sched);
  void alloc_sibs(sf_sched* tti_sched);
  void alloc_paging(sf_sched* tti_sched);

  // args
  const sched_cell_params_t* cc_cfg = nullptr;
  rrc_interface_mac*         rrc    = nullptr;
  srslog::basic_logger&      logger;

  std::array<sched_sib_t, sched_interface::MAX_SIBS> pending_sibs;

  // TTI specific
  tti_point current_tti{};
  uint32_t  bc_aggr_level = 2;
};

//! RAR/Msg3 scheduler
class ra_sched
{
public:
  using dl_sched_rar_info_t  = sched_interface::dl_sched_rar_info_t;
  using dl_sched_rar_t       = sched_interface::dl_sched_rar_t;
  using dl_sched_rar_grant_t = sched_interface::dl_sched_rar_grant_t;

  explicit ra_sched(const sched_cell_params_t& cfg_, sched_ue_list& ue_db_);
  void dl_sched(sf_sched* tti_sched);
  void ul_sched(sf_sched* sf_dl_sched, sf_sched* sf_msg3_sched);
  int  dl_rach_info(dl_sched_rar_info_t rar_info);
  void reset();

private:
  alloc_outcome_t allocate_pending_rar(sf_sched* tti_sched, const pending_rar_t& rar, uint32_t& nof_grants_alloc);

  // args
  srslog::basic_logger&      logger;
  const sched_cell_params_t* cc_cfg = nullptr;
  sched_ue_list*             ue_db  = nullptr;

  std::deque<pending_rar_t> pending_rars;
  uint32_t                  rar_aggr_level   = 2;
  static const uint32_t     PRACH_RAR_OFFSET = 3; // TS 36.321 Sec. 5.1.4
};

} // namespace srsenb

#endif // SRSLTE_SCHED_CARRIER_H
