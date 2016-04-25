/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
    included in this distribution in the file called "COPYING". If not,
    see <http://www.gnu.org/licenses/>.

  Contact Information
  OpenAirInterface Admin: openair_admin@eurecom.fr
  OpenAirInterface Tech : openair_tech@eurecom.fr
  OpenAirInterface Dev  : openair4g-devel@lists.eurecom.fr

  Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

*******************************************************************************/

/*! \file eNB_agent_scheduler_dataplane.c
 * \brief data plane procedures related to eNB scheduling
 * \author Xenofon Foukas
 * \date 2016
 * \email: x.foukas@sms.ed.ac.uk
 * \version 0.1
 * @ingroup _mac

 */

#include "assertions.h"
#include "PHY/defs.h"
#include "PHY/extern.h"

#include "SCHED/defs.h"
#include "SCHED/extern.h"

#include "LAYER2/MAC/enb_agent_mac_proto.h"
#include "LAYER2/MAC/defs.h"
#include "LAYER2/MAC/proto.h"
#include "LAYER2/MAC/extern.h"
#include "LAYER2/MAC/progran_dci_conversions.h"

#include "UTIL/LOG/log.h"
#include "UTIL/LOG/vcd_signal_dumper.h"
#include "UTIL/OPT/opt.h"
#include "OCG.h"
#include "OCG_extern.h"

#include "RRC/LITE/extern.h"
#include "RRC/L2_INTERFACE/openair_rrc_L2_interface.h"

#include "header.pb-c.h"
#include "progran.pb-c.h"

#include "SIMULATION/TOOLS/defs.h" // for taus


void apply_dl_scheduling_decisions(mid_t mod_id,
				   uint32_t frame,
				   uint32_t subframe,
				   int *mbsfn_flag,
				   const Protocol__ProgranMessage *dl_scheduling_info) {

  Protocol__PrpDlMacConfig *mac_config = dl_scheduling_info->dl_mac_config_msg;

  // Check if there is anything to schedule for random access
  if (mac_config->n_dl_rar > 0) {
    /*TODO: call the random access data plane function*/
  }

  // Check if there is anything to schedule for paging/broadcast
  if (mac_config->n_dl_broadcast > 0) {
    /*TODO: call the broadcast/paging data plane function*/
  }

  // Check if there is anything to schedule for the UEs
  if (mac_config->n_dl_ue_data > 0) {
    apply_ue_spec_scheduling_decisions(mod_id, frame, subframe, mbsfn_flag,
				       mac_config->n_dl_ue_data, mac_config->dl_ue_data);
  }
  
}


