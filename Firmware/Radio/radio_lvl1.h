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
    bool MayTx;
public:
    uint8_t Init();
    uint8_t TxPower = CC_Pwr0dBm;
    // Inner use
    void ITask();
    void StopTx() { MayTx = false; }
    rLevel1_t(): Pkt({0}), MayTx(true) {}
};

extern rLevel1_t Radio;

#endif /* RADIO_LVL1_H_ */
