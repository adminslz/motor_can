#ifndef DATA_ACQUISITION_H
#define DATA_ACQUISITION_H

#include <QObject>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QComboBox>
#include <QSpinBox>
#include <QLabel>
#include <QCheckBox>
#include <QTimer>
#include <QVector>
#include <QMap>
#include <QPair>
#include <QTime>
#include <QDateTime>
#include <QTextEdit>
#include <QLoggingCategory>
#include <QFormLayout>
#include <QGroupBox>
#include <QList>
#include <QFrame>
#include <QScrollArea>
#include <QMutex>
#include <QMutexLocker>
#include "param_dictionary.h"
#include "ControlCAN.h"
#include "can_communication_thread.h"
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsLineItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsTextItem>
#include <QGraphicsItem>
#include <QScrollBar>
#include <QBrush>
#include <QPen>
#include <QColor>
#include <QPointF>
#include <QRectF>

// 数据上抛命令类型定义
#define UPDATE_CMD_READ_SINGLE     0x01
#define UPDATE_CMD_READ_MULTI      0x02
#define UPDATE_CMD_WRITE_SINGLE    0x03
#define UPDATE_CMD_WRITE_MULTI     0x04
#define UPDATE_CMD_RESPONSE        0x05
#define UPDATE_CMD_STREAM_START    0x06
#define UPDATE_CMD_STREAM_STOP     0x07
#define UPDATE_CMD_STREAM_DATA     0x08

// 数据上抛基础ID
#define UPDATE_COB_ID_BASE 0x500

// 参数信息结构体
struct ParameterInfo {
    uint16_t index;
    uint8_t subindex;
    QString name;
    QString unit;
    double scale;
};

// 数据点结构体
struct DataPoint {
    double timestamp;
    double value;
    QString parameterName;
};

// 通道配置结构体
struct ChannelConfig {
    int channelIndex;
    uint16_t parameterIndex;
    uint8_t parameterSubindex;
    QString parameterName;
    bool enabled;
};

// 使用QGraphicsView的示波器控件
class OscilloscopeWidget : public QGraphicsView
{
    Q_OBJECT
public slots:
    void onDataPointAdded(const QString &channelName, double timestamp, double value, const QString &parameterName);
public:
    explicit OscilloscopeWidget(QWidget *parent = nullptr);
    ~OscilloscopeWidget();
    void addDataPoint(const QString &channelName, double timestamp, double value, const QString &parameterName);
    void clearData();
    void setTimeRange(double range);
    void setAutoScale(bool autoScale);
    void setFixedValueRange(double minValue, double maxValue);
    void setVisibleCurves(const QList<QString> &visibleKeys);
    
    // 缩放和坐标功能
    void zoomIn();
    void zoomOut();
    void resetZoom();
    void setZoomLevel(double level);
    void setPanOffset(double timeOffset, double valueOffset);
    
    // 全显功能
    void fitToData();
    
    // 颜色配置
    void initializeCurveColors();
    
    // 数值范围更新
    void updateValueRange();
    
    // 数据清理
    void cleanupOldData();
    
    // 鼠标事件处理
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void setupPlot();
    void createFitButton();
    void drawGrid();
    void drawCurves();
    void drawAxisLabels();
    void drawTimeAxis();
    void drawValueAxis();
    void adjustFontSizes(const QSize &size);
    QColor getCurveColor(const QString &channelName);
    QPointF dataToScreen(double timestamp, double value);
    QPointF screenToData(const QPointF &screenPoint);
    void updateMousePosition(const QPointF &mousePos);
    QPointF findValueAtTime(double timestamp);
    QMap<QString, double> findAllValuesAtTime(double timestamp);
    void updateFrequencyDisplay();
    void startSelection(const QPointF &startPos);
    void updateSelection(const QPointF &currentPos);
    void endSelection(const QPointF &endPos);
    void clearSelection();
    void zoomToSelection();
    
    QGraphicsScene *m_scene;
    QMap<QString, QVector<QPointF>> m_dataBuffers;
    QMap<QString, QColor> m_curveColors;
    QList<QString> m_visibleCurves;
    double m_timeRange;
    bool m_autoScale;
    
