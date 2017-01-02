/******************************************************************************
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

/*! \file flexran_agent_timer.h
 * \brief common message primitves and utilities 
 * \author Xenofon Foukas and Navid Nikaein
 * \date 2016
 * \version 0.1
 */

#include <stdio.h>
#include <time.h>

#include "flexran_agent_common.h"
#include "flexran_agent_common_internal.h"
#include "flexran_agent_extern.h"
#include "flexran_agent_defs.h"


# include "tree.h"
# include "intertask_interface.h"
# include "timer.h"



/*******************
 * timer primitves
 *******************/

#define TIMER_NULL                 -1 
#define TIMER_TYPE_INVALIDE        -2
#define	TIMER_SETUP_FAILED         -3
#define	TIMER_REMOVED_FAILED       -4
#define	TIMER_ELEMENT_NOT_FOUND    -5


/* Type of the callback executed when the timer expired */
typedef Protocol__FlexranMessage *(*flexran_agent_timer_callback_t)(void*);


typedef struct flexran_agent_timer_args_s{
  mid_t            mod_id;
  Protocol__FlexranMessage *msg;
} flexran_agent_timer_args_t;



typedef struct flexran_agent_timer_element_s{
  RB_ENTRY(flexran_agent_timer_element_s) entry;

  agent_id_t             agent_id;
  instance_t       instance;
  
  flexran_agent_timer_type_t  type;
  flexran_agent_timer_state_t state;

  uint32_t interval_sec;
  uint32_t interval_usec;

  long timer_id;  /* Timer id returned by the timer API*/
  xid_t xid; /*The id of the task as received by the controller
	       message*/
  
  flexran_agent_timer_callback_t cb;
  flexran_agent_timer_args_t *timer_args;
  
} flexran_agent_timer_element_t;

typedef struct flexran_agent_timer_instance_s{
  RB_HEAD(flexran_agent_map, flexran_agent_timer_element_s) flexran_agent_head;
}flexran_agent_timer_instance_t;


err_code_t flexran_agent_init_timer(void);

/* Create a timer for some agent related event with id xid. Will store the id 
   of the generated timer in timer_id */
err_code_t flexran_agent_create_timer(uint32_t interval_sec,
				  uint32_t interval_usec,
				  agent_id_t     agent_id,
				  instance_t     instance,
				  uint32_t timer_type,
				  xid_t xid,
				  flexran_agent_timer_callback_t cb,
				  void*    timer_args,
				  long *timer_id);

/* Destroy all existing timers */
err_code_t flexran_agent_destroy_timers(void);

/* Destroy the timer with the given timer_id */
err_code_t flexran_agent_destroy_timer(long timer_id);

/* Destroy the timer for task with id xid */
err_code_t flexran_agent_destroy_timer_by_task_id(xid_t xid);

/* Stop a timer */
err_code_t flexran_agent_stop_timer(long timer_id);

/* Restart the given timer */
err_code_t flexran_agent_restart_timer(long *timer_id);

/* Find the timer with the given timer_id */
struct flexran_agent_timer_element_s * get_timer_entry(long timer_id);

/* Obtain the protocol message stored in the given expired timer */
Protocol__FlexranMessage * flexran_agent_process_timeout(long timer_id, void* timer_args);

/* Comparator function comparing two timers. Decides the ordering of the timers */
int flexran_agent_compare_timer(struct flexran_agent_timer_element_s *a, struct flexran_agent_timer_element_s *b);

/*Specify a delay in nanoseconds to timespec and sleep until then*/
void flexran_agent_sleep_until(struct timespec *ts, int delay);

/* RB_PROTOTYPE is for .h files */
RB_PROTOTYPE(flexran_agent_map, flexran_agent_timer_element_s, entry, flexran_agent_compare_timer);
