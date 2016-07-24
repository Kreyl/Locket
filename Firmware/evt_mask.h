/*
 * evt_mask.h
 *
 *  Created on: Apr 12, 2013
 *      Author: g.kruglov
 */

#ifndef EVT_MASK_H_
#define EVT_MASK_H_

// Event masks
#define EVT_NOTHING             0

#define EVT_UART_NEW_CMD        EVENT_MASK(1)
#define EVT_SECOND              EVENT_MASK(2)
#define EVT_TXOFF               EVENT_MASK(3)
#define EVT_SOMEONE_NEAR        EVENT_MASK(4)
#define EVT_BUTTONS             EVENT_MASK(5)


#define EVT_OFF                 EVENT_MASK(31)


#endif /* EVT_MASK_H_ */
