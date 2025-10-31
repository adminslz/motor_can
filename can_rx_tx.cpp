 
#include "can_rx_tx.h"
#include <QDebug>
#include <cstring>
#include <cmath>
#include "global_vars.h"
#include <QThread>
#include "data_acquisition.h"
#include "control_param.h"
#include <QElapsedTimer>
#include <QDateTime>
#include "can_types.h"
#include "canthread.h"

// 定义全局CAN收发对象
CANTxRx *g_canTxRx = nullptr;

// 静态类型注册
namespace {
    void registerTypesOnce() {
        static bool registered = false;
        if (!registered) {
            registerCanTypes();  // 调用注册函数
            registered = true;
        }
    }
}

// CAN接收线程实现
CANReceiver::CANReceiver(QObject *parent)
    : QThread(parent)
    , m_running(false)
    , m_highSpeedMode(false)
{
}

CANReceiver::~CANReceiver()
{
    stop();
}

void CANReceiver::stop()
{
    m_running = false;
    m_queueCondition.wakeAll();
    if (!wait(1000)) {
        terminate();
        wait();
    }
}

void CANReceiver::setHighSpeedMode(bool enabled)
{
    m_highSpeedMode = enabled;
}

int CANReceiver::getQueueSize() const
{
    QMutexLocker locker(&m_queueMutex);
    return m_frameQueue.size();
}

void CANReceiver::pushFrames(const QList<VCI_CAN_OBJ> &frames)
{
    if (frames.isEmpty()) return;

    QMutexLocker locker(&m_queueMutex);

    if (m_frameQueue.size() >= MAX_QUEUE_SIZE) {
        if (!m_frameQueue.isEmpty()) {
            m_frameQueue.dequeue();
        }
        emit queueOverflow();
        return;
    }

    m_frameQueue.enqueue(frames);
    m_queueCondition.wakeOne();
}

void CANReceiver::run()
{
    m_running = true;

    const int BATCH_SIZE = m_highSpeedMode ? 50 : 10;
    const int BATCH_TIMEOUT_MS = m_highSpeedMode ? 10 : 50;

    QList<VCI_CAN_OBJ> currentBatch;
    QVector<QPair<DWORD, QVector<float>>> statusBatch;
    QElapsedTimer batchTimer;
    batchTimer.start();

    while (m_running) {
        QList<VCI_CAN_OBJ> frames;
        {
            QMutexLocker locker(&m_queueMutex);
            if (m_frameQueue.isEmpty()) {
                m_queueCondition.wait(&m_queueMutex, BATCH_TIMEOUT_MS);
            }

            if (!m_frameQueue.isEmpty()) {
                frames = m_frameQueue.dequeue();
            }
        }

        for (const auto &frame : frames) {
            currentBatch.append(frame);

            if (frame.ID < 0x80 && frame.DataLen == 8) {
                int16_t speed_int, current_int;
                int32_t position_int;

                memcpy(&speed_int, &frame.Data[0], 2);
                memcpy(&position_int, &frame.Data[2], 4);
                memcpy(&current_int, &frame.Data[6], 2);

                float speed = speed_int / 10.0f;
                float position = position_int / 1000.0f;
                float current = current_int / 1000.0f;

                if (!std::isnan(speed) && !std::isinf(speed) &&
                    !std::isnan(position) && !std::isinf(position) &&
                    !std::isnan(current) && !std::isinf(current)) {

                    QVector<float> statusData = {speed, position, current};
                    statusBatch.append(qMakePair(frame.ID, statusData));
                }
            }

            if (currentBatch.size() >= BATCH_SIZE ||
                batchTimer.elapsed() >= BATCH_TIMEOUT_MS) {

                if (!currentBatch.isEmpty()) {
                    emit framesProcessed(currentBatch);
                    currentBatch.clear();
                }

                if (!statusBatch.isEmpty()) {
                    emit statusDataBatchReceived(statusBatch);
                    statusBatch.clear();
                }

                batchTimer.restart();
            }
        }
    }

    if (!currentBatch.isEmpty()) {
        emit framesProcessed(currentBatch);
    }
    if (!statusBatch.isEmpty()) {
        emit statusDataBatchReceived(statusBatch);
    }
}