    // 鼠标位置显示
    QGraphicsTextItem *m_mousePositionLabel;
    QGraphicsLineItem *m_crosshairH;
    QGraphicsLineItem *m_crosshairV;
    double m_minValue;
    double m_maxValue;
    int m_maxDataPoints;
    
    // 缩放和交互功能
    double m_zoomLevel;
    double m_timeOffset;
    double m_valueOffset;
    double m_startTime;  // 记录起始时间，用于相对时间计算
    
    // 全显按钮
    QPushButton *m_fitButton;
    
    // 数据频率计算
    QTime m_lastDataTime;
    int m_dataCount;
    double m_dataFrequency;
    QGraphicsTextItem *m_frequencyLabel;
    
    // 60秒滚动显示
    double m_maxTimeWindow;  // 最大时间窗口（60秒）
    double m_currentTimeOffset;  // 当前时间偏移
    
    // 框选放大功能
    bool m_isSelecting;  // 是否正在框选
    QPointF m_selectionStart;  // 框选开始位置
    QPointF m_selectionEnd;  // 框选结束位置
    QGraphicsRectItem *m_selectionRect;  // 框选矩形
    QGraphicsLineItem *m_selectionH1;  // 框选水平线1
    QGraphicsLineItem *m_selectionH2;  // 框选水平线2
    QGraphicsLineItem *m_selectionV1;  // 框选垂直线1
    QGraphicsLineItem *m_selectionV2;  // 框选垂直线2
};

class DataAcquisition : public QWidget
{
    Q_OBJECT

public:
    explicit DataAcquisition(QWidget *parent = nullptr);
    ~DataAcquisition();

    // 数据采集控制
    void startAcquisition();
    void stopAcquisition();
    void clearData();

    // 参数管理
    void addParameterDefinition(uint16_t index, uint8_t subindex, const QString& name,
                              const QString& unit, double scale);
    QString getParameterName(uint16_t index, uint8_t subindex) const;

    // CAN通信接口
    void parseUpdateProtocol(const VCI_CAN_OBJ &frame);
    void setCANTxRx(CANTxRx* canTxRx);

public slots:
    void onStartStopClicked();
    void onClearClicked();
    void onChannelCountChanged(int count);
    void onParameterSelectionChanged(int channelIndex);
    void onTimeRangeChanged(int index);
    void onAutoScaleChanged(int state);
    void onAutoModeToggled(bool enabled);
    void onValueRangeChanged();
    void updatePlot();
    void requestParameters();
    void addDebugMessage(const QString &message);  // 请求参数值
    void parseParameterResponseThreadSafe(const VCI_CAN_OBJ &frame); // 线程安全版本
    
    // CAN通信线程槽函数
    void onCANFrameSent(uint32_t id, const QByteArray& data);
    void onCANStatisticsUpdated(int sendCount, int receiveCount, double sendFreq, double receiveFreq);
    
    // 示波器相关函数
    void onCANFrameReceived(const VCI_CAN_OBJ &frame);

private:
    void setupUI();
    void setupControlPanel();
    QWidget* createControlPanelWidget();
    QWidget* createChannelConfigWidget(int channelIndex);
    void populateParameterComboBox(QComboBox *comboBox, int category);
    void updateChannelConfigVisibility();
    void updateStreamingParameters();

    // CAN通信函数
    uint32_t buildUpdateCOBId(uint8_t cmdType, uint8_t nodeId);
    void sendCANFrame(uint32_t cobId, const QByteArray& data);
    void sendParameterRead(uint8_t nodeId, uint16_t index, uint8_t subindex);
    void sendMultiParameterRead(uint8_t nodeId, uint16_t index1, uint8_t subindex1, uint16_t index2, uint8_t subindex2);
    void startStreaming(uint8_t nodeId, const QVector<QPair<uint16_t, uint8_t>>& parameters);
    void stopStreaming(uint8_t nodeId);

    // 数据解析函数
    void parseParameterResponse(const VCI_CAN_OBJ &frame);
    void parseStreamData(const VCI_CAN_OBJ &frame);
    void parseMultiChannelData(const VCI_CAN_OBJ &frame, int startByte, int endByte, uint8_t nodeId);
    double convertParameterValue(uint16_t index, uint8_t subindex, const QByteArray& data) const;

    // 数据管理 - addDataPoint函数已删除
    void updateChannelWidgetAppearance(QWidget *channelWidget, bool enabled);

