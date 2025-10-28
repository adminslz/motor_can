#ifndef MOTOR_PARAM_H
#define MOTOR_PARAM_H

#include <QWidget>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QDoubleSpinBox>

class MotorParam : public QWidget
{
    Q_OBJECT

public:
    explicit MotorParam(QWidget *parent = nullptr);

private:
    void setupUI();
};

#endif // MOTOR_PARAM_H
