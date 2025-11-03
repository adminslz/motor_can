#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QTime>
#include <QMessageBox>
#include <QDebug>
#include <QApplication>
#include <QScreen>
#include <QTabWidget>
#include <QTimer>
#include <QStatusBar>
#include "global_vars.h"
#include "can_rx_tx.h"

// 全局指针用于日志重定向
static MotorDebug *g_motorDebug = nullptr;

// 自定义消息处理器
void customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_UNUSED(context)
    
    // 防止递归调用
    static bool isProcessing = false;
    if (isProcessing) {
        // 如果正在处理消息，直接输出到控制台避免递归
        fprintf(stderr, "Debug: %s\n", msg.toLocal8Bit().constData());
        return;
    }
    
    isProcessing = true;

    if (g_motorDebug) {
        switch (type) {
        case QtDebugMsg:
            g_motorDebug->logDebug(msg);
            break;
        case QtInfoMsg:
            g_motorDebug->logInfo(msg);
            break;
        case QtWarningMsg:
            g_motorDebug->logWarning(msg);
            break;
        case QtCriticalMsg:
            g_motorDebug->logError(msg);
            break;
        case QtFatalMsg:
            g_motorDebug->logError("FATAL: " + msg);
            break;
        }
    }

    // 同时输出到原始控制台（可选）
    QByteArray localMsg = msg.toLocal8Bit();
    switch (type) {
    case QtDebugMsg:
        fprintf(stderr, "Debug: %s\n", localMsg.constData());
        break;
    case QtInfoMsg:
        fprintf(stderr, "Info: %s\n", localMsg.constData());
        break;
    case QtWarningMsg:
        fprintf(stderr, "Warning: %s\n", localMsg.constData());
        break;
    case QtCriticalMsg:
        fprintf(stderr, "Critical: %s\n", localMsg.constData());
        break;
    case QtFatalMsg:
        fprintf(stderr, "Fatal: %s\n", localMsg.constData());
        break;
    }
    
    isProcessing = false;
}



MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , baseScaleFactor(1.0)
    , canthread(nullptr)
    , motorDebug(nullptr)
    , onlineStatusLabel(nullptr)
    , onlineStatusWidget(nullptr)
    , dataAcquisitionWidget(nullptr)
    , controlParamTab(nullptr)
    , motionControlTab(nullptr)
{
    qDebug() << "========== MainWindow 构造函数开始 ==========";
    //ui->setupUi(this);

    // 已删除调试日志系统，不再初始化
    // qDebug() << "【MainWindow】初始化调试日志系统...";
    // motorDebug = new MotorDebug(this);
    // g_motorDebug = motorDebug;
    // qDebug() << "【MainWindow】调试日志系统初始化完成";

    // 创建菜单栏和堆叠窗口
    // 已删除菜单栏，不再创建
    // qDebug() << "【MainWindow】创建菜单栏...";
    // createMenuBar();
    // qDebug() << "【MainWindow】菜单栏创建完成";
    
    qDebug() << "【MainWindow】创建堆叠窗口...";
    createStackedWidget();
    qDebug() << "【MainWindow】堆叠窗口创建完成";
    
    // 注意：setupUI() 被移到了 createStackedWidget() 内部
    qDebug() << "【MainWindow】设置字体样式...";
    set_word_style();
    qDebug() << "【MainWindow】字体样式设置完成";
    
    qDebug() << "【MainWindow】设置状态栏...";
    setupStatusBar();  // 初始化状态栏
    qDebug() << "【MainWindow】状态栏设置完成";

    // 设置窗口标志
    qDebug() << "【MainWindow】设置窗口标志和尺寸...";
    setWindowFlags(Qt::Window | Qt::WindowMinimizeButtonHint | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint);
    setMinimumSize(2000, 1200);
    qDebug() << "【MainWindow】窗口标志和尺寸设置完成";

    // CANThread将由can_init传递，不在此处创建

    // 初始状态为离线
    qDebug() << "【MainWindow】更新在线状态...";
    updateOnlineStatus(false);
    qDebug() << "【MainWindow】在线状态更新完成";
    
    qDebug() << "【MainWindow】调用setupUI...";
    setupUI();
    qDebug() << "【MainWindow】setupUI调用完成";
    
    // 暂时禁用自定义消息处理器来测试是否是它导致的崩溃
    qDebug() << "【MainWindow】跳过安装自定义消息处理器（调试模式）";
    // qInstallMessageHandler(customMessageHandler);
    qDebug() << "【MainWindow】继续执行...";
    
    // 测试日志输出
    qDebug() << "系统启动完成";
    qInfo() << "应用程序初始化成功";
    qDebug() << "========== MainWindow 构造函数结束 ==========";
}

