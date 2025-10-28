#include "motion_contr_current.h"
#include <QDebug>
#include "can_rx_tx.h"  // 包含CAN收发封装头文件

MotionControlCurrent::MotionControlCurrent(QWidget *parent)
    : QWidget(parent)
    , targetIqSpin(nullptr)
    , targetIdSpin(nullptr)
    , currentKpSpin(nullptr)
    , currentKiSpin(nullptr)
    , startStopBtn(nullptr)
    , modeSwitchBtn(nullptr)
    , isRunning(false)
{
    setupUI();
}

void MotionControlCurrent::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(8);

    // 电流模式参数区域
    QWidget *paramsWidget = new QWidget();
    paramsWidget->setStyleSheet(
        "QWidget {"
        "    background-color: #3a3a3a;"
        "    border: 1px solid #555555;"
        "    border-radius: 6px;"
        "}"
    );
    QVBoxLayout *paramsLayout = new QVBoxLayout(paramsWidget);
    paramsLayout->setContentsMargins(10, 10, 10, 10);
    paramsLayout->setSpacing(8);

    // 标题
    QLabel *titleLabel = new QLabel("电流模式控制参数");
    titleLabel->setStyleSheet(
        "QLabel {"
        "    color: #ff9800;"
        "    font-size: 25px;"
        "    font-weight: bold;"
        "    padding-bottom: 6px;"
        "}"
    );

    // 参数表单
    QWidget *formWidget = new QWidget();
    QFormLayout *formLayout = new QFormLayout(formWidget);
    formLayout->setVerticalSpacing(8);
    formLayout->setHorizontalSpacing(10);
    formLayout->setLabelAlignment(Qt::AlignRight);

    // 标签样式
    QString labelStyle = "QLabel { color: #ffffff; font-size: 20px; font-weight: bold; }";

    // 输入框样式
    QString spinBoxStyle =
        "QDoubleSpinBox {"
        "    background-color: #454545;"
        "    color: white;"
        "    border: 1px solid #555555;"
        "    border-radius: 4px;"
        "    padding: 6px;"
        "    min-width: 80px;"
        "    max-width: 120px;"
        "    font-size: 20px;"
        "}";

    // 单位标签样式
    QString unitLabelStyle = "QLabel { color: #cccccc; font-size: 18px; padding-left: 5px; }";

    // 目标Iq电流
    QLabel *targetIqLabel = new QLabel("目标Iq电流:");
    targetIqLabel->setStyleSheet(labelStyle);
    targetIqSpin = new QDoubleSpinBox();
    targetIqSpin->setRange(-30.00, 30.00);
    targetIqSpin->setValue(0.5);
    targetIqSpin->setDecimals(2);
    targetIqSpin->setSingleStep(0.1);
    targetIqSpin->setStyleSheet(spinBoxStyle);

    QLabel *targetIqUnit = new QLabel("A");
    targetIqUnit->setStyleSheet(unitLabelStyle);

    QHBoxLayout *targetIqLayout = new QHBoxLayout();
    targetIqLayout->setContentsMargins(0, 0, 0, 0);
    targetIqLayout->setSpacing(5);
    targetIqLayout->addWidget(targetIqSpin);
    targetIqLayout->addWidget(targetIqUnit);
    targetIqLayout->addStretch();

    // 目标Id电流
    QLabel *targetIdLabel = new QLabel("目标Id电流:");
    targetIdLabel->setStyleSheet(labelStyle);
    targetIdSpin = new QDoubleSpinBox();
    targetIdSpin->setRange(-30.00, 30.00);
    targetIdSpin->setValue(0.0);
    targetIdSpin->setDecimals(2);
    targetIdSpin->setSingleStep(0.1);
    targetIdSpin->setStyleSheet(spinBoxStyle);

    QLabel *targetIdUnit = new QLabel("A");
    targetIdUnit->setStyleSheet(unitLabelStyle);

    QHBoxLayout *targetIdLayout = new QHBoxLayout();
    targetIdLayout->setContentsMargins(0, 0, 0, 0);
    targetIdLayout->setSpacing(5);
    targetIdLayout->addWidget(targetIdSpin);
    targetIdLayout->addWidget(targetIdUnit);
    targetIdLayout->addStretch();

    // 电流环Kp
    QLabel *currentKpLabel = new QLabel("电流环Kp:");
    currentKpLabel->setStyleSheet(labelStyle);
    currentKpSpin = new QDoubleSpinBox();
    currentKpSpin->setRange(0, 1000);
    currentKpSpin->setValue(80);
    currentKpSpin->setDecimals(2);
    currentKpSpin->setSingleStep(5);
    currentKpSpin->setStyleSheet(spinBoxStyle);

    QLabel *currentKpUnit = new QLabel("");
    currentKpUnit->setStyleSheet(unitLabelStyle);

    QHBoxLayout *currentKpLayout = new QHBoxLayout();
    currentKpLayout->setContentsMargins(0, 0, 0, 0);
    currentKpLayout->setSpacing(5);
    currentKpLayout->addWidget(currentKpSpin);
    currentKpLayout->addWidget(currentKpUnit);
    currentKpLayout->addStretch();

    // 电流环Ki
    QLabel *currentKiLabel = new QLabel("电流环Ki:");
    currentKiLabel->setStyleSheet(labelStyle);
    currentKiSpin = new QDoubleSpinBox();
    currentKiSpin->setRange(0, 100);
    currentKiSpin->setValue(2.5);
    currentKiSpin->setDecimals(3);
    currentKiSpin->setSingleStep(0.1);
    currentKiSpin->setStyleSheet(spinBoxStyle);

    QLabel *currentKiUnit = new QLabel("");
    currentKiUnit->setStyleSheet(unitLabelStyle);

    QHBoxLayout *currentKiLayout = new QHBoxLayout();
    currentKiLayout->setContentsMargins(0, 0, 0, 0);
    currentKiLayout->setSpacing(5);
    currentKiLayout->addWidget(currentKiSpin);
    currentKiLayout->addWidget(currentKiUnit);
    currentKiLayout->addStretch();

    // 添加到表单布局
    formLayout->addRow(targetIqLabel, targetIqLayout);
    formLayout->addRow(targetIdLabel, targetIdLayout);
    formLayout->addRow(currentKpLabel, currentKpLayout);
    formLayout->addRow(currentKiLabel, currentKiLayout);

    // 应用按钮
    QPushButton *applyBtn = new QPushButton("应用参数");
    applyBtn->setStyleSheet(
        "QPushButton {"
        "    background-color: #ff9800;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 4px;"
        "    padding: 8px 16px;"
        "    font-weight: bold;"
        "    font-size: 25px;"
        "    min-height: 35px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #f57c00;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #ef6c00;"
        "}"
    );

    paramsLayout->addWidget(titleLabel);
    paramsLayout->addWidget(formWidget);
    paramsLayout->addWidget(applyBtn, 0, Qt::AlignRight);

    // 分隔符
    QFrame *separator = new QFrame();
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);
    separator->setStyleSheet("QFrame { color: #555555; margin: 8px 0px; }");
    separator->setMinimumHeight(2);
    separator->setMaximumHeight(2);

    // 控制按钮区域
    QWidget *controlWidget = new QWidget();
    controlWidget->setStyleSheet(
        "QWidget {"
        "    background-color: #3a3a3a;"
        "    border: 1px solid #555555;"
        "    border-radius: 6px;"
        "}"
    );
    QVBoxLayout *controlLayout = new QVBoxLayout(controlWidget);
    controlLayout->setContentsMargins(10, 10, 10, 10);
    controlLayout->setSpacing(8);

    // 运行使能控制标题
    QLabel *controlTitleLabel = new QLabel("运行使能控制");
    controlTitleLabel->setStyleSheet(
        "QLabel {"
        "    color: #ff9800;"
        "    font-size: 25px;"
        "    font-weight: bold;"
        "    padding-bottom: 6px;"
        "}"
    );

    // 按钮容器
    QWidget *buttonContainer = new QWidget();
    QHBoxLayout *buttonLayout = new QHBoxLayout(buttonContainer);
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->setSpacing(15);

    // 电流模式切换按钮
    modeSwitchBtn = new QPushButton("⭮");
    modeSwitchBtn->setStyleSheet(
        "QPushButton {"
        "    background-color: #3a3a3a;"
        "    color: #ffffff;"
        "    border: 1px solid #555555;"
        "    border-radius: 6px;"
        "    padding: 15px 20px;"
        "    font-weight: bold;"
        "    font-size: 40px;"
        "    min-height: 60px;"
        "    min-width: 80px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #454545;"
        "    border: 1px solid #666666;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #2e2e2e;"
        "}"
    );

    // 启动停止按钮
    startStopBtn = new QPushButton("▶");
    startStopBtn->setStyleSheet(
        "QPushButton {"
        "    background-color: #3a3a3a;"
        "    color: #ffffff;"
        "    border: 1px solid #555555;"
        "    border-radius: 6px;"
        "    padding: 15px 20px;"
        "    font-weight: bold;"
        "    font-size: 40px;"
        "    min-height: 60px;"
        "    min-width: 80px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #454545;"
        "    border: 1px solid #666666;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #2e2e2e;"
        "}"
    );

    // 按钮标签容器
    QWidget *labelContainer = new QWidget();
    QHBoxLayout *labelLayout = new QHBoxLayout(labelContainer);
    labelLayout->setContentsMargins(0, 0, 0, 0);
    labelLayout->setSpacing(15);

    QLabel *modeLabel = new QLabel("电流模式切换");
    modeLabel->setStyleSheet(
        "QLabel {"
        "    color: #cccccc;"
        "    font-size: 14px;"
        "    text-align: center;"
        "}"
    );

    QLabel *startStopTextLabel = new QLabel("电机启停");
    startStopTextLabel->setStyleSheet(
        "QLabel {"
        "    color: #cccccc;"
        "    font-size: 14px;"
        "    text-align: center;"
        "}"
    );

    // 添加到按钮布局
    buttonLayout->addWidget(modeSwitchBtn);
    buttonLayout->addWidget(startStopBtn);

    // 添加到标签布局
    labelLayout->addWidget(modeLabel);
    labelLayout->addWidget(startStopTextLabel);

    // 添加到控制布局
    controlLayout->addWidget(controlTitleLabel);
    controlLayout->addWidget(buttonContainer, 0, Qt::AlignCenter);
    controlLayout->addWidget(labelContainer, 0, Qt::AlignCenter);

    // 添加到主布局
    mainLayout->addWidget(paramsWidget);
    mainLayout->addWidget(separator);
    mainLayout->addWidget(controlWidget);

    // 连接信号槽
    connect(applyBtn, &QPushButton::clicked, this, &MotionControlCurrent::onApplyButtonClicked);
    connect(startStopBtn, &QPushButton::clicked, this, &MotionControlCurrent::onStartStopButtonClicked);
    connect(modeSwitchBtn, &QPushButton::clicked, this, &MotionControlCurrent::onModeSwitchButtonClicked);
}