// CANTxRx 构造函数
CANTxRx::CANTxRx(QObject *parent)
    : QObject(parent)
    , m_dataAcquisition(nullptr)
    , m_receiver(new CANReceiver(this))
    , m_canThread(nullptr)
    , m_deviceType(4)
    , m_deviceIndex(0)
    , m_canIndex(0)
    , m_isReady(false)
    , m_isReceiving(false)
    , m_highSpeedMode(false)
    , m_framesReceived(0)
    , m_framesSent(0)
    , m_parseErrors(0)
    , m_queueOverflows(0)
    , m_lastUpdateTime(0)
    , m_receiveCount(0)
    , m_lastLogTime(0)
    , m_currentFPS(0)
{
    // 确保类型注册
    registerTypesOnce();

    m_receiveTimer = new QTimer(this);
    m_receiveTimer->setTimerType(Qt::PreciseTimer);

    // 连接信号槽 - 使用队列连接
    connect(m_receiveTimer, &QTimer::timeout, this, &CANTxRx::onReceiveTimeout, Qt::QueuedConnection);

    // 检查类型是否已注册
    bool typesOk = true;
    if (QMetaType::type("QList<VCI_CAN_OBJ>") == QMetaType::UnknownType) {
        qWarning() << "QList<VCI_CAN_OBJ> not registered!";
        typesOk = false;
    }
    if (QMetaType::type("QVector<QPair<DWORD,QVector<float>>>") == QMetaType::UnknownType) {
        qWarning() << "QVector<QPair<DWORD,QVector<float>>> not registered!";
        typesOk = false;
    }

    if (typesOk) {
        // 类型注册成功，使用队列连接
        connect(m_receiver, &CANReceiver::framesProcessed,
                this, &CANTxRx::onFramesProcessed, Qt::QueuedConnection);
        connect(m_receiver, &CANReceiver::statusDataBatchReceived,
                this, &CANTxRx::onStatusBatchReceived, Qt::QueuedConnection);
        connect(m_receiver, &CANReceiver::queueOverflow,
                this, &CANTxRx::onQueueOverflow, Qt::QueuedConnection);
        qDebug() << "Using queued connections for CAN receiver";
    } else {
        // 类型注册失败，使用直接连接（注意线程安全）
        qWarning() << "Using direct connections due to type registration failure";
        connect(m_receiver, &CANReceiver::framesProcessed,
                this, &CANTxRx::onFramesProcessed, Qt::DirectConnection);
        connect(m_receiver, &CANReceiver::statusDataBatchReceived,
                this, &CANTxRx::onStatusBatchReceived, Qt::DirectConnection);
        connect(m_receiver, &CANReceiver::queueOverflow,
                this, &CANTxRx::onQueueOverflow, Qt::DirectConnection);
    }

    m_performanceTimer.start();
}

CANTxRx::~CANTxRx()
{
    stopReceiving();
    m_receiver->stop();
}

void CANTxRx::onFramesProcessed(const QList<VCI_CAN_OBJ> &frames)
{
    for (const auto &frame : frames) {
        parseAndLogCANFrame(frame);
        emit frameReceived(frame);
    }

    m_framesReceived += frames.size();
    updatePerformanceStats();
}

void CANTxRx::onStatusBatchReceived(const QVector<QPair<DWORD, QVector<float>>> &batchData)
{
    for (const auto &item : batchData) {
        DWORD canId = item.first;
        const QVector<float> &data = item.second;
        if (data.size() >= 3) {
            emit statusDataReceived(canId, data[0], data[1], data[2]);
            emit motorStatusReceived(data[0], data[1], data[2]);
        }
    }

    static int batchLogCounter = 0;
    if (!m_highSpeedMode && ++batchLogCounter % 50 == 0) {
        qDebug() << "批量处理状态数据:" << batchData.size() << "条";
        batchLogCounter = 0;
    }
}

void CANTxRx::onQueueOverflow()
{
    m_queueOverflows++;
    emit queueOverflowDetected();

    if (!m_highSpeedMode) {
        qWarning() << "CAN接收队列溢出，数据可能丢失！";
    }
}

void CANTxRx::setCANParams(DWORD deviceType, DWORD deviceIndex, DWORD canIndex)
{

    QMutexLocker locker(&m_mutex);

    m_deviceType = deviceType;
    m_deviceIndex = deviceIndex;
    m_canIndex = canIndex;
    m_isReady = true;

    qDebug() << "CAN参数设置 - 设备类型:" << m_deviceType
             << ", 设备索引:" << m_deviceIndex
             << ", CAN通道:" << m_canIndex;
}