void MainWindow::setupUI()
{
    qDebug() << "========== MainWindow::setupUI 开始 ==========";
    
    // 创建中央控件
    QWidget *centralWidget = new QWidget();
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->setSpacing(5);
    
    qDebug() << "【MainWindow】中央控件和主布局创建完成";

    // === 添加CAN ID配置区域 ===
    QWidget *canConfigWidget = new QWidget();
    canConfigWidget->setStyleSheet(
        "QWidget {"
        "    background-color: #3a3a3a;"
        "    border: 1px solid #555555;"
        "    border-radius: 6px;"
        "}"
    );
    QHBoxLayout *canConfigLayout = new QHBoxLayout(canConfigWidget);
    canConfigLayout->setContentsMargins(15, 10, 15, 10);
    canConfigLayout->setSpacing(10);

    // CAN ID标签
    QLabel *canIdLabel = new QLabel("CAN设备ID:");
    canIdLabel->setStyleSheet(
        "QLabel {"
        "    color: #ffffff;"
        "    font-size: 16px;"
        "    font-weight: bold;"
        "}"
    );

    // CAN ID输入框
    canIdSpinBox = new QSpinBox();
    canIdSpinBox->setRange(0, 127);  // 7位CAN ID范围：0-127
    canIdSpinBox->setValue(1);       // 默认值
    canIdSpinBox->setStyleSheet(
        "QSpinBox {"
        "    background-color: #454545;"
        "    color: white;"
        "    border: 1px solid #555555;"
        "    border-radius: 4px;"
        "    padding: 6px 10px;"
        "    font-size: 16px;"
        "    min-width: 80px;"
        "    max-width: 100px;"
        "}"
        "QSpinBox::up-button, QSpinBox::down-button {"
        "    background-color: #555555;"
        "    border: none;"
        "    width: 20px;"
        "}"
        "QSpinBox::up-button:hover, QSpinBox::down-button:hover {"
        "    background-color: #666666;"
        "}"
    );

    // 应用按钮
    QPushButton *applyCanIdBtn = new QPushButton("应用CAN ID");
    applyCanIdBtn->setStyleSheet(
        "QPushButton {"
        "    background-color: #2196f3;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 4px;"
        "    padding: 8px 16px;"
        "    font-weight: bold;"
        "    font-size: 14px;"
        "    min-height: 35px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #1976d2;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #1565c0;"
        "}"
    );

    // 连接应用按钮的信号槽
    connect(applyCanIdBtn, &QPushButton::clicked, this, &MainWindow::onApplyCanIdClicked);

    // 添加到布局
    canConfigLayout->addWidget(canIdLabel);
    canConfigLayout->addWidget(canIdSpinBox);
    canConfigLayout->addWidget(applyCanIdBtn);
    canConfigLayout->addStretch();  // 添加弹性空间

    // 创建选项卡控件
    QTabWidget *tabWidget = new QTabWidget();
    tabWidget->setDocumentMode(true);

    // 使用独立的类创建选项卡
    qDebug() << "========== 开始创建选项卡 ==========";
    
    qDebug() << "【MainWindow】创建运动控制选项卡...";
    motionControlTab = new MotionControl();
    qDebug() << "【MainWindow】运动控制选项卡创建成功";
    
    qDebug() << "【MainWindow】创建控制参数选项卡...";
    controlParamTab = new ControlParam();
    qDebug() << "【MainWindow】控制参数选项卡创建成功";
    
    qDebug() << "【MainWindow】创建电机参数选项卡...";
    MotorParam *motorParamTab = new MotorParam();
    qDebug() << "【MainWindow】电机参数选项卡创建成功";
    
    qDebug() << "【MainWindow】创建数据采集选项卡...";
    dataAcquisitionWidget = new DataAcquisition();
    qDebug() << "【MainWindow】数据采集选项卡创建成功";
    
    // 注意：CAN组件的设置会在setupDataAcquisition()中进行
    // 因为在setupUI()调用时，g_canTxRx可能还未创建

    // 添加选项卡
    qDebug() << "【MainWindow】开始添加选项卡到TabWidget...";
    tabWidget->addTab(motionControlTab, "运动控制");
    qDebug() << "【MainWindow】运动控制选项卡已添加";
    
    tabWidget->addTab(controlParamTab, "控制参数");
    qDebug() << "【MainWindow】控制参数选项卡已添加";
    
    tabWidget->addTab(motorParamTab, "电机参数");
    qDebug() << "【MainWindow】电机参数选项卡已添加";
    
    tabWidget->addTab(dataAcquisitionWidget, "数据采集");
    qDebug() << "【MainWindow】数据采集选项卡已添加";
    
    // 设置默认选项卡为"控制参数"（索引1）
    tabWidget->setCurrentIndex(1);
    qDebug() << "【MainWindow】默认选项卡已设置为: 控制参数";

//    // 连接控制参数与CAN发送（读取）
//    if (g_canTxRx && controlParamTab) {
//        connect(controlParamTab, &ControlParam::sdoReadRequest, this, [=](uint8_t nodeId, uint16_t index, uint8_t subindex){
//            g_canTxRx->sendParameterRead(nodeId, index, subindex);
//        });
    // 订阅CAN帧以解析SDO读取响应并回填到控制参数界面（主线程队列调用）
//        connect(g_canTxRx, &CANTxRx::frameReceived, this, [=](const VCI_CAN_OBJ &frame){
//            if ((frame.ID & 0xF80) != 0x580 || frame.DataLen < 8) return;

//            uint16_t index = static_cast<uint8_t>(frame.Data[1]) | (static_cast<uint8_t>(frame.Data[2]) << 8);
//            uint8_t sub = static_cast<uint8_t>(frame.Data[3]);
//            ODEntry od = controlParamTab->getParam(index, sub);

//            QVariant val;
//            if (od.type == OD_TYPE_FLOAT) {
//                float f = 0.0f; memcpy(&f, &frame.Data[4], 4); val = static_cast<double>(f);
//            } else if (od.type == OD_TYPE_INT16) {
//                int16_t v = 0; memcpy(&v, &frame.Data[4], 2); val = static_cast<int>(v);
//            } else if (od.type == OD_TYPE_UINT16) {
//                uint16_t v = 0; memcpy(&v, &frame.Data[4], 2); val = static_cast<int>(v);
//            } else if (od.type == OD_TYPE_UINT8) {
//                uint8_t v = static_cast<uint8_t>(frame.Data[4]); val = static_cast<int>(v);
//            } else if (od.type == OD_TYPE_BOOLEAN) {
//                uint8_t v = static_cast<uint8_t>(frame.Data[4]); val = (v != 0);
//            } else {
//                int32_t v = 0; memcpy(&v, &frame.Data[4], 4); val = static_cast<int>(v);
//            }

//            QMetaObject::invokeMethod(controlParamTab, [=]{
//                controlParamTab->updateParameterValue(index, sub, val);
//            }, Qt::QueuedConnection);
//        });

//        // 保留统一回填路径（不直接连接到ControlParam槽）
//    }

    // 添加到主布局
    qDebug() << "【MainWindow】开始添加控件到主布局...";
    mainLayout->addWidget(canConfigWidget);  // CAN ID配置区域
    qDebug() << "【MainWindow】CAN配置区域已添加";
    
    mainLayout->addWidget(tabWidget, 1);     // 选项卡区域（占据大部分空间）
    qDebug() << "【MainWindow】TabWidget已添加";

    // 已删除系统日志控件，不再添加

    setCentralWidget(centralWidget);
    qDebug() << "【MainWindow】中央控件已设置";
    qDebug() << "========== MainWindow::setupUI 完成 ==========";

    // 测试日志输出
    qDebug() << "系统启动完成";
    qInfo() << "应用程序初始化成功";
    
    // 调试信息：确认数据采集组件已初始化
    if (dataAcquisitionWidget) {
        qDebug() << "数据采集组件初始化成功";
    } else {
        qDebug() << "数据采集组件初始化失败！";
    }
}


