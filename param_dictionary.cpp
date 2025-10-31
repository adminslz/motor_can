#include "param_dictionary.h"
#include <QDebug>

ParamDictionary::ParamDictionary(QObject *parent)
    : QObject(parent)
{
    initializeDictionary();
}

void ParamDictionary::initializeDictionary()
{
    m_dictionary.clear();

    // =============== 基本控制参数 (分类0) ===============
    m_dictionary.append({0x6040, 0x00, OD_TYPE_UINT16, "控制字", "", 0, 0xFFFF, 0, true, true, 0, "系统控制"});
    m_dictionary.append({0x6042, 0x00, OD_TYPE_INT16, "操作模式", "", 0, 3, 0, true, true, 0, "系统控制"});

    // 位置控制
    m_dictionary.append({0x6062, 0x00, OD_TYPE_FLOAT, "目标位置", "rad", -12.57f, 12.57f, 0.0f, false, true, 0, "位置控制"});

    // 速度控制
    m_dictionary.append({0x606B, 0x00, OD_TYPE_FLOAT, "目标速度", "rad/s", -15.0f, 15.0f, 0.0f, false, true, 0, "速度控制"});

    // 电流控制
    m_dictionary.append({0x6071, 0x00, OD_TYPE_FLOAT, "目标q轴电流", "A", -30.0f, 30.0f, 0.0f, false, true, 0, "电流控制"});
    m_dictionary.append({0x6073, 0x00, OD_TYPE_FLOAT, "目标d轴电流", "A", -30.0f, 30.0f, 0.0f, false, true, 0, "电流控制"});

    // 系统参数
    m_dictionary.append({0x60D2, 0x00, OD_TYPE_UINT8, "CAN节点ID", "", 0, 127, 1, true, true, 0, "系统参数"});
    m_dictionary.append({0x60D4, 0x00, OD_TYPE_UINT16, "电机使能", "", 0, 1, 0, false, true, 0, "系统参数"});

    // 规划器参数
    m_dictionary.append({0x60F1, 0x00, OD_TYPE_FLOAT, "最大速度", "rad/s", 0.0f, 15.0f, 15.0f, true, true, 0, "运动规划"});
    m_dictionary.append({0x60F2, 0x00, OD_TYPE_FLOAT, "最大加速度", "rad/s²", 0.0f, 1000.0f, 100.0f, true, true, 0, "运动规划"});
    m_dictionary.append({0x60F3, 0x00, OD_TYPE_FLOAT, "减速距离", "rad", 0.0f, 12.57f, 1.0f, true, true, 0, "运动规划"});

    // =============== PID参数 (分类1) ===============
    // 位置PID
    m_dictionary.append({0x6065, 0x00, OD_TYPE_FLOAT, "位置比例增益", "", 0.0f, 1000.0f, 50.0f, true, true, 1, "位置PID"});
    m_dictionary.append({0x6066, 0x00, OD_TYPE_FLOAT, "位置积分增益", "", 0.0f, 1000.0f, 0.5f, true, true, 1, "位置PID"});
    m_dictionary.append({0x6067, 0x00, OD_TYPE_FLOAT, "位置积分限制", "", 0.0f, 1000.0f, 100.0f, true, true, 1, "位置PID"});
    m_dictionary.append({0x6068, 0x00, OD_TYPE_FLOAT, "位置输出限制", "", 0.0f, 1000.0f, 100.0f, true, true, 1, "位置PID"});

    // 速度PID
    m_dictionary.append({0x606D, 0x00, OD_TYPE_FLOAT, "速度比例增益", "", 0.0f, 1000.0f, 10.0f, true, true, 1, "速度PID"});
    m_dictionary.append({0x606E, 0x00, OD_TYPE_FLOAT, "速度积分增益", "", 0.0f, 1000.0f, 1.0f, true, true, 1, "速度PID"});
    m_dictionary.append({0x606F, 0x00, OD_TYPE_FLOAT, "速度积分限制", "", 0.0f, 30.0f, 1.0f, true, true, 1, "速度PID"});
    m_dictionary.append({0x6070, 0x00, OD_TYPE_FLOAT, "速度输出限制", "", 0.0f, 30.0f, 10.0f, true, true, 1, "速度PID"});

    // 电流PID
    m_dictionary.append({0x6073, 0x00, OD_TYPE_FLOAT, "Q轴比例增益", "V/A", 0.0f, 100.0f, 0.01f, true, true, 1, "电流PID"});
    m_dictionary.append({0x6074, 0x00, OD_TYPE_FLOAT, "Q轴积分增益", "V/As", 0.0f, 1000.0f, 0.1f, true, true, 1, "电流PID"});
    m_dictionary.append({0x6075, 0x00, OD_TYPE_FLOAT, "D轴比例增益", "V/A", 0.0f, 100.0f, 0.01f, true, true, 1, "电流PID"});
    m_dictionary.append({0x6076, 0x00, OD_TYPE_FLOAT, "D轴积分增益", "V/A", 0.0f, 1000.0f, 0.1f, true, true, 1, "电流PID"});

    // =============== 高级设置 (分类2) ===============
    // MIT控制
    m_dictionary.append({0x6081, 0x00, OD_TYPE_FLOAT, "MIT比例增益", "", 0.0f, 1000.0f, 10.0f, true, true, 2, "MIT控制"});
    m_dictionary.append({0x6082, 0x00, OD_TYPE_FLOAT, "MIT微分增益", "", 0.0f, 1000.0f, 1.0f, true, true, 2, "MIT控制"});
    m_dictionary.append({0x6083, 0x00, OD_TYPE_FLOAT, "MIT输出限制", "", 0.0f, 1000.0f, 100.0f, true, true, 2, "MIT控制"});

    // 滤波器
    m_dictionary.append({0x6091, 0x00, OD_TYPE_FLOAT, "速度滤波器", "", 0.0f, 1.0f, 0.1f, true, true, 2, "滤波器"});
    m_dictionary.append({0x6092, 0x00, OD_TYPE_FLOAT, "角度滤波器", "", 0.0f, 1.0f, 0.1f, true, true, 2, "滤波器"});
    m_dictionary.append({0x6093, 0x00, OD_TYPE_FLOAT, "Q轴电流滤波系数", "", 0.0f, 1.0f, 0.1f, true, true, 2, "滤波器"});
    m_dictionary.append({0x6094, 0x00, OD_TYPE_FLOAT, "D轴电流滤波系数", "", 0.0f, 1.0f, 0.1f, true, true, 2, "滤波器"});
    m_dictionary.append({0x6095, 0x00, OD_TYPE_FLOAT, "B相电流滤波系数", "", 0.0f, 1.0f, 0.1f, true, true, 2, "滤波器"});
    m_dictionary.append({0x6096, 0x00, OD_TYPE_FLOAT, "C相电流滤波系数", "", 0.0f, 1.0f, 0.1f, true, true, 2, "滤波器"});

    // 编码器
    m_dictionary.append({0x60B1, 0x00, OD_TYPE_INT16, "编码器线数", "", 1, 65535, 4096, true, true, 2, "编码器"});
    m_dictionary.append({0x60B2, 0x00, OD_TYPE_INT16, "编码器方向", "", -1, 1, 1, true, true, 2, "编码器"});
    m_dictionary.append({0x60B3, 0x00, OD_TYPE_FLOAT, "角度偏移", "rad", -360.0f, 360.0f, 220.0f, true, true, 2, "编码器"});

    // 电机参数
    m_dictionary.append({0x60D1, 0x00, OD_TYPE_INT16, "电机极对数", "", 1, 100, 7, true, true, 2, "电机参数"});

    // 观测器
    m_dictionary.append({0x6121, 0x00, OD_TYPE_FLOAT, "滑模观测器增益", "", 0.0f, 1000.0f, 10.0f, true, true, 2, "状态观测器"});
    m_dictionary.append({0x6122, 0x00, OD_TYPE_FLOAT, "电机电阻", "Ohm", 0.0f, 10.0f, 0.1f, true, true, 2, "状态观测器"});
    m_dictionary.append({0x6123, 0x00, OD_TYPE_FLOAT, "电机电感", "H", 0.0f, 0.01f, 0.001f, true, true, 2, "状态观测器"});

    // =============== 监控参数 (分类3) ===============
    m_dictionary.append({0x1001, 0x00, OD_TYPE_UINT8, "错误寄存器", "", 0, 0, 0, true, false, 3, "系统状态"});
    m_dictionary.append({0x6041, 0x00, OD_TYPE_UINT16, "状态字", "", 0, 0xFFFF, 0, true, false, 3, "系统状态"});
    m_dictionary.append({0x6060, 0x00, OD_TYPE_INT8, "当前模式显示", "", 0, 3, 0, true, false, 3, "系统状态"});

    m_dictionary.append({0x6064, 0x00, OD_TYPE_FLOAT, "实际位置", "rad", -12.57f, 12.57f, 0.0f, true, false, 3, "实时数据"});
    m_dictionary.append({0x606C, 0x00, OD_TYPE_FLOAT, "实际速度", "rad/s", -15.0f, 15.0f, 0.0f, true, false, 3, "实时数据"});
    m_dictionary.append({0x6072, 0x00, OD_TYPE_FLOAT, "实际q轴电流", "A", -30.0f, 30.0f, 0.0f, true, false, 3, "实时数据"});
    m_dictionary.append({0x6074, 0x00, OD_TYPE_FLOAT, "实际d轴电流", "A", -30.0f, 30.0f, 0.0f, true, false, 3, "实时数据"});

    m_dictionary.append({0x60B4, 0x00, OD_TYPE_FLOAT, "机械角度", "rad", -6.28f, 6.28f, 0.0f, true, false, 3, "编码器数据"});
    m_dictionary.append({0x60B5, 0x00, OD_TYPE_FLOAT, "电气角度", "rad", -6.28f, 6.28f, 0.0f, true, false, 3, "编码器数据"});

    m_dictionary.append({0x60D3, 0x00, OD_TYPE_UINT8, "电机类型", "", 0, 1, 0, true, false, 3, "电机信息"});

    m_dictionary.append({0x6101, 0x00, OD_TYPE_FLOAT, "学习电阻", "Ohm", 0.0f, 10.0f, 0.0f, true, false, 3, "参数辨识"});
    m_dictionary.append({0x6102, 0x00, OD_TYPE_FLOAT, "学习电感", "H", 0.0f, 0.01f, 0.0f, true, false, 3, "参数辨识"});
    m_dictionary.append({0x6103, 0x00, OD_TYPE_FLOAT, "学习磁链", "Wb", 0.0f, 1.0f, 0.0f, true, false, 3, "参数辨识"});

    // =============== 新增参数 (分类4) ===============
    // 系统状态监控
    m_dictionary.append({0x6141, 0x00, OD_TYPE_UINT16, "使能状态", "", 0, 3, 0, true, false, 4, "系统状态"});
    m_dictionary.append({0x6142, 0x00, OD_TYPE_UINT16, "错误代码", "", 0, 0xFFFF, 0, true, false, 4, "系统状态"});
    m_dictionary.append({0x6143, 0x00, OD_TYPE_UINT8, "校准状态", "", 0, 1, 0, true, false, 4, "系统状态"});
    m_dictionary.append({0x6144, 0x00, OD_TYPE_UINT8, "运行状态", "", 0, 1, 0, true, false, 4, "系统状态"});

    // 总线参数监控
    m_dictionary.append({0x6161, 0x00, OD_TYPE_FLOAT, "总线电压", "V", 0.0f, 50.0f, 0.0f, true, false, 4, "电源监控"});
    m_dictionary.append({0x6162, 0x00, OD_TYPE_FLOAT, "总线电流", "A", 0.0f, 30.0f, 0.0f, true, false, 4, "电源监控"});
    m_dictionary.append({0x6163, 0x00, OD_TYPE_FLOAT, "最大允许电流", "A", 0.0f, 30.0f, 10.0f, true, true, 4, "电源监控"});

    // 相电流监控
    m_dictionary.append({0x6181, 0x00, OD_TYPE_FLOAT, "A相电流", "A", -30.0f, 30.0f, 0.0f, true, false, 4, "相电流"});
    m_dictionary.append({0x6182, 0x00, OD_TYPE_FLOAT, "B相电流", "A", -30.0f, 30.0f, 0.0f, true, false, 4, "相电流"});
    m_dictionary.append({0x6183, 0x00, OD_TYPE_FLOAT, "C相电流", "A", -30.0f, 30.0f, 0.0f, true, false, 4, "相电流"});

    // PWM输出监控
    m_dictionary.append({0x61A1, 0x00, OD_TYPE_FLOAT, "PWM占空比A", "", -1.0f, 1.0f, 0.0f, true, false, 4, "PWM输出"});
    m_dictionary.append({0x61A2, 0x00, OD_TYPE_FLOAT, "PWM占空比B", "", -1.0f, 1.0f, 0.0f, true, false, 4, "PWM输出"});
    m_dictionary.append({0x61A3, 0x00, OD_TYPE_FLOAT, "PWM占空比C", "", -1.0f, 1.0f, 0.0f, true, false, 4, "PWM输出"});

    // 测试参数
    m_dictionary.append({0x61C1, 0x00, OD_TYPE_FLOAT, "测试编码器1角度", "deg", 0.0f, 360.0f, 0.0f, true, false, 4, "测试数据"});
    m_dictionary.append({0x61C2, 0x00, OD_TYPE_FLOAT, "测试编码器2角度", "deg", 0.0f, 360.0f, 0.0f, true, false, 4, "测试数据"});
    m_dictionary.append({0x61C3, 0x00, OD_TYPE_FLOAT, "测试角度误差", "deg", -180.0f, 180.0f, 0.0f, true, false, 4, "测试数据"});

    // =============== 运动规划参数 (分类5) ===============
    m_dictionary.append({0x6201, 0x00, OD_TYPE_FLOAT, "S曲线最大加加速度", "rad/s³", 0.0f, 10000.0f, 1000.0f, true, true, 5, "S曲线规划"});
    m_dictionary.append({0x6202, 0x00, OD_TYPE_FLOAT, "S曲线当前位置", "rad", -12.57f, 12.57f, 0.0f, true, false, 5, "S曲线规划"});
    m_dictionary.append({0x6203, 0x00, OD_TYPE_FLOAT, "S曲线当前速度", "rad/s", -15.0f, 15.0f, 0.0f, true, false, 5, "S曲线规划"});
    m_dictionary.append({0x6204, 0x00, OD_TYPE_FLOAT, "S曲线当前位置误差", "rad", -12.57f, 12.57f, 0.0f, true, false, 5, "S曲线规划"});

    // 角度预测器
    m_dictionary.append({0x6211, 0x00, OD_TYPE_FLOAT, "预测角度", "rad", -6.28f, 6.28f, 0.0f, true, false, 5, "角度预测"});
    m_dictionary.append({0x6212, 0x00, OD_TYPE_FLOAT, "预测角速度", "rad/s", -15.0f, 15.0f, 0.0f, true, false, 5, "角度预测"});
    m_dictionary.append({0x6213, 0x00, OD_TYPE_FLOAT, "滤波后角速度", "rad/s", -15.0f, 15.0f, 0.0f, true, false, 5, "角度预测"});

    // =============== 谐波补偿参数 (分类6) ===============
    m_dictionary.append({0x6301, 0x00, OD_TYPE_FLOAT, "谐波补偿A1", "deg", -10.0f, 10.0f, 0.0f, true, true, 6, "谐波补偿"});
    m_dictionary.append({0x6302, 0x00, OD_TYPE_FLOAT, "谐波补偿B1", "deg", -10.0f, 10.0f, 0.0f, true, true, 6, "谐波补偿"});
    m_dictionary.append({0x6303, 0x00, OD_TYPE_FLOAT, "速度前馈系数", "deg/(rev/s)", -10.0f, 10.0f, 0.0f, true, true, 6, "谐波补偿"});

    // =============== 调试参数 (分类7) ===============
    m_dictionary.append({0x6401, 0x00, OD_TYPE_FLOAT, "观测器Alpha电流", "A", -30.0f, 30.0f, 0.0f, true, false, 7, "观测器调试"});
    m_dictionary.append({0x6402, 0x00, OD_TYPE_FLOAT, "观测器Beta电流", "A", -30.0f, 30.0f, 0.0f, true, false, 7, "观测器调试"});
    m_dictionary.append({0x6403, 0x00, OD_TYPE_FLOAT, "滑模观测器Zalpha", "", -100.0f, 100.0f, 0.0f, true, false, 7, "观测器调试"});
    m_dictionary.append({0x6404, 0x00, OD_TYPE_FLOAT, "滑模观测器Zbeta", "", -100.0f, 100.0f, 0.0f, true, false, 7, "观测器调试"});
    m_dictionary.append({0x6405, 0x00, OD_TYPE_FLOAT, "观测器角度", "rad", -6.28f, 6.28f, 0.0f, true, false, 7, "观测器调试"});

    // 电流控制调试
    m_dictionary.append({0x6411, 0x00, OD_TYPE_FLOAT, "D轴电压", "V", -50.0f, 50.0f, 0.0f, true, false, 7, "电流控制"});
    m_dictionary.append({0x6412, 0x00, OD_TYPE_FLOAT, "Q轴电压", "V", -50.0f, 50.0f, 0.0f, true, false, 7, "电流控制"});
    m_dictionary.append({0x6413, 0x00, OD_TYPE_FLOAT, "Alpha轴电压", "V", -50.0f, 50.0f, 0.0f, true, false, 7, "电流控制"});
    m_dictionary.append({0x6414, 0x00, OD_TYPE_FLOAT, "Beta轴电压", "V", -50.0f, 50.0f, 0.0f, true, false, 7, "电流控制"});

    // =============== 追加：与下位机一致的新增条目（仅新增，不修改已有） ===============
    // 设备类型
    m_dictionary.append({0x1000, 0x00, OD_TYPE_UINT32, "Device_Type", "", 0, 0, 0, true, false, 4, "系统状态"});

    // 参数保存/恢复控制位
    m_dictionary.append({0x6145, 0x00, OD_TYPE_UINT32, "Param_Save", "", 0, 1, 0, true, true, 4, "参数控制"});
    m_dictionary.append({0x6146, 0x00, OD_TYPE_UINT32, "Param_Restore", "", 0, 1, 0, true, true, 4, "参数控制"});
}

QVector<ODEntry> ParamDictionary::getParametersByCategory(int category) const
{
    QVector<ODEntry> result;
    for (const auto& entry : m_dictionary) {
        if (entry.tabCategory == category) {
            result.append(entry);
        }
    }
    return result;
}

QVector<ODEntry> ParamDictionary::getAllParameters() const
{
    return m_dictionary;
}

ODEntry ParamDictionary::getParameter(uint16_t index, uint8_t subindex) const
{
    for (const auto& entry : m_dictionary) {
        if (entry.index == index && entry.subindex == subindex) {
            return entry;
        }
    }
    return ODEntry();
}