bool CANTxRx::sendCANFrame(DWORD canId, const QByteArray &data, bool extendedFrame)
{
    VCI_CAN_OBJ frame = createCANFrame(canId, data, extendedFrame);
    return sendCANFrame(frame);
}

bool CANTxRx::sendCANFrame(const VCI_CAN_OBJ &frame)
{
    if (!m_canThread) {
        m_lastError = "CAN线程未设置，请先设置CAN线程";
        qDebug() << m_lastError;
        emit errorOccurred(m_lastError);
        return false;
    }

    // 使用CANThread发送数据
    bool success = m_canThread->sendData(m_canIndex, frame.ID, frame.RemoteFlag, 
                                        frame.ExternFlag, frame.Data, frame.DataLen);

//    if (success) {
//        updateStatistics(false);
////        if (!m_highSpeedMode) {
////            qDebug() << "CAN帧发送成功 - ID: 0x" << QString::number(frame.ID, 16).toUpper()
////                     << "数据:" << QByteArray((char*)frame.Data, frame.DataLen).toHex(' ').toUpper();
////        }
//        emit frameSent(true, frame);
//    } else {
//        m_lastError = "CAN帧发送失败";
//        qDebug() << m_lastError;
//        emit frameSent(false, frame);
//        emit errorOccurred(m_lastError);
//    }

    return success;
}

VCI_CAN_OBJ CANTxRx::createCANFrame(DWORD canId, const QByteArray &data, bool extendedFrame)
{
    VCI_CAN_OBJ frame;
    memset(&frame, 0, sizeof(VCI_CAN_OBJ));

    frame.ID = canId;
    frame.SendType = 0;
    frame.RemoteFlag = 0;
    frame.ExternFlag = extendedFrame ? 1 : 0;
    frame.DataLen = qMin(data.size(), 8);

    for (int i = 0; i < frame.DataLen; i++) {
        frame.Data[i] = static_cast<BYTE>(data[i]);
    }

    return frame;
}

QList<VCI_CAN_OBJ> CANTxRx::receiveCANFrames(int maxFrames)
{
    QList<VCI_CAN_OBJ> frames;

    if (!m_canThread) {
        return frames;
    }

    // 注意：接收数据现在由CANThread的run()方法处理
    // 这里返回空列表，实际接收通过CANThread的信号处理
    // 如果需要同步接收，可以在这里添加实现
    return frames;
}

void CANTxRx::processReceivedFrames(const QList<VCI_CAN_OBJ> &frames)
{
    if (!frames.isEmpty()) {
        m_receiver->pushFrames(frames);
    }
}

void CANTxRx::parseAndLogCANFrame(const VCI_CAN_OBJ &frame)
{
    // 添加总的数据接收计数
    static int totalFrameCount = 0;
    totalFrameCount++;
    
    // 🆕 数据上抛协议处理 (0x500-0x5FF) - 最高优先级
    if (frame.ID >= 0x500 && frame.ID <= 0x5FF) {
        QString dataHex = QByteArray((char*)frame.Data, frame.DataLen).toHex(' ').toUpper();
//        qDebug() << QString("📊 数据上抛协议帧 - ID:0x%1 长度:%2 数据:%3")
//                    .arg(frame.ID, 0, 16)
//                    .arg(frame.DataLen)
//                    .arg(dataHex);
        
        // 只有在示波器采集状态下才解析示波器数据
        if (m_dataAcquisition && g_oscilloscopeAcquiring == 1) {
            qDebug() << "✅ 数据采集组件存在，正在解析数据上抛协议";
            m_dataAcquisition->parseUpdateProtocol(frame);
        } else if (!m_dataAcquisition) {
            qDebug() << "❌ 数据采集组件为空，无法处理数据上抛协议";
        } else {
            qDebug() << "⏸️ 示波器未采集，跳过数据解析";
        }

        // 额外分发一份给UI：让控制参数页也能收到0x580-0x5FF的SDO响应
        // 这样即使当前不在数据采集页，也能更新"当前值"文本框
       // emit frameReceived(frame);
        if ( frame.ID >= 0x580 && frame.ID <= 0x5FF) {
            // 检查m_controlParam是否为空，避免空指针访问导致死机
            if (m_controlParam) {
                qDebug() << "✅ 处理当前值";
                // 使用QMetaObject::invokeMethod确保UI操作在主线程中执行，避免跨线程UI访问导致死机
                QMetaObject::invokeMethod(m_controlParam, "onSdoReadResponse", 
                                          Qt::QueuedConnection,
                                          Q_ARG(VCI_CAN_OBJ, frame));
            } else {
                qDebug() << "⚠️ m_controlParam为空，跳过SDO响应处理";
            }
        }
        return;
    }

    // 状态反馈处理 (0x000-0x07F)
    if (frame.ID < 0x80) { 
        
        parseStatusFeedback(frame);


    }
}