// 获取参数值
double MotionControlCurrent::getTargetIqCurrent() const
{
    if (targetIqSpin) {
        return targetIqSpin->value();
    }
    return 0.0;
}

double MotionControlCurrent::getTargetIdCurrent() const
{
    if (targetIdSpin) {
        return targetIdSpin->value();
    }
    return 0.0;
}

double MotionControlCurrent::getCurrentKp() const
{
    if (currentKpSpin) {
        return currentKpSpin->value();
    }
    return 0.0;
}

double MotionControlCurrent::getCurrentKi() const
{
    if (currentKiSpin) {
        return currentKiSpin->value();
    }
    return 0.0;
}

// 设置参数值
void MotionControlCurrent::setTargetIqCurrent(double iq)
{
    if (targetIqSpin) {
        targetIqSpin->setValue(iq);
    }
}

void MotionControlCurrent::setTargetIdCurrent(double id)
{
    if (targetIdSpin) {
        targetIdSpin->setValue(id);
    }
}

void MotionControlCurrent::setCurrentKp(double kp)
{
    if (currentKpSpin) {
        currentKpSpin->setValue(kp);
    }
}

void MotionControlCurrent::setCurrentKi(double ki)
{
    if (currentKiSpin) {
        currentKiSpin->setValue(ki);
    }
}

