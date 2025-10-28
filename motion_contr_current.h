#ifndef MOTION_CONTROL_CURRENT_H
#define MOTION_CONTROL_CURRENT_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QFrame>

class MotionControlCurrent : public QWidget
{
    Q_OBJECT

public:
    explicit MotionControlCurrent(QWidget *parent = nullptr);

    // 获取参数值
    double getTargetIqCurrent() const;
    double getTargetIdCurrent() const;
    double getCurrentKp() const;
    double getCurrentKi() const;

    // 设置参数值
    void setTargetIqCurrent(double iq);
    void setTargetIdCurrent(double id);
    void setCurrentKp(double kp);
    void setCurrentKi(double ki);

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
    bool sendCurrentCommand(float iq, float id, float kp, float ki);

    // 控件指针
    QDoubleSpinBox *targetIqSpin;    // 目标Iq电流
    QDoubleSpinBox *targetIdSpin;    // 目标Id电流
    QDoubleSpinBox *currentKpSpin;   // 电流环Kp
    QDoubleSpinBox *currentKiSpin;   // 电流环Ki

    QPushButton *startStopBtn;
    QPushButton *modeSwitchBtn;

    bool isRunning;
};

#endif // MOTION_CONTROL_CURRENT_H
