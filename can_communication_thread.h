#ifndef CAN_COMMUNICATION_THREAD_H
#define CAN_COMMUNICATION_THREAD_H

#include <QThread>
#include <QMutex>
#include <QQueue>
#include <QTimer>
#include <QByteArray>
#include <QTime>

// 前向声明
class CANTxRx;

// CAN帧结构
struct CANFrame {
    uint32_t id;
    QByteArray data;
    QTime timestamp;
    
    CANFrame(uint32_t frameId = 0, const QByteArray& frameData = QByteArray()) 
        : id(frameId), data(frameData), timestamp(QTime::currentTime()) {}
};

// CAN通信线程类
class CANCommunicationThread : public QThread
{
    Q_OBJECT

public:
    explicit CANCommunicationThread(QObject *parent = nullptr);
    ~CANCommunicationThread();

    // 设置CAN组件
    void setCANTxRx(CANTxRx* canTxRx);
    
    // 发送CAN帧（非阻塞）
    void sendCANFrame(uint32_t id, const QByteArray& data);
    
    // 启动/停止通信
    void startCommunication();
    void stopCommunication();
    
    // 设置发送频率
    void setSendInterval(int intervalMs);
    
    // 获取统计信息
    int getSendCount() const { return m_sendCount; }
    int getReceiveCount() const { return m_receiveCount; }
    double getSendFrequency() const { return m_sendFrequency; }
    double getReceiveFrequency() const { return m_receiveFrequency; }

signals:
    // 接收到CAN帧信号
    void canFrameReceived(const CANFrame& frame);
    
    // 发送完成信号
    void canFrameSent(uint32_t id, const QByteArray& data);
    
    // 统计信息更新信号
    void statisticsUpdated(int sendCount, int receiveCount, double sendFreq, double receiveFreq);

protected:
    void run() override;

private slots:
    void processSendQueue();
    void updateStatistics();

private:
    CANTxRx* m_canTxRx;
    
    // 发送队列
    QQueue<CANFrame> m_sendQueue;
    QMutex m_sendMutex;
    
    // 接收队列
    QQueue<CANFrame> m_receiveQueue;
    QMutex m_receiveMutex;
    
    // 控制变量
    volatile bool m_running;
    volatile bool m_communicationActive;
    
    // 定时器
    QTimer* m_sendTimer;
    QTimer* m_statsTimer;
    
    // 统计信息
    int m_sendCount;
    int m_receiveCount;
    QTime m_lastStatsTime;
    double m_sendFrequency;
    double m_receiveFrequency;
    
    // 发送间隔
    int m_sendInterval;
    
    // 处理接收数据
    void processReceivedFrames();
};

#endif // CAN_COMMUNICATION_THREAD_H
