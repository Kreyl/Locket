/*
 * radio_lvl1.h
 *
 *  Created on: Nov 17, 2013
 *      Author: kreyl
 */

#ifndef RADIO_LVL1_H_
#define RADIO_LVL1_H_

#include <kl_lib.h>
#include "ch.h"
#include "rlvl1_defins.h"
#include "cc1101.h"
#include "kl_buf.h"

class rLevel1_t {
private:
    rPkt_t Pkt;
public:
    int8_t Sensitivity=-111;
    uint8_t Init();
    CountingBuf_t<uint8_t, RXTABLE_SZ> RxTable[3];
    // Inner use
    void ITask();
};

extern rLevel1_t Radio;

#endif /* RADIO_LVL1_H_ */
