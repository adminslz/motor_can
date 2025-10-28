#include "motion_contr_mentionctr.h"
#include <QDebug>
#include "can_rx_tx.h"  // 包含CAN收发封装头文件

MotionControlMotion::MotionControlMotion(QWidget *parent)
    : QWidget(parent)
    , kpSpin(nullptr)
    , kdSpin(nullptr)
    , targetPositionSpin(nullptr)
    , targetVelocitySpin(nullptr)
    , targetCurrentSpin(nullptr)
    , startStopBtn(nullptr)
    , modeSwitchBtn(nullptr)
    , isRunning(false)
{
    setupUI();
}

void MotionControlMotion::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(8);

    // 运控模式参数区域
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
    QLabel *titleLabel = new QLabel("运控模式控制参数");
    titleLabel->setStyleSheet(
        "QLabel {"
        "    color: #9c27b0;"
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

    // 比例增益 Kp
    QLabel *kpLabel = new QLabel("比例增益 Kp:");
    kpLabel->setStyleSheet(labelStyle);
    kpSpin = new QDoubleSpinBox();
    kpSpin->setRange(0, 5000);
    kpSpin->setValue(50);
    kpSpin->setDecimals(2);
    kpSpin->setSingleStep(1);
    kpSpin->setStyleSheet(spinBoxStyle);

    QLabel *kpUnit = new QLabel("");
    kpUnit->setStyleSheet(unitLabelStyle);

    QHBoxLayout *kpLayout = new QHBoxLayout();
    kpLayout->setContentsMargins(0, 0, 0, 0);
    kpLayout->setSpacing(5);
    kpLayout->addWidget(kpSpin);
    kpLayout->addWidget(kpUnit);
    kpLayout->addStretch();

    // 微分增益 Kd
    QLabel *kdLabel = new QLabel("微分增益 Kd:");
    kdLabel->setStyleSheet(labelStyle);
    kdSpin = new QDoubleSpinBox();
    kdSpin->setRange(0, 100);
    kdSpin->setValue(2);
    kdSpin->setDecimals(3);
    kdSpin->setSingleStep(0.1);
    kdSpin->setStyleSheet(spinBoxStyle);

    QLabel *kdUnit = new QLabel("");
    kdUnit->setStyleSheet(unitLabelStyle);

    QHBoxLayout *kdLayout = new QHBoxLayout();
    kdLayout->setContentsMargins(0, 0, 0, 0);
    kdLayout->setSpacing(5);
    kdLayout->addWidget(kdSpin);
    kdLayout->addWidget(kdUnit);
    kdLayout->addStretch();

    // 目标位置
    QLabel *targetPositionLabel = new QLabel("目标位置:");
    targetPositionLabel->setStyleSheet(labelStyle);
    targetPositionSpin = new QDoubleSpinBox();
    targetPositionSpin->setRange(-50, 50);
    targetPositionSpin->setValue(5);
    targetPositionSpin->setDecimals(2);
    targetPositionSpin->setSingleStep(10);
    targetPositionSpin->setStyleSheet(spinBoxStyle);

    QLabel *targetPositionUnit = new QLabel("°");
    targetPositionUnit->setStyleSheet(unitLabelStyle);

    QHBoxLayout *targetPositionLayout = new QHBoxLayout();
    targetPositionLayout->setContentsMargins(0, 0, 0, 0);
    targetPositionLayout->setSpacing(5);
    targetPositionLayout->addWidget(targetPositionSpin);
    targetPositionLayout->addWidget(targetPositionUnit);
    targetPositionLayout->addStretch();

    // 目标转速
    QLabel *targetVelocityLabel = new QLabel("目标转速:");
    targetVelocityLabel->setStyleSheet(labelStyle);
    targetVelocitySpin = new QDoubleSpinBox();
    targetVelocitySpin->setRange(-15, 15);
    targetVelocitySpin->setValue(5);
    targetVelocitySpin->setSingleStep(50);
    targetVelocitySpin->setStyleSheet(spinBoxStyle);

    QLabel *targetVelocityUnit = new QLabel("rad/s");
    targetVelocityUnit->setStyleSheet(unitLabelStyle);

    QHBoxLayout *targetVelocityLayout = new QHBoxLayout();
    targetVelocityLayout->setContentsMargins(0, 0, 0, 0);
    targetVelocityLayout->setSpacing(5);
    targetVelocityLayout->addWidget(targetVelocitySpin);
    targetVelocityLayout->addWidget(targetVelocityUnit);
    targetVelocityLayout->addStretch();

    // 目标电流
    QLabel *targetCurrentLabel = new QLabel("目标电流:");
    targetCurrentLabel->setStyleSheet(labelStyle);
    targetCurrentSpin = new QDoubleSpinBox();
    targetCurrentSpin->setRange(-50, 50);
    targetCurrentSpin->setValue(5.0);
    targetCurrentSpin->setDecimals(2);
    targetCurrentSpin->setSingleStep(0.5);
    targetCurrentSpin->setStyleSheet(spinBoxStyle);

    QLabel *targetCurrentUnit = new QLabel("A");
    targetCurrentUnit->setStyleSheet(unitLabelStyle);

    QHBoxLayout *targetCurrentLayout = new QHBoxLayout();
    targetCurrentLayout->setContentsMargins(0, 0, 0, 0);
    targetCurrentLayout->setSpacing(5);
    targetCurrentLayout->addWidget(targetCurrentSpin);
    targetCurrentLayout->addWidget(targetCurrentUnit);
    targetCurrentLayout->addStretch();

    // 添加到表单布局
    formLayout->addRow(kpLabel, kpLayout);
    formLayout->addRow(kdLabel, kdLayout);
    formLayout->addRow(targetPositionLabel, targetPositionLayout);
    formLayout->addRow(targetVelocityLabel, targetVelocityLayout);
    formLayout->addRow(targetCurrentLabel, targetCurrentLayout);

    // 应用按钮
    QPushButton *applyBtn = new QPushButton("应用参数");
    applyBtn->setStyleSheet(
        "QPushButton {"
        "    background-color: #9c27b0;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 4px;"
        "    padding: 8px 16px;"
        "    font-weight: bold;"
        "    font-size: 25px;"
        "    min-height: 35px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #7b1fa2;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #6a1b9a;"
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

    // 控制按钮区域 - 改为与其他模式相同的格式
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
        "    color: #9c27b0;"
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

    // 运控模式切换按钮 - 使用符号 "⭮" 表示切换
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

    QLabel *modeLabel = new QLabel("运控模式切换");
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
    connect(applyBtn, &QPushButton::clicked, this, &MotionControlMotion::onApplyButtonClicked);
    connect(startStopBtn, &QPushButton::clicked, this, &MotionControlMotion::onStartStopButtonClicked);
    connect(modeSwitchBtn, &QPushButton::clicked, this, &MotionControlMotion::onModeSwitchButtonClicked);
}

// 其他成员函数保持不变...

void MotionControlMotion::onStartStopButtonClicked()
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

void MotionControlMotion::onModeSwitchButtonClicked()
{
    qDebug() << "运控模式切换按钮被点击";

    // 使用封装的CAN发送函数
    if (g_canTxRx && g_canTxRx->isDeviceReady()) {
        bool success = g_canTxRx->sendMotionModeSwitch();
        if (success) {
            qDebug() << "运控模式切换指令发送成功";
        } else {
            qDebug() << "运控模式切换指令发送失败:" << g_canTxRx->getLastError();
        }
    } else {
        qDebug() << "CAN设备未就绪，无法发送运控模式切换指令";
    }

    emit modeSwitchButtonClicked();
}

void MotionControlMotion::onApplyButtonClicked()
{
    double kp = getKp();
    double kd = getKd();
    double targetPosition = getTargetPosition();
    double targetVelocity = getTargetVelocity();
    double targetCurrent = getTargetCurrent();

    qDebug() << "应用运控模式参数:"
             << "Kp:" << kp << ","
             << "Kd:" << kd << ","
             << "目标位置:" << targetPosition << "°,"
             << "目标转速:" << targetVelocity << "rpm,"
             << "目标电流:" << targetCurrent << "A";

    // 发送运控指令到CAN
    if (g_canTxRx && g_canTxRx->isDeviceReady()) {
        bool success = g_canTxRx->sendMotionCommand(static_cast<float>(kp),
                                       static_cast<float>(kd),
                                       static_cast<float>(targetPosition),
                                       static_cast<float>(targetVelocity),
                                       static_cast<float>(targetCurrent));
        if (success) {
            qDebug() << "运控指令发送成功";
        } else {
            qDebug() << "运控指令发送失败:" << g_canTxRx->getLastError();
        }
    } else {
        qDebug() << "CAN设备未就绪，无法发送运控指令";
    }

    emit parametersApplied();
}


// 参数获取函数
double MotionControlMotion::getKp() const
{
    if (kpSpin) {
        return kpSpin->value();
    }
    return 0.0;
}

double MotionControlMotion::getKd() const
{
    if (kdSpin) {
        return kdSpin->value();
    }
    return 0.0;
}

double MotionControlMotion::getTargetPosition() const
{
    if (targetPositionSpin) {
        return targetPositionSpin->value();
    }
    return 0.0;
}

double MotionControlMotion::getTargetVelocity() const
{
    if (targetVelocitySpin) {
        return targetVelocitySpin->value();
    }
    return 0.0;
}

double MotionControlMotion::getTargetCurrent() const
{
    if (targetCurrentSpin) {
        return targetCurrentSpin->value();
    }
    return 0.0;
}

// 参数设置函数
void MotionControlMotion::setKp(double kp)
{
    if (kpSpin) {
        kpSpin->setValue(kp);
    }
}

void MotionControlMotion::setKd(double kd)
{
    if (kdSpin) {
        kdSpin->setValue(kd);
    }
}

void MotionControlMotion::setTargetPosition(double position)
{
    if (targetPositionSpin) {
        targetPositionSpin->setValue(position);
    }
}

void MotionControlMotion::setTargetVelocity(double velocity)
{
    if (targetVelocitySpin) {
        targetVelocitySpin->setValue(velocity);
    }
}

void MotionControlMotion::setTargetCurrent(double current)
{
    if (targetCurrentSpin) {
        targetCurrentSpin->setValue(current);
    }
}
