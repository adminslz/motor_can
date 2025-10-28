#include "can_init.h"
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QApplication>

CANInit::CANInit(QWidget *parent)
    : QWidget(parent)
    , deviceIndexCombo(nullptr)
    , baudRateCombo(nullptr)
    , canIdEdit(nullptr)
    , loginButton(nullptr)
    , cancelButton(nullptr)
    , refreshButton(nullptr)
    , statusLabel(nullptr)
    , canThread(nullptr)
    , deviceCheckTimer(nullptr)
    , isCheckingDevice(false)
{
    // 初始化CAN线程
    canThread = new CANThread();

    setupUI();
    scanCANDevices();
}

void CANInit::setupUI()
{
    setWindowTitle("CAN通信配置");
    setFixedSize(500, 500);

    // 设置窗口样式
    setStyleSheet(R"(
        QWidget {
            background-color: #2d2d2d;
            color: white;
            font-family: "Microsoft YaHei", "Segoe UI", sans-serif;
        }
        QLabel {
            color: #ffffff;
            font-size: 14px;
            font-weight: bold;
        }
        QComboBox, QLineEdit {
            background-color: #454545;
            color: white;
            border: 1px solid #555555;
            border-radius: 4px;
            padding: 8px;
            font-size: 14px;
            min-height: 20px;
        }
        QComboBox::drop-down {
            border: none;
            width: 20px;
        }
        QComboBox QAbstractItemView {
            background-color: #454545;
            color: white;
            selection-background-color: #555555;
        }
        QPushButton {
            border: none;
            border-radius: 4px;
            padding: 10px 20px;
            font-size: 14px;
            font-weight: bold;
            min-width: 80px;
        }
    )");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(30, 30, 30, 30);
    mainLayout->setSpacing(20);

    // 标题
    QLabel *titleLabel = new QLabel("CAN通信配置");
    titleLabel->setStyleSheet(
        "QLabel {"
        "    color: #4fc3f7;"
        "    font-size: 24px;"
        "    font-weight: bold;"
        "    text-align: center;"
        "    padding-bottom: 10px;"
        "}"
    );
    titleLabel->setAlignment(Qt::AlignCenter);

    // 配置表单
    QWidget *formWidget = new QWidget();
    QFormLayout *formLayout = new QFormLayout(formWidget);
    formLayout->setVerticalSpacing(15);
    formLayout->setHorizontalSpacing(20);
    formLayout->setLabelAlignment(Qt::AlignRight);

    // 设备索引
    QLabel *deviceLabel = new QLabel("CAN设备:");
    QWidget *deviceWidget = new QWidget();
    QHBoxLayout *deviceLayout = new QHBoxLayout(deviceWidget);
    deviceLayout->setContentsMargins(0, 0, 0, 0);
    deviceLayout->setSpacing(10);

    deviceIndexCombo = new QComboBox();
    deviceIndexCombo->setMinimumWidth(200);

    refreshButton = new QPushButton("刷新");
    refreshButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #2196f3;"
        "    color: white;"
        "    padding: 6px 12px;"
        "    font-size: 12px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #1976d2;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #1565c0;"
        "}"
    );

    deviceLayout->addWidget(deviceIndexCombo);
    deviceLayout->addWidget(refreshButton);

    // 波特率
    QLabel *baudRateLabel = new QLabel("波特率:");
    baudRateCombo = new QComboBox();
    baudRateCombo->addItem("125 Kbps", 125);
    baudRateCombo->addItem("250 Kbps", 250);
    baudRateCombo->addItem("500 Kbps", 500);
    baudRateCombo->addItem("1000 Kbps", 1000);
    baudRateCombo->setCurrentIndex(3);

    // CAN ID
    QLabel *canIdLabel = new QLabel("CAN ID:");
    canIdEdit = new QLineEdit();
    canIdEdit->setText("0x001");
    canIdEdit->setPlaceholderText("支持标准帧(0x000-0x7FF)和扩展帧(0x00000000-0x1FFFFFFF)");
    canIdEdit->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9A-Fa-fxX]+"), this));

    // 状态标签
    statusLabel = new QLabel("正在扫描CAN设备...");
    statusLabel->setStyleSheet(
        "QLabel {"
        "    color: #ff9800;"
        "    font-size: 12px;"
        "    font-weight: bold;"
        "    padding: 5px;"
        "    background-color: #333333;"
        "    border-radius: 3px;"
        "}"
    );
    statusLabel->setAlignment(Qt::AlignCenter);

    // 添加到表单
    formLayout->addRow(deviceLabel, deviceWidget);
    formLayout->addRow(baudRateLabel, baudRateCombo);
    formLayout->addRow(canIdLabel, canIdEdit);
    formLayout->addRow(statusLabel);

    // 按钮区域
    QWidget *buttonWidget = new QWidget();
    QHBoxLayout *buttonLayout = new QHBoxLayout(buttonWidget);
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->setSpacing(20);

    cancelButton = new QPushButton("取消");
    cancelButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #5a5a5a;"
        "    color: white;"
        "}"
        "QPushButton:hover {"
        "    background-color: #666666;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #4a4a4a;"
        "}"
    );

    loginButton = new QPushButton("连接设备");
    loginButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #5a5a5a;"
        "    color: #888888;"
        "}"
        "QPushButton:hover {"
        "    background-color: #666666;"
        "}"
        "QPushButton:enabled {"
        "    background-color: #4caf50;"
        "    color: white;"
        "}"
    );
    loginButton->setEnabled(false);

    buttonLayout->addStretch();
    buttonLayout->addWidget(cancelButton);
    buttonLayout->addWidget(loginButton);

    // 添加到主布局
    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(formWidget);
    mainLayout->addStretch();
    mainLayout->addWidget(buttonWidget);

    // 初始化定时器
    deviceCheckTimer = new QTimer(this);
    deviceCheckTimer->setSingleShot(true);

    // 连接信号槽
    connect(loginButton, &QPushButton::clicked, this, &CANInit::onLoginButtonClicked);
    connect(cancelButton, &QPushButton::clicked, this, &CANInit::onCancelButtonClicked);
    connect(refreshButton, &QPushButton::clicked, this, &CANInit::onRefreshDevices);
    connect(deviceCheckTimer, &QTimer::timeout, this, &CANInit::onDeviceCheckTimeout);
}

