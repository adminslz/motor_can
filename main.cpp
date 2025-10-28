#include "mainwindow.h"
#include "can_init.h"
#include "global_vars.h"
#include <QApplication>
#include <QObject>
#include "can_types.h"
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 创建登录界面
    CANInit loginWindow;
    MainWindow mainWindow;

    // 注册CAN类型
       registerCanTypes();
    // 在登录成功后的连接中添加
    QObject::connect(&loginWindow, &CANInit::loginSuccess, [&](int deviceIndex, int baudRate, int canId, CANThread* canThread) {
        loginWindow.hide();

        // 设置CAN线程到MainWindow
        mainWindow.setCANThread(canThread);

        // 初始化CAN收发对象
        if (!g_canTxRx) {
            g_canTxRx = new CANTxRx();
        }

        // 设置CAN线程到CANTxRx
        g_canTxRx->setCANThread(canThread);

        // 设置CAN参数
        g_canTxRx->setCANParams(4, deviceIndex, 0);

        // 启动CAN数据接收
        g_canTxRx->startReceiving(1, true);  // 1ms间隔接收，高频模式

        // 设置数据采集组件给CANTxRx
        mainWindow.setupDataAcquisition();

        qDebug() << "CAN配置: 设备索引=" << deviceIndex
                 << ", 波特率=" << baudRate << "Kbps"
                 << ", CAN ID=" << QString::number(canId, 16).toUpper();

        mainWindow.show();
        mainWindow.setWindowState(Qt::WindowActive);
    });

    // 程序退出时清理资源
    QObject::connect(&a, &QApplication::aboutToQuit, []() {
        if (g_canTxRx) {
            delete g_canTxRx;
            g_canTxRx = nullptr;
            qDebug() << "CAN资源已清理";
        }
    });

    // 显示登录界面
    loginWindow.show();

    return a.exec();
}