    // 辅助函数
    int findParameterIndex(QComboBox *comboBox, uint16_t index, uint8_t subindex);
    void ensureAllChannelsHaveValidParameters();
    
    // 示波器相关函数
    void parseCANFrameForOscilloscope(const VCI_CAN_OBJ &frame);

private:
    // UI组件
    OscilloscopeWidget *m_oscilloscope;
    QPushButton *m_startStopButton;
    QPushButton *m_clearButton;
    QPushButton *m_autoModeButton;
    QComboBox *m_channelCountComboBox;
    QComboBox *m_timeRangeComboBox;
    QSpinBox *m_nodeIdSpinBox;
    QCheckBox *m_autoScaleCheckBox;
    QLabel *m_statusLabel;
    QLabel *m_dataRateLabel;
    
    // 自动模式控制
    QDoubleSpinBox *m_minValueSpinBox;
    QDoubleSpinBox *m_maxValueSpinBox;
    bool m_autoModeEnabled;

    // 通道配置组件
    QVector<QComboBox*> m_categoryComboBoxes;
    QVector<QComboBox*> m_parameterComboBoxes;
    QVector<QLabel*> m_channelLabels;

    // 通道配置容器
    QWidget *m_channelConfigContainer;
    QVBoxLayout *m_channelConfigLayout;

    // 数据存储
    QMap<QString, ParameterInfo> m_parameterDefinitions;
    QMap<uint8_t, bool> m_streamingStatus;
    QVector<ChannelConfig> m_channelConfigs;

    // 参数字典
    ParamDictionary *m_paramDictionary;

    // 控制变量
    bool m_isAcquiring;
    double m_startTime;
    double m_timeRange;
    int m_channelCount;

    // 定时器
    QTimer *m_plotUpdateTimer;
    QTimer *m_parameterRequestTimer;  // 参数请求定时器
    
    // CAN通信线程
    CANCommunicationThread *m_canCommThread;
    
    // CAN通信组件
    CANTxRx *m_canTxRx;
    
    // 请求状态管理
    bool m_isRequesting;              // 是否正在请求参数
    QTime m_lastRequestTime;          // 上次请求时间
    int m_pendingRequests;            // 待处理请求数量
    
    // 线程同步
    QMutex m_dataMutex;               // 数据缓冲区互斥锁
    QMutex m_stateMutex;              // 状态变量互斥锁
    
    // 队列显示
    QTextEdit *m_debugQueue;
    QStringList m_debugMessages;
    int m_maxDebugMessages;
    
    // 频率统计（测试用）
    int m_requestCount;                  // 请求计数
    int m_responseCount;                 // 响应计数
    QTime m_testStartTime;               // 测试开始时间
    
    // 通道轮询管理
    int m_currentChannelIndex;           // 当前请求的通道索引

    // 分类定义 - 与参数字典保持一致
    enum ParameterCategory {
        CATEGORY_BASIC_CONTROL = 0,    // 基本控制参数
        CATEGORY_PID_PARAMS = 1,        // PID参数
        CATEGORY_ADVANCED_SETTINGS = 2, // 高级设置
        CATEGORY_MONITORING = 3,        // 监控参数
        CATEGORY_SYSTEM_STATUS = 4,     // 系统状态
        CATEGORY_POWER_MONITOR = 5,     // 电源监控
        CATEGORY_MOTION_PLANNING = 6,   // 运动规划
        CATEGORY_OBSERVER_DEBUG = 7     // 观测器调试
    };

signals:
    void logMessage(const QString &message);
    void parameterValueReceived(uint8_t nodeId, uint16_t index, uint8_t subindex,
                               const QByteArray& data, double convertedValue);
    void streamingDataReceived(uint8_t nodeId, uint16_t index, uint8_t subindex,
                              const QByteArray& data, double convertedValue);
    void streamingStatusChanged(uint8_t nodeId, bool streaming);
    void parameterWriteResponse(uint8_t nodeId, uint16_t index, uint8_t subindex, uint8_t status);

    void dataPointAdded(const QString &key, double timestamp, double value, const QString &name);
       void streamingDataProcessed(uint16_t index, uint8_t subindex, double value);
};

#endif // DATA_ACQUISITION_H