// ... 其余代码保持不变 ...

//MainWindow::~MainWindow()
//{
//    delete ui;
//}



void MainWindow::onApplyCanIdClicked()
{
    int canId = canIdSpinBox->value();
    Can_id = (uint8_t)canId;
    // 这里可以添加CAN ID应用的逻辑
    // 例如：更新全局CAN ID配置、发送配置命令等

    qInfo() << "应用CAN ID:" << canId;

    // 示例：更新状态栏显示当前CAN ID
    QStatusBar *statusBar = this->statusBar();
    if (statusBar) {
        // 移除旧的CAN ID显示（如果存在）
        QList<QLabel*> labels = statusBar->findChildren<QLabel*>();
        for (QLabel *label : labels) {
            if (label->objectName() == "canIdLabel") {
                statusBar->removeWidget(label);
                label->deleteLater();
                break;
            }
        }

        // 添加新的CAN ID显示
        QLabel *canIdLabel = new QLabel(QString("CAN ID: %1").arg(canId));
        canIdLabel->setObjectName("canIdLabel");
        canIdLabel->setStyleSheet("QLabel { color: #888888; font-size: 12px; margin-left: 10px; }");
        statusBar->insertPermanentWidget(1, canIdLabel);  // 插入到版本信息之前
    }

    // 可以在这里调用CAN配置函数
    // setCANConfig(0, 1000, canId, false);
}


