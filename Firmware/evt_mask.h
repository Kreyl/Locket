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

#define EVTMSK_UART_NEW_CMD     EVENT_MASK(1)
#define EVTMSK_BUTTONS          EVENT_MASK(2)
#define EVTMSK_SAVE             EVENT_MASK(4)

#define EVTMSK_RADIO_RX         EVENT_MASK(21)
#define EVTMSK_RADIO_ON_TIMEOUT EVENT_MASK(22)


#endif /* EVT_MASK_H_ */
