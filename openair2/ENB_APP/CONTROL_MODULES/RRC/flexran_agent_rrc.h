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

  Address      : Eurecom, Compus SophiaTech 450, route des chappes, 06451 Biot, France.

 *******************************************************************************/

/*! \file flexran_agent_rrc.h
 * \brief FlexRAN agent message handler APIs for MAC layer
 * \author Navid Nikaein
 * \date 2016
 * \version 0.1
 */

#ifndef FLEXRAN_AGENT_RRC_H_
#define FLEXRAN_AGENT_RRC_H_

#include "header.pb-c.h"
#include "flexran.pb-c.h"
#include "stats_messages.pb-c.h"
#include "stats_common.pb-c.h"

#include "flexran_agent_common.h"
#include "flexran_agent_extern.h"


/* Initialization function for the agent structures etc */
void flexran_agent_init_rrc_agent(mid_t mod_id);


/**********************************
 * FlexRAN agent - technology RRC API
 **********************************/

/// Send to the controller all the mac stat updates that occured during this subframe
/// based on the stats request configuration
void flexran_agent_send_update_rrc_stats(mid_t mod_id);

/// Provide to the scheduler a pending dl_mac_config message
//void flexran_agent_get_pending_dl_rrc_config(mid_t mod_id, Protocol__FlexranMessage **msg);

/*Register technology specific interface callbacks*/
int flexran_agent_register_rrc_xface(mid_t mod_id, AGENT_MAC_xface *xface);

/*Unregister technology specific callbacks*/
int flexran_agent_unregister_rrc_xface(mid_t mod_id, AGENT_MAC_xface*xface);

#endif