// 添加状态栏设置方法
void MainWindow::setupStatusBar()
{
    // 创建状态栏
    QStatusBar *statusBar = new QStatusBar(this);
    setStatusBar(statusBar);

    // 创建在线状态控件
    onlineStatusWidget = new QWidget();
    QHBoxLayout *statusLayout = new QHBoxLayout(onlineStatusWidget);
    statusLayout->setContentsMargins(8, 2, 8, 2);
    statusLayout->setSpacing(6);

    // 绿色圆点
    QLabel *dotLabel = new QLabel();
    dotLabel->setFixedSize(12, 12);
    dotLabel->setStyleSheet(
        "QLabel {"
        "    background-color: #f44336;"  // 初始为红色（离线）
        "    border-radius: 6px;"
        "    border: 1px solid #cccccc;"
        "}"
    );
    dotLabel->setObjectName("statusDot");

    // 在线文字
    onlineStatusLabel = new QLabel("离线");
    onlineStatusLabel->setStyleSheet(
        "QLabel {"
        "    color: #cccccc;"
        "    font-size: 12px;"
        "    font-weight: bold;"
        "}"
    );

    statusLayout->addWidget(dotLabel);
    statusLayout->addWidget(onlineStatusLabel);
    statusLayout->addStretch();

    // 添加到状态栏右侧
    statusBar->addPermanentWidget(onlineStatusWidget);

    // 添加其他状态信息（可选）
    QLabel *versionLabel = new QLabel("电机控制系统 v1.0");
    versionLabel->setStyleSheet("QLabel { color: #888888; font-size: 12px; }");
    statusBar->addPermanentWidget(versionLabel);
}

