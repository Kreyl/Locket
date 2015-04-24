/*
 * evt_mask.h
 *
 *  Created on: Apr 12, 2013
 *      Author: g.kruglov
 */

#ifndef EVT_MASK_H_
#define EVT_MASK_H_

// Event masks
#define EVTMSK_NO_MASK          0

#define EVTMSK_BUTTONS          EVENT_MASK(1)
#define EVTMSK_MSNS_ON          EVENT_MASK(2)
#define EVTMSK_MSNS_OFF         EVENT_MASK(3)
#define EVTMSK_SAVE             EVENT_MASK(4)

#define EVTMSK_MSNS_AFTEROFF_TIMEOUT EVENT_MASK(5)
#define EVTMSK_DEADTIME_END     EVENT_MASK(6)

#define EVTMSK_BCKLT_OFF        EVENT_MASK(11)

#define EVTMSK_RADIO_RX         EVENT_MASK(21)
#define EVTMSK_RADIO_ON_TIMEOUT EVENT_MASK(22)

//#define EVTMSK_UART_RX_POLL     EVENT_MASK(11)



#endif /* EVT_MASK_H_ */