void CANTxRx::logStatusFrame(const VCI_CAN_OBJ &frame)
{
    QString dataHex = QByteArray((char*)frame.Data, frame.DataLen).toHex(' ').toUpper();
    QString frameInfo = QString("CAN状态帧 - ID:0x%1 数据:%2")
                       .arg(frame.ID, 0, 16)
                       .arg(dataHex);

    qDebug().noquote() << frameInfo;
    emit canDataReceived(frameInfo);
}

void CANTxRx::parseStatusFeedback(const VCI_CAN_OBJ &frame)
{
    // 这个函数现在主要在接收线程中处理
    // 保留用于单帧处理的情况
    if (frame.DataLen == 8) {
        int16_t speed_int;
        int32_t position_int;
        int16_t current_int;

        memcpy(&speed_int, &frame.Data[0], 2);
        memcpy(&position_int, &frame.Data[2], 4);
        memcpy(&current_int, &frame.Data[6], 2);

        float speed = speed_int / 10.0f;
        float position = position_int / 1000.0f;
        float current = current_int / 1000.0f;

        // 数据有效性检查
        if (isValidFloat(speed) && isValidFloat(position) && isValidFloat(current)) {
            // 打印速度、位置和电流数据
            qDebug() << QString("状态反馈 - ID:0x%1 速度:%2 位置:%3 电流:%4")
                        .arg(frame.ID, 0, 16)
                        .arg(speed, 0, 'f', 2)
                        .arg(position, 0, 'f', 3)
                        .arg(current, 0, 'f', 3);
            
            // 单帧情况下仍然发射信号
            emit statusDataReceived(frame.ID, speed, position, current);
        }
    }
}

void CANTxRx::startReceiving(int intervalMs, bool highSpeedMode)
{
    if (!m_canThread) {
        m_lastError = "CAN线程未设置，无法启动接收";
        qWarning() << m_lastError;
        return;
    }
    
    // 确保CANThread已启动
    if (!m_canThread->isRunning()) {
        m_canThread->start();
    }

    if (!m_isReceiving) {
        m_highSpeedMode = highSpeedMode;
        m_receiver->setHighSpeedMode(highSpeedMode);

        if (!m_receiver->isRunning()) {
            m_receiver->start();
        }

        if (m_highSpeedMode) {
            m_receiveTimer->setInterval(1);
            qDebug() << "启动高速CAN接收模式，间隔: 1ms";
        } else {
            m_receiveTimer->setInterval(intervalMs);
            qDebug() << "启动CAN接收，间隔:" << intervalMs << "ms";
        }

        m_receiveTimer->start();
        m_isReceiving = true;

        // 重置性能统计
        m_performanceTimer.restart();
        m_receiveCount = 0;
        m_lastLogTime = 0;
        m_queueOverflows = 0;

        emit receptionStarted();

        if (m_highSpeedMode) {
            emit canDataReceived("高速CAN接收模式已启动 (1ms间隔)");
        } else {
            emit canDataReceived(QString("CAN接收已启动 (%1ms间隔)").arg(intervalMs));
        }
    }
}

void CANTxRx::stopReceiving()
{
    if (m_isReceiving) {
        m_receiveTimer->stop();
        m_isReceiving = false;
        m_highSpeedMode = false;
        qDebug() << "CAN接收停止";
        emit receptionStopped();
        emit canDataReceived("CAN接收已停止");
    }
}

void CANTxRx::onReceiveTimeout()
{
    // 接收数据现在由CANThread的run()方法处理
    // 通过CANThread的getProtocolData信号传递数据
    // 这里不需要主动接收数据
}