// 更新在线状态
void MainWindow::updateOnlineStatus(bool online)
{
    QLabel *dotLabel = onlineStatusWidget->findChild<QLabel*>("statusDot");
    if (dotLabel) {
        if (online) {
            dotLabel->setStyleSheet(
                "QLabel {"
                "    background-color: #4caf50;"  // 绿色
                "    border-radius: 6px;"
                "    border: 1px solid #388e3c;"
                "}"
            );
            onlineStatusLabel->setText("在线");
            onlineStatusLabel->setStyleSheet(
                "QLabel {"
                "    color: #4caf50;"
                "    font-size: 12px;"
                "    font-weight: bold;"
                "}"
            );
            qInfo() << "系统状态：在线";
        } else {
            dotLabel->setStyleSheet(
                "QLabel {"
                "    background-color: #f44336;"  // 红色
                "    border-radius: 6px;"
                "    border: 1px solid #d32f2f;"
                "}"
            );
            onlineStatusLabel->setText("离线");
            onlineStatusLabel->setStyleSheet(
                "QLabel {"
                "    color: #f44336;"
                "    font-size: 12px;"
                "    font-weight: bold;"
                "}"
            );
            qWarning() << "系统状态：离线";
        }
    }
}

// 设置CAN配置（可选实现）
void MainWindow::setCANConfig(int deviceIndex, int baudRate, int canId, bool extendedFrame)
{
    // 这里可以实现将配置参数传递给CAN线程
    qDebug() << "接收到CAN配置: 设备" << deviceIndex << ", 波特率" << baudRate << "Kbps";

    // 模拟连接成功，更新在线状态
    QTimer::singleShot(1000, this, [this]() {
        updateOnlineStatus(true);
    });

//    // 告知底层，直接回调控制参数页槽函数（类似 parseUpdateProtocol 的直接调用）
//    if (g_canTxRx) {
//        g_canTxRx->setControlParam(controlParamTab);
//    }
}

// 在打开设备的地方更新在线状态
void MainWindow::on_pushButton_clicked()
{
    if(ui->pushButton->text() == tr("打开设备"))
    {
        // 固定设备索引为0，固定波特率为1Mbps (1000)
        UINT deviceIndex = 0;  // 固定设备索引
        UINT baundRate = 1000; // 固定波特率为1Mbps

        bool dev = canthread->openDevice(4, deviceIndex, baundRate);
        if(dev == true)
        {
            ui->pushButton->setText(tr("关闭设备"));
            updateOnlineStatus(true);  // 更新为在线状态
        }
        else
        {
            QMessageBox::warning(this,"警告","打开设备失败！");
            updateOnlineStatus(false); // 保持离线状态
        }
    }
    else if(ui->pushButton->text() == tr("关闭设备"))
    {
        ui->pushButton->setText(tr("打开设备"));
        canthread->stop();
        canthread->closeDevice();
        updateOnlineStatus(false); // 更新为离线状态
    }
}



//QWidget* MainWindow::createDataMonitorTab()
//{
//    QWidget *tab = new QWidget();
//    QVBoxLayout *mainLayout = new QVBoxLayout(tab);

//    // 控制按钮区域
//    QWidget *controlWidget = new QWidget();
//    QHBoxLayout *controlLayout = new QHBoxLayout(controlWidget);

//    QPushButton *startBtn = new QPushButton("开始采集");
//    QPushButton *stopBtn = new QPushButton("停止采集");
//    QPushButton *clearBtn = new QPushButton("清空数据");


//    // ... 其余代码保持不变 ...



//    return tab;
//}



