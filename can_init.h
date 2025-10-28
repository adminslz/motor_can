#ifndef CAN_INIT_H
#define CAN_INIT_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QMessageBox>
#include <QTimer>
#include "canthread.h"

class CANInit : public QWidget
{
    Q_OBJECT

public:
    explicit CANInit(QWidget *parent = nullptr);
    ~CANInit() = default;

    // 获取配置参数
    int getDeviceIndex() const;
    int getBaudRate() const;
    int getCANID() const;
    
    // 获取CAN线程实例
    CANThread* getCANThread() const { return canThread; }

signals:
    void loginSuccess(int deviceIndex, int baudRate, int canId, CANThread* canThread);

private slots:
    void onLoginButtonClicked();
    void onCancelButtonClicked();
    void onRefreshDevices();
    void onDeviceCheckTimeout();

private:
    void setupUI();
    bool validateInput();
    void scanCANDevices();
    bool testDeviceConnection(int deviceIndex, int baudRate);
    QString getDeviceInfo(int deviceIndex);  // 添加这个声明

    QComboBox *deviceIndexCombo;
    QComboBox *baudRateCombo;
    QLineEdit *canIdEdit;
    QPushButton *loginButton;
    QPushButton *cancelButton;
    QPushButton *refreshButton;
    QLabel *statusLabel;

    CANThread *canThread;
    QTimer *deviceCheckTimer;
    bool isCheckingDevice;
};

#endif // CAN_INIT_H
