#ifndef GLOBAL_VARS_H
#define GLOBAL_VARS_H

#include "can_rx_tx.h"

// 声明全局CAN收发对象
extern CANTxRx *g_canTxRx;
extern uint8_t Can_id;

// 声明全局示波器采集状态变量（0:未采集，1:采集中）
extern int g_oscilloscopeAcquiring;

#endif // GLOBAL_VARS_H
