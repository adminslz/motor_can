#include "motor_status.h"
#include <QDebug>

MotorStatus::MotorStatus(QWidget *parent)
    : QWidget(parent)
    , motorTempLabel(nullptr)
    , controlBoardTempLabel(nullptr)
    , currentSpeedLabel(nullptr)
    , currentCurrentLabel(nullptr)
    , currentPositionLabel(nullptr)
    , motorTemperature(0.0)
    , controlBoardTemperature(0.0)
    , currentSpeed(0.0)
    , currentCurrent(0.0)
    , currentPosition(0.0)
{
    setupUI();
}

void MotorStatus::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(12);

    // 标题
    QLabel *titleLabel = new QLabel("电机状态");
    titleLabel->setStyleSheet(
        "QLabel {"
        "    color: #ffffff;"
        "    font-size: 18px;"
        "    font-weight: bold;"
        "    padding: 8px 0px;"
        "    border-bottom: 1px solid #555555;"
        "}"
    );
    titleLabel->setAlignment(Qt::AlignCenter);

    // 状态显示容器
    QWidget *statusContainer = new QWidget();
    statusContainer->setStyleSheet(
        "QWidget {"
        "    background-color: #454545;"
        "    border-radius: 6px;"
        "    border: 1px solid #555555;"
        "}"
    );
    QFormLayout *statusLayout = new QFormLayout(statusContainer);
    statusLayout->setVerticalSpacing(10);
    statusLayout->setHorizontalSpacing(12);
    statusLayout->setContentsMargins(15, 15, 15, 15);

    // 标签样式（左侧文字说明）
    QString labelStyle =
        "QLabel {"
        "    color: #ffffff;"
        "    font-size: 18px;"
        "    font-weight: bold;"
        "    min-width: 80px;"
        "}";

    // 状态值样式（右侧数据）
    QString valueStyle =
        "QLabel {"
        "    color: #4fc3f7;"
        "    font-size: 18px;"
        "    font-weight: bold;"
        "    background-color: #3a3a3a;"
        "    border: 1px solid #555555;"
        "    border-radius: 4px;"
        "    padding: 8px 10px;"
        "    min-width: 90px;"
        "}";

    // 电机温度
    QLabel *motorTempText = new QLabel("电机温度:");
    motorTempText->setStyleSheet(labelStyle);
    motorTempLabel = new QLabel("0.0 °C");
    motorTempLabel->setStyleSheet(valueStyle);
    motorTempLabel->setAlignment(Qt::AlignCenter);
    motorTempLabel->setToolTip("当前电机温度");

    // 控制板温度
    QLabel *controlBoardTempText = new QLabel("控制板温度:");
    controlBoardTempText->setStyleSheet(labelStyle);
    controlBoardTempLabel = new QLabel("0.0 °C");
    controlBoardTempLabel->setStyleSheet(valueStyle);
    controlBoardTempLabel->setAlignment(Qt::AlignCenter);
    controlBoardTempLabel->setToolTip("当前控制板温度");

    // 当前速度
    QLabel *currentSpeedText = new QLabel("当前速度:");
    currentSpeedText->setStyleSheet(labelStyle);
    currentSpeedLabel = new QLabel("0.0 rad/s");
    currentSpeedLabel->setStyleSheet(valueStyle);
    currentSpeedLabel->setAlignment(Qt::AlignCenter);
    currentSpeedLabel->setToolTip("当前电机转速");

    // 当前电流
    QLabel *currentCurrentText = new QLabel("当前电流:");
    currentCurrentText->setStyleSheet(labelStyle);
    currentCurrentLabel = new QLabel("0.0 A");
    currentCurrentLabel->setStyleSheet(valueStyle);
    currentCurrentLabel->setAlignment(Qt::AlignCenter);
    currentCurrentLabel->setToolTip("当前电机电流");

    // 当前位置
    QLabel *currentPositionText = new QLabel("当前位置:");
    currentPositionText->setStyleSheet(labelStyle);
    currentPositionLabel = new QLabel("0.000 rad");
    currentPositionLabel->setStyleSheet(valueStyle);
    currentPositionLabel->setAlignment(Qt::AlignCenter);
    currentPositionLabel->setToolTip("当前电机位置");

    // 添加到布局
    statusLayout->addRow(motorTempText, motorTempLabel);
    statusLayout->addRow(controlBoardTempText, controlBoardTempLabel);
    statusLayout->addRow(currentSpeedText, currentSpeedLabel);
    statusLayout->addRow(currentCurrentText, currentCurrentLabel);
    statusLayout->addRow(currentPositionText, currentPositionLabel);

    // 状态说明区域
    QWidget *descContainer = new QWidget();
    descContainer->setStyleSheet(
        "QWidget {"
        "    background-color: #454545;"
        "    border-radius: 6px;"
        "    border: 1px solid #555555;"
        "    padding: 10px;"
        "}"
    );
    QVBoxLayout *descLayout = new QVBoxLayout(descContainer);

    QLabel *descTitle = new QLabel("状态说明");
    descTitle->setStyleSheet(
        "QLabel {"
        "    color: #4fc3f7;"
        "    font-size: 20px;"
        "    font-weight: bold;"
        "    padding-bottom: 6px;"
        "}"
    );

    QLabel *descLabel = new QLabel("实时显示电机的运行状态参数。包括温度、速度、电流和位置等信息。");
    descLabel->setWordWrap(true);
    descLabel->setStyleSheet(
        "QLabel {"
        "    color: #cccccc;"
        "    font-size: 15px;"
        "    line-height: 1.4;"
        "}"
    );

    descLayout->addWidget(descTitle);
    descLayout->addWidget(descLabel);

    // 状态更新按钮
    QPushButton *refreshBtn = new QPushButton("刷新状态");
    refreshBtn->setStyleSheet(
        "QPushButton {"
        "    background-color: #2196f3;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 4px;"
        "    padding: 8px 16px;"
        "    font-weight: bold;"
        "    font-size: 20px;"
        "    min-height: 35px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #1976d2;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #1565c0;"
        "}"
    );

    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(statusContainer);
    mainLayout->addWidget(descContainer);
    mainLayout->addWidget(refreshBtn);
    mainLayout->addStretch();

    // 连接信号槽
    connect(refreshBtn, &QPushButton::clicked, this, &MotorStatus::onRefreshButtonClicked);
}

