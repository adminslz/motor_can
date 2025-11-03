#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QMainWindow>
#include <QMenuBar>
#include <QAction>
#include <QStackedWidget>
#include <QWidget>
#include <QLabel>
#include <QSpinBox>
#include "ControlCAN.h"
#include "canthread.h"
#include "motion_control.h"
#include "control_param.h"
#include "motor_param.h"
#include "data_acquisition.h"
#include "motor_debug.h"  // 添加调试日志头文件
QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    // 设置CAN配置（可选）
       void setCANConfig(int deviceIndex, int baudRate, int canId, bool extendedFrame);
    
    // 设置数据采集组件给CANTxRx
    void setupDataAcquisition();
    
    // 设置CAN线程实例
    void setCANThread(CANThread* thread);

private slots:
    void onGetProtocolData(VCI_CAN_OBJ *vci, unsigned int dwRel, unsigned int channel);
    void on_open_close_button_clicked();
    void on_pushButton_clicked();
    
    // 菜单栏槽函数
    void showHomePage();
    void showControlPage();
    void showMonitorPage();
    void showSettingsPage();

private:
    Ui::MainWindow *ui;
    double baseScaleFactor;
    CANThread *canthread;
    void setupUI();
    void redirectQDebug();  // 重定向qDebug输出
    void set_word_style();
    QString versionStr(USHORT ver);
    MotorDebug *motorDebug;  // 调试日志系统

    void setupStatusBar();  // 添加状态栏设置
    void updateOnlineStatus(bool online);  // 更新在线状态
    
    // 菜单栏相关
    void createMenuBar();
    void createStackedWidget();
    QMenuBar *menuBar;
    QAction *homeAction;
    QAction *controlAction;
    QAction *monitorAction;
    QAction *settingsAction;
    QStackedWidget *stackedWidget;
    
    // 示波器数据更新
    void updateOscilloscopeData(const VCI_CAN_OBJ &canObj, unsigned int channel);
    // 新增槽函数
       void onApplyCanIdClicked();  // 应用CAN ID按钮点击
    // 状态栏组件
       QLabel *onlineStatusLabel;
       QWidget *onlineStatusWidget;
       QSpinBox *canIdSpinBox;  // CAN ID输入框
       
    // 数据采集组件
    DataAcquisition *dataAcquisitionWidget;
    
    // 控制参数组件
    ControlParam *controlParamTab;
    
    // 运动控制组件
    MotionControl *motionControlTab;

};

#endif // MAINWINDOW_H