bool CANTxRx::isReceiving() const
{
    return m_isReceiving;
}

bool CANTxRx::isHighSpeedMode() const
{
    return m_highSpeedMode;
}

float CANTxRx::bytesToFloat(uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3) const
{
    union {
        float f;
        uint8_t b[4];
    } u;

    u.b[0] = b0;
    u.b[1] = b1;
    u.b[2] = b2;
    u.b[3] = b3;

    return u.f;
}

bool CANTxRx::isValidFloat(float value) const
{
    return !std::isnan(value) && !std::isinf(value) && (fabs(value) < 1e10f);
}

bool CANTxRx::checkDataRange(float speed, float position, float current) const
{
    // 合理的数据范围检查
    return (fabs(speed) < 1000.0f) &&
           (fabs(position) < 10000.0f) &&
           (fabs(current) < 1000.0f);
}

bool CANTxRx::validateCANFrame(const VCI_CAN_OBJ &frame) const
{
    return (frame.DataLen >= 0 && frame.DataLen <= 8) &&
           (frame.ID < 0x80000000); // 有效的CAN ID范围
}

CANTxRx::CANStatistics CANTxRx::getStatistics() const
{
    QMutexLocker locker(&m_mutex);
    CANStatistics stats;
    stats.framesReceived = m_framesReceived;
    stats.framesSent = m_framesSent;
    stats.parseErrors = m_parseErrors;
    stats.lastUpdateTime = m_lastUpdateTime;
    stats.currentFPS = m_currentFPS;
    stats.highSpeedMode = m_highSpeedMode;
    stats.queueSize = m_receiver->getQueueSize();
    stats.queueOverflows = m_queueOverflows;
    return stats;
}

void CANTxRx::updateStatistics(bool isReceived, bool isError)
{
    QMutexLocker locker(&m_mutex);
    if (isReceived) {
        m_framesReceived++;
    } else {
        m_framesSent++;
    }
    if (isError) {
        m_parseErrors++;
    }
    m_lastUpdateTime = QDateTime::currentMSecsSinceEpoch();
}

void CANTxRx::updatePerformanceStats()
{
    qint64 currentTime = m_performanceTimer.elapsed();
    if (currentTime - m_lastLogTime >= 2000) {
        m_currentFPS = (m_receiveCount * 1000.0) / (currentTime - m_lastLogTime);
        m_receiveCount = 0;
        m_lastLogTime = currentTime;

        // 每10秒输出一次性能统计（高速模式下减少输出）
        static int performanceLogCounter = 0;
        if (!m_highSpeedMode || ++performanceLogCounter % 5 == 0) {
            qDebug() << QString("CAN性能统计 - 帧率: %1 fps, 队列: %2, 溢出: %3")
                            .arg(m_currentFPS, 0, 'f', 1)
                            .arg(m_receiver->getQueueSize())
                            .arg(m_queueOverflows);
            performanceLogCounter = 0;
        }

        emit performanceStatsUpdated(getStatistics());
    }
}

// 原有的发送命令函数保持不变
bool CANTxRx::sendSpeedCommand(float targetSpeed, float outputLimit)
{
    QByteArray data;
    data.resize(8);

    uint32_t speed_uint32;
    uint32_t limit_uint32;

    memcpy(&speed_uint32, &targetSpeed, sizeof(float));
    memcpy(&limit_uint32, &outputLimit, sizeof(float));

    if (!m_highSpeedMode) {
        qDebug() << "发送速度指令 - 目标速度:" << targetSpeed
                 << ", 输出限制:" << outputLimit;
    }

    data[0] = static_cast<char>(speed_uint32 & 0xFF);
    data[1] = static_cast<char>((speed_uint32 >> 8) & 0xFF);
    data[2] = static_cast<char>((speed_uint32 >> 16) & 0xFF);
    data[3] = static_cast<char>((speed_uint32 >> 24) & 0xFF);
    data[4] = static_cast<char>(limit_uint32 & 0xFF);
    data[5] = static_cast<char>((limit_uint32 >> 8) & 0xFF);
    data[6] = static_cast<char>((limit_uint32 >> 16) & 0xFF);
    data[7] = static_cast<char>((limit_uint32 >> 24) & 0xFF);

    DWORD can_id = (0x02 << 7) | (Can_id & 0x7F);
    bool success = sendCANFrame(can_id, data, false);

    return success;
}