void MotorStatus::updateMotorTemperature(double temperature)
{
    motorTemperature = temperature;
    motorTempLabel->setText(QString::number(temperature, 'f', 1) + " °C");
}

void MotorStatus::updateControlBoardTemperature(double temperature)
{
    controlBoardTemperature = temperature;
    controlBoardTempLabel->setText(QString::number(temperature, 'f', 1) + " °C");
}

void MotorStatus::updateCurrentSpeed(double speed)
{
    currentSpeed = speed;
    currentSpeedLabel->setText(QString::number(speed, 'f', 1) + " rad/s");
}

void MotorStatus::updateCurrentCurrent(double current)
{
    currentCurrent = current;
    currentCurrentLabel->setText(QString::number(current, 'f', 1) + " A");
}

void MotorStatus::updateCurrentPosition(double position)
{
    currentPosition = position;
    currentPositionLabel->setText(QString::number(position, 'f', 3) + " rad");
}

double MotorStatus::getMotorTemperature() const
{
    return motorTemperature;
}

double MotorStatus::getControlBoardTemperature() const
{
    return controlBoardTemperature;
}

double MotorStatus::getCurrentSpeed() const
{
    return currentSpeed;
}

double MotorStatus::getCurrentCurrent() const
{
    return currentCurrent;
}

double MotorStatus::getCurrentPosition() const
{
    return currentPosition;
}

void MotorStatus::onRefreshButtonClicked()
{
    qDebug() << "请求刷新电机状态";
    emit statusRefreshRequested();
}
