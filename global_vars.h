#ifndef GLOBAL_VARS_H
#define GLOBAL_VARS_H

#include "can_rx_tx.h"

// 声明全局CAN收发对象
extern CANTxRx *g_canTxRx;
extern uint8_t Can_id;
#endif // GLOBAL_VARS_H
