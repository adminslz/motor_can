#ifndef CAN_RX_TX_H
#define CAN_RX_TX_H

#include <QObject>
#include <QMetaType>
#include <QTimer>
#include <QMutex>
#include <QElapsedTimer>
#include <QQueue>
#include <QThread>
#include <QWaitCondition>
#include "ControlCAN.h"  // 包含原始头文件
#include "data_acquisition.h"
#include "control_param.h"
class DataAcquisition;
class CANThread;

// CAN接收线程类
class CANReceiver : public QThread
{
    Q_OBJECT
public:
    explicit CANReceiver(QObject *parent = nullptr);
    ~CANReceiver();

    void stop();
    void setHighSpeedMode(bool enabled);
    void pushFrames(const QList<VCI_CAN_OBJ> &frames);
    int getQueueSize() const;

protected:
    void run() override;

signals:
    void framesProcessed(const QList<VCI_CAN_OBJ> &frames);
    void statusDataBatchReceived(const QVector<QPair<DWORD, QVector<float>>> &batchData);
    void queueOverflow();

private:
    QQueue<QList<VCI_CAN_OBJ>> m_frameQueue;
    mutable QMutex m_queueMutex;
    bool m_running;
    bool m_highSpeedMode;
    QWaitCondition m_queueCondition;
    const int MAX_QUEUE_SIZE = 1000;
};

class CANTxRx : public QObject
{
    Q_OBJECT

public:
    explicit CANTxRx(QObject *parent = nullptr);
    ~CANTxRx();

    void setDataAcquisition(DataAcquisition *dataAcquisition) {
        m_dataAcquisition = dataAcquisition;
    }
    
    // 设置CAN线程实例
    void setCANThread(CANThread* canThread) {
        m_canThread = canThread;
    }

    bool sendParameterData(DWORD nodeId, uint16_t index, uint8_t subindex, const QByteArray& data);
    bool sendParameterRead(DWORD nodeId, uint16_t index, uint8_t subindex);
    void setCANParams(DWORD deviceType, DWORD deviceIndex, DWORD canIndex);
    bool sendCANFrame(DWORD canId, const QByteArray &data, bool extendedFrame = false);
    bool sendCANFrame(const VCI_CAN_OBJ &frame);
    QList<VCI_CAN_OBJ> receiveCANFrames(int maxFrames = 100);
    
    // 处理从CANThread接收到的帧
    void processReceivedFrames(const QList<VCI_CAN_OBJ> &frames);

    void startReceiving(int intervalMs = 10, bool highSpeedMode = false);
    void stopReceiving();
    bool isReceiving() const;
    bool isHighSpeedMode() const;
    bool reinitializeCAN();

    // 各种发送命令函数...
    bool sendSpeedCommand(float targetSpeed, float outputLimit);
    bool sendPositionCommand(float targetPosition, float maxSpeed);
    bool sendSpeedModeSwitch();
    bool sendPositionModeSwitch();
    bool sendTorqueModeSwitch();
    bool sendMotionModeSwitch();
    bool sendStartCommand();
    bool sendStopCommand();
    bool sendJogForwardCommand();
    bool sendJogBackwardCommand();
    bool sendCurrentCommand(float iq, float id);
    bool sendMotionCommand(float kp, float kd, float position, float velocity, float current);

    bool isDeviceReady() const;
    QString getLastError() const;

    struct CANStatistics {
        int framesReceived;
        int framesSent;
        int parseErrors;
        qint64 lastUpdateTime;
        double currentFPS;
        bool highSpeedMode;
        int queueSize;
        int queueOverflows;
    };
    CANStatistics getStatistics() const;

    float bytesToFloat(uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3) const;
    bool isValidFloat(float value) const;

signals:
    void frameReceived(const VCI_CAN_OBJ &frame);
    void frameSent(bool success, const VCI_CAN_OBJ &frame);
    void errorOccurred(const QString &errorMessage);
    void canDataReceived(const QString &logMessage);
    void motorSpeedReceived(float speed, float current);
    void motorPositionReceived(float position);
    void motorTorqueReceived(float torque);
    void motorStatusReceived(float speed, float position, float current);
    void statusDataReceived(DWORD canId, float speed, float position, float current);
    void statusDataBatchReceived(const QVector<QPair<DWORD, QVector<float>>> &batchData);
    void receptionStarted();
    void receptionStopped();
    void performanceStatsUpdated(const CANStatistics &stats);
    void queueOverflowDetected();

private slots:
    void onReceiveTimeout();
    void onFramesProcessed(const QList<VCI_CAN_OBJ> &frames);
    void onStatusBatchReceived(const QVector<QPair<DWORD, QVector<float>>> &batchData);
    void onQueueOverflow();

private:
    VCI_CAN_OBJ createCANFrame(DWORD canId, const QByteArray &data, bool extendedFrame = false);
    void parseAndLogCANFrame(const VCI_CAN_OBJ &frame);
    void parseStatusFeedback(const VCI_CAN_OBJ &frame);
    void logStatusFrame(const VCI_CAN_OBJ &frame);
    bool checkDataRange(float speed, float position, float current) const;
    bool validateCANFrame(const VCI_CAN_OBJ &frame) const;
    void updateStatistics(bool isReceived, bool isError = false);
    void updatePerformanceStats();

    DataAcquisition *m_dataAcquisition;
    CANReceiver *m_receiver;
    CANThread *m_canThread;
    DWORD m_deviceType;
    DWORD m_deviceIndex;
    DWORD m_canIndex;
    bool m_isReady;
    bool m_isReceiving;
    bool m_highSpeedMode;
    QString m_lastError;

    QTimer *m_receiveTimer;
    mutable QMutex m_mutex;

    int m_framesReceived;
    int m_framesSent;
    int m_parseErrors;
    int m_queueOverflows;
    qint64 m_lastUpdateTime;

    QElapsedTimer m_performanceTimer;
    int m_receiveCount;
    qint64 m_lastLogTime;
    double m_currentFPS;

    // 直接通知控制参数界面的指针，由上层在MainWindow中设置
public:
    void setControlParam(class ControlParam* ctrl) { m_controlParam = ctrl; }
private:
    class ControlParam* m_controlParam { nullptr };
};

extern CANTxRx *g_canTxRx;

#endif // CAN_RX_TX_H
