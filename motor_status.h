#ifndef MOTOR_STATUS_H
#define MOTOR_STATUS_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>

class MotorStatus : public QWidget
{
    Q_OBJECT

public:
    explicit MotorStatus(QWidget *parent = nullptr);

    // 更新状态的方法
    void updateMotorTemperature(double temperature);
    void updateControlBoardTemperature(double temperature);
    void updateCurrentSpeed(double speed);
    void updateCurrentCurrent(double current);
    void updateCurrentPosition(double position);

    // 获取当前状态值
    double getMotorTemperature() const;
    double getControlBoardTemperature() const;
    double getCurrentSpeed() const;
    double getCurrentCurrent() const;
    double getCurrentPosition() const;

signals:
    void statusRefreshRequested();

private slots:
    void onRefreshButtonClicked();

private:
    void setupUI();

    // UI 组件
    QLabel *motorTempLabel;
    QLabel *controlBoardTempLabel;
    QLabel *currentSpeedLabel;
    QLabel *currentCurrentLabel;
    QLabel *currentPositionLabel;

    // 状态值
    double motorTemperature;
    double controlBoardTemperature;
    double currentSpeed;
    double currentCurrent;
    double currentPosition;
};

#endif // MOTOR_STATUS_H
