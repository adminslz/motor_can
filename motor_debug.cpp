#include "motor_debug.h"
#include <QFile>
#include <QTextStream>
#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>
#include <QTextDocument>  // 添加这个头文件
#include <QTextCursor>    // 添加这个头文件
#include <QDateTime>      // 添加这个头文件
#include <QApplication>
#include "can_rx_tx.h"

// 静态成员初始化
MotorDebug* MotorDebug::instance = nullptr;
MotorDebug::MotorDebug(QObject *parent)
    : QObject(parent)
    , logWidget(nullptr)
    , logTextEdit(nullptr)
    , clearButton(nullptr)
    , saveButton(nullptr)
{
    setupUI();
    
    // 设置全局实例
    instance = this;
    
    // 注释掉qDebug消息处理器
    // qInstallMessageHandler(messageHandler);
    
    // 在 MotorDebug 构造函数中添加
    if (g_canTxRx) {
        connect(g_canTxRx, &CANTxRx::canDataReceived, this, &MotorDebug::logCAN);
    }
}

MotorDebug::~MotorDebug()
{
     qDebug() << "【MotorDebug】析构函数开始";
     // 注释掉消息处理器恢复
     // qInstallMessageHandler(nullptr);
     instance = nullptr;
}

// 注释掉messageHandler函数，因为不再使用qDebug重定向
/*
void MotorDebug::messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    if (instance && instance->logTextEdit) {
        QString timestamp = QTime::currentTime().toString("hh:mm:ss.zzz");
        QString level;
        QString color;
        
        switch (type) {
        case QtDebugMsg:
            level = "DEBUG";
            color = "#ffffff";
            break;
        case QtInfoMsg:
            level = "INFO";
            color = "#4fc3f7";
            break;
        case QtWarningMsg:
            level = "WARN";
            color = "#ffb74d";
            break;
        case QtCriticalMsg:
        case QtFatalMsg:
            level = "ERROR";
            color = "#f44336";
            break;
        }
        
        QString logMessage = QString("[%1] [%2] %3")
                            .arg(timestamp)
                            .arg(level)
                            .arg(msg);
        
        // 使用HTML格式添加颜色
        QString htmlMessage = QString("<span style='color: %1'>%2</span>")
                             .arg(color)
                             .arg(logMessage.toHtmlEscaped());
        
        instance->logTextEdit->append(htmlMessage);
        
        // 自动滚动到底部
        QTextCursor cursor = instance->logTextEdit->textCursor();
        cursor.movePosition(QTextCursor::End);
        instance->logTextEdit->setTextCursor(cursor);
    }
}
*/

void MotorDebug::setupUI()
{
    // 创建日志主控件
    logWidget = new QWidget();
    logWidget->setObjectName("systemLogWidget");
    QVBoxLayout *mainLayout = new QVBoxLayout(logWidget);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->setSpacing(5);

    // 标题栏
    QWidget *titleWidget = new QWidget();
    QHBoxLayout *titleLayout = new QHBoxLayout(titleWidget);
    titleLayout->setContentsMargins(0, 0, 0, 0);

    QLabel *titleLabel = new QLabel("系统日志");
    titleLabel->setStyleSheet(
        "QLabel {"
        "    color: #4fc3f7;"
        "    font-size: 18px;"  // 增大标题字体
        "    font-weight: bold;"
        "}"
    );

    // 按钮容器
    QWidget *buttonWidget = new QWidget();
    QHBoxLayout *buttonLayout = new QHBoxLayout(buttonWidget);
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->setSpacing(5);

    // 清空按钮
    clearButton = new QPushButton("清空日志");
    clearButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #5a5a5a;"
        "    color: white;"
        "    border: 1px solid #666666;"
        "    border-radius: 3px;"
        "    padding: 6px 12px;"  // 增大按钮内边距
        "    font-size: 14px;"    // 增大按钮字体
        "    min-width: 70px;"    // 增大按钮最小宽度
        "}"
        "QPushButton:hover {"
        "    background-color: #666666;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #4a4a4a;"
        "}"
    );

    // 保存按钮
    saveButton = new QPushButton("保存日志");
    saveButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #2196f3;"
        "    color: white;"
        "    border: 1px solid #1976d2;"
        "    border-radius: 3px;"
        "    padding: 6px 12px;"  // 增大按钮内边距
        "    font-size: 14px;"    // 增大按钮字体
        "    min-width: 70px;"    // 增大按钮最小宽度
        "}"
        "QPushButton:hover {"
        "    background-color: #1976d2;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #1565c0;"
        "}"
    );

    buttonLayout->addWidget(clearButton);
    buttonLayout->addWidget(saveButton);
    buttonLayout->addStretch();

    titleLayout->addWidget(titleLabel);
    titleLayout->addStretch();
    titleLayout->addWidget(buttonWidget);

    // 日志显示区域
    logTextEdit = new QTextEdit();
    logTextEdit->setReadOnly(true);
    logTextEdit->setAcceptRichText(true);  // 启用HTML格式
    logTextEdit->setStyleSheet(
        "QTextEdit {"
        "    background-color: #1e1e1e;"
        "    color: #cccccc;"
        "    border: 1px solid #404040;"
        "    border-radius: 4px;"
        "    font-family: 'Consolas', 'Monaco', monospace;"
        "    font-size: 24px;"
        "    padding: 8px;"
        "    line-height: 1.4;"
        "}"
    );

    QFont font = logTextEdit->font();
    font.setPointSize(30);  // 16磅，非常大的字体
    font.setFamily("Microsoft YaHei");
    font.setWeight(QFont::Medium);  // 中等字重
    logTextEdit->setFont(font);

    // 添加到主布局
    mainLayout->addWidget(titleWidget);
    mainLayout->addWidget(logTextEdit);

    // 连接信号槽
    connect(clearButton, &QPushButton::clicked, this, &MotorDebug::onClearButtonClicked);
    connect(saveButton, &QPushButton::clicked, this, &MotorDebug::onSaveButtonClicked);

    // 添加欢迎信息
    logInfo("系统日志初始化完成");
    // 设置样式后的调试
       qDebug() << "=== 设置样式后的字体信息 ===";
       qDebug() << "设置后字体家族:" << logTextEdit->font().family();
       qDebug() << "设置后字体大小:" << logTextEdit->font().pointSize();
       qDebug() << "设置后字体像素:" << logTextEdit->font().pixelSize();
       qDebug() << "【MotorDebug】构造函数完成";
}