void CANInit::scanCANDevices()
{
    deviceIndexCombo->clear();
    statusLabel->setText("正在扫描CAN设备...");
    statusLabel->setStyleSheet("QLabel { color: #ff9800; background-color: #333333; }");
    loginButton->setEnabled(false);

    QList<QString> deviceNames;
    QList<int> deviceIndices;

    // 使用CANThread来检测设备
    for (int i = 0; i < 4; i++) {
        bool deviceExists = testDeviceConnection(i, 1000);

        if (deviceExists) {
            QString deviceName = QString("CAN设备 %1").arg(i);
            deviceNames.append(deviceName);
            deviceIndices.append(i);
            qDebug() << "发现CAN设备:" << deviceName << "索引:" << i;
        }
    }

    // 更新设备列表
    for (int i = 0; i < deviceNames.size(); i++) {
        deviceIndexCombo->addItem(deviceNames[i], deviceIndices[i]);
    }

    if (deviceNames.isEmpty()) {
        statusLabel->setText("未发现CAN设备，请检查设备连接");
        statusLabel->setStyleSheet("QLabel { color: #f44336; background-color: #333333; }");
    } else {
        statusLabel->setText(QString("发现 %1 个CAN设备").arg(deviceNames.size()));
        statusLabel->setStyleSheet("QLabel { color: #4caf50; background-color: #333333; }");
        loginButton->setEnabled(true);
    }
}

QString CANInit::getDeviceInfo(int deviceIndex)
{
    // 简化版本，直接返回设备索引
    return QString("CAN设备 %1").arg(deviceIndex);
}

bool CANInit::testDeviceConnection(int deviceIndex, int baudRate)
{
    // 使用CANThread来测试设备连接
    // 这里调用CANThread的打开设备方法来测试
    DWORD deviceType = 4; // USBCAN-2A/U
    DWORD devIndex = deviceIndex;
    DWORD baundRate = baudRate;

    // 使用CANThread的方法来测试连接
    bool connected = canThread->openDevice(deviceType, devIndex, baundRate);

    if (connected) {
        // 如果连接成功，立即关闭设备（这只是测试）
        canThread->closeDevice();
        qDebug() << "设备" << deviceIndex << "连接测试成功";
        return true;
    } else {
        qDebug() << "设备" << deviceIndex << "连接测试失败";
        return false;
    }
}