bool CANTxRx::sendPositionCommand(float targetPosition, float maxSpeed)
{
    QByteArray data;
    data.resize(8);

    uint32_t position_uint32;
    uint32_t speed_uint32;

    memcpy(&position_uint32, &targetPosition, sizeof(float));
    memcpy(&speed_uint32, &maxSpeed, sizeof(float));

    if (!m_highSpeedMode) {
        qDebug() << "发送位置指令 - 目标位置:" << targetPosition
                 << ", 最大转速:" << maxSpeed;
    }

    data[0] = static_cast<char>(position_uint32 & 0xFF);
    data[1] = static_cast<char>((position_uint32 >> 8) & 0xFF);
    data[2] = static_cast<char>((position_uint32 >> 16) & 0xFF);
    data[3] = static_cast<char>((position_uint32 >> 24) & 0xFF);
    data[4] = static_cast<char>(speed_uint32 & 0xFF);
    data[5] = static_cast<char>((speed_uint32 >> 8) & 0xFF);
    data[6] = static_cast<char>((speed_uint32 >> 16) & 0xFF);
    data[7] = static_cast<char>((speed_uint32 >> 24) & 0xFF);

    DWORD can_id = (0x01 << 7) | (Can_id & 0x7F);
    return sendCANFrame(can_id, data, false);
}

// 模式切换命令
bool CANTxRx::sendSpeedModeSwitch()
{
    QByteArray data;
    data.append(static_cast<char>(0xFF));
    data.append(static_cast<char>(0xFF));
    data.append(static_cast<char>(0xFF));
    data.append(static_cast<char>(0xFF));
    data.append(static_cast<char>(0xFF));
    data.append(static_cast<char>(0xFF));
    data.append(static_cast<char>(0x02));
    data.append(static_cast<char>(0xFC));

    DWORD can_id = (0x00 << 7) | (Can_id & 0x7F);
    return sendCANFrame(can_id, data, false);
}

bool CANTxRx::sendPositionModeSwitch()
{
    QByteArray data;
    data.append(static_cast<char>(0xFF));
    data.append(static_cast<char>(0xFF));
    data.append(static_cast<char>(0xFF));
    data.append(static_cast<char>(0xFF));
    data.append(static_cast<char>(0xFF));
    data.append(static_cast<char>(0xFF));
    data.append(static_cast<char>(0x01));
    data.append(static_cast<char>(0xFC));

    DWORD can_id = (0x00 << 7) | (Can_id & 0x7F);
    return sendCANFrame(can_id, data, false);
}

bool CANTxRx::sendStartCommand()
{
    QByteArray data;
    data.append(static_cast<char>(0xFF));
    data.append(static_cast<char>(0xFF));
    data.append(static_cast<char>(0xFF));
    data.append(static_cast<char>(0xFF));
    data.append(static_cast<char>(0xFF));
    data.append(static_cast<char>(0xFF));
    data.append(static_cast<char>(0xFF));
    data.append(static_cast<char>(0xFC));

    DWORD can_id = (0x00 << 7) | (Can_id & 0x7F);
    return sendCANFrame(can_id, data, false);
}

bool CANTxRx::sendStopCommand()
{
    QByteArray data;
    data.append(static_cast<char>(0xFF));
    data.append(static_cast<char>(0xFF));
    data.append(static_cast<char>(0xFF));
    data.append(static_cast<char>(0xFF));
    data.append(static_cast<char>(0xFF));
    data.append(static_cast<char>(0xFF));
    data.append(static_cast<char>(0xFF));
    data.append(static_cast<char>(0xFD));

    DWORD can_id = (0x00 << 7) | (Can_id & 0x7F);
    return sendCANFrame(can_id, data, false);
}

bool CANTxRx::sendTorqueModeSwitch()
{
    QByteArray data;
    data.append(static_cast<char>(0xFF));
    data.append(static_cast<char>(0xFF));
    data.append(static_cast<char>(0xFF));
    data.append(static_cast<char>(0xFF));
    data.append(static_cast<char>(0xFF));
    data.append(static_cast<char>(0xFF));
    data.append(static_cast<char>(0x03));
    data.append(static_cast<char>(0xFC));

    DWORD can_id = (0x00 << 7) | (Can_id & 0x7F);
    return sendCANFrame(can_id, data, false);
}

