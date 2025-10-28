#include "motor_param.h"

MotorParam::MotorParam(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
}

void MotorParam::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // 电机参数网格
    QGridLayout *gridLayout = new QGridLayout();
    gridLayout->setSpacing(15);

    // 基本电机参数
    QGroupBox *basicGroup = new QGroupBox("基本参数");
    QFormLayout *basicLayout = new QFormLayout(basicGroup);

    QDoubleSpinBox *ratedPowerSpin = new QDoubleSpinBox();
    ratedPowerSpin->setRange(0, 10000);
    ratedPowerSpin->setValue(750);
    ratedPowerSpin->setSuffix(" W");

    QDoubleSpinBox *ratedVoltageSpin = new QDoubleSpinBox();
    ratedVoltageSpin->setRange(0, 1000);
    ratedVoltageSpin->setValue(220);
    ratedVoltageSpin->setSuffix(" V");

    QDoubleSpinBox *ratedCurrentSpin = new QDoubleSpinBox();
    ratedCurrentSpin->setRange(0, 100);
    ratedCurrentSpin->setValue(3.5);
    ratedCurrentSpin->setSuffix(" A");

    basicLayout->addRow("额定功率:", ratedPowerSpin);
    basicLayout->addRow("额定电压:", ratedVoltageSpin);
    basicLayout->addRow("额定电流:", ratedCurrentSpin);

    // 性能参数
    QGroupBox *performanceGroup = new QGroupBox("性能参数");
    QFormLayout *performanceLayout = new QFormLayout(performanceGroup);

    QDoubleSpinBox *maxSpeedSpin = new QDoubleSpinBox();
    maxSpeedSpin->setRange(0, 10000);
    maxSpeedSpin->setValue(3000);
    maxSpeedSpin->setSuffix(" RPM");

    QDoubleSpinBox *maxTorqueSpin = new QDoubleSpinBox();
    maxTorqueSpin->setRange(0, 1000);
    maxTorqueSpin->setValue(100);
    maxTorqueSpin->setSuffix(" Nm");

    QDoubleSpinBox *torqueConstantSpin = new QDoubleSpinBox();
    torqueConstantSpin->setRange(0, 10);
    torqueConstantSpin->setValue(1.2);
    torqueConstantSpin->setSuffix(" Nm/A");

    performanceLayout->addRow("最大转速:", maxSpeedSpin);
    performanceLayout->addRow("最大扭矩:", maxTorqueSpin);
    performanceLayout->addRow("扭矩常数:", torqueConstantSpin);

    // 机械参数
    QGroupBox *mechanicalGroup = new QGroupBox("机械参数");
    QFormLayout *mechanicalLayout = new QFormLayout(mechanicalGroup);

    QDoubleSpinBox *inertiaSpin = new QDoubleSpinBox();
    inertiaSpin->setRange(0, 1);
    inertiaSpin->setValue(0.002);
    inertiaSpin->setSuffix(" kg·m²");

    QDoubleSpinBox *backEMFSpin = new QDoubleSpinBox();
    backEMFSpin->setRange(0, 100);
    backEMFSpin->setValue(25.5);
    backEMFSpin->setSuffix(" V/kRPM");

    mechanicalLayout->addRow("转动惯量:", inertiaSpin);
    mechanicalLayout->addRow("反电动势常数:", backEMFSpin);

    gridLayout->addWidget(basicGroup, 0, 0);
    gridLayout->addWidget(performanceGroup, 0, 1);
    gridLayout->addWidget(mechanicalGroup, 1, 0, 1, 2);

    mainLayout->addLayout(gridLayout);
    mainLayout->setContentsMargins(15, 15, 15, 15);
}
