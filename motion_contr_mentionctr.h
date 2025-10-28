#ifndef MOTION_CONTR_MENTIONCTR_H
#define MOTION_CONTR_MENTIONCTR_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QFrame>

class MotionControlMotion : public QWidget
{
    Q_OBJECT

public:
    explicit MotionControlMotion(QWidget *parent = nullptr);

    // 获取参数值
    double getKp() const;
    double getKd() const;
    double getTargetPosition() const;
    double getTargetVelocity() const;
    double getTargetCurrent() const;

    // 设置参数值
    void setKp(double kp);
    void setKd(double kd);
    void setTargetPosition(double position);
    void setTargetVelocity(double velocity);
    void setTargetCurrent(double current);

    // 运行状态控制
    void setRunningState(bool running);
    bool getRunningState() const;

signals:
    void parametersApplied();
    void startButtonClicked();
    void stopButtonClicked();
    void modeSwitchButtonClicked();

private slots:
    void onApplyButtonClicked();
    void onStartStopButtonClicked();
    void onModeSwitchButtonClicked();

private:
    void setupUI();
    bool sendMotionCommand(float kp, float kd, float position, float velocity, float current);

    // UI 组件
    QDoubleSpinBox *kpSpin;
    QDoubleSpinBox *kdSpin;
    QDoubleSpinBox *targetPositionSpin;
    QDoubleSpinBox *targetVelocitySpin;
    QDoubleSpinBox *targetCurrentSpin;

    QPushButton *startStopBtn;
    QPushButton *modeSwitchBtn;

    bool isRunning;
};

#endif // MOTION_CONTROL_MOTION_H
