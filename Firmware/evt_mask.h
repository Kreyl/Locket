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
#define EVTMSK_SECOND           EVENT_MASK(2)
#define EVTMSK_CLR_SWITCH       EVENT_MASK(3)
#define EVTMSK_VIBRO            EVENT_MASK(4)
#define EVTMSK_BTN              EVENT_MASK(5)

#define EVTMSK_OFF              EVENT_MASK(31)


#endif /* EVT_MASK_H_ */