void MainWindow::onGetProtocolData(VCI_CAN_OBJ *vci, unsigned int dwRel, unsigned int channel)
{
    QList<VCI_CAN_OBJ> frames;
    
    for(unsigned int i = 0; i < dwRel; i++)
    {
        QString dataStr = "";
        if(vci[i].RemoteFlag == 0)
        {
            for(int j = 0; j < vci[i].DataLen; j++)
                dataStr += QString::number(vci[i].Data[j], 16).toUpper() + " ";
        }

//        // 打印原始数据
//        qDebug() << QString("[%1] CH%2 接收 ID:0x%3 %4 %5 长度:%6 数据:%7")
//                    .arg(QTime::currentTime().toString("hh:mm:ss"))
//                    .arg(channel)
//                    .arg(vci[i].ID, 8, 16, QChar('0'))
//                    .arg(vci[i].ExternFlag == 1 ? "扩展帧" : "标准帧")
//                    .arg(vci[i].RemoteFlag == 1 ? "远程帧" : "数据帧")
//                    .arg(vci[i].DataLen)
//                    .arg(dataStr);
        
        // 添加到帧列表
        frames.append(vci[i]);
        
        // 数据采集组件的处理已移至can_rx_tx.cpp中，避免重复处理
    }
    
    // 将接收到的帧传递给CANTxRx处理
    if (g_canTxRx && !frames.isEmpty()) {
        g_canTxRx->processReceivedFrames(frames);
    }
    
//    // 将CAN帧传递给数据采集组件的示波器
//    if (dataAcquisitionWidget && !frames.isEmpty()) {
//        for (const VCI_CAN_OBJ &frame : frames) {
//            dataAcquisitionWidget->onCANFrameReceived(frame);
//        }
//    }
}

QString versionStr(USHORT ver)
{
    return "V" + QString::number((ver & 0x0FFF) /0x100,16).toUpper() + "." + QString("%1 ").arg((ver & 0x0FFF) % 0x100,2,16,QChar('0')).toUpper();
}



void MainWindow::on_open_close_button_clicked()
{


}

//void MainWindow::on_pushButton_clicked()
//{
//    if(ui->pushButton->text() == tr("打开设备"))
//    {
//        // 固定设备索引为0，固定波特率为1Mbps (1000)
//        UINT deviceIndex = 0;  // 固定设备索引
//        UINT baundRate = 1000; // 固定波特率为1Mbps

//        bool dev = canthread->openDevice(4, deviceIndex, baundRate);
//        if(dev == true)
//        {

//            ui->pushButton->setText(tr("关闭设备"));
//        }
//        else
//            QMessageBox::warning(this,"警告","打开设备失败！");
//    }
//    else if(ui->pushButton->text() == tr("关闭设备"))
//    {
//        ui->pushButton->setText(tr("打开设备"));
//        canthread->stop();
//        canthread->closeDevice();
//    }
//}



void MainWindow::set_word_style()
{
    // 设置全局样式表 - 白色圆润字体
    setStyleSheet(R"(
        QMainWindow {
            background-color: #2d2d2d;
            color: white;
        }
        QWidget {
            background-color: #2d2d2d;
            color: white;
            font-family: "Segoe UI", "Microsoft YaHei", "PingFang SC", sans-serif;
            font-size: 13pt;
        }
        QTabWidget::pane {
            border: 1px solid #404040;
            background-color: #2d2d2d;
        }
        QTabBar::tab {
            background-color: #3d3d3d;
            color: #cccccc;  /* 默认字体颜色改为浅灰色 */
            padding: 8px 16px;
            border: none;
            border-radius: 4px;
            margin-right: 2px;
            font-weight: 500;
        }
        QTabBar::tab:selected {
            background-color: #4a4a4a;  /* 选中时背景变浅 */
            color: #0078d4;             /* 选中时字体变蓝色 */
            border-bottom: 2px solid #0078d4; /* 可选：添加底部边框强调 */
        }
        QTabBar::tab:hover:!selected {
            background-color: #4d4d4d;
            color: white;  /* 悬停时字体变白 */
        }
        // ... 其余样式保持不变 ...
    )");
}

