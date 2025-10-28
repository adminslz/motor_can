#include "motion_contr_speed.h"
#include <QDebug>
#include "can_rx_tx.h"  // 包含CAN收发封装头文件
#include "global_vars.h"

MotionControlSpeed::MotionControlSpeed(QWidget *parent)
    : QWidget(parent)
    , targetSpeedSpin(nullptr)
    , maxTorqueSpin(nullptr)
    , accelerationSpin(nullptr)
    , decelerationSpin(nullptr)
    , startStopBtn(nullptr)
    , modeSwitchBtn(nullptr)
    , jogForwardBtn(nullptr)
    , jogBackwardBtn(nullptr)
    , isRunning(false)
{
    setupUI();
}

void MotionControlSpeed::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(8);

    // 速度模式参数区域
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
    QLabel *titleLabel = new QLabel("速度模式控制参数");
    titleLabel->setStyleSheet(
        "QLabel {"
        "    color: #4fc3f7;"
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
        "    min-width: 80px;"  // 减小最小宽度
        "    max-width: 120px;" // 限制最大宽度
        "    font-size: 20px;"
        "}";

    // 单位标签样式
    QString unitLabelStyle = "QLabel { color: #cccccc; font-size: 18px; padding-left: 5px; }";

    // 目标速度
    QLabel *targetSpeedLabel = new QLabel("目标速度:");
    targetSpeedLabel->setStyleSheet(labelStyle);
    targetSpeedSpin = new QDoubleSpinBox();
    targetSpeedSpin->setRange(-1000, 1000);
    targetSpeedSpin->setValue(100);
    targetSpeedSpin->setSuffix(""); // 移除后缀
    targetSpeedSpin->setStyleSheet(spinBoxStyle);

    QLabel *targetSpeedUnit = new QLabel("rad/s");
    targetSpeedUnit->setStyleSheet(unitLabelStyle);

    QHBoxLayout *targetSpeedLayout = new QHBoxLayout();
    targetSpeedLayout->setContentsMargins(0, 0, 0, 0);
    targetSpeedLayout->setSpacing(5);
    targetSpeedLayout->addWidget(targetSpeedSpin);
    targetSpeedLayout->addWidget(targetSpeedUnit);
    targetSpeedLayout->addStretch();

    // 最大扭矩
    QLabel *maxTorqueLabel = new QLabel("最大扭矩:");
    maxTorqueLabel->setStyleSheet(labelStyle);
    maxTorqueSpin = new QDoubleSpinBox();
    maxTorqueSpin->setRange(0, 1000);
    maxTorqueSpin->setValue(50);
    maxTorqueSpin->setSuffix(""); // 移除后缀
    maxTorqueSpin->setStyleSheet(spinBoxStyle);

    QLabel *maxTorqueUnit = new QLabel("Nm");
    maxTorqueUnit->setStyleSheet(unitLabelStyle);

    QHBoxLayout *maxTorqueLayout = new QHBoxLayout();
    maxTorqueLayout->setContentsMargins(0, 0, 0, 0);
    maxTorqueLayout->setSpacing(5);
    maxTorqueLayout->addWidget(maxTorqueSpin);
    maxTorqueLayout->addWidget(maxTorqueUnit);
    maxTorqueLayout->addStretch();

    // 加速度
    QLabel *accelerationLabel = new QLabel("加速度:");
    accelerationLabel->setStyleSheet(labelStyle);
    accelerationSpin = new QDoubleSpinBox();
    accelerationSpin->setRange(0, 10000);
    accelerationSpin->setValue(1000);
    accelerationSpin->setSuffix(""); // 移除后缀
    accelerationSpin->setStyleSheet(spinBoxStyle);

    QLabel *accelerationUnit = new QLabel("rad/s²");
    accelerationUnit->setStyleSheet(unitLabelStyle);

    QHBoxLayout *accelerationLayout = new QHBoxLayout();
    accelerationLayout->setContentsMargins(0, 0, 0, 0);
    accelerationLayout->setSpacing(5);
    accelerationLayout->addWidget(accelerationSpin);
    accelerationLayout->addWidget(accelerationUnit);
    accelerationLayout->addStretch();

    // 减速度
    QLabel *decelerationLabel = new QLabel("减速度:");
    decelerationLabel->setStyleSheet(labelStyle);
    decelerationSpin = new QDoubleSpinBox();
    decelerationSpin->setRange(0, 10000);
    decelerationSpin->setValue(1000);
    decelerationSpin->setSuffix(""); // 移除后缀
    decelerationSpin->setStyleSheet(spinBoxStyle);

    QLabel *decelerationUnit = new QLabel("rad/s²");
    decelerationUnit->setStyleSheet(unitLabelStyle);

    QHBoxLayout *decelerationLayout = new QHBoxLayout();
    decelerationLayout->setContentsMargins(0, 0, 0, 0);
    decelerationLayout->setSpacing(5);
    decelerationLayout->addWidget(decelerationSpin);
    decelerationLayout->addWidget(decelerationUnit);
    decelerationLayout->addStretch();

    // 添加到表单布局
    formLayout->addRow(targetSpeedLabel, targetSpeedLayout);
    formLayout->addRow(maxTorqueLabel, maxTorqueLayout);
    formLayout->addRow(accelerationLabel, accelerationLayout);
    formLayout->addRow(decelerationLabel, decelerationLayout);

    // 应用按钮
    QPushButton *applyBtn = new QPushButton("应用参数");
    applyBtn->setStyleSheet(
        "QPushButton {"
        "    background-color: #4caf50;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 4px;"
        "    padding: 8px 16px;"
        "    font-weight: bold;"
        "    font-size: 25px;"
        "    min-height: 35px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #388e3c;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #2e7d32;"
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

    // 运动控制面板 - 改为与速度模式相同的格式
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

    // 运行使能控制标题 - 与速度模式格式相同
    QLabel *controlTitleLabel = new QLabel("运行使能控制");
    controlTitleLabel->setStyleSheet(
        "QLabel {"
        "    color: #4fc3f7;"
        "    font-size: 25px;"
        "    font-weight: bold;"
        "    padding-bottom: 6px;"
        "}"
    );

    // 使用水平布局包含启动停止区和点动区
    QHBoxLayout *controlMainLayout = new QHBoxLayout();
    controlMainLayout->setSpacing(20); // 增加间距
    controlMainLayout->setContentsMargins(0, 0, 0, 0);

    // 启动停止区域
    QWidget *startStopWidget = new QWidget();
    QVBoxLayout *startStopLayout = new QVBoxLayout(startStopWidget);
    startStopLayout->setContentsMargins(0, 0, 0, 0);
    startStopLayout->setSpacing(8);

    QLabel *startStopLabel = new QLabel("模式控制");
    startStopLabel->setStyleSheet(
        "QLabel {"
        "    color: #ffffff;"
        "    font-size: 18px;"
        "    font-weight: bold;"
        "    text-align: center;"
        "}"
    );

    // 按钮容器 - 水平排列两个按钮
    QWidget *buttonContainer = new QWidget();
    QHBoxLayout *buttonLayout = new QHBoxLayout(buttonContainer);
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->setSpacing(15);

    // 速度模式切换按钮 - 使用符号 "⭮" 表示切换
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

    QLabel *modeLabel = new QLabel("速度模式切换");
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

    // 添加到主布局
    startStopLayout->addWidget(startStopLabel, 0, Qt::AlignCenter);
    startStopLayout->addWidget(buttonContainer, 0, Qt::AlignCenter);
    startStopLayout->addWidget(labelContainer, 0, Qt::AlignCenter);
    startStopLayout->addStretch();

    // 垂直分隔符
    QFrame *verticalSeparator = new QFrame();
    verticalSeparator->setFrameShape(QFrame::VLine);
    verticalSeparator->setFrameShadow(QFrame::Sunken);
    verticalSeparator->setStyleSheet("QFrame { color: #555555; margin: 0px 10px; }");
    verticalSeparator->setMinimumWidth(2);
    verticalSeparator->setMaximumWidth(2);

    // 点动控制区域
    QWidget *jogWidget = new QWidget();
    QVBoxLayout *jogLayout = new QVBoxLayout(jogWidget);
    jogLayout->setContentsMargins(0, 0, 0, 0);
    jogLayout->setSpacing(8);

    QLabel *jogLabel = new QLabel("点动控制");
    jogLabel->setStyleSheet(
        "QLabel {"
        "    color: #ffffff;"
        "    font-size: 18px;"
        "    font-weight: bold;"
        "    text-align: center;"
        "}"
    );

    // 点动按钮使用与背景更接近的颜色
    jogForwardBtn = createControlButton("正向点动", "#5a5a5a");  // 更接近背景色
    jogBackwardBtn = createControlButton("反向点动", "#5a5a5a"); // 更接近背景色

    // 设置点动按钮样式
    QString jogButtonStyle =
        "QPushButton {"
        "    background-color: #5a5a5a;"
        "    color: #e0e0e0;"
        "    border: 1px solid #666666;"
        "    border-radius: 4px;"
        "    padding: 12px 16px;"
        "    font-weight: bold;"
        "    font-size: 18px;"
        "    min-height: 50px;"
        "    min-width: 120px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #666666;"
        "    color: #ffffff;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #4a4a4a;"
        "}";

    jogForwardBtn->setStyleSheet(jogButtonStyle);
    jogBackwardBtn->setStyleSheet(jogButtonStyle);

    // 点动按钮布局
    QHBoxLayout *jogButtonLayout = new QHBoxLayout();
    jogButtonLayout->setSpacing(15);
    jogButtonLayout->setContentsMargins(0, 0, 0, 0);

    jogButtonLayout->addStretch();
    jogButtonLayout->addWidget(jogForwardBtn);
    jogButtonLayout->addWidget(jogBackwardBtn);
    jogButtonLayout->addStretch();

    jogLayout->addWidget(jogLabel, 0, Qt::AlignCenter);
    jogLayout->addLayout(jogButtonLayout);
    jogLayout->addStretch();

    // 添加到主控制布局
    controlMainLayout->addWidget(startStopWidget);
    controlMainLayout->addWidget(verticalSeparator);
    controlMainLayout->addWidget(jogWidget);
    controlMainLayout->setStretchFactor(startStopWidget, 1);
    controlMainLayout->setStretchFactor(jogWidget, 2);

    // 添加到控制布局
    controlLayout->addWidget(controlTitleLabel);
    controlLayout->addLayout(controlMainLayout);

    // 添加到主布局
    mainLayout->addWidget(paramsWidget);
    mainLayout->addWidget(separator);  // 添加分隔符
    mainLayout->addWidget(controlWidget);

    // 连接信号槽
    connect(applyBtn, &QPushButton::clicked, this, &MotionControlSpeed::onApplyButtonClicked);
    connect(startStopBtn, &QPushButton::clicked, this, &MotionControlSpeed::onStartStopButtonClicked);
    connect(modeSwitchBtn, &QPushButton::clicked, this, &MotionControlSpeed::onModeSwitchButtonClicked);
    connect(jogForwardBtn, &QPushButton::clicked, this, &MotionControlSpeed::onJogForwardButtonClicked);
    connect(jogBackwardBtn, &QPushButton::clicked, this, &MotionControlSpeed::onJogBackwardButtonClicked);
}

QPushButton* MotionControlSpeed::createControlButton(const QString &text, const QString &color)
{
    QPushButton *button = new QPushButton(text);
    button->setMinimumHeight(45);
    return button;
}

QString MotionControlSpeed::adjustColor(const QString &color, double factor)
{
    if (color == "#f44336") return "#d32f2f";
    if (color == "#4caf50") return "#388e3c";
    if (color == "#2196f3") return "#1976d2";
    if (color == "#ff9800") return "#f57c00";
    if (color == "#9c27b0") return "#7b1fa2";
    if (color == "#795548") return "#5d4037";
    return "#607d8b";
}

double MotionControlSpeed::getTargetSpeed() const
{
    return targetSpeedSpin->value();
}

double MotionControlSpeed::getMaxTorque() const
{
    return maxTorqueSpin->value();
}

double MotionControlSpeed::getAcceleration() const
{
    return accelerationSpin->value();
}

double MotionControlSpeed::getDeceleration() const
{
    return decelerationSpin->value();
}

void MotionControlSpeed::setTargetSpeed(double speed)
{
    targetSpeedSpin->setValue(speed);
}

void MotionControlSpeed::setMaxTorque(double torque)
{
    maxTorqueSpin->setValue(torque);
}

void MotionControlSpeed::setAcceleration(double acceleration)
{
    accelerationSpin->setValue(acceleration);
}

void MotionControlSpeed::setDeceleration(double deceleration)
{
    decelerationSpin->setValue(deceleration);
}

void MotionControlSpeed::onStartStopButtonClicked()
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

void MotionControlSpeed::onModeSwitchButtonClicked()
{
    qDebug() << "速度模式切换按钮被点击";

    // 使用封装的CAN发送函数
    if (g_canTxRx && g_canTxRx->isDeviceReady()) {
        bool success = g_canTxRx->sendSpeedModeSwitch();
        if (success) {
            qDebug() << "速度模式切换指令发送成功";
        } else {
            qDebug() << "速度模式切换指令发送失败:" << g_canTxRx->getLastError();
        }
    } else {
        qDebug() << "CAN设备未就绪，无法发送速度模式切换指令";
    }

    emit modeSwitchButtonClicked();
}

void MotionControlSpeed::onJogForwardButtonClicked()
{
    qDebug() << "正向点动按钮被点击";

    if (g_canTxRx && g_canTxRx->isDeviceReady()) {
        bool success = g_canTxRx->sendJogForwardCommand();
        if (success) {
            qDebug() << "正向点动指令发送成功";
        } else {
            qDebug() << "正向点动指令发送失败:" << g_canTxRx->getLastError();
        }
    } else {
        qDebug() << "CAN设备未就绪，无法发送正向点动指令";
    }

    emit jogForwardButtonClicked();
}

void MotionControlSpeed::onJogBackwardButtonClicked()
{
    qDebug() << "反向点动按钮被点击";

    if (g_canTxRx && g_canTxRx->isDeviceReady()) {
        bool success = g_canTxRx->sendJogBackwardCommand();
        if (success) {
            qDebug() << "反向点动指令发送成功";
        } else {
            qDebug() << "反向点动指令发送失败:" << g_canTxRx->getLastError();
        }
    } else {
        qDebug() << "CAN设备未就绪，无法发送反向点动指令";
    }

    emit jogBackwardButtonClicked();
}

void MotionControlSpeed::onApplyButtonClicked()
{
    double targetSpeed = getTargetSpeed();
    double maxTorque = getMaxTorque();  // 这里假设maxTorque对应outputLimit

    qDebug() << "应用速度模式参数:"
             << "目标速度:" << targetSpeed << "rad/s,"
             << "最大扭矩:" << maxTorque << "Nm,"
             << "加速度:" << getAcceleration() << "rad/s²,"
             << "减速度:" << getDeceleration() << "rad/s²";

    // 发送速度指令到CAN
    if (g_canTxRx && g_canTxRx->isDeviceReady()) {
        bool success = g_canTxRx->sendSpeedCommand(static_cast<float>(targetSpeed),
                                                  static_cast<float>(maxTorque));
        if (success) {
            qDebug() << "速度指令发送成功 - 目标速度:" << targetSpeed
                     << "rad/s, 输出限制:" << maxTorque << "Nm";
        } else {
            qDebug() << "速度指令发送失败:" << g_canTxRx->getLastError();
        }
    } else {
        qDebug() << "CAN设备未就绪，无法发送速度指令";
    }

    emit parametersApplied();
}



// 外部控制方法
void MotionControlSpeed::setRunningState(bool running)
{
    if (running != isRunning) {
        onStartStopButtonClicked(); // 切换状态
    }
}

bool MotionControlSpeed::getRunningState() const
{
    return isRunning;
}
