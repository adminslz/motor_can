#ifndef MOTION_CONTROL_H
#define MOTION_CONTROL_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QButtonGroup>
#include <QRadioButton>
#include <QDoubleSpinBox>
#include <QFrame>
#include "motor_status.h"
#include "motion_contr_speed.h"  // 添加速度模式头文件
#include "motion_contr_current.h"  // 添加电流控制头文件
#include "motion_contr_mentionctr.h"
#include "motor_contr_position.h"  // 添加位置模式头文件
class MotionControl : public QWidget
{
    Q_OBJECT

public:
    explicit MotionControl(QWidget *parent = nullptr);

private slots:
    void onControlModeChanged(int mode);

private:
    void setupUI();
    void setupControlModePanel(QVBoxLayout *leftLayout);
    void setupMotionControlPanel(QVBoxLayout *centerLayout);

    // 辅助方法
    QPushButton* createModeButton(const QString &title, const QString &description, int modeId);
    QFrame* createSeparator();
    QPushButton* createControlButton(const QString &text, const QString &color);
    QString adjustColor(const QString &color, double factor);
    void updateControlParams(int mode);

    QButtonGroup *controlModeGroup;

    // UI 组件
    MotionControlSpeed *speedControlWidget;  // 改为使用 MotionControlSpeed
    MotorStatus *motorStatus;
    MotionControlCurrent *currentControlWidget;  // 添加电流控制组件
    MotionControlMotion *motionControlWidget;  // 添加运控控制组件
    MotionControlPosition *positionControlWidget;  // 添加位置控制组件
    
    // 控制模式常量
    enum ControlMode {
        SPEED_MODE = 0,
        POSITION_MODE,
        TORQUE_MODE,
        MOTION_CONTROL_MODE
    };
};

#endif // MOTION_CONTROL_H