void MainWindow::createMenuBar()
{
    // 创建菜单栏
    menuBar = new QMenuBar(this);
    setMenuBar(menuBar);
    
    // 创建菜单项
    homeAction = new QAction("主页", this);
    controlAction = new QAction("控制", this);
    monitorAction = new QAction("监测", this);
    settingsAction = new QAction("设置", this);
    
    // 设置菜单项样式
    QString menuStyle = "QMenuBar { background-color: #3C3C3C; color: white; border-bottom: 1px solid #555; }"
                       "QMenuBar::item { background-color: transparent; padding: 8px 16px; font-size: 16px; }"
                       "QMenuBar::item:selected { background-color: #555; }"
                       "QMenuBar::item:pressed { background-color: #666; }";
    menuBar->setStyleSheet(menuStyle);
    
    // 添加菜单项到菜单栏
    menuBar->addAction(homeAction);
    menuBar->addAction(controlAction);
    menuBar->addAction(monitorAction);
    menuBar->addAction(settingsAction);
    
    // 连接信号和槽
    connect(homeAction, &QAction::triggered, this, &MainWindow::showHomePage);
    connect(controlAction, &QAction::triggered, this, &MainWindow::showControlPage);
    connect(monitorAction, &QAction::triggered, this, &MainWindow::showMonitorPage);
    connect(settingsAction, &QAction::triggered, this, &MainWindow::showSettingsPage);
    
    // 默认选中主页
    homeAction->setCheckable(true);
    controlAction->setCheckable(true);
    monitorAction->setCheckable(true);
    settingsAction->setCheckable(true);
    homeAction->setChecked(true);
}

void MainWindow::createStackedWidget()
{
    // 创建堆叠窗口，但不立即设置为中央控件
    stackedWidget = new QStackedWidget(this);
    
    // 创建其他页面（非主页）
    QWidget *controlPage = new QWidget();
    controlPage->setStyleSheet("background-color: #2d2d2d; color: white;");
    QVBoxLayout *controlLayout = new QVBoxLayout(controlPage);
    QLabel *controlTitle = new QLabel("控制页面");
    controlTitle->setStyleSheet("font-size: 18px; font-weight: bold; margin: 20px;");
    controlTitle->setAlignment(Qt::AlignCenter);
    controlLayout->addWidget(controlTitle);
    controlLayout->addStretch();
    
    QWidget *monitorPage = new QWidget();
    monitorPage->setStyleSheet("background-color: #2d2d2d; color: white;");
    QVBoxLayout *monitorLayout = new QVBoxLayout(monitorPage);
    QLabel *monitorTitle = new QLabel("监测页面");
    monitorTitle->setStyleSheet("font-size: 18px; font-weight: bold; margin: 20px;");
    monitorTitle->setAlignment(Qt::AlignCenter);
    monitorLayout->addWidget(monitorTitle);
    monitorLayout->addStretch();
    
    QWidget *settingsPage = new QWidget();
    settingsPage->setStyleSheet("background-color: #2d2d2d; color: white;");
    QVBoxLayout *settingsLayout = new QVBoxLayout(settingsPage);
    QLabel *settingsTitle = new QLabel("设置页面");
    settingsTitle->setStyleSheet("font-size: 18px; font-weight: bold; margin: 20px;");
    settingsTitle->setAlignment(Qt::AlignCenter);
    settingsLayout->addWidget(settingsTitle);
    settingsLayout->addStretch();
    
    // 添加页面到堆叠窗口（主页稍后添加）
    stackedWidget->addWidget(controlPage);
    stackedWidget->addWidget(monitorPage);
    stackedWidget->addWidget(settingsPage);
    
    // 默认显示第一个页面
    stackedWidget->setCurrentIndex(0);
}

void MainWindow::showHomePage()
{
    // 主页显示原来的界面，不需要切换页面
    // 因为原来的界面就是中央控件
    homeAction->setChecked(true);
    controlAction->setChecked(false);
    monitorAction->setChecked(false);
    settingsAction->setChecked(false);
}

