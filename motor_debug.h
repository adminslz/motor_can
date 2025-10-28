#ifndef MOTOR_DEBUG_H
#define MOTOR_DEBUG_H

#include <QObject>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QTime>
#include <QScrollBar>
#include <QTextDocument>  // 添加这个头文件
#include <QTextCursor>    // 添加这个头文件

class MotorDebug : public QObject
{
    Q_OBJECT

public:
    explicit MotorDebug(QObject *parent = nullptr);
    ~MotorDebug();

    // 获取日志显示控件
    QWidget* getLogWidget();

    // 日志记录方法
    void logDebug(const QString &message);
    void logInfo(const QString &message);
    void logWarning(const QString &message);
    void logError(const QString &message);
    
    // qDebug重定向（已禁用）
    // static void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg);
    static MotorDebug* instance;
    void logCAN(const QString &message);

    // 清空日志
    void clearLog();

private slots:
    void onClearButtonClicked();
    void onSaveButtonClicked();

private:
    void setupUI();
    void addLogMessage(const QString &message, const QString &color = "#ffffff");

    QWidget *logWidget;
    QTextEdit *logTextEdit;
    QPushButton *clearButton;
    QPushButton *saveButton;

    static const int MAX_LOG_LINES = 1000; // 最大日志行数
};

#endif // MOTOR_DEBUG_H