bool CANTxRx::sendMotionModeSwitch()
{
    QByteArray data;
    data.append(static_cast<char>(0xFF));
    data.append(static_cast<char>(0xFF));
    data.append(static_cast<char>(0xFF));
    data.append(static_cast<char>(0xFF));
    data.append(static_cast<char>(0xFF));
    data.append(static_cast<char>(0xFF));
    data.append(static_cast<char>(0x00));
    data.append(static_cast<char>(0xFC));

    DWORD can_id = (0x00 << 7) | (Can_id & 0x7F);
    return sendCANFrame(can_id, data, false);
}

bool CANTxRx::sendJogForwardCommand()
{
    QByteArray data;
    data.append(static_cast<char>(0x01));
    data.append(static_cast<char>(0x00));
    data.append(static_cast<char>(0x00));
    data.append(static_cast<char>(0x00));
    data.append(static_cast<char>(0x00));
    data.append(static_cast<char>(0x00));
    data.append(static_cast<char>(0x00));
    data.append(static_cast<char>(0x00));

    DWORD can_id = (0x00 << 7) | (Can_id & 0x7F);
    return sendCANFrame(can_id, data, false);
}

bool CANTxRx::sendJogBackwardCommand()
{
    QByteArray data;
    data.append(static_cast<char>(0x02));
    data.append(static_cast<char>(0x00));
    data.append(static_cast<char>(0x00));
    data.append(static_cast<char>(0x00));
    data.append(static_cast<char>(0x00));
    data.append(static_cast<char>(0x00));
    data.append(static_cast<char>(0x00));
    data.append(static_cast<char>(0x00));

    DWORD can_id = (0x00 << 7) | (Can_id & 0x7F);
    return sendCANFrame(can_id, data, false);
}

bool CANTxRx::sendCurrentCommand(float iq, float id)
{
    QByteArray data;
    data.resize(8);

    uint32_t iq_uint32, id_uint32;
    memcpy(&iq_uint32, &iq, sizeof(float));
    memcpy(&id_uint32, &id, sizeof(float));

    if (!m_highSpeedMode) {
        qDebug() << "发送电流指令 - Iq电流:" << iq
                 << ", Id电流:" << id;
    }

    data[0] = static_cast<char>(iq_uint32 & 0xFF);
    data[1] = static_cast<char>((iq_uint32 >> 8) & 0xFF);
    data[2] = static_cast<char>((iq_uint32 >> 16) & 0xFF);
    data[3] = static_cast<char>((iq_uint32 >> 24) & 0xFF);
    data[4] = static_cast<char>(id_uint32 & 0xFF);
    data[5] = static_cast<char>((id_uint32 >> 8) & 0xFF);
    data[6] = static_cast<char>((id_uint32 >> 16) & 0xFF);
    data[7] = static_cast<char>((id_uint32 >> 24) & 0xFF);

    DWORD can_id = (0x03 << 7) | (Can_id & 0x7F);
    return sendCANFrame(can_id, data, false);
}