void CANInit::onRefreshDevices()
{
    scanCANDevices();
}

void CANInit::onLoginButtonClicked()
{
    if (!validateInput()) {
        return;
    }

    int deviceIndex = getDeviceIndex();
    int baudRate = getBaudRate();
    int canId = getCANID();

    statusLabel->setText("正在连接设备...");
    statusLabel->setStyleSheet("QLabel { color: #ff9800; background-color: #333333; }");
    loginButton->setEnabled(false);
    cancelButton->setEnabled(false);

    // 使用CANThread进行真实连接
    isCheckingDevice = true;
    deviceCheckTimer->start(3000); // 3秒超时

    // 使用CANThread的实际连接方法
    DWORD deviceType = 4; // USBCAN-2A/U
    bool connected = canThread->openDevice(deviceType, deviceIndex, baudRate);

    if (connected) {
        deviceCheckTimer->stop();
        isCheckingDevice = false;

        statusLabel->setText("设备连接成功！");
        statusLabel->setStyleSheet("QLabel { color: #4caf50; background-color: #333333; }");

        QMessageBox::information(this, "连接成功",
            QString("CAN设备连接成功！\n设备: %1\n波特率: %2 Kbps\nCAN ID: 0x%3")
                .arg(deviceIndexCombo->currentText())
                .arg(baudRate)
                .arg(QString::number(canId, 16).toUpper()));

        // 发射登录成功信号，传递CAN线程实例
        emit loginSuccess(deviceIndex, baudRate, canId, canThread);
    } else {
        deviceCheckTimer->stop();
        isCheckingDevice = false;
        statusLabel->setText("设备连接失败，请检查设备");
        statusLabel->setStyleSheet("QLabel { color: #f44336; background-color: #333333; }");
        loginButton->setEnabled(true);
        cancelButton->setEnabled(true);

        QMessageBox::warning(this, "连接失败",
            "无法连接到CAN设备，请检查：\n"
            "1. 设备是否正确连接\n"
            "2. 驱动程序是否安装\n"
            "3. 设备是否被其他程序占用");
    }
}

void CANInit::onDeviceCheckTimeout()
{
    if (isCheckingDevice) {
        isCheckingDevice = false;
        statusLabel->setText("设备连接超时，请重试");
        statusLabel->setStyleSheet("QLabel { color: #f44336; background-color: #333333; }");
        loginButton->setEnabled(true);
        cancelButton->setEnabled(true);
    }
}

void CANInit::onCancelButtonClicked()
{
    QApplication::quit();
}

int CANInit::getDeviceIndex() const
{
    return deviceIndexCombo->currentData().toInt();
}

int CANInit::getBaudRate() const
{
    return baudRateCombo->currentData().toInt();
}

int CANInit::getCANID() const
{
    QString canIdStr = canIdEdit->text();
    bool ok;
    int canId = canIdStr.toInt(&ok, 0);
    return ok ? canId : 0x001;
}

bool CANInit::validateInput()
{
    if (deviceIndexCombo->count() == 0) {
        QMessageBox::warning(this, "错误", "没有可用的CAN设备，请刷新设备列表。");
        return false;
    }

    QString canIdStr = canIdEdit->text();
    bool ok;
    int canId = canIdStr.toInt(&ok, 0);

    if (!ok) {
        QMessageBox::warning(this, "输入错误", "CAN ID格式不正确，请输入有效的十六进制或十进制数值。");
        canIdEdit->setFocus();
        return false;
    }

    if (canId < 0) {
        QMessageBox::warning(this, "输入错误", "CAN ID不能为负数");
        canIdEdit->setFocus();
        return false;
    }

    if (canId > 0x1FFFFFFF) {
        QMessageBox::warning(this, "输入错误", "CAN ID范围：0x00000000 - 0x1FFFFFFF");
        canIdEdit->setFocus();
        return false;
    }

    return true;
}