void MainWindow::showControlPage()
{
    stackedWidget->setCurrentIndex(0);
    homeAction->setChecked(false);
    controlAction->setChecked(true);
    monitorAction->setChecked(false);
    settingsAction->setChecked(false);
}

void MainWindow::showMonitorPage()
{
    stackedWidget->setCurrentIndex(1);
    homeAction->setChecked(false);
    controlAction->setChecked(false);
    monitorAction->setChecked(true);
    settingsAction->setChecked(false);
}

void MainWindow::showSettingsPage()
{
    stackedWidget->setCurrentIndex(2);
    homeAction->setChecked(false);
    controlAction->setChecked(false);
    monitorAction->setChecked(false);
    settingsAction->setChecked(true);
}

void MainWindow::setupDataAcquisition()
{
    if (dataAcquisitionWidget && g_canTxRx) {
        // 设置CAN组件到数据采集组件
        dataAcquisitionWidget->setCANTxRx(g_canTxRx);
        qDebug() << "✅ CAN组件已设置到数据采集组件";
        
        // 设置数据采集组件给CANTxRx
        g_canTxRx->setDataAcquisition(dataAcquisitionWidget);
        qDebug() << "✅ 数据采集组件已设置给CANTxRx";
    } else {
        if (!dataAcquisitionWidget) {
            qDebug() << "❌ 数据采集组件为空，无法设置";
        }
        if (!g_canTxRx) {
            qDebug() << "❌ 全局CANTxRx为空，无法设置数据采集组件";
        }
    }
    
    // 设置ControlParam到CANTxRx，用于接收SDO响应（在g_canTxRx创建后调用）
    if (controlParamTab && g_canTxRx) {
        g_canTxRx->setControlParam(controlParamTab);
        qDebug() << "✅ ControlParam已设置到CANTxRx，可以接收SDO响应";
    } else {
        if (!controlParamTab) {
            qDebug() << "❌ ControlParam组件为空，无法设置";
        }
        if (!g_canTxRx) {
            qDebug() << "❌ 全局CANTxRx为空，无法设置ControlParam";
        }
    }
    
    // 设置MotionControl的信号连接，用于更新电机状态显示（在g_canTxRx创建后调用）
    if (motionControlTab && g_canTxRx) {
        // 连接已经在MotionControl::setupConnections()中建立，但如果g_canTxRx当时为空，需要重新连接
        motionControlTab->setupConnections();
        qDebug() << "✅ MotionControl已连接CAN状态数据";
    } else {
        if (!motionControlTab) {
            qDebug() << "❌ MotionControl组件为空，无法设置";
        }
        if (!g_canTxRx) {
            qDebug() << "❌ 全局CANTxRx为空，无法设置MotionControl";
        }
    }
}

void MainWindow::setCANThread(CANThread* thread)
{
    if (thread) {
        canthread = thread;
        connect(canthread, &CANThread::getProtocolData, this, &MainWindow::onGetProtocolData);
        qDebug() << "✅ CAN线程已设置到MainWindow";
    } else {
        qWarning() << "❌ 传入的CAN线程为空";
    }
}

MainWindow::~MainWindow()
{
    // 停止CAN线程（但不删除，因为它由can_init管理）
    if (canthread) {
        canthread->stop();
        canthread->closeDevice();
        // 不删除canthread，因为它由can_init管理
        canthread = nullptr;
    }
    
    // 清理数据采集组件
    if (dataAcquisitionWidget) {
        dataAcquisitionWidget->stopAcquisition();
        delete dataAcquisitionWidget;
        dataAcquisitionWidget = nullptr;
    }
    
    // 已删除调试日志系统，不再清理
    // if (motorDebug) {
    //     delete motorDebug;
    //     motorDebug = nullptr;
    // }
    
    // 清理UI组件
    if (ui) {
        delete ui;
        ui = nullptr;
    }
    
    qDebug() << "MainWindow 析构函数执行完成，资源已清理";
}

