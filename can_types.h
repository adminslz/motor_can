#ifndef CAN_TYPES_H
#define CAN_TYPES_H

#include <QMetaType>
#include "ControlCAN.h"  // 包含原始定义

// 不再重新定义 VCI_CAN_OBJ，直接使用 ControlCAN.h 中的定义

// 注册函数声明
void registerCanTypes();

#endif // CAN_TYPES_H
