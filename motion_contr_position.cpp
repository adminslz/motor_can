#include "motor_contr_position.h"
#include <QDebug>
#include "can_rx_tx.h"  // 包含CAN收发封装头文件
MotionControlPosition::MotionControlPosition(QWidget *parent)
    : QWidget(parent)
    , kpSpin(nullptr)
    , kiSpin(nullptr)
    , targetPositionSpin(nullptr)
    , accelerationSpin(nullptr)
    , decelerationSpin(nullptr)
    , jerkSpin(nullptr)
    , motionProfileCombo(nullptr)
    , startStopBtn(nullptr)
    , modeSwitchBtn(nullptr)
    , homeBtn(nullptr)
    , isRunning(false)
{
    setupUI();
}

void MotionControlPosition::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
       mainLayout->setContentsMargins(10, 10, 10, 10);
       mainLayout->setSpacing(8);

       // 位置模式参数区域
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
       QLabel *titleLabel = new QLabel("位置模式控制参数");
       titleLabel->setStyleSheet(
           "QLabel {"
           "    color: #2196f3;"
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

       // 下拉框样式
       QString comboBoxStyle =
           "QComboBox {"
           "    background-color: #454545;"
           "    color: white;"
           "    border: 1px solid #555555;"
           "    border-radius: 4px;"
           "    padding: 6px;"
           "    min-width: 120px;"
           "    font-size: 20px;"
           "}"
           "QComboBox::drop-down {"
           "    border: none;"
           "}"
           "QComboBox::down-arrow {"
           "    image: none;"
           "    border-left: 1px solid #555555;"
           "    width: 20px;"
           "}"
           "QComboBox QAbstractItemView {"
           "    background-color: #454545;"
           "    color: white;"
           "    selection-background-color: #555555;"
           "}";

       // 单位标签样式
       QString unitLabelStyle = "QLabel { color: #cccccc; font-size: 18px; padding-left: 5px; }";

       // 直流比例增益
       QLabel *kpLabel = new QLabel("直流比例增益:");
       kpLabel->setStyleSheet(labelStyle);
       kpSpin = new QDoubleSpinBox();
       kpSpin->setRange(0, 1000);
       kpSpin->setValue(80);
       kpSpin->setDecimals(2);
       kpSpin->setSingleStep(5);
       kpSpin->setStyleSheet(spinBoxStyle);

       QLabel *kpUnit = new QLabel("");
       kpUnit->setStyleSheet(unitLabelStyle);

       QHBoxLayout *kpLayout = new QHBoxLayout();
       kpLayout->setContentsMargins(0, 0, 0, 0);
       kpLayout->setSpacing(5);
       kpLayout->addWidget(kpSpin);
       kpLayout->addWidget(kpUnit);
       kpLayout->addStretch();

       // 积分增益
       QLabel *kiLabel = new QLabel("积分增益:");
       kiLabel->setStyleSheet(labelStyle);
       kiSpin = new QDoubleSpinBox();
       kiSpin->setRange(0, 100);
       kiSpin->setValue(2.5);
       kiSpin->setDecimals(3);
       kiSpin->setSingleStep(0.1);
       kiSpin->setStyleSheet(spinBoxStyle);

       QLabel *kiUnit = new QLabel("");
       kiUnit->setStyleSheet(unitLabelStyle);

       QHBoxLayout *kiLayout = new QHBoxLayout();
       kiLayout->setContentsMargins(0, 0, 0, 0);
       kiLayout->setSpacing(5);
       kiLayout->addWidget(kiSpin);
       kiLayout->addWidget(kiUnit);
       kiLayout->addStretch();

       // 目标位置
       QLabel *targetPositionLabel = new QLabel("目标位置:");
       targetPositionLabel->setStyleSheet(labelStyle);
       targetPositionSpin = new QDoubleSpinBox();
       targetPositionSpin->setRange(-100.0, 100.0); // -2π to 2π rad
       targetPositionSpin->setValue(1.57);        // π/2 rad ≈ 90°
       targetPositionSpin->setDecimals(3);
       targetPositionSpin->setSingleStep(0.1);
       targetPositionSpin->setStyleSheet(spinBoxStyle);

       QLabel *targetPositionUnit = new QLabel("rad");
       targetPositionUnit->setStyleSheet(unitLabelStyle);

       QHBoxLayout *targetPositionLayout = new QHBoxLayout();
       targetPositionLayout->setContentsMargins(0, 0, 0, 0);
       targetPositionLayout->setSpacing(5);
       targetPositionLayout->addWidget(targetPositionSpin);
       targetPositionLayout->addWidget(targetPositionUnit);
       targetPositionLayout->addStretch();

       // 最大转速
       QLabel *maxSpeedLabel = new QLabel("最大转速:");
       maxSpeedLabel->setStyleSheet(labelStyle);
       maxSpeedSpin = new QDoubleSpinBox();
       maxSpeedSpin->setRange(-15.0, 15.0);
       maxSpeedSpin->setValue(3.14);
       maxSpeedSpin->setDecimals(1);
       maxSpeedSpin->setSingleStep(10);
       maxSpeedSpin->setStyleSheet(spinBoxStyle);

       QLabel *maxSpeedUnit = new QLabel("rad/s");
       maxSpeedUnit->setStyleSheet(unitLabelStyle);

       QHBoxLayout *maxSpeedLayout = new QHBoxLayout();
       maxSpeedLayout->setContentsMargins(0, 0, 0, 0);
       maxSpeedLayout->setSpacing(5);
       maxSpeedLayout->addWidget(maxSpeedSpin);
       maxSpeedLayout->addWidget(maxSpeedUnit);
       maxSpeedLayout->addStretch();

       // 加速度
       QLabel *accelerationLabel = new QLabel("加速度:");
       accelerationLabel->setStyleSheet(labelStyle);
       accelerationSpin = new QDoubleSpinBox();
       accelerationSpin->setRange(0, 10000);
       accelerationSpin->setValue(1500);
       accelerationSpin->setSingleStep(100);
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
       decelerationSpin->setValue(1500);
       decelerationSpin->setSingleStep(100);
       decelerationSpin->setStyleSheet(spinBoxStyle);

       QLabel *decelerationUnit = new QLabel("rad/s²");
       decelerationUnit->setStyleSheet(unitLabelStyle);

       QHBoxLayout *decelerationLayout = new QHBoxLayout();
       decelerationLayout->setContentsMargins(0, 0, 0, 0);
       decelerationLayout->setSpacing(5);
       decelerationLayout->addWidget(decelerationSpin);
       decelerationLayout->addWidget(decelerationUnit);
       decelerationLayout->addStretch();

       // 加加速度 (Jerk)
       QLabel *jerkLabel = new QLabel("加加速度:");
       jerkLabel->setStyleSheet(labelStyle);
       jerkSpin = new QDoubleSpinBox();
       jerkSpin->setRange(0, 50000);
       jerkSpin->setValue(8000);
       jerkSpin->setSingleStep(500);
       jerkSpin->setStyleSheet(spinBoxStyle);

       QLabel *jerkUnit = new QLabel("rad/s³");
       jerkUnit->setStyleSheet(unitLabelStyle);

       QHBoxLayout *jerkLayout = new QHBoxLayout();
       jerkLayout->setContentsMargins(0, 0, 0, 0);
       jerkLayout->setSpacing(5);
       jerkLayout->addWidget(jerkSpin);
       jerkLayout->addWidget(jerkUnit);
       jerkLayout->addStretch();

       // 曲线规划
       QLabel *motionProfileLabel = new QLabel("曲线规划:");
       motionProfileLabel->setStyleSheet(labelStyle);
       motionProfileCombo = new QComboBox();
       motionProfileCombo->addItem("梯形规划", 0);
       motionProfileCombo->addItem("S曲线规划", 1);
       motionProfileCombo->addItem("多项式规划", 2);
       motionProfileCombo->addItem("正弦规划", 3);
       motionProfileCombo->setStyleSheet(comboBoxStyle);

       QHBoxLayout *motionProfileLayout = new QHBoxLayout();
       motionProfileLayout->setContentsMargins(0, 0, 0, 0);
       motionProfileLayout->setSpacing(5);
       motionProfileLayout->addWidget(motionProfileCombo);
       motionProfileLayout->addStretch();

       // 添加到表单布局
       formLayout->addRow(kpLabel, kpLayout);
       formLayout->addRow(kiLabel, kiLayout);
       formLayout->addRow(targetPositionLabel, targetPositionLayout);
       formLayout->addRow(maxSpeedLabel, maxSpeedLayout);
       formLayout->addRow(accelerationLabel, accelerationLayout);
       formLayout->addRow(decelerationLabel, decelerationLayout);
       formLayout->addRow(jerkLabel, jerkLayout);
       formLayout->addRow(motionProfileLabel, motionProfileLayout);

    // 应用按钮
    QPushButton *applyBtn = new QPushButton("应用参数");
    applyBtn->setStyleSheet(
        "QPushButton {"
        "    background-color: #2196f3;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 4px;"
        "    padding: 8px 16px;"
        "    font-weight: bold;"
        "    font-size: 25px;"
        "    min-height: 35px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #1976d2;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #1565c0;"
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
    // 控制按钮区域 - 改为与速度模式相同的格式
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
           "    color: #2196f3;"  // 位置模式使用蓝色
           "    font-size: 25px;"
           "    font-weight: bold;"
           "    padding-bottom: 6px;"
           "}"
       );

       // 使用水平布局包含模式控制区和回零区
       QHBoxLayout *controlMainLayout = new QHBoxLayout();
       controlMainLayout->setSpacing(20);
       controlMainLayout->setContentsMargins(0, 0, 0, 0);

       // 模式控制区域
       QWidget *modeControlWidget = new QWidget();
       QVBoxLayout *modeControlLayout = new QVBoxLayout(modeControlWidget);
       modeControlLayout->setContentsMargins(0, 0, 0, 0);
       modeControlLayout->setSpacing(8);

       QLabel *modeControlLabel = new QLabel("模式控制");
       modeControlLabel->setStyleSheet(
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

       // 位置模式切换按钮 - 使用符号 "⭮" 表示切换
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

       QLabel *modeLabel = new QLabel("位置模式切换");
       modeLabel->setStyleSheet(
           "QLabel {"
           "    color: #cccccc;"
           "    font-size: 14px;"
           "    text-align: center;"
           "}"
       );

       QLabel *startStopTextLabel = new QLabel("位置控制使能");
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

       // 添加到模式控制布局
       modeControlLayout->addWidget(modeControlLabel, 0, Qt::AlignCenter);
       modeControlLayout->addWidget(buttonContainer, 0, Qt::AlignCenter);
       modeControlLayout->addWidget(labelContainer, 0, Qt::AlignCenter);
       modeControlLayout->addStretch();

       // 垂直分隔符
       QFrame *verticalSeparator = new QFrame();
       verticalSeparator->setFrameShape(QFrame::VLine);
       verticalSeparator->setFrameShadow(QFrame::Sunken);
       verticalSeparator->setStyleSheet("QFrame { color: #555555; margin: 0px 10px; }");
       verticalSeparator->setMinimumWidth(2);
       verticalSeparator->setMaximumWidth(2);

       // 回零控制区域
       QWidget *homeWidget = new QWidget();
       QVBoxLayout *homeLayout = new QVBoxLayout(homeWidget);
       homeLayout->setContentsMargins(0, 0, 0, 0);
       homeLayout->setSpacing(8);

       QLabel *homeLabel = new QLabel("回零控制");
       homeLabel->setStyleSheet(
           "QLabel {"
           "    color: #ffffff;"
           "    font-size: 18px;"
           "    font-weight: bold;"
           "    text-align: center;"
           "}"
       );

       // 回零按钮
       homeBtn = new QPushButton("H");
       homeBtn->setStyleSheet(
           "QPushButton {"
           "    background-color: #5a5a5a;"
           "    color: #e0e0e0;"
           "    border: 1px solid #666666;"
           "    border-radius: 6px;"
           "    padding: 15px 20px;"
           "    font-weight: bold;"
           "    font-size: 40px;"
           "    min-height: 60px;"
           "    min-width: 80px;"
           "}"
           "QPushButton:hover {"
           "    background-color: #666666;"
           "    color: #ffffff;"
           "    border: 1px solid #777777;"
           "}"
           "QPushButton:pressed {"
           "    background-color: #4a4a4a;"
           "    border: 1px solid #555555;"
           "}"
       );

       // 回零按钮标签
       QLabel *homeTextLabel = new QLabel("回零操作");
       homeTextLabel->setStyleSheet(
           "QLabel {"
           "    color: #cccccc;"
           "    font-size: 14px;"
           "    text-align: center;"
           "}"
       );

       homeLayout->addWidget(homeLabel, 0, Qt::AlignCenter);
       homeLayout->addWidget(homeBtn, 0, Qt::AlignCenter);
       homeLayout->addWidget(homeTextLabel, 0, Qt::AlignCenter);
       homeLayout->addStretch();

       // 添加到主控制布局
       controlMainLayout->addWidget(modeControlWidget);
       controlMainLayout->addWidget(verticalSeparator);
       controlMainLayout->addWidget(homeWidget);
       controlMainLayout->setStretchFactor(modeControlWidget, 1);
       controlMainLayout->setStretchFactor(homeWidget, 1);

       // 添加到控制布局
       controlLayout->addWidget(controlTitleLabel);
       controlLayout->addLayout(controlMainLayout);

       // 添加到主布局
       mainLayout->addWidget(paramsWidget);
       mainLayout->addWidget(separator);
       mainLayout->addWidget(controlWidget);

       // 连接信号槽
       connect(applyBtn, &QPushButton::clicked, this, &MotionControlPosition::onApplyButtonClicked);
       connect(startStopBtn, &QPushButton::clicked, this, &MotionControlPosition::onStartStopButtonClicked);
       connect(modeSwitchBtn, &QPushButton::clicked, this, &MotionControlPosition::onModeSwitchButtonClicked);
       connect(homeBtn, &QPushButton::clicked, this, &MotionControlPosition::onHomeButtonClicked);

}

QPushButton* MotionControlPosition::createControlButton(const QString &text, const QString &color)
{
    QPushButton *button = new QPushButton(text);
    button->setMinimumHeight(45);
    return button;
}

double MotionControlPosition::getKp() const
{
    return kpSpin->value();
}

double MotionControlPosition::getKi() const
{
    return kiSpin->value();
}

double MotionControlPosition::getTargetPosition() const
{
    return targetPositionSpin->value();
}

double MotionControlPosition::getAcceleration() const
{
    return accelerationSpin->value();
}

double MotionControlPosition::getDeceleration() const
{
    return decelerationSpin->value();
}

double MotionControlPosition::getJerk() const
{
    return jerkSpin->value();
}

int MotionControlPosition::getMotionProfile() const
{
    return motionProfileCombo->currentData().toInt();
}

void MotionControlPosition::setKp(double kp)
{
    kpSpin->setValue(kp);
}

void MotionControlPosition::setKi(double ki)
{
    kiSpin->setValue(ki);
}

void MotionControlPosition::setTargetPosition(double position)
{
    targetPositionSpin->setValue(position);
}

void MotionControlPosition::setAcceleration(double acceleration)
{
    accelerationSpin->setValue(acceleration);
}

void MotionControlPosition::setDeceleration(double deceleration)
{
    decelerationSpin->setValue(deceleration);
}

void MotionControlPosition::setJerk(double jerk)
{
    jerkSpin->setValue(jerk);
}

void MotionControlPosition::setMotionProfile(int profile)
{
    int index = motionProfileCombo->findData(profile);
    if (index >= 0) {
        motionProfileCombo->setCurrentIndex(index);
    }
}

double MotionControlPosition::getMaxSpeed() const
{
    if (maxSpeedSpin) {
        return maxSpeedSpin->value();
    }
    return 0.0;
}

void MotionControlPosition::setMaxSpeed(double speed)
{
    if (maxSpeedSpin) {
        maxSpeedSpin->setValue(speed);
    }
}



void MotionControlPosition::onApplyButtonClicked()
{
    double targetPosition = getTargetPosition();
    double maxSpeed = getMaxSpeed();

    qDebug() << "应用位置模式参数:"
             << "目标位置:" << targetPosition << "rad,"
             << "最大转速:" << maxSpeed << "rad/s,"
             << "直流比例增益:" << getKp() << ","
             << "积分增益:" << getKi() << ","
             << "加速度:" << getAcceleration() << "rad/s²,"
             << "减速度:" << getDeceleration() << "rad/s²,"
             << "加加速度:" << getJerk() << "rad/s³,"
             << "曲线规划:" << motionProfileCombo->currentText();

    // 发送位置指令到CAN
    if (g_canTxRx && g_canTxRx->isDeviceReady()) {
        bool success = g_canTxRx->sendPositionCommand(static_cast<float>(targetPosition),
                                         static_cast<float>(maxSpeed));
        if (success) {
            qDebug() << "位置指令发送成功 - 目标位置:" << targetPosition
                     << "rad, 最大转速:" << maxSpeed << "rad/s";
        } else {
            qDebug() << "位置指令发送失败:" << g_canTxRx->getLastError();
        }
    } else {
        qDebug() << "CAN设备未就绪，无法发送位置指令";
    }

    emit parametersApplied();
}

void MotionControlPosition::onStartStopButtonClicked()
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
        qDebug() << "启动按钮被点击 - 开始位置控制";

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
        qDebug() << "停止按钮被点击 - 停止位置控制";

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

void MotionControlPosition::onModeSwitchButtonClicked()
{
    qDebug() << "位置模式切换按钮被点击";

    // 使用封装的CAN发送函数
    if (g_canTxRx && g_canTxRx->isDeviceReady()) {
        bool success = g_canTxRx->sendPositionModeSwitch();
        if (success) {
            qDebug() << "位置模式切换指令发送成功";
        } else {
            qDebug() << "位置模式切换指令发送失败:" << g_canTxRx->getLastError();
        }
    } else {
        qDebug() << "CAN设备未就绪，无法发送位置模式切换指令";
    }

    emit modeSwitchButtonClicked();
}

void MotionControlPosition::onHomeButtonClicked()
{
    qDebug() << "回零按钮被点击";
    emit homeButtonClicked();
}

void MotionControlPosition::setRunningState(bool running)
{
    if (running != isRunning) {
        onStartStopButtonClicked();
    }
}

bool MotionControlPosition::getRunningState() const
{
    return isRunning;
}