bool CANTxRx::sendMotionCommand(float kp, float kd, float position, float velocity, float current)
{
    QByteArray data;
    data.resize(8);

    // 位置转换：-12.57rad ~ 12.57rad -> 0-65535
    uint16_t position_raw = static_cast<uint16_t>((position + 12.57f) / 25.14f * 65535.0f);
    position_raw = qBound(0, static_cast<int>(position_raw), 65535);

    // 速度转换：-50rad/s ~ 50rad/s -> 0-4096
    uint16_t velocity_raw = static_cast<uint16_t>((velocity + 50.0f) / 100.0f * 4096.0f);
    velocity_raw = qBound(0, static_cast<int>(velocity_raw), 4095);

    // Kp转换：0-5000 -> 0-4096
    uint16_t kp_raw = static_cast<uint16_t>(kp / 5000.0f * 4096.0f);
    kp_raw = qBound(0, static_cast<int>(kp_raw), 4095);

    // Kd转换：0-100 -> 0-4096
    uint16_t kd_raw = static_cast<uint16_t>(kd / 100.0f * 4096.0f);
    kd_raw = qBound(0, static_cast<int>(kd_raw), 4095);

    // 力矩转换：-36N·m ~ 36N·m -> 0-4096
    uint16_t torque_raw = static_cast<uint16_t>((current + 36.0f) / 72.0f * 4096.0f);
    torque_raw = qBound(0, static_cast<int>(torque_raw), 4095);

    if (!m_highSpeedMode) {
        qDebug() << "发送运控指令 - 位置:" << position << "rad ->" << position_raw
                 << ", 速度:" << velocity << "rad/s ->" << velocity_raw
                 << ", Kp:" << kp << "->" << kp_raw
                 << ", Kd:" << kd << "->" << kd_raw
                 << ", 力矩:" << current << "N·m ->" << torque_raw;
    }

    // MIT协议格式
    data[0] = static_cast<char>((position_raw >> 8) & 0xFF);
    data[1] = static_cast<char>(position_raw & 0xFF);
    data[2] = static_cast<char>((velocity_raw >> 4) & 0xFF);
    data[3] = static_cast<char>(((velocity_raw & 0x0F) << 4) | ((kp_raw >> 8) & 0x0F));
    data[4] = static_cast<char>(kp_raw & 0xFF);
    data[5] = static_cast<char>((kd_raw >> 4) & 0xFF);
    data[6] = static_cast<char>(((kd_raw & 0x0F) << 4) | ((torque_raw >> 8) & 0x0F));
    data[7] = static_cast<char>(torque_raw & 0xFF);

    DWORD can_id = (0x00 << 7) | (Can_id & 0x7F);
    return sendCANFrame(can_id, data, false);
}

bool CANTxRx::sendParameterData(DWORD nodeId, uint16_t index, uint8_t subindex, const QByteArray& data)
{
    DWORD canId = 0x600 + nodeId;

    QByteArray sdoData;
    sdoData.resize(8);
    sdoData.fill(0x00);

    BYTE commandSpecifier = 0x22; // 默认：段下载（多段传输）

    if (data.size() == 1) {
        commandSpecifier = 0x2F;
    } else if (data.size() == 2) {
        commandSpecifier = 0x2B;
    } else if (data.size() == 4) {
        commandSpecifier = 0x23;
    }

    sdoData[0] = static_cast<char>(commandSpecifier);
    sdoData[1] = static_cast<char>(index & 0xFF);
    sdoData[2] = static_cast<char>((index >> 8) & 0xFF);
    sdoData[3] = static_cast<char>(subindex);

    for (int i = 0; i < data.size() && i < 4; i++) {
        sdoData[4 + i] = data[i];
    }

    if (!m_highSpeedMode) {
        qDebug() << "发送参数设置 - NodeID:" << nodeId
                 << "索引: 0x" << QString::number(index, 16).toUpper()
                 << "子索引: 0x" << QString::number(subindex, 16).toUpper()
                 << "数据:" << data.toHex(' ').toUpper()
                 << "命令:" << QString::number(commandSpecifier, 16).toUpper();
    }

    return sendCANFrame(canId, sdoData, false);
}

bool CANTxRx::sendParameterRead(DWORD nodeId, uint16_t index, uint8_t subindex)
{
    DWORD canId = 0x600 + nodeId;
    QByteArray sdoData;
    sdoData.resize(8);
    sdoData.fill(0x00);

    // SDO Upload Initiate: 0x40, index little-endian in payload
    sdoData[0] = static_cast<char>(0x40);
    sdoData[1] = static_cast<char>(index & 0xFF);
    sdoData[2] = static_cast<char>((index >> 8) & 0xFF);
    sdoData[3] = static_cast<char>(subindex);

    if (!m_highSpeedMode) {
        qDebug() << "发送参数读取 - NodeID:" << nodeId
                 << "索引: 0x" << QString::number(index, 16).toUpper()
                 << "子索引: 0x" << QString::number(subindex, 16).toUpper();
    }

    return sendCANFrame(canId, sdoData, false);
}

bool CANTxRx::reinitializeCAN()
{
    // 重新初始化CAN设备的实现
    stopReceiving();
    // 这里添加重新初始化CAN硬件的代码
    qDebug() << "CAN设备重新初始化";
    return true;
}

bool CANTxRx::isDeviceReady() const
{
    return m_isReady;
}

QString CANTxRx::getLastError() const
{
    return m_lastError;
}
