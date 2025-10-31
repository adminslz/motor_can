#include "global_vars.h"

// 定义全局CAN收发对象
//CANTxRx *g_canTxRx = nullptr;
uint8_t Can_id = 0;

// 定义全局示波器采集状态变量（0:未采集，1:采集中）
int g_oscilloscopeAcquiring = 0;