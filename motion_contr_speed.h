#ifndef MOTION_CONTROL_SPEED_H
#define MOTION_CONTROL_SPEED_H

#include <QWidget>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QFrame>

class MotionControlSpeed : public QWidget
{
    Q_OBJECT

public:
    explicit MotionControlSpeed(QWidget *parent = nullptr);

    // 获取参数值的方法
    double getTargetSpeed() const;
    double getMaxTorque() const;
    double getAcceleration() const;
    double getDeceleration() const;

    // 设置参数值的方法
    void setTargetSpeed(double speed);
    void setMaxTorque(double torque);
    void setAcceleration(double acceleration);
    void setDeceleration(double deceleration);

    // 运行状态控制方法
    void setRunningState(bool running);
    bool getRunningState() const;

signals:
    void parametersApplied();
    void startButtonClicked();
    void stopButtonClicked();  // 新增停止信号
    void modeSwitchButtonClicked();  // 新增速度模式切换信号
    void jogForwardButtonClicked();
    void jogBackwardButtonClicked();

private slots:
    void onApplyButtonClicked();
    void onStartStopButtonClicked();  // 修改为启动/停止槽函数
    void onModeSwitchButtonClicked();  // 新增速度模式切换槽函数
    void onJogForwardButtonClicked();
    void onJogBackwardButtonClicked();

private:
    void setupUI();
    QPushButton* createControlButton(const QString &text, const QString &color);
    QString adjustColor(const QString &color, double factor);

    // UI 组件
    QDoubleSpinBox *targetSpeedSpin;
    QDoubleSpinBox *maxTorqueSpin;
    QDoubleSpinBox *accelerationSpin;
    QDoubleSpinBox *decelerationSpin;

    // 运动控制按钮
    QPushButton *startStopBtn;  // 修改为启动/停止按钮
    QPushButton *modeSwitchBtn;  // 新增速度模式切换按钮
    QPushButton *jogForwardBtn;
    QPushButton *jogBackwardBtn;

    // 运行状态
    bool isRunning;
};

#endif // MOTION_CONTROL_SPEED_H
