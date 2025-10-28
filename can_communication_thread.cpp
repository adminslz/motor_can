#include "can_communication_thread.h"
#include "can_rx_tx.h"
#include <QDebug>
#include <QCoreApplication>

CANCommunicationThread::CANCommunicationThread(QObject *parent)
    : QThread(parent)
    , m_canTxRx(nullptr)
    , m_running(false)
    , m_communicationActive(false)
    , m_sendCount(0)
    , m_receiveCount(0)
    , m_sendFrequency(0.0)
    , m_receiveFrequency(0.0)
    , m_sendInterval(1) // 默认1ms间隔
    , m_sendTimer(nullptr)
    , m_statsTimer(nullptr)
{
    // 定时器将在run()函数中创建，避免线程问题
    m_lastStatsTime = QTime::currentTime();
}

CANCommunicationThread::~CANCommunicationThread()
{
    stopCommunication();
    quit();
    wait();
    
    if (m_sendTimer) {
        delete m_sendTimer;
        m_sendTimer = nullptr;
    }
    if (m_statsTimer) {
        delete m_statsTimer;
        m_statsTimer = nullptr;
    }
}

void CANCommunicationThread::setCANTxRx(CANTxRx* canTxRx)
{
    m_canTxRx = canTxRx;
}

void CANCommunicationThread::sendCANFrame(uint32_t id, const QByteArray& data)
{
    if (!m_communicationActive) {
        return;
    }
    
    CANFrame frame(id, data);
    
    QMutexLocker locker(&m_sendMutex);
    m_sendQueue.enqueue(frame);
}

void CANCommunicationThread::startCommunication()
{
    m_communicationActive = true;
    m_running = true;
    
    if (!isRunning()) {
        start();
    }
}

void CANCommunicationThread::stopCommunication()
{
    m_communicationActive = false;
    m_running = false;
    
    if (m_sendTimer && m_sendTimer->isActive()) {
        m_sendTimer->stop();
    }
    if (m_statsTimer && m_statsTimer->isActive()) {
        m_statsTimer->stop();
    }
}

void CANCommunicationThread::setSendInterval(int intervalMs)
{
    m_sendInterval = qMax(1, intervalMs);
    if (m_sendTimer && m_sendTimer->isActive()) {
        m_sendTimer->setInterval(m_sendInterval);
    }
}

void CANCommunicationThread::run()
{
    // 在线程中创建定时器
    m_sendTimer = new QTimer();
    m_statsTimer = new QTimer();
    
    // 连接定时器信号
    connect(m_sendTimer, &QTimer::timeout, this, &CANCommunicationThread::processSendQueue);
    connect(m_statsTimer, &QTimer::timeout, this, &CANCommunicationThread::updateStatistics);
    
    // 设置统计更新间隔
    m_statsTimer->setInterval(1000); // 每秒更新一次统计
    
    // 启动定时器
    m_sendTimer->start(m_sendInterval);
    m_statsTimer->start(1000);
    
    qDebug() << "🚀 CAN通信线程已启动，发送间隔:" << m_sendInterval << "ms";
    
    // 主循环
    while (m_running) {
        // 处理接收数据
        processReceivedFrames();
        
        // 短暂休眠，避免CPU占用过高
        msleep(1);
    }
    
    // 停止定时器
    if (m_sendTimer) {
        m_sendTimer->stop();
    }
    if (m_statsTimer) {
        m_statsTimer->stop();
    }
    
    qDebug() << "🛑 CAN通信线程已停止";
}

void CANCommunicationThread::processSendQueue()
{
    if (!m_communicationActive || !m_canTxRx) {
        return;
    }
    
    QMutexLocker locker(&m_sendMutex);
    
    if (!m_sendQueue.isEmpty()) {
        CANFrame frame = m_sendQueue.dequeue();
        
        // 发送CAN帧
        if (m_canTxRx->sendCANFrame(frame.id, frame.data)) {
            m_sendCount++;
            
            // 发送成功信号
            emit canFrameSent(frame.id, frame.data);
            
            // 调试输出（每100次输出一次）
            if (m_sendCount % 100 == 0) {
                qDebug() << QString("📤 CAN发送成功 - ID:0x%1 数据:%2 (总计:%3次)")
                            .arg(frame.id, 0, 16)
                            .arg(QString(frame.data.toHex(' ').toUpper()))
                            .arg(m_sendCount);
            }
        } else {
            qDebug() << "❌ CAN发送失败 - ID:0x" << QString::number(frame.id, 16);
        }
    }
}

void CANCommunicationThread::updateStatistics()
{
    QTime currentTime = QTime::currentTime();
    int elapsed = m_lastStatsTime.msecsTo(currentTime);
    
    if (elapsed > 0) {
        m_sendFrequency = m_sendCount * 1000.0 / elapsed;
        m_receiveFrequency = m_receiveCount * 1000.0 / elapsed;
        
        // 发送统计信息信号
        emit statisticsUpdated(m_sendCount, m_receiveCount, m_sendFrequency, m_receiveFrequency);
        
        // 重置计数器和时间
        m_sendCount = 0;
        m_receiveCount = 0;
        m_lastStatsTime = currentTime;
    }
}

void CANCommunicationThread::processReceivedFrames()
{
    // 这里可以添加接收处理逻辑
    // 目前由CANTxRx直接处理接收，这里预留接口
}
