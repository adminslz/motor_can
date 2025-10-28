#ifndef MOTOR_CONTR_POSITION_H
#define MOTOR_CONTR_POSITION_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QFrame>
#include <QComboBox>

class MotionControlPosition : public QWidget
{
    Q_OBJECT

public:
    explicit MotionControlPosition(QWidget *parent = nullptr);
    ~MotionControlPosition() = default;

    // 获取参数值
    double getKp() const;
    double getKi() const;
    double getTargetPosition() const;
    double getAcceleration() const;
    double getDeceleration() const;
    double getJerk() const;
    int getMotionProfile() const;
    double getMaxSpeed() const;
    void setMaxSpeed(double speed);

    // 设置参数值
    void setKp(double kp);
    void setKi(double ki);
    void setTargetPosition(double position);
    void setAcceleration(double acceleration);
    void setDeceleration(double deceleration);
    void setJerk(double jerk);
    void setMotionProfile(int profile);

    // 运行状态控制
    void setRunningState(bool running);
    bool getRunningState() const;

signals:
    void parametersApplied();
    void startButtonClicked();
    void stopButtonClicked();
    void modeSwitchButtonClicked();
    void homeButtonClicked();

private slots:
    void onApplyButtonClicked();
    void onStartStopButtonClicked();
    void onModeSwitchButtonClicked();
    void onHomeButtonClicked();

private:
    void setupUI();
    QPushButton* createControlButton(const QString &text, const QString &color = "#2196f3");

    // UI 组件
    QDoubleSpinBox *kpSpin;
    QDoubleSpinBox *kiSpin;
    QDoubleSpinBox *targetPositionSpin;
    QDoubleSpinBox *accelerationSpin;
    QDoubleSpinBox *decelerationSpin;
    QDoubleSpinBox *jerkSpin;
    QDoubleSpinBox *maxSpeedSpin;  // 添加最大转速控件
    QComboBox *motionProfileCombo;

    QPushButton *startStopBtn;
    QPushButton *modeSwitchBtn;
    QPushButton *homeBtn;

    bool isRunning;
};

#endif // MOTION_CONTROL_POSITION_H