void MotorDebug::addLogMessage(const QString &message, const QString &color)
{
    QString formattedMessage = QString("<span style='color: %1; font-size: 13px;'>%2</span>")
                                  .arg(color)
                                  .arg(message.toHtmlEscaped());

    // 使用纯文本，通过QTextCharFormat设置颜色
       QTextCursor cursor(logTextEdit->document());
       cursor.movePosition(QTextCursor::End);

       QTextCharFormat format;
       if (color == "#4fc3f7") format.setForeground(QColor("#4fc3f7")); // INFO - 蓝色
       else if (color == "#ff9800") format.setForeground(QColor("#ff9800")); // WARNING - 橙色
       else if (color == "#f44336") format.setForeground(QColor("#f44336")); // ERROR - 红色
       else if (color == "#4caf50") format.setForeground(QColor("#4caf50")); // CAN - 绿色
       else if (color == "#888888") format.setForeground(QColor("#888888")); // DEBUG - 灰色
       else format.setForeground(QColor("#cccccc")); // 默认颜色

       cursor.insertText(message + "\n", format);

    // 限制日志行数
    QTextDocument *doc = logTextEdit->document();
    int lineCount = doc->blockCount();
    if (lineCount > MAX_LOG_LINES) {
        QTextCursor cursor(doc);
        cursor.movePosition(QTextCursor::Start);
        cursor.movePosition(QTextCursor::Down, QTextCursor::KeepAnchor, lineCount - MAX_LOG_LINES);
        cursor.removeSelectedText();
    }

    // 自动滚动到底部
    QScrollBar *scrollBar = logTextEdit->verticalScrollBar();
    scrollBar->setValue(scrollBar->maximum());
}


QWidget* MotorDebug::getLogWidget()
{
    return logWidget;
}

void MotorDebug::logDebug(const QString &message)
{
    addLogMessage(QString("[%1] DEBUG: %2").arg(QTime::currentTime().toString("hh:mm:ss")).arg(message), "#888888");
}

void MotorDebug::logInfo(const QString &message)
{
    addLogMessage(QString("[%1] INFO: %2").arg(QTime::currentTime().toString("hh:mm:ss")).arg(message), "#4fc3f7");
}

void MotorDebug::logWarning(const QString &message)
{
    addLogMessage(QString("[%1] WARNING: %2").arg(QTime::currentTime().toString("hh:mm:ss")).arg(message), "#ff9800");
}

void MotorDebug::logError(const QString &message)
{
    addLogMessage(QString("[%1] ERROR: %2").arg(QTime::currentTime().toString("hh:mm:ss")).arg(message), "#f44336");
}

void MotorDebug::logCAN(const QString &message)
{
    addLogMessage(QString("[%1] CAN: %2").arg(QTime::currentTime().toString("hh:mm:ss")).arg(message), "#4caf50");
}

void MotorDebug::clearLog()
{
    logTextEdit->clear();
    logInfo("日志已清空");
}


void MotorDebug::onClearButtonClicked()
{
    clearLog();
}

void MotorDebug::onSaveButtonClicked()
{
    QString fileName = QFileDialog::getSaveFileName(
        logWidget,
        "保存日志文件",
        QString("motor_system_log_%1.txt").arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss")),
        "文本文件 (*.txt);;所有文件 (*)"
    );

    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream stream(&file);
            stream << logTextEdit->toPlainText();
            file.close();
            logInfo(QString("日志已保存到: %1").arg(fileName));
        } else {
            logError("保存日志文件失败");
        }
    }
}