void apply_ue_spec_scheduling_decisions(mid_t mod_id,
					uint32_t frame,
					uint32_t subframe,
					int *mbsfn_flag,
					uint32_t n_dl_ue_data,
					const Protocol__PrpDlData **dl_ue_data) {

  uint8_t               CC_id;
  int                   UE_id;
  int                   N_RBG[MAX_NUM_CCs];
  unsigned char         aggregation;
  mac_rlc_status_resp_t rlc_status;
  unsigned char         ta_len=0;
  unsigned char         header_len = 0, header_len_tmp = 0;
  unsigned char         sdu_lcids[11],offset,num_sdus=0;
  uint16_t              nb_rb,nb_rb_temp,total_nb_available_rb[MAX_NUM_CCs],nb_available_rb;
  uint16_t              TBS,j,sdu_lengths[11],rnti,padding=0,post_padding=0;
  unsigned char         dlsch_buffer[MAX_DLSCH_PAYLOAD_BYTES];
  unsigned char         round            = 0;
  unsigned char         harq_pid         = 0;
  
  LTE_eNB_UE_stats     *eNB_UE_stats     = NULL;
  uint16_t              sdu_length_total = 0;
  int                   mcs;
  uint16_t              min_rb_unit[MAX_NUM_CCs];
  short                 ta_update        = 0;
  eNB_MAC_INST         *eNB      = &eNB_mac_inst[mod_id];
  UE_list_t            *UE_list  = &eNB->UE_list;
  LTE_DL_FRAME_PARMS   *frame_parms[MAX_NUM_CCs];
  int32_t                 normalized_rx_power, target_rx_power;
  int32_t                 tpc=1;
  static int32_t          tpc_accumulated=0;
  UE_sched_ctrl           *ue_sched_ctl;

  int i;

  Protocol__PrpDlData *dl_data;
  Protocol__PrpDlDci *dl_dci;

  uint32_t rlc_size, n_lc, lcid;
  
  
  // For each UE-related command
  for (i = 0; i < n_dl_ue_data; i++) {
      
    dl_data = dl_ue_data[i];
    dl_dci = dl_data->dl_dci;

    CC_id = dl_data->serv_cell_index;
    frame_parms[CC_id] = mac_xface->get_lte_frame_parms(mod_id, CC_id);
    
    rnti = dl_data->rnti;
    UE_id = find_ue(rnti, PHY_vars_eNB_g[mod_id][CC_id]);

    ue_sched_ctl = &UE_list->UE_sched_ctrl[UE_id];
    eNB_UE_stats = mac_xface->get_eNB_UE_stats(mod_id,CC_id,rnti);
    
    round = dl_dci->rv[0];
    harq_pid = dl_dci->harq_process;
    
    // If this is a new transmission
    if (round == 0) {
      // First we have to deal with the creation of the PDU based on the message instructions
      rlc_status.bytes_in_buffer = 0;
      
      TBS = dl_dci->tbs_size[0];
      
      if (dl_data->n_ce_bitmap > 0) {
	//Check if there is TA command and set the length appropriately
	ta_len = (dl_data->ce_bitmap[0] & PROTOCOL__PRP_CE_TYPE__PRPCET_TA) ? 2 : 0; 
      }

      if (ta_len > 0) {
	// Reset the measurement
	ue_sched_ctl->ta_timer = 20;
	eNB_UE_stats->timing_advance_update = 0;
      }
      
      n_lc = dl_data->n_rlc_pdu;
      num_sdus = 0;
      sdu_length_total = 0;
      // Go through each one of the channel commands and create SDUs
      for (i = 0; i < n_lc; i++) {
	lcid = dl_data->rlc_pdu[i]->rlc_pdu_tb[0]->logical_channel_id;
	rlc_size = dl_data->rlc_pdu[i]->rlc_pdu_tb[0]->size;
	LOG_D(MAC,"[TEST] [eNB %d] Frame %d, LCID %d, CC_id %d, Requesting %d bytes from RLC (RRC message)\n",
	      mod_id, frame, lcid, CC_id, rlc_size);
	if (rlc_size > 0) {
	  
	  rlc_status = mac_rlc_status_ind(mod_id,
	   				  rnti,
	   				  mod_id,
	   				  frame,
	   				  ENB_FLAG_YES,
	   				  MBMS_FLAG_NO,
	   				  lcid,
	   				  0);

	  if (rlc_status.bytes_in_buffer > 0) {

	    rlc_status = mac_rlc_status_ind(mod_id,
					    rnti,
					    mod_id,
					    frame,
					    ENB_FLAG_YES,
					    MBMS_FLAG_NO,
					    lcid,
					    rlc_size); // transport block set size
	  
	    sdu_lengths[i] = 0;
	  
	    LOG_D(MAC, "[TEST] RLC can give %d bytes for LCID %d during second call\n", rlc_status.bytes_in_buffer, lcid);
	  
	    if (rlc_status.bytes_in_buffer > 0) {
	      
	      sdu_lengths[i] += mac_rlc_data_req(mod_id,
						 rnti,
						 mod_id,
						 frame,
						 ENB_FLAG_YES,
						 MBMS_FLAG_NO,
						 lcid,
						 (char *)&dlsch_buffer[sdu_length_total]);
	      
	      LOG_D(MAC,"[eNB %d][LCID %d] CC_id %d Got %d bytes from RLC\n",mod_id, lcid, CC_id, sdu_lengths[i]);
	      sdu_length_total += sdu_lengths[i];
	      sdu_lcids[i] = lcid;
	      
	      UE_list->eNB_UE_stats[CC_id][UE_id].num_pdu_tx[lcid] += 1;
	      UE_list->eNB_UE_stats[CC_id][UE_id].num_bytes_tx[lcid] += sdu_lengths[i];
	      
	      if (sdu_lengths[i] < 128) {
		header_len += 2;
	      } else {
		header_len += 3;
	      }
	      
	      num_sdus++;
	    }
	  }
	}
      } // SDU creation end
      
      
      if (((sdu_length_total + header_len) > 0)) {

	header_len_tmp = header_len;
	
	// If we have only a single SDU, header length becomes 1
	if (header_len == 2 || header_len == 3) {
	  header_len = 1;
	} else {
	  header_len--;
	}
	
	// there is a payload
	if (((sdu_length_total + header_len) > 0)) {
	  // If we need a 1 or 2 bit padding or no padding at all
	  if ((TBS - header_len - sdu_length_total - ta_len) <= 2
	      || (TBS - header_len - sdu_length_total - ta_len) > TBS) { //protect from overflow
	    padding = (TBS - header_len - sdu_length_total - ta_len);
	    post_padding = 0;
	  } else { // The last sdu needs to have a length field, since we add padding
	    padding = 0;
	    header_len = header_len_tmp;	    
	    post_padding = TBS - sdu_length_total - header_len - ta_len - 1; // 1 is for the postpadding header
	  }
	}
	
	ta_update = (ta_len > 0) ? ue_sched_ctl->ta_update : 0;

	// If there is nothing to schedule, just leave
	if ((sdu_length_total) <= 0) { 
	  return;
	}
	
	offset = generate_dlsch_header((unsigned char*)UE_list->DLSCH_pdu[CC_id][0][UE_id].payload[0],
				       num_sdus,              //num_sdus
				       sdu_lengths,  //
				       sdu_lcids,
				       255,                                   // no drx
				       ta_update, // timing advance
				       NULL,                                  // contention res id
				       padding,
				       post_padding);
	
#ifdef DEBUG_eNB_SCHEDULER
	LOG_T(MAC,"[eNB %d] First 16 bytes of DLSCH : \n");
	
	for (i=0; i<16; i++) {
	  LOG_T(MAC,"%x.",dlsch_buffer[i]);
	}
	
	LOG_T(MAC,"\n");
#endif
	// cycle through SDUs and place in dlsch_buffer
	memcpy(&UE_list->DLSCH_pdu[CC_id][0][UE_id].payload[0][offset],dlsch_buffer,sdu_length_total);
	// memcpy(&eNB_mac_inst[0].DLSCH_pdu[0][0].payload[0][offset],dcch_buffer,sdu_lengths[0]);
	
	// fill remainder of DLSCH with random data
	for (j=0; j<(TBS-sdu_length_total-offset); j++) {
	  UE_list->DLSCH_pdu[CC_id][0][UE_id].payload[0][offset+sdu_length_total+j] = (char)(taus()&0xff);
          }
	
	//eNB_mac_inst[0].DLSCH_pdu[0][0].payload[0][offset+sdu_lengths[0]+j] = (char)(taus()&0xff);
	if (opt_enabled == 1) {
	  trace_pdu(1, (uint8_t *)UE_list->DLSCH_pdu[CC_id][0][UE_id].payload[0],
		    TBS, mod_id, 3, UE_RNTI(mod_id, UE_id),
		    eNB->subframe,0,0);
	  LOG_D(OPT,"[eNB %d][DLSCH] CC_id %d Frame %d  rnti %x  with size %d\n",
		mod_id, CC_id, frame, UE_RNTI(mod_id,UE_id), TBS);
	}
	
	// store stats
	eNB->eNB_stats[CC_id].dlsch_bytes_tx+=sdu_length_total;
	eNB->eNB_stats[CC_id].dlsch_pdus_tx+=1;
	UE_list->eNB_UE_stats[CC_id][UE_id].dl_cqi= eNB_UE_stats->DL_cqi[0];
	
	UE_list->eNB_UE_stats[CC_id][UE_id].crnti= rnti;
	UE_list->eNB_UE_stats[CC_id][UE_id].rrc_status=mac_eNB_get_rrc_status(mod_id, rnti);
	UE_list->eNB_UE_stats[CC_id][UE_id].harq_pid = harq_pid; 
	UE_list->eNB_UE_stats[CC_id][UE_id].harq_round = round;
	
	//nb_rb = UE_list->UE_template[CC_id][UE_id].nb_rb[harq_pid];
	//Find the number of resource blocks and set them to the template for retransmissions
	nb_rb = get_min_rb_unit(mod_id, CC_id);
	uint16_t stats_tbs = mac_xface->get_TBS_DL(dl_dci->mcs[0], nb_rb);

	while (stats_tbs < TBS) {
	  nb_rb += get_min_rb_unit(mod_id, CC_id);
	  stats_tbs = mac_xface->get_TBS_DL(dl_dci->mcs[0], nb_rb);
	}
	
	UE_list->eNB_UE_stats[CC_id][UE_id].rbs_used = nb_rb;
	UE_list->eNB_UE_stats[CC_id][UE_id].total_rbs_used += nb_rb;
	UE_list->eNB_UE_stats[CC_id][UE_id].dlsch_mcs1=dl_dci->mcs[0];
	UE_list->eNB_UE_stats[CC_id][UE_id].dlsch_mcs2=dl_dci->mcs[0];
	UE_list->eNB_UE_stats[CC_id][UE_id].TBS = TBS;
	
	UE_list->eNB_UE_stats[CC_id][UE_id].overhead_bytes= TBS - sdu_length_total;
	UE_list->eNB_UE_stats[CC_id][UE_id].total_sdu_bytes+= sdu_length_total;
	UE_list->eNB_UE_stats[CC_id][UE_id].total_pdu_bytes+= TBS;
	UE_list->eNB_UE_stats[CC_id][UE_id].total_num_pdus+=1;
	
	//eNB_UE_stats->dlsch_mcs1 = cqi_to_mcs[eNB_UE_stats->DL_cqi[0]];
	//eNB_UE_stats->dlsch_mcs1 = cmin(eNB_UE_stats->dlsch_mcs1, openair_daq_vars.target_ue_dl_mcs);
      }
    } else {
      // No need to create anything apart of DCI in case of retransmission
      
      /*TODO: Must add these */
      //      eNB_UE_stats->dlsch_trials[round]++;
      //UE_list->eNB_UE_stats[CC_id][UE_id].num_retransmission+=1;
      //UE_list->eNB_UE_stats[CC_id][UE_id].rbs_used_retx=nb_rb;
      //UE_list->eNB_UE_stats[CC_id][UE_id].total_rbs_used_retx+=nb_rb;
      //UE_list->eNB_UE_stats[CC_id][UE_id].ncce_used_retx=nCCECC_id];
    }

    UE_list->UE_template[CC_id][UE_id].oldNDI[dl_dci->harq_process] = dl_dci->ndi[0];
    eNB_UE_stats->dlsch_mcs1 = dl_dci->mcs[0];

    //Fill the proper DCI of OAI
    fill_oai_dci(mod_id, CC_id, rnti, dl_dci);
  }
}