void MotionControlCurrent::onApplyButtonClicked()
{
    double targetIq = getTargetIqCurrent();
    double targetId = getTargetIdCurrent();

    qDebug() << "应用电流模式参数:"
             << "目标Iq电流:" << targetIq << "A,"
             << "目标Id电流:" << targetId << "A,"
             << "电流环Kp:" << getCurrentKp() << ","
             << "电流环Ki:" << getCurrentKi();

    // 发送电流指令到CAN（只发送Iq和Id电流）
    if (g_canTxRx && g_canTxRx->isDeviceReady()) {
        bool success = g_canTxRx->sendCurrentCommand(static_cast<float>(targetIq),
                                                   static_cast<float>(targetId));
        if (success) {
            qDebug() << "电流指令发送成功 - Iq:" << targetIq << "A, Id:" << targetId << "A";
        } else {
            qDebug() << "电流指令发送失败:" << g_canTxRx->getLastError();
        }
    } else {
        qDebug() << "CAN设备未就绪，无法发送电流指令";
    }

    emit parametersApplied();
}



void MotionControlCurrent::onStartStopButtonClicked()
{
    if (!isRunning) {
        // 切换到运行状态
        isRunning = true;
        startStopBtn->setText("■");
        QFont font = startStopBtn->font();
        font.setPointSize(40);
        startStopBtn->setFont(font);
        startStopBtn->setStyleSheet(
            "QPushButton {"
            "    background-color: #3a3a3a;"
            "    color: #f44336;"
            "    border: 1px solid #555555;"
            "    border-radius: 6px;"
            "    padding: 15px 20px;"
            "    font-weight: bold;"
            "    font-size: 40px;"
            "    min-height: 60px;"
            "    min-width: 80px;"
            "}"
            "QPushButton:hover {"
            "    background-color: #454545;"
            "    border: 1px solid #666666;"
            "}"
            "QPushButton:pressed {"
            "    background-color: #2e2e2e;"
            "}"
        );
        qDebug() << "启动按钮被点击 - 开始运行";

        // 发送启动CAN指令
        if (g_canTxRx && g_canTxRx->isDeviceReady()) {
            bool success = g_canTxRx->sendStartCommand();
            if (success) {
                qDebug() << "启动指令发送成功";
            } else {
                qDebug() << "启动指令发送失败:" << g_canTxRx->getLastError();
            }
        } else {
            qDebug() << "CAN设备未就绪，无法发送启动指令";
        }

        emit startButtonClicked();
    } else {
        // 切换到停止状态
        isRunning = false;
        startStopBtn->setText("▶");
        QFont font = startStopBtn->font();
        font.setPointSize(40);
        startStopBtn->setFont(font);
        startStopBtn->setStyleSheet(
            "QPushButton {"
            "    background-color: #3a3a3a;"
            "    color: #4caf50;"
            "    border: 1px solid #555555;"
            "    border-radius: 6px;"
            "    padding: 15px 20px;"
            "    font-weight: bold;"
            "    font-size: 40px;"
            "    min-height: 60px;"
            "    min-width: 80px;"
            "}"
            "QPushButton:hover {"
            "    background-color: #454545;"
            "    border: 1px solid #666666;"
            "}"
            "QPushButton:pressed {"
            "    background-color: #2e2e2e;"
            "}"
        );
        qDebug() << "停止按钮被点击 - 停止运行";

        // 发送停止CAN指令
        if (g_canTxRx && g_canTxRx->isDeviceReady()) {
            bool success = g_canTxRx->sendStopCommand();
            if (success) {
                qDebug() << "停止指令发送成功";
            } else {
                qDebug() << "停止指令发送失败:" << g_canTxRx->getLastError();
            }
        } else {
            qDebug() << "CAN设备未就绪，无法发送停止指令";
        }

        emit stopButtonClicked();
    }
}

void MotionControlCurrent::onModeSwitchButtonClicked()
{
    qDebug() << "电流模式切换按钮被点击";

    // 使用封装的CAN发送函数
    if (g_canTxRx && g_canTxRx->isDeviceReady()) {
        bool success = g_canTxRx->sendTorqueModeSwitch();
        if (success) {
            qDebug() << "电流模式切换指令已发送，等待回复...";
        } else {
            qDebug() << "电流模式切换指令发送失败:" << g_canTxRx->getLastError();
        }
    } else {
        qDebug() << "CAN设备未就绪，无法发送电流模式切换指令";
    }

    emit modeSwitchButtonClicked();
}

void MotionControlCurrent::setRunningState(bool running)
{
    if (running != isRunning) {
        onStartStopButtonClicked();
    }
}

bool MotionControlCurrent::getRunningState() const
{
    return isRunning;
}