void fill_oai_dci(mid_t mod_id, uint32_t CC_id, uint32_t rnti,
		  const Protocol__PrpDlDci *dl_dci) {

  void         *DLSCH_dci        = NULL;
  DCI_PDU      *DCI_pdu;

  unsigned char         round            = 0;
  unsigned char         harq_pid         = 0;
  LTE_DL_FRAME_PARMS   *frame_parms[MAX_NUM_CCs];
  int           size_bits, size_bytes;
  eNB_MAC_INST         *eNB      = &eNB_mac_inst[mod_id];
  UE_list_t            *UE_list  = &eNB->UE_list;
  LTE_eNB_UE_stats *eNB_UE_stats = NULL;

  int UE_id = find_ue(rnti, PHY_vars_eNB_g[mod_id][CC_id]);

  uint32_t format;

  harq_pid = dl_dci->harq_process;
  round = dl_dci->rv[0];
  
  // Note this code is for a specific DCI format
  DLSCH_dci = (void *)UE_list->UE_template[CC_id][UE_id].DLSCH_DCI[harq_pid];
  DCI_pdu = &eNB->common_channels[CC_id].DCI_pdu;
  
  frame_parms[CC_id] = mac_xface->get_lte_frame_parms(mod_id, CC_id);

  if (dl_dci->has_tpc == 1) {
    // Check if tpc has been set and reset measurement */
    if ((dl_dci->tpc == 0) || (dl_dci->tpc == 2)) {
      eNB_UE_stats = mac_xface->get_eNB_UE_stats(mod_id, CC_id, rnti);
      eNB_UE_stats->Po_PUCCH_update = 0;
    }
  }
  
  
  switch (frame_parms[CC_id]->N_RB_DL) {
  case 6:
    if (frame_parms[CC_id]->frame_type == TDD) {
      if (dl_dci->format ==  PROTOCOL__PRP_DCI_FORMAT__PRDCIF_1) {
	FILL_DCI_TDD_1(DCI1_1_5MHz_TDD_t, DLSCH_dci, dl_dci);
	size_bytes = sizeof(DCI1_1_5MHz_TDD_t);
	size_bits  = sizeof_DCI1_1_5MHz_TDD_t;
      } else if (dl_dci->format ==  PROTOCOL__PRP_DCI_FORMAT__PRDCIF_2A) {
	//TODO
      } else if (dl_dci->format ==  PROTOCOL__PRP_DCI_FORMAT__PRDCIF_1D) {
	//TODO
      }
    } else {
      if (dl_dci->format ==  PROTOCOL__PRP_DCI_FORMAT__PRDCIF_1) {
	FILL_DCI_FDD_1(DCI1_1_5MHz_FDD_t, DLSCH_dci, dl_dci);
	size_bytes = sizeof(DCI1_1_5MHz_FDD_t);
	size_bits  = sizeof_DCI1_1_5MHz_FDD_t;
      } else if (dl_dci->format ==  PROTOCOL__PRP_DCI_FORMAT__PRDCIF_2A) {
	//TODO
      } else if (dl_dci->format ==  PROTOCOL__PRP_DCI_FORMAT__PRDCIF_1D) {
	  //TODO
      }
    }
    break;
  case 25:
    if (frame_parms[CC_id]->frame_type == TDD) {
      if (dl_dci->format ==  PROTOCOL__PRP_DCI_FORMAT__PRDCIF_1) {
	FILL_DCI_TDD_1(DCI1_5MHz_TDD_t, DLSCH_dci, dl_dci);
	size_bytes = sizeof(DCI1_5MHz_TDD_t);
	size_bits  = sizeof_DCI1_5MHz_TDD_t;
      } else if (dl_dci->format ==  PROTOCOL__PRP_DCI_FORMAT__PRDCIF_2A) {
	//TODO
      } else if (dl_dci->format ==  PROTOCOL__PRP_DCI_FORMAT__PRDCIF_1D) {
	//TODO
      }
    } else {
      if (dl_dci->format ==  PROTOCOL__PRP_DCI_FORMAT__PRDCIF_1) {
	FILL_DCI_FDD_1(DCI1_5MHz_FDD_t, DLSCH_dci, dl_dci);
	size_bytes = sizeof(DCI1_5MHz_FDD_t);
	size_bits  = sizeof_DCI1_5MHz_FDD_t;
      } else if (dl_dci->format ==  PROTOCOL__PRP_DCI_FORMAT__PRDCIF_2A) {
	//TODO
      } else if (dl_dci->format ==  PROTOCOL__PRP_DCI_FORMAT__PRDCIF_1D) {
	//TODO
      }
    }
    break;
  case 50:
    if (frame_parms[CC_id]->frame_type == TDD) {
      if (dl_dci->format ==  PROTOCOL__PRP_DCI_FORMAT__PRDCIF_1) {
	FILL_DCI_TDD_1(DCI1_10MHz_TDD_t, DLSCH_dci, dl_dci);
	size_bytes = sizeof(DCI1_10MHz_TDD_t);
	size_bits  = sizeof_DCI1_10MHz_TDD_t;
      } else if (dl_dci->format ==  PROTOCOL__PRP_DCI_FORMAT__PRDCIF_2A) {
	//TODO
      } else if (dl_dci->format ==  PROTOCOL__PRP_DCI_FORMAT__PRDCIF_1D) {
	//TODO
      }
    } else {
      if (dl_dci->format ==  PROTOCOL__PRP_DCI_FORMAT__PRDCIF_1) {
	FILL_DCI_FDD_1(DCI1_10MHz_FDD_t, DLSCH_dci, dl_dci);
	size_bytes = sizeof(DCI1_10MHz_FDD_t);
	size_bits  = sizeof_DCI1_10MHz_FDD_t;
      } else if (dl_dci->format ==  PROTOCOL__PRP_DCI_FORMAT__PRDCIF_2A) {
	//TODO
      } else if (dl_dci->format ==  PROTOCOL__PRP_DCI_FORMAT__PRDCIF_1D) {
	//TODO
      }
    }
    break;
  case 100:
    if (frame_parms[CC_id]->frame_type == TDD) {
      if (dl_dci->format ==  PROTOCOL__PRP_DCI_FORMAT__PRDCIF_1) {
	FILL_DCI_TDD_1(DCI1_20MHz_TDD_t, DLSCH_dci, dl_dci);
	size_bytes = sizeof(DCI1_20MHz_TDD_t);
	size_bits  = sizeof_DCI1_20MHz_TDD_t;
      } else if (dl_dci->format ==  PROTOCOL__PRP_DCI_FORMAT__PRDCIF_2A) {
	//TODO
      } else if (dl_dci->format ==  PROTOCOL__PRP_DCI_FORMAT__PRDCIF_1D) {
	//TODO
      }
    } else {
      if (dl_dci->format ==  PROTOCOL__PRP_DCI_FORMAT__PRDCIF_1) {
	FILL_DCI_FDD_1(DCI1_20MHz_FDD_t, DLSCH_dci, dl_dci);
	size_bytes = sizeof(DCI1_20MHz_FDD_t);
	size_bits  = sizeof_DCI1_20MHz_FDD_t;
      } else if (dl_dci->format ==  PROTOCOL__PRP_DCI_FORMAT__PRDCIF_2A) {
	//TODO
      } else if (dl_dci->format ==  PROTOCOL__PRP_DCI_FORMAT__PRDCIF_1D) {
	//TODO
      }
    }
    break;
  }

  //Set format to the proper type
  switch(dl_dci->format) {
  case PROTOCOL__PRP_DCI_FORMAT__PRDCIF_1:
    format = format1;
    break;
  case PROTOCOL__PRP_DCI_FORMAT__PRDCIF_1A:
    format = format1A;
    break;
  case PROTOCOL__PRP_DCI_FORMAT__PRDCIF_1B:
    format = format1B;
    break;
  case PROTOCOL__PRP_DCI_FORMAT__PRDCIF_1C:
    format = format1C;
    break;
  case PROTOCOL__PRP_DCI_FORMAT__PRDCIF_1D:
    format = format1E_2A_M10PRB;
    break;
  case PROTOCOL__PRP_DCI_FORMAT__PRDCIF_2:
    format  = format2;
    break;
  case PROTOCOL__PRP_DCI_FORMAT__PRDCIF_2A:
    format = format2A;
    break;
  case PROTOCOL__PRP_DCI_FORMAT__PRDCIF_2B:
    format = format2B;
    break;
  case PROTOCOL__PRP_DCI_FORMAT__PRDCIF_3:
    format = 3;
    break;
  default:
    /*TODO: Need to deal with unsupported DCI type*/
    return;
  }
  
  add_ue_spec_dci(DCI_pdu,
		  DLSCH_dci,
		  rnti,
		  size_bytes,
		  dl_dci->aggr_level,
		  size_bits,
		  format,
		  0);
}