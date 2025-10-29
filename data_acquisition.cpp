#include "data_acquisition.h"
#include <QDebug>
#include <QByteArray>
#include <QFormLayout>
#include <QGroupBox>
#include <QDateTime>
#include <QScrollArea>
#include <QScrollBar>
#include <QTimer>
#include <QPushButton>
#include <QComboBox>
#include <QSpinBox>
#include <QLabel>
#include <QCheckBox>
#include <QTextEdit>
#include <QGraphicsScene>
#include <QGraphicsLineItem>
#include <QGraphicsPathItem>
#include <QGraphicsItem>
#include <QBrush>
#include <QPen>
#include <QColor>
#include <QPointF>
#include <QRectF>
#include <QPainterPath>
#include <algorithm>
#include <limits>
#include <iterator>
#include "can_rx_tx.h"

// 添加类型定义
#ifndef uint8_t
typedef unsigned char uint8_t;
#endif

#ifndef uint16_t
typedef unsigned short uint16_t;
#endif

// ==================== OscilloscopeWidget 实现 (使用QGraphicsView) ====================

OscilloscopeWidget::OscilloscopeWidget(QWidget *parent)
    : QGraphicsView(parent)
    , m_scene(nullptr)
    , m_timeRange(60.0)  // 2分钟时间范围
    , m_autoScale(true)
    , m_minValue(-10)
    , m_maxValue(10)
    , m_maxDataPoints(2000)  // 减少数据点限制，避免内存溢出
    , m_zoomLevel(1.0)
    , m_timeOffset(0.0)
    , m_valueOffset(0.0)
    , m_startTime(-1.0)  // 初始化为-1，表示未设置
    , m_fitButton(nullptr)
    , m_mousePositionLabel(nullptr)
    , m_crosshairH(nullptr)
    , m_crosshairV(nullptr)
    , m_dataCount(0)
    , m_dataFrequency(0.0)
    , m_frequencyLabel(nullptr)
    , m_maxTimeWindow(60.0)  // 60秒时间窗口
    , m_currentTimeOffset(0.0)
    , m_isSelecting(false)
    , m_selectionStart(0, 0)
    , m_selectionEnd(0, 0)
    , m_selectionRect(nullptr)
    , m_selectionH1(nullptr)
    , m_selectionH2(nullptr)
    , m_selectionV1(nullptr)
    , m_selectionV2(nullptr)
{
    qDebug() << "【示波器】开始创建OscilloscopeWidget";
    
    try {
        setMinimumSize(400, 200); // 设置最小尺寸，但允许扩展
        qDebug() << "【示波器】设置最小尺寸完成";
        
        setStyleSheet("background-color: #1e1e1e;");
        qDebug() << "【示波器】设置样式表完成";
        
        // 创建场景
        m_scene = new QGraphicsScene(this);
        qDebug() << "【示波器】创建QGraphicsScene完成";
        
        setScene(m_scene);
        qDebug() << "【示波器】设置场景完成";
        
        // 初始化颜色配置
        initializeCurveColors();
        qDebug() << "【示波器】初始化颜色配置完成";
        
        // 设置绘图
        setupPlot();
        qDebug() << "【示波器】设置绘图完成";
        
        // 创建全显按钮
        createFitButton();
        qDebug() << "【示波器】创建全显按钮完成";
        
        qDebug() << "【示波器】OscilloscopeWidget创建成功";
    } catch (const std::exception &e) {
        qCritical() << "【示波器】创建失败，异常:" << e.what();
    } catch (...) {
        qCritical() << "【示波器】创建失败，未知异常";
    }
}

OscilloscopeWidget::~OscilloscopeWidget()
{
    // 清理所有图形项
    if (m_scene) {
        m_scene->clear();
    }
    
    // 清理数据缓冲区
    m_dataBuffers.clear();
    m_curveColors.clear();
}

void OscilloscopeWidget::setupPlot()
{
    qDebug() << "【示波器】开始setupPlot";
    
    if (!m_scene) {
        qCritical() << "【示波器】m_scene为空，无法设置绘图";
        return;
    }
    
    // 设置场景大小（初始值，会在resizeEvent中动态调整）
    m_scene->setSceneRect(0, 0, 800,600);
    qDebug() << "【示波器】场景大小设置完成";
    
    // 设置背景色
    m_scene->setBackgroundBrush(QBrush(QColor(30, 30, 30)));
    qDebug() << "【示波器】背景色设置完成";
    
    // 绘制网格
    qDebug() << "【示波器】准备绘制网格...";
    drawGrid();
    qDebug() << "【示波器】网格绘制完成";
    
    // 创建鼠标位置显示组件
    m_mousePositionLabel = new QGraphicsTextItem();
    m_mousePositionLabel->setDefaultTextColor(QColor(255, 255, 255));
    // 设置自适应字体大小
    QFont mouseFont("Arial");
    mouseFont.setPixelSize(12);  // 基础字体大小
    m_mousePositionLabel->setFont(mouseFont);
    m_mousePositionLabel->setZValue(1000); // 确保在最上层
    m_scene->addItem(m_mousePositionLabel);
    
    // 创建十字线
    m_crosshairH = new QGraphicsLineItem();
    m_crosshairH->setPen(QPen(QColor(255, 255, 255, 100), 1, Qt::DashLine));
    m_crosshairH->setZValue(999);
    m_scene->addItem(m_crosshairH);
    
    m_crosshairV = new QGraphicsLineItem();
    m_crosshairV->setPen(QPen(QColor(255, 255, 255, 100), 1, Qt::DashLine));
    m_crosshairV->setZValue(999);
    m_scene->addItem(m_crosshairV);
    
    // 初始时隐藏十字线
    m_crosshairH->setVisible(false);
    m_crosshairV->setVisible(false);
    
    // 创建框选相关图形项
    m_selectionRect = new QGraphicsRectItem();
    m_selectionRect->setPen(QPen(QColor(255, 255, 0, 150), 2, Qt::DashLine));
    m_selectionRect->setBrush(QBrush(QColor(255, 255, 0, 30)));
    m_selectionRect->setZValue(998);
    m_selectionRect->setVisible(false);
    m_scene->addItem(m_selectionRect);
    
    // 创建框选边界线
    m_selectionH1 = new QGraphicsLineItem();
    m_selectionH1->setPen(QPen(QColor(255, 255, 0, 200), 1, Qt::DashLine));
    m_selectionH1->setZValue(998);
    m_selectionH1->setVisible(false);
    m_scene->addItem(m_selectionH1);
    
    m_selectionH2 = new QGraphicsLineItem();
    m_selectionH2->setPen(QPen(QColor(255, 255, 0, 200), 1, Qt::DashLine));
    m_selectionH2->setZValue(998);
    m_selectionH2->setVisible(false);
    m_scene->addItem(m_selectionH2);
    
    m_selectionV1 = new QGraphicsLineItem();
    m_selectionV1->setPen(QPen(QColor(255, 255, 0, 200), 1, Qt::DashLine));
    m_selectionV1->setZValue(998);
    m_selectionV1->setVisible(false);
    m_scene->addItem(m_selectionV1);
    
    m_selectionV2 = new QGraphicsLineItem();
    m_selectionV2->setPen(QPen(QColor(255, 255, 0, 200), 1, Qt::DashLine));
    m_selectionV2->setZValue(998);
    m_selectionV2->setVisible(false);
    m_scene->addItem(m_selectionV2);
    
    qDebug() << "【示波器】鼠标位置显示组件创建完成";
}

void OscilloscopeWidget::initializeCurveColors()
{
    // 预定义的颜色列表 - 更丰富的颜色
    QList<QColor> colors = {
        QColor(255, 100, 100),    // 红色
        QColor(100, 255, 100),    // 绿色
        QColor(100, 100, 255),    // 蓝色
        QColor(255, 255, 100),    // 黄色
        QColor(255, 100, 255),    // 紫色
        QColor(100, 255, 255),    // 青色
        QColor(255, 150, 100),    // 橙色
        QColor(150, 100, 255),    // 紫罗兰
        QColor(255, 200, 200),    // 浅红色
        QColor(200, 255, 200),    // 浅绿色
        QColor(200, 200, 255),    // 浅蓝色
        QColor(255, 255, 200),    // 浅黄色
        QColor(255, 200, 255),    // 浅紫色
        QColor(200, 255, 255),    // 浅青色
        QColor(255, 180, 120),    // 浅橙色
        QColor(180, 120, 255)     // 浅紫罗兰
    };
    
    // 为不同的通道ID和参数类型分配颜色
    for (int i = 0; i < 16; ++i) {
        QString channelId = QString("CH%1").arg(i);
        m_curveColors[channelId + "_速度"] = colors[i % colors.size()];
        m_curveColors[channelId + "_位置"] = QColor(static_cast<int>(colors[i % colors.size()].red() * 0.8),
                                                   static_cast<int>(colors[i % colors.size()].green() * 0.8),
                                                   static_cast<int>(colors[i % colors.size()].blue() * 0.8));
        m_curveColors[channelId + "_电流"] = QColor(static_cast<int>(colors[i % colors.size()].red() * 0.6),
                                                   static_cast<int>(colors[i % colors.size()].green() * 0.6),
                                                   static_cast<int>(colors[i % colors.size()].blue() * 0.6));
    }
}

void OscilloscopeWidget::onDataPointAdded(const QString &channelName, double timestamp, double value, const QString &parameterName)
{
    // 添加数据点到指定通道
    addDataPoint(channelName, timestamp, value, parameterName);
}

void OscilloscopeWidget::addDataPoint(const QString &channelName, double timestamp, double value, const QString &parameterName)
{
    // 检查通道名称有效性
    if (channelName.isEmpty()) {
        return;
    }
    
    // 检查数值有效性
    if (qIsNaN(value) || qIsInf(value)) {
        return; // 跳过无效数据
    }
    
    // 根据参数类型限制数值范围
    if (channelName.contains("速度")) {
        if (qAbs(value) > 10000) return; // 速度范围限制
    } else if (channelName.contains("位置")) {
        if (qAbs(value) > 2000000000) return; // 位置范围限制
    } else if (channelName.contains("电流")) {
        if (qAbs(value) > 1000) return; // 电流范围限制
    } else if (channelName.contains("实际位置")) {
        if (qAbs(value) > 2000000000) return; // 实际位置范围限制
    }
    
    // 添加数据点
    m_dataBuffers[channelName].append(QPointF(timestamp, value));
    
    // 计算数据频率
    QTime currentTime = QTime::currentTime();
    if (m_lastDataTime.isValid()) {
        m_dataCount++;
        int elapsedMs = m_lastDataTime.msecsTo(currentTime);
        if (elapsedMs >= 1000) { // 每秒更新一次频率
            m_dataFrequency = m_dataCount * 1000.0 / elapsedMs;
            m_dataCount = 0;
            m_lastDataTime = currentTime;
            updateFrequencyDisplay();
        }
    } else {
        m_lastDataTime = currentTime;
        m_dataCount = 1;
    }
    
    // 60秒滚动显示：删除超过时间窗口的数据
    double dataTime = timestamp;
    double windowStart = dataTime - m_maxTimeWindow;
    
    // 删除超出时间窗口的数据点
    QVector<QPointF>& dataBuffer = m_dataBuffers[channelName];
    while (!dataBuffer.isEmpty() && dataBuffer.first().x() < windowStart) {
        dataBuffer.removeFirst();
    }
    
    // 限制数据点数量，防止内存无限增长
    if (dataBuffer.size() > m_maxDataPoints) {
        // 使用更高效的方式移除旧数据
        dataBuffer.remove(0, dataBuffer.size() - m_maxDataPoints + 100);
    }
    
    // 更新显示范围 - 60秒滚动显示
    // 计算目标时间范围（跟随最新数据）
    double currentMaxTime = dataTime;
    double targetMinTime = qMax(0.0, currentMaxTime - m_maxTimeWindow);
    
    // 实时滚动：时间窗口跟随最新数据，保持波形相对静止
    // 直接设置时间偏移为目标值，实现实时跟随
    m_timeOffset = targetMinTime;
    
    // 更新时间范围
    m_timeRange = m_maxTimeWindow;
    
    // 更新数值范围（只有在自动缩放模式下才自动更新）
    if (m_autoScale) {
        updateValueRange();
    }
    
    // 重新绘制
    drawGrid();
    drawCurves();
    
    // 重新绘制 - 添加频率限制避免内存溢出
    static QTime lastDrawTime = QTime::currentTime();
    QTime drawTime = QTime::currentTime();
    if (lastDrawTime.msecsTo(drawTime) >= 25) { // 限制到40fps
        drawCurves();
        lastDrawTime = drawTime;
    }
}

QColor OscilloscopeWidget::getCurveColor(const QString &channelName)
{
    // 检查是否是通道1（绿色）
    if (channelName.startsWith("CH1_")) {
        return QColor(0, 255, 0);  // 绿色
    }
    
    // 检查是否是通道2（蓝色）
    if (channelName.startsWith("CH2_")) {
        return QColor(0, 100, 255);  // 蓝色
    }
    
    // 如果已经定义了颜色，使用定义的颜色
    if (m_curveColors.contains(channelName)) {
        return m_curveColors[channelName];
    }
    
    // 默认颜色（红色）
    return QColor(255, 100, 100);
}

QPointF OscilloscopeWidget::dataToScreen(double timestamp, double value)
{
    QRectF sceneRect = m_scene->sceneRect();
    double width = sceneRect.width();
    double height = sceneRect.height();
    
    // 计算X坐标（时间轴）
    // 使用时间偏移来计算相对位置
    double relativeTime = timestamp - m_timeOffset;
    double timeRatio = relativeTime / m_timeRange;
    timeRatio = qBound(0.0, timeRatio, 1.0);
    
    double x = 50 + timeRatio * (width - 100); // 留出边距
    
    // 计算Y坐标（数值轴）
    double valueRange = m_maxValue - m_minValue;
    if (valueRange < 1e-6) valueRange = 1.0;
    
    double valueRatio = (value - m_minValue) / valueRange;
    valueRatio = qBound(0.0, valueRatio, 1.0);
    
    double y = height - 50 - valueRatio * (height - 100); // 留出边距，Y轴翻转
    
//    // 添加调试信息
//    qDebug() << QString("【坐标转换】输入: 时间=%1 值=%2 -> 输出: X=%3 Y=%4 场景尺寸: %5x%6")
//                .arg(timestamp, 0, 'f', 6)
//                .arg(value, 0, 'f', 6)
//                .arg(x, 0, 'f', 1)
//                .arg(y, 0, 'f', 1)
//                .arg(width, 0, 'f', 1)
//                .arg(height, 0, 'f', 1);
    
    return QPointF(x, y);
}

QPointF OscilloscopeWidget::screenToData(const QPointF &screenPoint)
{
    QRectF sceneRect = m_scene->sceneRect();
    double width = sceneRect.width();
    double height = sceneRect.height();
    
    // 计算时间值
    double timeRatio = (screenPoint.x() - 50) / (width - 100);
    timeRatio = qBound(0.0, timeRatio, 1.0);
    double timestamp = timeRatio * m_timeRange;
    
    // 计算数值
    double valueRange = m_maxValue - m_minValue;
    if (valueRange < 1e-6) valueRange = 1.0;
    
    double valueRatio = (height - 50 - screenPoint.y()) / (height - 100);
    valueRatio = qBound(0.0, valueRatio, 1.0);
    double value = m_minValue + valueRatio * valueRange;
    
    return QPointF(timestamp, value);
}

QPointF OscilloscopeWidget::findValueAtTime(double timestamp)
{
    QPointF result(timestamp, 0.0);
    bool found = false;
    double closestValue = 0.0;
    double minTimeDiff = std::numeric_limits<double>::max();
    
    // 遍历所有可见的通道数据
    for (auto it = m_dataBuffers.begin(); it != m_dataBuffers.end(); ++it) {
        const QString &channelName = it.key();
        
        // 只处理可见的通道
        if (!m_visibleCurves.isEmpty() && !m_visibleCurves.contains(channelName)) {
            continue;
        }
        
        const QVector<QPointF> &data = it.value();
        
        // 在数据中查找最接近指定时间的数据点
        for (int i = 0; i < data.size(); ++i) {
            double timeDiff = qAbs(data[i].x() - timestamp);
            if (timeDiff < minTimeDiff) {
                minTimeDiff = timeDiff;
                closestValue = data[i].y();
                found = true;
            }
        }
    }
    
    if (found) {
        result.setY(closestValue);
    }
    
    return result;
}

QMap<QString, double> OscilloscopeWidget::findAllValuesAtTime(double timestamp)
{
    QMap<QString, double> result;
    
    // 遍历所有数据缓冲区，找到每个通道最接近时间戳的数据点
    for (auto it = m_dataBuffers.begin(); it != m_dataBuffers.end(); ++it) {
        const QString &channelName = it.key();
        const QVector<QPointF> &data = it.value();
        
        double minTimeDiff = std::numeric_limits<double>::max();
        double closestValue = 0.0;
        bool found = false;
        
        for (int i = 0; i < data.size(); ++i) {
            double timeDiff = qAbs(data[i].x() - timestamp);
            if (timeDiff < minTimeDiff) {
                minTimeDiff = timeDiff;
                closestValue = data[i].y();
                found = true;
            }
        }
        
        if (found) {
            result[channelName] = closestValue;
        }
    }
    
    return result;
}

void OscilloscopeWidget::updateMousePosition(const QPointF &mousePos)
{
    if (!m_mousePositionLabel || !m_crosshairH || !m_crosshairV) {
        return;
    }
    
    QRectF sceneRect = m_scene->sceneRect();
    QPointF dataPoint = screenToData(mousePos);
    
    // 查找所有通道在对应时间点的波形数值
    QMap<QString, double> allValues = findAllValuesAtTime(dataPoint.x());
    
    // 构建显示文本
    QString timeText;
    if (m_timeRange < 1.0) {
        // 小于1秒，显示毫秒
        timeText = QString("时间: %1 ms").arg(dataPoint.x() * 1000, 0, 'f', 1);
    } else {
        // 大于等于1秒，显示秒
        timeText = QString("时间: %1 s").arg(dataPoint.x(), 0, 'f', 3);
    }
    QString positionText = timeText + "\n";
    
    if (allValues.isEmpty()) {
        positionText += "无数据";
    } else {
        // 按通道名称排序显示
        QStringList channelNames = allValues.keys();
        channelNames.sort();
        
        for (const QString &channelName : channelNames) {
            double value = allValues[channelName];
            QColor channelColor = getCurveColor(channelName);
            
            // 提取通道号（CH1_, CH2_等）
            QString channelNumber = channelName;
            if (channelName.startsWith("CH")) {
                int underscorePos = channelName.indexOf('_');
                if (underscorePos > 0) {
                    channelNumber = channelName.left(underscorePos);
                }
            }
            
            positionText += QString("%1: %2\n")
                            .arg(channelNumber)
                            .arg(value, 0, 'f', 3);
        }
    }
    
    m_mousePositionLabel->setPlainText(positionText);
    m_mousePositionLabel->setPos(mousePos.x() + 10, mousePos.y() - 30);
    
    // 更新十字线 - 垂直线跟随鼠标，水平线显示在第一个通道的数值位置
    if (!allValues.isEmpty()) {
        QString firstChannel = allValues.keys().first();
        double firstValue = allValues[firstChannel];
        QPointF firstChannelScreenPos = dataToScreen(dataPoint.x(), firstValue);
        m_crosshairH->setLine(50, firstChannelScreenPos.y(), sceneRect.width() - 50, firstChannelScreenPos.y());
    } else {
        // 如果没有数据，水平线显示在鼠标位置
        m_crosshairH->setLine(50, mousePos.y(), sceneRect.width() - 50, mousePos.y());
    }
    
    m_crosshairV->setLine(mousePos.x(), 50, mousePos.x(), sceneRect.height() - 50);
    
    // 显示十字线
    m_crosshairH->setVisible(true);
    m_crosshairV->setVisible(true);
}

void OscilloscopeWidget::mouseMoveEvent(QMouseEvent *event)
{
    QGraphicsView::mouseMoveEvent(event);
    
    if (m_scene) {
        QPointF scenePos = mapToScene(event->pos());
        
        if (m_isSelecting) {
            // 正在框选，更新选择区域
            updateSelection(scenePos);
        } else {
            // 正常鼠标移动，更新位置显示
            updateMousePosition(scenePos);
        }
    }
}

void OscilloscopeWidget::mousePressEvent(QMouseEvent *event)
{
    QGraphicsView::mousePressEvent(event);
    
    if (event->button() == Qt::RightButton && m_scene) {
        QPointF scenePos = mapToScene(event->pos());
        startSelection(scenePos);
    }
}

void OscilloscopeWidget::mouseReleaseEvent(QMouseEvent *event)
{
    QGraphicsView::mouseReleaseEvent(event);
    
    if (event->button() == Qt::RightButton && m_isSelecting && m_scene) {
        QPointF scenePos = mapToScene(event->pos());
        endSelection(scenePos);
    }
}

void OscilloscopeWidget::leaveEvent(QEvent *event)
{
    QGraphicsView::leaveEvent(event);
    
    // 隐藏十字线和位置标签
    if (m_crosshairH) m_crosshairH->setVisible(false);
    if (m_crosshairV) m_crosshairV->setVisible(false);
    if (m_mousePositionLabel) m_mousePositionLabel->setPlainText("");
}

void OscilloscopeWidget::resizeEvent(QResizeEvent *event)
{
    QGraphicsView::resizeEvent(event);
    
    if (m_scene) {
        // 根据组件大小动态调整场景尺寸
        QSize newSize = event->size();
        m_scene->setSceneRect(0, 0, newSize.width(), newSize.height());
        
        // 根据组件大小动态调整字体大小
        adjustFontSizes(newSize);
        
        // 重新绘制网格和坐标轴
        drawGrid();
        
        // 更新全显按钮位置到中间最上面
        if (m_fitButton) {
            int buttonWidth = 200;  // 按钮宽度
            int buttonHeight = 40;  // 按钮高度
            int margin = 15;        // 边距
            int centerX = (newSize.width() - buttonWidth) / 2;
            int topY = margin;  // 放在最上面
            m_fitButton->setGeometry(centerX, topY, buttonWidth, buttonHeight);
        }
        
        // 更新频率标签位置
        if (m_frequencyLabel) {
            QRectF sceneRect = m_scene->sceneRect();
            m_frequencyLabel->setPos(sceneRect.width() - m_frequencyLabel->boundingRect().width() - 10, 10);
        }
        
        qDebug() << QString("【示波器】尺寸调整: %1x%2")
                    .arg(newSize.width())
                    .arg(newSize.height());
    }
}

void OscilloscopeWidget::updateFrequencyDisplay()
{
    if (!m_scene) return;
    
    // 移除旧的频率标签
    if (m_frequencyLabel) {
        m_scene->removeItem(m_frequencyLabel);
        delete m_frequencyLabel;
        m_frequencyLabel = nullptr;
    }
    
    // 创建新的频率标签
    QString frequencyText = QString("数据率: %1 Hz").arg(QString::number(m_dataFrequency, 'f', 1));
    m_frequencyLabel = new QGraphicsTextItem(frequencyText);
    m_frequencyLabel->setDefaultTextColor(Qt::white);
    
    // 设置字体大小
    QFont font = m_frequencyLabel->font();
    font.setPointSize(12);
    m_frequencyLabel->setFont(font);
    
    // 设置位置（右上角）
    QRectF sceneRect = m_scene->sceneRect();
    m_frequencyLabel->setPos(sceneRect.width() - m_frequencyLabel->boundingRect().width() - 10, 10);
    
    // 添加到场景
    m_scene->addItem(m_frequencyLabel);
}

void OscilloscopeWidget::adjustFontSizes(const QSize &size)
{
    // 根据组件大小计算合适的字体大小
    int baseFontSize = qMax(8, qMin(16, size.width() / 60));  // 基础字体大小，范围8-16
    int titleFontSize = qMax(10, qMin(18, size.width() / 50)); // 标题字体大小，范围10-18
    int labelFontSize = qMax(8, qMin(14, size.width() / 70));  // 标签字体大小，范围8-14
    
    // 更新鼠标位置标签字体
    if (m_mousePositionLabel) {
        QFont mouseFont = m_mousePositionLabel->font();
        mouseFont.setPixelSize(baseFontSize);
        m_mousePositionLabel->setFont(mouseFont);
    }
    
    // 更新所有轴标签的字体大小
    QList<QGraphicsItem*> items = m_scene->items();
    for (QGraphicsItem* item : items) {
        if (item->data(0).toString() == "axis_label") {
            QGraphicsTextItem* textItem = qgraphicsitem_cast<QGraphicsTextItem*>(item);
            if (textItem) {
                QFont font = textItem->font();
                // 根据文本内容判断是标题还是标签
                QString text = textItem->toPlainText();
                if (text.contains("时间") || text.contains("数值")) {
                    font.setPixelSize(titleFontSize);
                } else {
                    font.setPixelSize(labelFontSize);
                }
                textItem->setFont(font);
            }
        }
    }
}

void OscilloscopeWidget::drawGrid()
{
    qDebug() << "【示波器】开始drawGrid";
    
    if (!m_scene) {
        qCritical() << "【示波器】drawGrid: m_scene为空";
        return;
    }
    
    // 清除现有网格
    QList<QGraphicsItem*> items = m_scene->items();
    qDebug() << "【示波器】当前场景中有" << items.size() << "个图形项";
    
    for (QGraphicsItem* item : items) {
        if (item && item->data(0).toString() == "grid") {
            m_scene->removeItem(item);
            delete item;
        }
    }
    qDebug() << "【示波器】清除旧网格完成";
    
    QRectF sceneRect = m_scene->sceneRect();
    double width = sceneRect.width();
    double height = sceneRect.height();
    qDebug() << "【示波器】场景尺寸: width=" << width << ", height=" << height;
    
    // 绘制网格线
    QPen gridPen(QColor(80, 80, 80), 1, Qt::DotLine);
    
    // 垂直网格线（时间轴）
    qDebug() << "【示波器】开始绘制垂直网格线...";
    for (int i = 0; i <= 10; i++) {
        double x = 50 + (width - 100) * i / 10.0;
        QGraphicsLineItem* line = m_scene->addLine(x, 50, x, height - 50, gridPen);
        if (line) {
            line->setData(0, "grid");
        }
    }
    qDebug() << "【示波器】垂直网格线绘制完成";
    
    // 水平网格线（数值轴）
    qDebug() << "【示波器】开始绘制水平网格线...";
    for (int i = 0; i <= 10; i++) {
        double y = 50 + (height - 100) * i / 10.0;
        QGraphicsLineItem* line = m_scene->addLine(50, y, width - 50, y, gridPen);
        if (line) {
            line->setData(0, "grid");
        }
    }
    qDebug() << "【示波器】水平网格线绘制完成";
    
    // 绘制坐标轴刻度标签
    drawAxisLabels();
    qDebug() << "【示波器】坐标轴刻度标签绘制完成";
    qDebug() << "【示波器】drawGrid完成";
}

void OscilloscopeWidget::drawAxisLabels()
{
    if (!m_scene) {
        return;
    }
    
    QRectF sceneRect = m_scene->sceneRect();
    double width = sceneRect.width();
    double height = sceneRect.height();
    
    // 清除旧的标签
    QList<QGraphicsItem*> items = m_scene->items();
    for (QGraphicsItem* item : items) {
        if (item && item->data(0).toString() == "axis_label") {
            m_scene->removeItem(item);
            delete item;
        }
    }
    
    // 绘制时间轴标签
    drawTimeAxis();
    
    // 绘制数值轴标签
    drawValueAxis();
}

void OscilloscopeWidget::drawTimeAxis()
{
    if (!m_scene) {
        return;
    }
    
    QRectF sceneRect = m_scene->sceneRect();
    double width = sceneRect.width();
    double height = sceneRect.height();
    
    // 时间轴标签（底部）
    for (int i = 0; i <= 10; i++) {
        double x = 50 + (width - 100) * i / 10.0;
        double timeValue = m_timeRange * i / 10.0;
        
        QGraphicsTextItem* label = new QGraphicsTextItem();
        
        // 根据时间范围智能选择显示格式
        QString timeText;
        if (m_timeRange < 0.1) {
            // 小于0.1秒，显示毫秒
            timeText = QString("%1 ms").arg(timeValue * 1000, 0, 'f', 1);
        } else if (m_timeRange < 1.0) {
            // 小于1秒，显示毫秒（3位小数）
            timeText = QString("%1 ms").arg(timeValue * 1000, 0, 'f', 0);
        } else if (m_timeRange < 10.0) {
            // 小于10秒，显示秒（3位小数）
            timeText = QString("%1 s").arg(timeValue, 0, 'f', 3);
        } else if (m_timeRange < 100.0) {
            // 小于100秒，显示秒（2位小数）
            timeText = QString("%1 s").arg(timeValue, 0, 'f', 2);
        } else {
            // 大于100秒，显示秒（1位小数）
            timeText = QString("%1 s").arg(timeValue, 0, 'f', 1);
        }
        
        label->setPlainText(timeText);
        label->setDefaultTextColor(QColor(200, 200, 200));
        // 设置自适应字体大小
        QFont axisFont("Arial");
        axisFont.setPixelSize(20);  // 轴标签字体大小（增大一倍）
        label->setFont(axisFont);
        label->setPos(x - 20, height - 30);
        label->setData(0, "axis_label");
        m_scene->addItem(label);
    }
    
    // 时间轴标题
    QGraphicsTextItem* timeTitle = new QGraphicsTextItem();
    
    // 根据时间范围选择标题
    QString titleText;
    if (m_timeRange < 1.0) {
        titleText = "时间 (ms)";
    } else {
        titleText = "时间 (s)";
    }
    
    timeTitle->setPlainText(titleText);
    timeTitle->setDefaultTextColor(QColor(255, 255, 255));
    // 设置自适应字体大小
    QFont titleFont("Arial");
    titleFont.setPixelSize(24);  // 标题字体大小（增大一倍）
    titleFont.setBold(true);
    timeTitle->setFont(titleFont);
    timeTitle->setPos(width / 2 - 30, height - 15);
    timeTitle->setData(0, "axis_label");
    m_scene->addItem(timeTitle);
}

void OscilloscopeWidget::drawValueAxis()
{
    if (!m_scene) {
        return;
    }
    
    QRectF sceneRect = m_scene->sceneRect();
    double width = sceneRect.width();
    double height = sceneRect.height();
    
//    // 添加调试信息
//    qDebug() << QString("【纵轴刻度】数值范围: %1 到 %2")
//                .arg(m_minValue, 0, 'f', 3)
//                .arg(m_maxValue, 0, 'f', 3);
    
    // 数值轴标签（左侧）
    for (int i = 0; i <= 10; i++) {
        double y = 50 + (height - 100) * i / 10.0;
        double valueRange = m_maxValue - m_minValue;
        if (valueRange < 1e-6) {
            valueRange = 1.0;
            qDebug() << "【纵轴刻度】警告：数值范围太小，使用默认值1.0";
        }
        double value = m_maxValue - valueRange * i / 10.0;
        
        QGraphicsTextItem* label = new QGraphicsTextItem();
        label->setPlainText(QString("%1").arg(value, 0, 'f', 2));
        label->setDefaultTextColor(QColor(200, 200, 200));
        // 设置自适应字体大小
        QFont axisFont("Arial");
        axisFont.setPixelSize(20);  // 轴标签字体大小（增大一倍）
        label->setFont(axisFont);
        label->setPos(5, y - 8);
        label->setData(0, "axis_label");
        m_scene->addItem(label);
        
//        // 添加调试信息
//        qDebug() << QString("【纵轴刻度】位置 %1: Y=%2, 值=%3")
//                    .arg(i)
//                    .arg(y, 0, 'f', 1)
//                    .arg(value, 0, 'f', 3);
    }
    
    // 数值轴标题
    QGraphicsTextItem* valueTitle = new QGraphicsTextItem();
    valueTitle->setPlainText("数值");
    valueTitle->setDefaultTextColor(QColor(255, 255, 255));
    // 设置自适应字体大小
    QFont valueTitleFont("Arial");
    valueTitleFont.setPixelSize(24);  // 标题字体大小（增大一倍）
    valueTitleFont.setBold(true);
    valueTitle->setFont(valueTitleFont);
    valueTitle->setPos(10, 20);
    valueTitle->setData(0, "axis_label");
    m_scene->addItem(valueTitle);
}

void OscilloscopeWidget::drawCurves()
{
    if (!m_scene) {
        return;
    }
    
    // 清除现有曲线
    QList<QGraphicsItem*> items = m_scene->items();
    for (QGraphicsItem* item : items) {
        if (item && item->data(0).toString() == "curve") {
            m_scene->removeItem(item);
            delete item;
        }
    }
    
    // 限制绘制的数据点数量，避免内存溢出
    const int MAX_DRAW_POINTS = 200; // 进一步减少到200个点
    
    // 绘制所有通道的曲线
    for (auto it = m_dataBuffers.begin(); it != m_dataBuffers.end(); ++it) {
        const QString &channelName = it.key();
        const QVector<QPointF> &data = it.value();
        
        if (data.isEmpty()) continue;
        
        QColor color = getCurveColor(channelName);
        QPen curvePen(color, 2);
        
        // 计算采样步长，避免绘制过多点
        int step = qMax(1, data.size() / MAX_DRAW_POINTS);
        
        // 使用QPainterPath减少QGraphicsItem数量
        QPainterPath path;
        bool firstPoint = true;
        
        for (int i = 0; i < data.size(); i += step) {
            QPointF screenPoint = dataToScreen(data[i].x(), data[i].y());
            
            // 检查点是否在可见范围内
            if (screenPoint.x() >= 0 && screenPoint.x() <= m_scene->width()) {
                if (firstPoint) {
                    path.moveTo(screenPoint);
                    firstPoint = false;
                } else {
                    path.lineTo(screenPoint);
                }
            }
        }
        
        // 只创建一个QGraphicsPathItem而不是多个QGraphicsLineItem
        if (!path.isEmpty()) {
            QGraphicsPathItem* pathItem = m_scene->addPath(path, curvePen);
            pathItem->setData(0, "curve");
        }
    }
}

void OscilloscopeWidget::clearData()
{
    // 清空所有数据
    m_dataBuffers.clear();
    
    // 重置时间
    m_startTime = -1.0;
    
    // 重新绘制
    drawCurves();
}

void OscilloscopeWidget::setTimeRange(double range)
{
    m_timeRange = range;
    drawGrid();
    drawCurves();
}

void OscilloscopeWidget::setAutoScale(bool autoScale)
{
    m_autoScale = autoScale;
    if (autoScale) {
        updateValueRange();
    }
}

void OscilloscopeWidget::setFixedValueRange(double minValue, double maxValue)
{
    qDebug() << QString("【固定范围】setFixedValueRange被调用: %1 ~ %2")
                .arg(minValue, 0, 'f', 3)
                .arg(maxValue, 0, 'f', 3);
    
    m_minValue = minValue;
    m_maxValue = maxValue;
    m_autoScale = false;  // 禁用自动缩放
    
    qDebug() << "【固定范围】m_autoScale设置为false";
    
    // 重新绘制整个示波器
    qDebug() << "【固定范围】开始重绘示波器";
    drawGrid();
    drawAxisLabels();
    drawCurves();
    
    qDebug() << QString("【固定范围】设置完成，当前范围: %1 ~ %2")
                .arg(m_minValue, 0, 'f', 3)
                .arg(m_maxValue, 0, 'f', 3);
}

void OscilloscopeWidget::setVisibleCurves(const QList<QString> &visibleKeys)
{
    m_visibleCurves = visibleKeys;
    drawCurves();
}

void OscilloscopeWidget::zoomIn()
{
    scale(1.2, 1.2);
}

void OscilloscopeWidget::zoomOut()
{
    scale(0.8, 0.8);
}

void OscilloscopeWidget::resetZoom()
{
    resetTransform();
}

void OscilloscopeWidget::setZoomLevel(double level)
{
    m_zoomLevel = level;
    resetTransform();
    scale(level, level);
}

void OscilloscopeWidget::setPanOffset(double timeOffset, double valueOffset)
{
    m_timeOffset = timeOffset;
    m_valueOffset = valueOffset;
    // 平移功能可以通过QGraphicsView的滚动实现
}

void OscilloscopeWidget::fitToData()
{
    if (m_dataBuffers.isEmpty()) {
        return;
    }
    
    // 计算所有数据的时间范围和数值范围
    double minTime = std::numeric_limits<double>::max();
    double maxTime = std::numeric_limits<double>::lowest();
    double minValue = std::numeric_limits<double>::max();
    double maxValue = std::numeric_limits<double>::lowest();
    
    bool hasData = false;
    
    // 遍历所有通道的数据
    for (auto it = m_dataBuffers.begin(); it != m_dataBuffers.end(); ++it) {
        const QVector<QPointF> &data = it.value();
        for (const QPointF &point : data) {
            double time = point.x();
            double value = point.y();
            
            if (!qIsNaN(time) && !qIsInf(time) && !qIsNaN(value) && !qIsInf(value)) {
                minTime = qMin(minTime, time);
                maxTime = qMax(maxTime, time);
                minValue = qMin(minValue, value);
                maxValue = qMax(maxValue, value);
                hasData = true;
            }
        }
    }
    
    if (!hasData) {
        return;
    }
    
    // 添加一些边距
    double timeRange = maxTime - minTime;
    double valueRange = maxValue - minValue;
    
    if (timeRange < 1e-6) timeRange = 1.0;  // 最小时间范围
    if (valueRange < 1e-6) valueRange = 1.0;  // 最小数值范围
    
    double timeMargin = timeRange * 0.05;  // 5%边距
    double valueMargin = valueRange * 0.1;  // 10%边距
    
    minTime -= timeMargin;
    maxTime += timeMargin;
    minValue -= valueMargin;
    maxValue += valueMargin;
    
    // 更新显示范围
    m_timeRange = maxTime - minTime;
    m_minValue = minValue;
    m_maxValue = maxValue;
    
    // 重置缩放和平移
    m_zoomLevel = 1.0;
    m_timeOffset = 0.0;
    m_valueOffset = 0.0;
    
    // 恢复到自动模式
    m_autoScale = true;
    
    // 重新绘制
    drawGrid();
    drawAxisLabels();
    drawCurves();
    
    qDebug() << QString("【全显】时间范围: %1 - %2, 数值范围: %3 - %4")
                .arg(minTime, 0, 'f', 3)
                .arg(maxTime, 0, 'f', 3)
                .arg(minValue, 0, 'f', 3)
                .arg(maxValue, 0, 'f', 3);
}

void OscilloscopeWidget::startSelection(const QPointF &startPos)
{
    if (!m_autoScale) {
        // 只有在自动模式下才允许框选
        return;
    }
    
    m_isSelecting = true;
    m_selectionStart = startPos;
    m_selectionEnd = startPos;
    
    // 隐藏十字线
    if (m_crosshairH) m_crosshairH->setVisible(false);
    if (m_crosshairV) m_crosshairV->setVisible(false);
    
    // 显示框选图形项
    if (m_selectionRect) m_selectionRect->setVisible(true);
    if (m_selectionH1) m_selectionH1->setVisible(true);
    if (m_selectionH2) m_selectionH2->setVisible(true);
    if (m_selectionV1) m_selectionV1->setVisible(true);
    if (m_selectionV2) m_selectionV2->setVisible(true);
    
    qDebug() << "【框选】开始框选，起始位置:" << startPos;
}

void OscilloscopeWidget::updateSelection(const QPointF &currentPos)
{
    if (!m_isSelecting) return;
    
    m_selectionEnd = currentPos;
    
    // 计算选择区域
    QRectF selectionRect = QRectF(m_selectionStart, m_selectionEnd).normalized();
    
    // 限制在有效区域内
    QRectF sceneRect = m_scene->sceneRect();
    QRectF validRect(50, 50, sceneRect.width() - 100, sceneRect.height() - 100);
    selectionRect = selectionRect.intersected(validRect);
    
    // 更新矩形
    if (m_selectionRect) {
        m_selectionRect->setRect(selectionRect);
    }
    
    // 更新边界线
    if (m_selectionH1) {
        m_selectionH1->setLine(selectionRect.left(), selectionRect.top(), 
                              selectionRect.right(), selectionRect.top());
    }
    if (m_selectionH2) {
        m_selectionH2->setLine(selectionRect.left(), selectionRect.bottom(), 
                              selectionRect.right(), selectionRect.bottom());
    }
    if (m_selectionV1) {
        m_selectionV1->setLine(selectionRect.left(), selectionRect.top(), 
                              selectionRect.left(), selectionRect.bottom());
    }
    if (m_selectionV2) {
        m_selectionV2->setLine(selectionRect.right(), selectionRect.top(), 
                              selectionRect.right(), selectionRect.bottom());
    }
}

void OscilloscopeWidget::endSelection(const QPointF &endPos)
{
    if (!m_isSelecting) return;
    
    m_selectionEnd = endPos;
    
    // 计算选择区域
    QRectF selectionRect = QRectF(m_selectionStart, m_selectionEnd).normalized();
    
    // 检查选择区域是否足够大
    if (selectionRect.width() < 10 || selectionRect.height() < 10) {
        // 选择区域太小，取消选择
        clearSelection();
        return;
    }
    
    // 执行放大
    zoomToSelection();
    
    // 清除选择状态
    clearSelection();
    
    qDebug() << "【框选】结束框选，选择区域:" << selectionRect;
}

void OscilloscopeWidget::clearSelection()
{
    m_isSelecting = false;
    
    // 隐藏框选图形项
    if (m_selectionRect) m_selectionRect->setVisible(false);
    if (m_selectionH1) m_selectionH1->setVisible(false);
    if (m_selectionH2) m_selectionH2->setVisible(false);
    if (m_selectionV1) m_selectionV1->setVisible(false);
    if (m_selectionV2) m_selectionV2->setVisible(false);
}

void OscilloscopeWidget::zoomToSelection()
{
    if (!m_scene) return;
    
    // 计算选择区域的数据坐标
    QPointF startData = screenToData(m_selectionStart);
    QPointF endData = screenToData(m_selectionEnd);
    
    // 计算新的时间范围
    double newTimeMin = qMin(startData.x(), endData.x());
    double newTimeMax = qMax(startData.x(), endData.x());
    double newTimeRange = newTimeMax - newTimeMin;
    
    // 计算新的数值范围
    double newValueMin = qMin(startData.y(), endData.y());
    double newValueMax = qMax(startData.y(), endData.y());
    double newValueRange = newValueMax - newValueMin;
    
    // 添加10%的边距
    double timeMargin = newTimeRange * 0.1;
    double valueMargin = newValueRange * 0.1;
    
    newTimeMin -= timeMargin;
    newTimeMax += timeMargin;
    newValueMin -= valueMargin;
    newValueMax += valueMargin;
    
    // 更新显示范围
    m_timeOffset = newTimeMin;
    m_timeRange = newTimeMax - newTimeMin;
    m_minValue = newValueMin;
    m_maxValue = newValueMax;
    m_autoScale = false;  // 切换到固定范围模式
    
    // 重新绘制
    drawGrid();
    drawAxisLabels();
    drawCurves();
    
    qDebug() << QString("【框选放大】时间范围: %1 - %2, 数值范围: %3 - %4")
                .arg(newTimeMin, 0, 'f', 3)
                .arg(newTimeMax, 0, 'f', 3)
                .arg(newValueMin, 0, 'f', 3)
                .arg(newValueMax, 0, 'f', 3);
}

void OscilloscopeWidget::updateValueRange()
{
    if (!m_autoScale) return;
    
    double minVal = std::numeric_limits<double>::max();
    double maxVal = std::numeric_limits<double>::lowest();
    
    // 遍历所有数据
    for (auto it = m_dataBuffers.begin(); it != m_dataBuffers.end(); ++it) {
        const QVector<QPointF> &data = it.value();
        for (const QPointF &point : data) {
            double value = point.y();
            if (!qIsNaN(value) && !qIsInf(value)) {
                minVal = qMin(minVal, value);
                maxVal = qMax(maxVal, value);
            }
        }
    }
    
    if (minVal != std::numeric_limits<double>::max() && maxVal != std::numeric_limits<double>::lowest()) {
        double range = maxVal - minVal;
        if (range < 1e-6) { range = 1.0; }
        
        double newMinValue = minVal - range * 0.1;
        double newMaxValue = maxVal + range * 0.1;
        
        // 应用硬限制
        newMinValue = qBound(-2000000.0, newMinValue, 2000000.0);
        newMaxValue = qBound(-2000000.0, newMaxValue, 2000000.0);
        
        if (newMinValue < newMaxValue) {
            m_minValue = newMinValue;
            m_maxValue = newMaxValue;
            
            // 重新绘制坐标轴标签
            drawAxisLabels();
            
//            qDebug() << QString("【数值范围更新】新范围: %1 到 %2")
//                        .arg(m_minValue, 0, 'f', 3)
//                        .arg(m_maxValue, 0, 'f', 3);
        }
    }
}

void OscilloscopeWidget::cleanupOldData()
{
    double currentTime = QDateTime::currentDateTime().toMSecsSinceEpoch() / 1000.0;
    double cutoffTime = currentTime - m_timeRange * 2;
    
    for (auto it = m_dataBuffers.begin(); it != m_dataBuffers.end(); ++it) {
        QVector<QPointF> &data = it.value();
        data.erase(std::remove_if(data.begin(), data.end(),
            [cutoffTime](const QPointF &point) {
                return point.x() < cutoffTime;
            }), data.end());
    }
}

void OscilloscopeWidget::createFitButton()
{
    // 创建全显按钮
    m_fitButton = new QPushButton("全显", this);
    
    // 延迟设置位置，避免在构造时使用width()
    int buttonWidth = 200;  // 增加按钮宽度一倍
    int buttonHeight = 40;  // 增加按钮高度
    int margin = 15;        // 增加边距
    int widgetWidth = qMax(width(), 800); // 使用最小宽度避免除0
    int widgetHeight = qMax(height(), 600); // 使用最小高度避免除0
    
    // 计算中间最上面位置
    int centerX = (widgetWidth - buttonWidth) / 2;
    int topY = margin;  // 放在最上面，使用边距
    
    m_fitButton->setGeometry(centerX, topY, buttonWidth, buttonHeight);
    m_fitButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #404040;"
        "    color: white;"
        "    border: 1px solid #606060;"
        "    border-radius: 3px;"
        "    padding: 5px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #505050;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #303030;"
        "}"
    );
    
    connect(m_fitButton, &QPushButton::clicked, this, &OscilloscopeWidget::fitToData);
}

// ==================== DataAcquisition 实现 ====================

DataAcquisition::DataAcquisition(QWidget *parent) : QWidget(parent)
    , m_isAcquiring(false)
    , m_startTime(0)
    , m_timeRange(10.0)
    , m_channelCount(1)
    , m_paramDictionary(nullptr)
    , m_canTxRx(nullptr)
    , m_isRequesting(false)
    , m_autoModeEnabled(false)
    , m_pendingRequests(0)
    , m_maxDebugMessages(1000)
    , m_requestCount(0)
    , m_responseCount(0)
    , m_currentChannelIndex(0)
    , m_oscilloscope(nullptr)
{
    qDebug() << "【数据采集】开始创建DataAcquisition";
    
    try {
        // 初始化参数字典
        m_paramDictionary = new ParamDictionary(this);
        qDebug() << "【数据采集】参数字典创建完成";

        // 先创建示波器组件（在setupUI之前，因为setupUI会使用它）
        qDebug() << "【数据采集】准备创建示波器组件...";
            m_oscilloscope = new OscilloscopeWidget(this);
        qDebug() << "【数据采集】示波器组件创建完成";
        
        // 连接信号
        connect(this, &DataAcquisition::dataPointAdded,
                m_oscilloscope, &OscilloscopeWidget::onDataPointAdded);
        qDebug() << "【数据采集】信号连接完成";

        // 创建UI
        qDebug() << "【数据采集】准备创建UI...";
        setupUI();
        qDebug() << "【数据采集】UI创建完成";
    } catch (const std::exception &e) {
        qCritical() << "【数据采集】创建失败，异常:" << e.what();
    } catch (...) {
        qCritical() << "【数据采集】创建失败，未知异常";
    }
    
    // 初始化绘图更新定时器
    m_plotUpdateTimer = new QTimer(this);
    m_plotUpdateTimer->setInterval(25); // 40Hz更新频率
    connect(m_plotUpdateTimer, &QTimer::timeout, this, &DataAcquisition::updatePlot);
    
    // 初始化参数请求定时器
    m_parameterRequestTimer = new QTimer(this);
    m_parameterRequestTimer->setInterval(25); // 40Hz间隔
    m_parameterRequestTimer->setTimerType(Qt::PreciseTimer);
    connect(m_parameterRequestTimer, &QTimer::timeout, this, &DataAcquisition::requestParameters);
}

DataAcquisition::~DataAcquisition()
{
    // 停止数据采集
    stopAcquisition();
    
    // 确保定时器被停止
    if (m_plotUpdateTimer) {
        m_plotUpdateTimer->stop();
    }
    if (m_parameterRequestTimer) {
        m_parameterRequestTimer->stop();
    }
    
    // 清理示波器数据
    if (m_oscilloscope) {
        m_oscilloscope->clearData();
    }
}

void DataAcquisition::setupUI()
{
    qDebug() << "【UI创建】开始setupUI";
    
    // 创建主布局：垂直排列
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(15);
    
    // 先创建通道配置容器（setupControlPanel需要用到）
    m_channelConfigContainer = new QWidget();
    m_channelConfigContainer->setStyleSheet(
        "QWidget {"
        "    background-color: #353535;"
        "    border: 1px solid #555555;"
        "    border-radius: 5px;"
        "    padding: 6px;"
        "    font-size: 0.125em;"
        "}"
        "QComboBox {"
        "    font-size: 0.125em;"
        "    padding: 0.05em 0.1em;"
        "}"
        "QLabel {"
        "    font-size: 0.125em;"
        "}"
    );
    m_channelConfigLayout = new QVBoxLayout(m_channelConfigContainer);
    m_channelConfigLayout->setContentsMargins(10, 10, 10, 10);  // 减少边距
    m_channelConfigLayout->setSpacing(0);  // 去掉间距
    
    // 创建控制面板（现在可以安全调用updateChannelConfigVisibility）
    setupControlPanel();
    
    // 创建调试队列
    m_debugQueue = new QTextEdit();
    m_debugQueue->setMaximumHeight(80);
    m_debugQueue->setStyleSheet(
        "QTextEdit {"
        "    background-color: #2b2b2b;"
        "    color: #ffffff;"
        "    border: 1px solid #555555;"
        "    font-family: 'Consolas', 'Monaco', monospace;"
        "    font-size: 0.25em;"
        "}"
    );
    m_debugQueue->setReadOnly(true);

    // 创建上方控制区域（水平布局）
    QHBoxLayout *topLayout = new QHBoxLayout();
    topLayout->setSpacing(15);  // 减少间距
    
    // 左侧：控制面板 - 占1/2宽度
    QWidget *controlPanel = createControlPanelWidget();
    controlPanel->setMaximumHeight(220);  // 限制最大高度
    controlPanel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);  // 允许水平拉伸
    topLayout->addWidget(controlPanel, 1);  // 拉伸比例1
    
    // 右侧：通道配置 - 占1/2宽度
    QVBoxLayout *channelLayout = new QVBoxLayout();
    channelLayout->setSpacing(5);
    
    m_channelConfigContainer->setMaximumHeight(220);  // 限制最大高度
    m_channelConfigContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);  // 允许水平拉伸
    channelLayout->addWidget(m_channelConfigContainer);
    topLayout->addLayout(channelLayout, 1);  // 拉伸比例1
    
    // 添加示波器
    if (m_oscilloscope) {
        // 移除固定高度限制，让示波器自适应容器大小
        m_oscilloscope->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    } else {
        qCritical() << "【UI创建】示波器组件为空！";
    }
    
    // 创建状态栏
    QHBoxLayout* statusLayout = new QHBoxLayout();
    m_statusLabel = new QLabel("状态: 未连接");
    m_statusLabel->setStyleSheet("color: #ffffff; font-size: 0.3em;");
    statusLayout->addWidget(m_statusLabel);
    
    statusLayout->addStretch();
    
    m_dataRateLabel = new QLabel("数据率: 0 Hz");
    m_dataRateLabel->setStyleSheet("color: #ffffff; font-size: 0.3em;");
    statusLayout->addWidget(m_dataRateLabel);
    
    // 添加到主布局，设置拉伸比例
    mainLayout->addLayout(topLayout, 2);        // 上方容器拉伸，变高一倍
    mainLayout->addWidget(m_oscilloscope, 3);   // 示波器拉伸，相应变短
    mainLayout->addLayout(statusLayout, 0);     // 状态栏不拉伸
    mainLayout->addWidget(m_debugQueue, 0);     // 调试队列不拉伸
    
    qDebug() << "【UI创建】setupUI完成";
}

void DataAcquisition::setupControlPanel()
{
    qDebug() << "【控制面板】开始创建控制按钮...";
    // 控制按钮
    m_startStopButton = new QPushButton("开始采集");
    qDebug() << "【控制面板】开始采集按钮创建完成";
    
    m_clearButton = new QPushButton("清空数据");
    qDebug() << "【控制面板】清空数据按钮创建完成";
    
    // 创建自动模式按钮
    m_autoModeButton = new QPushButton("自动模式");
    m_autoModeButton->setCheckable(true);
    m_autoModeButton->setChecked(false);
    m_autoModeButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_autoModeButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #404040;"
        "    color: white;"
        "    border: 1px solid #606060;"
        "    border-radius: 3px;"
        "    padding: 5px 10px;"
        "    font-weight: bold;"
        "    text-align: center;"
        "}"
        "QPushButton:hover {"
        "    background-color: #505050;"
        "}"
        "QPushButton:checked {"
        "    background-color: #2196f3;"
        "    color: white;"
        "    border-color: #1976d2;"
        "    text-align: center;"
        "}"
    );
    qDebug() << "【控制面板】自动模式按钮创建完成";
    
    // 创建上下限文本框
    m_minValueSpinBox = new QDoubleSpinBox();
    m_minValueSpinBox->setRange(-999999.0, 999999.0);
    m_minValueSpinBox->setValue(-10.0);
    m_minValueSpinBox->setDecimals(3);
    m_minValueSpinBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_minValueSpinBox->setStyleSheet(
        "QDoubleSpinBox {"
        "    background-color: #505050;"
        "    color: #ffffff;"
        "    border: 1px solid #666666;"
        "    border-radius: 3px;"
        "    padding: 2px 5px;"
        "    font-size: 0.8em;"
        "}"
    );
    qDebug() << "【控制面板】下限文本框创建完成";
    
    m_maxValueSpinBox = new QDoubleSpinBox();
    m_maxValueSpinBox->setRange(-999999.0, 999999.0);
    m_maxValueSpinBox->setValue(10.0);
    m_maxValueSpinBox->setDecimals(3);
    m_maxValueSpinBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_maxValueSpinBox->setStyleSheet(
        "QDoubleSpinBox {"
        "    background-color: #505050;"
        "    color: #ffffff;"
        "    border: 1px solid #666666;"
        "    border-radius: 3px;"
        "    padding: 2px 5px;"
        "    font-size: 0.8em;"
        "}"
    );
    qDebug() << "【控制面板】上限文本框创建完成";

    // 通道数量选择
    qDebug() << "【控制面板】创建通道数量选择...";
    m_channelCountComboBox = new QComboBox();
    m_channelCountComboBox->addItem("1个通道", 1);
    m_channelCountComboBox->addItem("2个通道", 2);
    m_channelCountComboBox->addItem("4个通道", 4);
    m_channelCountComboBox->addItem("8个通道", 8);
    m_channelCountComboBox->addItem("16个通道", 16);
    qDebug() << "【控制面板】通道数量选择创建完成";

    // 删除时间范围选择，不再需要
    
    // 节点ID输入
    qDebug() << "【控制面板】创建节点ID输入...";
    m_nodeIdSpinBox = new QSpinBox();
    m_nodeIdSpinBox->setRange(1, 127);
    m_nodeIdSpinBox->setValue(5);
    qDebug() << "【控制面板】节点ID输入创建完成";
    
    // 自动缩放选项
    qDebug() << "【控制面板】创建自动缩放选项...";
    m_autoScaleCheckBox = new QCheckBox("自动缩放");
    m_autoScaleCheckBox->setChecked(true);
    qDebug() << "【控制面板】自动缩放选项创建完成";

    // 状态标签
    qDebug() << "【控制面板】创建状态标签...";
    m_statusLabel = new QLabel("就绪");
    m_statusLabel->setStyleSheet("color: #00ff00; font-weight: bold;");
    qDebug() << "【控制面板】状态标签创建完成";
    
    // 连接信号
    qDebug() << "【控制面板】开始连接信号...";
    connect(m_startStopButton, &QPushButton::clicked, this, &DataAcquisition::onStartStopClicked);
    qDebug() << "【控制面板】开始采集按钮信号已连接";
    
    connect(m_clearButton, &QPushButton::clicked, this, &DataAcquisition::onClearClicked);
    qDebug() << "【控制面板】清空数据按钮信号已连接";
    
    connect(m_channelCountComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &DataAcquisition::onChannelCountChanged);
    qDebug() << "【控制面板】通道数量选择信号已连接";
    
    connect(m_timeRangeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &DataAcquisition::onTimeRangeChanged);
    qDebug() << "【控制面板】时间范围选择信号已连接";
    
    connect(m_autoScaleCheckBox, &QCheckBox::stateChanged,
            this, &DataAcquisition::onAutoScaleChanged);
    qDebug() << "【控制面板】自动缩放选项信号已连接";
    
    // 连接自动模式按钮信号
    connect(m_autoModeButton, &QPushButton::toggled,
            this, &DataAcquisition::onAutoModeToggled);
    qDebug() << "【控制面板】自动模式按钮信号已连接";
    
    // 连接数值范围变化信号
    connect(m_minValueSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &DataAcquisition::onValueRangeChanged);
    connect(m_maxValueSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &DataAcquisition::onValueRangeChanged);
    qDebug() << "【控制面板】数值范围变化信号已连接";

    // 初始化通道配置
    qDebug() << "【控制面板】初始化通道配置...";
    qDebug() << "【控制面板】当前通道数ComboBox索引:" << m_channelCountComboBox->currentIndex();
    qDebug() << "【控制面板】当前通道数ComboBox数据:" << m_channelCountComboBox->currentData().toInt();
    
    // 手动触发通道配置更新
    m_channelCount = m_channelCountComboBox->itemData(0).toInt(); // 获取第一项的值
    qDebug() << "【控制面板】设置通道数量为:" << m_channelCount;
    updateChannelConfigVisibility();
    
    qDebug() << "【控制面板】通道配置初始化完成";
    qDebug() << "【控制面板】setupControlPanel完成";
}

QWidget* DataAcquisition::createControlPanelWidget()
{
    QWidget *controlWidget = new QWidget();
    controlWidget->setStyleSheet(
        "QWidget {"
        "    background-color: #2a2a2a;"
        "    border: 1px solid #444444;"
        "    border-radius: 5px;"
        "    padding: 6px;"
        "    font-size: 0.15em;"
        "}"
        "QPushButton {"
        "    font-size: 0.175em;"
        "    padding: 0.1em 0.2em;"
        "    background-color: #404040;"
        "    border: 1px solid #555555;"
        "    border-radius: 3px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #505050;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #353535;"
        "}"
        "QComboBox {"
        "    font-size: 0.15em;"
        "    padding: 0.075em 0.125em;"
        "    background-color: #404040;"
        "    border: 1px solid #555555;"
        "    border-radius: 3px;"
        "}"
        "QSpinBox {"
        "    font-size: 0.15em;"
        "    padding: 0.075em 0.125em;"
        "    background-color: #404040;"
        "    border: 1px solid #555555;"
        "    border-radius: 3px;"
        "}"
        "QCheckBox {"
        "    font-size: 0.15em;"
        "}"
        "QLabel {"
        "    font-size: 0.15em;"
        "    color: #ffffff;"
        "}"
    );

    // 创建主布局 - 使用网格布局
    QGridLayout *mainLayout = new QGridLayout(controlWidget);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(0);  // 去掉间距
    
    // 第一行：节点ID
    QLabel *nodeLabel = new QLabel("节点ID:");
    nodeLabel->setStyleSheet("color: #ffffff; font-weight: bold;");
    nodeLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    mainLayout->addWidget(nodeLabel, 0, 0);  // 第一行第一列
    
    m_nodeIdSpinBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    mainLayout->addWidget(m_nodeIdSpinBox, 0, 1);  // 第一行第二列
    
    // 第二行：通道数
    QLabel *channelLabel = new QLabel("通道数:");
    channelLabel->setStyleSheet("color: #ffffff; font-weight: bold;");
    channelLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    mainLayout->addWidget(channelLabel, 1, 0);  // 第二行第一列
    
    m_channelCountComboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    mainLayout->addWidget(m_channelCountComboBox, 1, 1);  // 第二行第二列
    
    // 设置列宽比例：第一列和第二列各占1/5，间距占1/4，按钮区域占剩余空间
    mainLayout->setColumnStretch(0, 1);  // 第一列占1/5
    mainLayout->setColumnStretch(1, 1);  // 第二列占1/5
    mainLayout->setColumnStretch(2, 1);  // 间距列占1/4
    mainLayout->setColumnStretch(3, 2);  // 按钮区域占剩余空间
    
    // 按钮区域（右侧）
    QVBoxLayout *buttonLayout = new QVBoxLayout();
    buttonLayout->setSpacing(5);  // 微小间距
    
    // 创建上下限文本框容器
    QWidget *rangeContainer = new QWidget();
    rangeContainer->setStyleSheet("background-color: transparent;");
    QHBoxLayout *rangeLayout = new QHBoxLayout(rangeContainer);
    rangeLayout->setContentsMargins(0, 0, 0, 0);
    rangeLayout->setSpacing(5);
    
    // 下限文本框
    QLabel *minLabel = new QLabel("下限:");
    minLabel->setStyleSheet("color: #ffffff; font-weight: bold; font-size: 0.8em;");
    
    // 上限文本框
    QLabel *maxLabel = new QLabel("上限:");
    maxLabel->setStyleSheet("color: #ffffff; font-weight: bold; font-size: 0.8em;");
    
    rangeLayout->addWidget(minLabel);
    rangeLayout->addWidget(m_minValueSpinBox);
    rangeLayout->addWidget(maxLabel);
    rangeLayout->addWidget(m_maxValueSpinBox);
    
    // 设置按钮尺寸策略 - 每个按钮占1/5容器宽度
    m_autoModeButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_startStopButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_clearButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    buttonLayout->addWidget(m_autoModeButton);
    buttonLayout->addWidget(rangeContainer);
    buttonLayout->addWidget(m_startStopButton);
    buttonLayout->addWidget(m_clearButton);
    
    // 将按钮布局添加到第3列，跨越两行
    mainLayout->addLayout(buttonLayout, 0, 3, 2, 1);  // 从第0行第3列开始，跨越2行1列

    return controlWidget;
}

QWidget* DataAcquisition::createChannelConfigWidget(int channelIndex)
{
    QWidget *channelWidget = new QWidget();
    channelWidget->setStyleSheet(
        "QWidget {"
        "    background-color: #404040;"
        "    border: 1px solid #666666;"
        "    border-radius: 4px;"
        "    margin: 2px;"
        "}"
    );

    // 使用网格布局重新设计通道配置
    QGridLayout *layout = new QGridLayout(channelWidget);
    layout->setContentsMargins(2, 2, 2, 2);  // 最小边距
    layout->setSpacing(0);  // 去掉间距

    // 通道标签（第一列）
    QLabel *channelLabel = new QLabel(QString("通道%1:").arg(channelIndex + 1));
    channelLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    channelLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);  // 左对齐
    channelLabel->setStyleSheet(
        "QLabel {"
        "    color: #ffffff;"
        "    font-weight: bold;"
        "    text-align: left;"
        "}"
    );
    layout->addWidget(channelLabel, 0, 0);  // 第一列
    
    // 参数类别选择
    QComboBox *categoryComboBox = new QComboBox();
    categoryComboBox->addItem("基本控制参数", CATEGORY_BASIC_CONTROL);
    categoryComboBox->addItem("PID参数", CATEGORY_PID_PARAMS);
    categoryComboBox->addItem("高级设置", CATEGORY_ADVANCED_SETTINGS);
    categoryComboBox->addItem("监控参数", CATEGORY_MONITORING);
    categoryComboBox->addItem("系统状态", CATEGORY_SYSTEM_STATUS);
    categoryComboBox->addItem("电源监控", CATEGORY_POWER_MONITOR);
    categoryComboBox->addItem("运动规划", CATEGORY_MOTION_PLANNING);
    categoryComboBox->addItem("观测器调试", CATEGORY_OBSERVER_DEBUG);
    
    // 设置分类选择框样式（第二列）
    categoryComboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    categoryComboBox->setStyleSheet(
        "QComboBox {"
        "    background-color: #505050;"
        "    color: #ffffff;"
        "    border: 1px solid #666666;"
        "    border-radius: 4px;"
        "    text-align: left;"
        "    padding-left: 5px;"
        "}"
        "QComboBox::drop-down {"
        "    subcontrol-origin: padding;"
        "    subcontrol-position: top right;"
        "    width: 20px;"
        "    border-left-width: 1px;"
        "    border-left-color: #666666;"
        "    border-left-style: solid;"
        "}"
        "QComboBox QAbstractItemView {"
        "    background-color: #505050;"
        "    color: #ffffff;"
        "    border: 1px solid #666666;"
        "    selection-background-color: #606060;"
        "    outline: none;"
        "}"
    );

    // 为不同通道设置智能默认分类
    int defaultCategory = CATEGORY_MONITORING; // 默认监控参数
    switch (channelIndex) {
    case 0: // 通道1：监控参数 - 实际位置
        defaultCategory = CATEGORY_MONITORING;
        break;
    case 1: // 通道2：监控参数 - 实际速度
        defaultCategory = CATEGORY_MONITORING;
        break;
    case 2: // 通道3：系统状态 - 实际电流
        defaultCategory = CATEGORY_SYSTEM_STATUS;
        break;
    case 3: // 通道4：电源监控 - 总线电压
        defaultCategory = CATEGORY_POWER_MONITOR;
        break;
    default:
        defaultCategory = CATEGORY_BASIC_CONTROL;
        break;
    }
    
    // 设置默认分类
    for (int i = 0; i < categoryComboBox->count(); i++) {
        if (categoryComboBox->itemData(i).toInt() == defaultCategory) {
            categoryComboBox->setCurrentIndex(i);
            break;
        }
    }

    // 参数选择（第三列）
    QComboBox *parameterComboBox = new QComboBox();
    parameterComboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    parameterComboBox->setStyleSheet(
        "QComboBox {"
        "    background-color: #505050;"
        "    color: #ffffff;"
        "    border: 1px solid #666666;"
        "    border-radius: 4px;"
        "    text-align: left;"
        "    padding-left: 5px;"
        "}"
        "QComboBox::drop-down {"
        "    subcontrol-origin: padding;"
        "    subcontrol-position: top right;"
        "    width: 20px;"
        "    border-left-width: 1px;"
        "    border-left-color: #666666;"
        "    border-left-style: solid;"
        "}"
        "QComboBox QAbstractItemView {"
        "    background-color: #505050;"
        "    color: #ffffff;"
        "    border: 1px solid #666666;"
        "    selection-background-color: #606060;"
        "    outline: none;"
        "}"
    );
    populateParameterComboBox(parameterComboBox, defaultCategory);
    
    // 为不同通道设置智能默认参数
    int defaultParamIndex = -1;
    switch (channelIndex) {
    case 0: // 通道1：实际位置
        defaultParamIndex = findParameterIndex(parameterComboBox, 0x6064, 0x00);
        break;
    case 1: // 通道2：实际速度
        defaultParamIndex = findParameterIndex(parameterComboBox, 0x606C, 0x00);
        break;
    case 2: // 通道3：实际电流
        defaultParamIndex = findParameterIndex(parameterComboBox, 0x6072, 0x00);
        break;
    case 3: // 通道4：总线电压
        defaultParamIndex = findParameterIndex(parameterComboBox, 0x6161, 0x00);
        break;
    }
    
    if (defaultParamIndex >= 0) {
        parameterComboBox->setCurrentIndex(defaultParamIndex);
    } else if (parameterComboBox->count() > 0) {
        parameterComboBox->setCurrentIndex(0);
    }
    
    // 添加到列表
    m_categoryComboBoxes.append(categoryComboBox);
    m_parameterComboBoxes.append(parameterComboBox);
    m_channelLabels.append(channelLabel);
    
    // 连接信号
    connect(categoryComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [this, channelIndex, categoryComboBox, parameterComboBox](int index) {
        int category = categoryComboBox->itemData(index).toInt();
        populateParameterComboBox(parameterComboBox, category);
        
        // 分类改变后自动选择第一个有效参数
        if (parameterComboBox->count() > 0) {
            parameterComboBox->setCurrentIndex(0);
        }
        onParameterSelectionChanged(channelIndex);
    });
    
    connect(parameterComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [this, channelIndex]() { onParameterSelectionChanged(channelIndex); });

    // 添加到网格布局
    layout->addWidget(categoryComboBox, 0, 1);  // 第二列
    layout->addWidget(parameterComboBox, 0, 2);  // 第三列
    
    // 设置列宽比例：第一列1/5，第二列1/4，第三列1/3，剩余空间给第三列
    layout->setColumnStretch(0, 1);  // 第一列（通道标签）占1/5
    layout->setColumnStretch(1, 1);  // 第二列（分类）占1/4
    layout->setColumnStretch(2, 2);  // 第三列（参数）占1/3 + 剩余空间
    
    // 设置左对齐
    layout->setAlignment(Qt::AlignLeft);

    return channelWidget;
}

void DataAcquisition::populateParameterComboBox(QComboBox *comboBox, int category)
{
    comboBox->clear();

    if (!m_paramDictionary) return;
    
    // 根据类别获取参数
    QVector<ODEntry> parameters = m_paramDictionary->getParametersByCategory(category);

    for (const ODEntry &entry : parameters) {
        QString displayText = QString("0x%1.%2 - %3 (%4)")
            .arg(entry.index, 4, 16, QLatin1Char('0'))
            .arg(entry.subindex, 2, 16, QLatin1Char('0'))
            .arg(entry.name)
            .arg(entry.unit);
        
        QVariant data;
        data.setValue(QPair<uint16_t, uint8_t>(entry.index, entry.subindex));
        comboBox->addItem(displayText, data);
    }
}

void DataAcquisition::onStartStopClicked()
{
    if (!m_isAcquiring) {
        startAcquisition();
    } else {
        stopAcquisition();
    }
}

void DataAcquisition::onClearClicked()
{
    clearData();
}

void DataAcquisition::onChannelCountChanged(int count)
{
    qDebug() << "【通道切换】onChannelCountChanged被调用，索引:" << count;
    m_channelCount = m_channelCountComboBox->itemData(count).toInt();
    qDebug() << "【通道切换】获取到通道数量:" << m_channelCount;
    updateChannelConfigVisibility();
}

void DataAcquisition::onParameterSelectionChanged(int channelIndex)
{
    if (channelIndex < m_channelConfigs.size()) {
        QComboBox *paramComboBox = m_parameterComboBoxes[channelIndex];
        QVariant data = paramComboBox->currentData();
        
        if (data.isValid()) {
            QPair<uint16_t, uint8_t> param = data.value<QPair<uint16_t, uint8_t>>();
            m_channelConfigs[channelIndex].parameterIndex = param.first;
            m_channelConfigs[channelIndex].parameterSubindex = param.second;
            m_channelConfigs[channelIndex].parameterName = getParameterName(param.first, param.second);
        }
    }
}

void DataAcquisition::onTimeRangeChanged(int index)
{
    m_timeRange = m_timeRangeComboBox->itemData(index).toDouble();
    m_oscilloscope->setTimeRange(m_timeRange);
}

void DataAcquisition::onAutoScaleChanged(int state)
{
    m_oscilloscope->setAutoScale(state == Qt::Checked);
}

void DataAcquisition::onAutoModeToggled(bool enabled)
{
    qDebug() << "【自动模式】onAutoModeToggled被调用，enabled:" << enabled;
    m_autoModeEnabled = enabled;
    
    if (enabled) {
        // 启用固定模式：使用固定的上下限
        qDebug() << "【自动模式】切换到固定模式";
        m_oscilloscope->setAutoScale(false);
        double minValue = m_minValueSpinBox->value();
        double maxValue = m_maxValueSpinBox->value();
        qDebug() << "【自动模式】设置范围:" << minValue << "~" << maxValue;
        m_oscilloscope->setFixedValueRange(minValue, maxValue);
        m_autoModeButton->setText("固定模式");
        m_autoModeButton->repaint();
        m_autoModeButton->update();
        qDebug() << "【自动模式】按钮文字设置为: 固定模式，当前文字:" << m_autoModeButton->text();
        addDebugMessage("已切换到固定范围模式");
    } else {
        // 禁用固定模式：恢复自动缩放
        qDebug() << "【自动模式】切换到自动模式";
        m_oscilloscope->setAutoScale(true);
        m_autoModeButton->setText("自动模式");
        m_autoModeButton->repaint();
        m_autoModeButton->update();
        qDebug() << "【自动模式】按钮文字设置为: 自动模式，当前文字:" << m_autoModeButton->text();
        addDebugMessage("已切换到自动缩放模式");
    }
}

void DataAcquisition::onValueRangeChanged()
{
    qDebug() << "【数值范围】onValueRangeChanged被调用，m_autoModeEnabled:" << m_autoModeEnabled;
    
    if (m_autoModeEnabled) {
        // 只有在固定模式启用时才更新范围
        double minValue = m_minValueSpinBox->value();
        double maxValue = m_maxValueSpinBox->value();
        
        qDebug() << "【数值范围】获取到范围值:" << minValue << "~" << maxValue;
        
        // 确保上限大于下限
        if (maxValue <= minValue) {
            maxValue = minValue + 1.0;
            m_maxValueSpinBox->setValue(maxValue);
            qDebug() << "【数值范围】调整上限为:" << maxValue;
        }
        
        qDebug() << "【数值范围】调用setFixedValueRange";
        m_oscilloscope->setFixedValueRange(minValue, maxValue);
        addDebugMessage(QString("数值范围已更新: %1 ~ %2").arg(minValue).arg(maxValue));
        
        qDebug() << QString("【数值范围】固定模式范围更新: %1 ~ %2")
                    .arg(minValue, 0, 'f', 3)
                    .arg(maxValue, 0, 'f', 3);
    } else {
        qDebug() << "【数值范围】自动模式，忽略范围更新";
    }
}

void DataAcquisition::startAcquisition()
{
    qDebug() << "【采集】startAcquisition被调用";
    
    if (m_isAcquiring) {
        qDebug() << "【采集】已经在采集中，忽略";
        return;
    }

    m_isAcquiring = true;
    
    // 只有在第一次采集或数据被清空后才设置起始时间
    if (m_startTime <= 0) {
        m_startTime = QDateTime::currentDateTime().toMSecsSinceEpoch() / 1000.0;
        qDebug() << "【采集】设置新的startTime =" << m_startTime;
    } else {
        qDebug() << "【采集】使用现有的startTime =" << m_startTime << "，继续从上次位置采集";
    }
    
    // 更新UI
    m_startStopButton->setText("停止采集");
    m_startStopButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #d32f2f;"
        "    color: white;"
        "    border: 1px solid #b71c1c;"
        "    border-radius: 3px;"
        "    padding: 5px 10px;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "    background-color: #f44336;"
        "}"
    );

    m_statusLabel->setText("采集中...");
    m_statusLabel->setStyleSheet("color: #ff9800; font-weight: bold;");
    qDebug() << "【采集】UI更新完成";
    
    // 启动定时器
    if (m_plotUpdateTimer) {
        m_plotUpdateTimer->start();
        qDebug() << "【采集】绘图更新定时器已启动";
    }
    
    if (m_parameterRequestTimer) {
        m_parameterRequestTimer->start();
        qDebug() << "【采集】参数请求定时器已启动";
    }
    
    // 重置统计
    m_requestCount = 0;
    m_responseCount = 0;
    m_testStartTime = QTime::currentTime();
    
    qDebug() << "【采集】通道配置数量:" << m_channelConfigs.size();
    qDebug() << "【采集】m_channelCount值:" << m_channelCount;
    
    if (m_channelConfigs.size() == 0) {
        qCritical() << "【采集】❌ 警告：通道配置为空！尝试重新初始化...";
        // 尝试重新创建通道配置
        if (m_channelCount > 0) {
            updateChannelConfigVisibility();
            qDebug() << "【采集】重新初始化后通道配置数量:" << m_channelConfigs.size();
        }
    }
    
    // 统计启用的通道数量
    int enabledChannels = 0;
    for (const auto& config : m_channelConfigs) {
        if (config.enabled) {
            enabledChannels++;
        }
    }
    
    qDebug() << "【采集】总通道数:" << m_channelConfigs.size() << "启用通道数:" << enabledChannels;
    
    if (enabledChannels > 0) {
        qDebug() << "【采集】第一个启用通道: index=0x" << QString::number(m_channelConfigs[0].parameterIndex, 16)
                 << " subindex=0x" << QString::number(m_channelConfigs[0].parameterSubindex, 16);
    } else {
        qCritical() << "【采集】❌ 错误：没有启用的通道，数据采集将无法工作！";
    }

    addDebugMessage("数据采集已启动");
    qDebug() << "【采集】数据采集启动完成";
}

void DataAcquisition::stopAcquisition()
{
    if (!m_isAcquiring) {
        return;
    }

        m_isAcquiring = false;
        
    // 停止定时器
    m_plotUpdateTimer->stop();
    m_parameterRequestTimer->stop();
    
    // 更新UI
            m_startStopButton->setText("开始采集");
            m_startStopButton->setStyleSheet(
                "QPushButton {"
                "    background-color: #4caf50;"
                "    color: white;"
        "    border: 1px solid #388e3c;"
        "    border-radius: 3px;"
        "    padding: 5px 10px;"
                "    font-weight: bold;"
                "}"
                "QPushButton:hover {"
        "    background-color: #66bb6a;"
                "}"
            );
    
    m_statusLabel->setText("已停止");
    m_statusLabel->setStyleSheet("color: #f44336; font-weight: bold;");
    
    addDebugMessage("数据采集已停止");
}

void DataAcquisition::clearData()
{
    if (!m_oscilloscope) {
        return;
    }
    
    // 清空示波器数据
    m_oscilloscope->clearData();
    
    // 重置起始时间，下次采集会重新开始
    m_startTime = -1.0;
    
    addDebugMessage("数据已清空");
}

void DataAcquisition::updatePlot()
{
    // 定时更新显示
    if (m_oscilloscope && m_isAcquiring) {
        // QGraphicsView会自动更新，这里可以添加额外的更新逻辑
    }
}

// ==================== CAN通信函数 ====================

uint32_t DataAcquisition::buildUpdateCOBId(uint8_t cmdType, uint8_t nodeId)
{
    return UPDATE_COB_ID_BASE + (cmdType << 4) + (nodeId & 0x0F);
}

void DataAcquisition::sendCANFrame(uint32_t cobId, const QByteArray& data)
{
    VCI_CAN_OBJ frame;
    memset(&frame, 0, sizeof(VCI_CAN_OBJ));

    frame.ID = cobId;
    frame.SendType = 0;
    frame.RemoteFlag = 0;
    frame.ExternFlag = 0;
    frame.DataLen = data.size();
    
    for (int i = 0; i < data.size() && i < 8; i++) {
        frame.Data[i] = static_cast<unsigned char>(data.at(i));
    }
    
    // 详细打印发送的CAN数据
    QString dataHex = "";
    for (int i = 0; i < frame.DataLen; i++) {
        dataHex += QString("%1 ").arg(frame.Data[i], 2, 16, QLatin1Char('0')).toUpper();
    }
    
    qDebug() << QString("【CAN发送详情】ID:0x%1 Len:%2 Data:[%3]")
                .arg(frame.ID, 0, 16)
                .arg(frame.DataLen)
                .arg(dataHex.trimmed());
    
    if (m_canTxRx) {
        m_canTxRx->sendCANFrame(frame);
        qDebug() << "【CAN发送】已调用m_canTxRx->sendCANFrame()";
    } else {
        qCritical() << "【CAN发送】❌ m_canTxRx为空，无法发送CAN帧！";
    }
}

void DataAcquisition::sendParameterRead(uint8_t nodeId, uint16_t index, uint8_t subindex)
{
    QByteArray data;
    data.append(static_cast<char>((index >> 8) & 0xFF));
    data.append(static_cast<char>(index & 0xFF));
    data.append(static_cast<char>(subindex));
    data.append(static_cast<char>(0)); // 保留字节

    uint32_t cobId = buildUpdateCOBId(UPDATE_CMD_READ_SINGLE, nodeId);
    
    // 每10次请求打印一次调试信息（减少打印频率）
    static int debugCount = 0;
    if (++debugCount % 10 == 0) {
        qDebug() << QString("【CAN发送】NodeID:%1 Index:0x%2 SubIndex:0x%3 COB-ID:0x%4")
                    .arg(nodeId)
                    .arg(index, 4, 16, QLatin1Char('0'))
                    .arg(subindex, 2, 16, QLatin1Char('0'))
                    .arg(cobId, 0, 16);
    }
    
    sendCANFrame(cobId, data);
}

void DataAcquisition::sendMultiParameterRead(uint8_t nodeId, uint16_t index1, uint8_t subindex1, uint16_t index2, uint8_t subindex2)
{
    QByteArray data;
    // 前三个字节是示波器通道1的索引和子索引
    data.append(static_cast<char>((index1 >> 8) & 0xFF));
    data.append(static_cast<char>(index1 & 0xFF));
    data.append(static_cast<char>(subindex1));
    
    // 第4到第6个字节是示波器通道2的索引和子索引
    data.append(static_cast<char>((index2 >> 8) & 0xFF));
    data.append(static_cast<char>(index2 & 0xFF));
    data.append(static_cast<char>(subindex2));
    
    // 最后两个字节默认给00 00
    data.append(static_cast<char>(0x00));
    data.append(static_cast<char>(0x00));

    uint32_t cobId = buildUpdateCOBId(UPDATE_CMD_READ_MULTI, nodeId);
    
    // 每10次请求打印一次调试信息（减少打印频率）
    static int debugCount = 0;
    if (++debugCount % 10 == 0) {
        qDebug() << QString("【CAN多通道发送】NodeID:%1 Ch1:0x%2:0x%3 Ch2:0x%4:0x%5 COB-ID:0x%6")
                    .arg(nodeId)
                    .arg(index1, 4, 16, QLatin1Char('0'))
                    .arg(subindex1, 2, 16, QLatin1Char('0'))
                    .arg(index2, 4, 16, QLatin1Char('0'))
                    .arg(subindex2, 2, 16, QLatin1Char('0'))
                    .arg(cobId, 0, 16);
    }
    
    sendCANFrame(cobId, data);
}

void DataAcquisition::onCANFrameReceived(const VCI_CAN_OBJ &frame)
{
    static int frameCount = 0;
    frameCount++;
    
    // 详细打印接收的CAN数据
    QString dataHex = "";
    for (int i = 0; i < frame.DataLen; i++) {
        dataHex += QString("%1 ").arg(frame.Data[i], 2, 16, QLatin1Char('0')).toUpper();
    }
    
    qDebug() << QString("【CAN接收详情】ID:0x%1 Len:%2 Data:[%3]")
                .arg(frame.ID, 0, 16)
                .arg(frame.DataLen)
                .arg(dataHex.trimmed());
    
    // 每10帧打印一次统计日志
    if (frameCount % 10 == 0) {
        qDebug() << "【CAN接收统计】已接收" << frameCount << "帧";
    }
    
    // 更新接收计数
    m_responseCount++;
    
    // 解析CAN帧并添加到示波器
    parseCANFrameForOscilloscope(frame);
}

void DataAcquisition::parseCANFrameForOscilloscope(const VCI_CAN_OBJ &frame)
{
    // 添加基本安全检查
    if (!frame.Data || frame.DataLen < 4 || frame.DataLen > 8) {
        qDebug() << "【示波器】无效的CAN帧：DataLen=" << frame.DataLen;
        return; // 无效的CAN帧
    }
    
    // 检查示波器组件是否存在
    if (!m_oscilloscope) {
        qDebug() << "【示波器】示波器组件未初始化";
        return; // 示波器组件未初始化
    }
    
    if(frame.ID>0x5ff  || frame.ID <0x500)
        return;
    // 使用正确的COB-ID解析方式
    uint8_t receivedNodeId = frame.ID & 0x0F;  // 低4位是NodeID
    uint8_t cmdType = (frame.ID >> 4) & 0x0F; // 高4位是命令类型
    uint8_t expectedNodeId = m_nodeIdSpinBox->value();
    
    qDebug() << QString("【COB-ID解析】ID:0x%1 NodeID:%2 CmdType:%3")
                .arg(frame.ID, 0, 16)
                .arg(receivedNodeId)
                .arg(cmdType);
    
    // 只处理与当前请求匹配的响应数据
    if (receivedNodeId != expectedNodeId) {
        // 不是我们请求的节点，忽略
        qDebug() << QString("【示波器过滤】忽略NodeID %1，期望 %2").arg(receivedNodeId).arg(expectedNodeId);
        return;
    }
    
    // 打印接收到的CAN帧信息
    QString dataHex = "";
    for (int i = 0; i < frame.DataLen; i++) {
        dataHex += QString("%1 ").arg(frame.Data[i], 2, 16, QLatin1Char('0')).toUpper();
    }
    qDebug() << QString("【示波器解析】ID:0x%1 Len:%2 Data:[%3]")
                .arg(frame.ID, 0, 16)
                .arg(frame.DataLen)
                .arg(dataHex.trimmed());
    
    // 根据当前启用的通道数量决定解析方式
    try {
        // 检查当前启用的通道数量
        int enabledChannelCount = 0;
        for (const auto& config : m_channelConfigs) {
            if (config.enabled) {
                enabledChannelCount++;
            }
        }
        
        // 如果启用的通道数量为2，解析为多通道数据
        if (enabledChannelCount == 2 && cmdType == UPDATE_CMD_RESPONSE && frame.DataLen == 8) {
            // 多通道响应：前4个字节是第一个通道的数据，后4个字节是第二个通道的数据
            qDebug() << "【示波器解析】检测到多通道响应数据，通道数:" << enabledChannelCount;
            
            // 解析第一个通道的数据（前4个字节）
            parseMultiChannelData(frame, 0, 4, receivedNodeId);
            
            // 解析第二个通道的数据（后4个字节）
            parseMultiChannelData(frame, 4, 8, receivedNodeId);
        } else {
            // 单通道响应：解析参数数据（使用已解析的NodeID）
            uint16_t parameterIndex = (frame.Data[0] << 8) | frame.Data[1];
            uint8_t parameterSubindex = frame.Data[2];
            
            qDebug() << QString("【示波器解析】NodeID:%1 Index:0x%2 SubIndex:0x%3")
                        .arg(receivedNodeId)
                        .arg(parameterIndex, 4, 16, QLatin1Char('0'))
                        .arg(parameterSubindex, 2, 16, QLatin1Char('0'));
            
            // 获取参数值（从第4字节开始）
            if (frame.DataLen >= 8) {
                // 根据参数字典解析数据
                ODEntry paramEntry = m_paramDictionary->getParameter(parameterIndex, parameterSubindex);
                
                if (paramEntry.index != 0) { // 找到参数定义
                    float floatValue = 0.0f;
                    
                    // 根据参数类型解析数据
                    switch (paramEntry.type) {
                        case OD_TYPE_FLOAT:
                            memcpy(&floatValue, &frame.Data[4], 4);
                            break;
                        case OD_TYPE_INT32:
                            {
                                int32_t intValue;
                                memcpy(&intValue, &frame.Data[4], 4);
                                floatValue = static_cast<float>(intValue);
                            }
                            break;
                        case OD_TYPE_UINT32:
                            {
                                uint32_t uintValue;
                                memcpy(&uintValue, &frame.Data[4], 4);
                                floatValue = static_cast<float>(uintValue);
                            }
                            break;
                        case OD_TYPE_INT16:
                            {
                                int16_t intValue;
                                memcpy(&intValue, &frame.Data[4], 2);
                                floatValue = static_cast<float>(intValue);
                            }
                            break;
                        case OD_TYPE_UINT16:
                            {
                                uint16_t uintValue;
                                memcpy(&uintValue, &frame.Data[4], 2);
                                floatValue = static_cast<float>(uintValue);
                            }
                            break;
                        case OD_TYPE_INT8:
                            {
                                int8_t intValue;
                                memcpy(&intValue, &frame.Data[4], 1);
                                floatValue = static_cast<float>(intValue);
                            }
                            break;
                        case OD_TYPE_UINT8:
                            {
                                uint8_t uintValue;
                                memcpy(&uintValue, &frame.Data[4], 1);
                                floatValue = static_cast<float>(uintValue);
                            }
                            break;
                        default:
                            // 默认按32位整数处理
                            {
                                int32_t intValue;
                                memcpy(&intValue, &frame.Data[4], 4);
                                floatValue = static_cast<float>(intValue);
                            }
                            break;
                    }
                    
                    // 检查数据有效性
                    if (!qIsNaN(floatValue) && !qIsInf(floatValue)) {
                        // 获取当前时间戳
                        double currentTime = QDateTime::currentDateTime().toMSecsSinceEpoch() / 1000.0;
                        
                        // 如果是第一次接收数据，设置起始时间
                        if (m_startTime <= 0) {
                            m_startTime = currentTime;
                        }
                        
                        // 计算相对时间戳（相对于起始时间）
                        double relativeTime = currentTime - m_startTime;

                        // 根据通道配置确定通道名称
                        QString channelName;
                        QString displayName;
                        
                        // 查找匹配的通道配置
                        bool foundMatchingChannel = false;
                        for (const auto& config : m_channelConfigs) {
                            if (config.enabled && 
                                config.parameterIndex == parameterIndex && 
                                config.parameterSubindex == parameterSubindex) {
                                channelName = QString("CH%1_%2").arg(config.channelIndex + 1).arg(paramEntry.name);
                                displayName = QString("通道%1: %2 (%3)").arg(config.channelIndex + 1).arg(paramEntry.name).arg(paramEntry.unit);
                                foundMatchingChannel = true;
                                break;
                            }
                        }
                        
                        // 如果没有找到匹配的通道配置，使用默认通道名称
                        if (!foundMatchingChannel) {
                            channelName = QString("CH%1_%2").arg(expectedNodeId).arg(paramEntry.name);
                            displayName = QString("%1 (%2)").arg(paramEntry.name).arg(paramEntry.unit);
                        }
                            
                        emit dataPointAdded(channelName, relativeTime, floatValue, displayName);
                    }
                } else {
                    // 未找到参数定义，使用原始数据
                    int32_t paramValue;
                    memcpy(&paramValue, &frame.Data[4], 4);
                    float floatValue = static_cast<float>(paramValue);
                    
                    qDebug() << QString("【示波器原始数据】参数值:0x%1 (%2)")
                                .arg(paramValue, 8, 16, QLatin1Char('0'))
                                .arg(floatValue, 0, 'f', 3);
                    
                    // 获取当前时间戳
                    double currentTime = QDateTime::currentDateTime().toMSecsSinceEpoch() / 1000.0;
                    
                    // 如果是第一次接收数据，设置起始时间
                    if (m_startTime <= 0) {
                        m_startTime = currentTime;
                    }
                    
                    // 计算相对时间戳（相对于起始时间）
                    double relativeTime = currentTime - m_startTime;

                    // 添加数据点到示波器
                    QString paramName = QString("参数0x%1.%2").arg(parameterIndex, 4, 16, QLatin1Char('0')).arg(parameterSubindex, 2, 16, QLatin1Char('0'));
                    QString channelName = QString("CH%1_%2").arg(expectedNodeId).arg(paramName);
                    
                    qDebug() << QString("【示波器原始数据】通道:%1 时间:%2 值:%3")
                                .arg(channelName)
                                .arg(relativeTime, 0, 'f', 3)
                                .arg(floatValue, 0, 'f', 3);
                    
                    emit dataPointAdded(channelName, relativeTime, floatValue, paramName);
                }
            }
        }
    } catch (const std::exception &e) {
        qDebug() << "解析CAN帧时发生异常:" << e.what();
    } catch (...) {
        qDebug() << "解析CAN帧时发生未知异常";
    }
}

void DataAcquisition::parseMultiChannelData(const VCI_CAN_OBJ &frame, int startByte, int endByte, uint8_t nodeId)
{
    // 解析4字节数据
    if (endByte - startByte != 4) {
        qDebug() << "【多通道解析】数据长度错误，期望4字节，实际" << (endByte - startByte) << "字节";
        return;
    }
    
    // 根据通道索引确定通道配置
    int channelIndex = (startByte == 0) ? 0 : 1; // 前4字节是通道1，后4字节是通道2
    
    // 查找匹配的通道配置
    ChannelConfig* channelConfig = nullptr;
    for (auto& config : m_channelConfigs) {
        if (config.enabled && config.channelIndex == channelIndex) {
            channelConfig = &config;
            break;
        }
    }
    
    if (!channelConfig) {
        qDebug() << QString("【多通道解析】未找到通道%1的配置").arg(channelIndex + 1);
        return;
    }
    
    // 获取当前时间戳
    double currentTime = QDateTime::currentDateTime().toMSecsSinceEpoch() / 1000.0;
    
    // 如果是第一次接收数据，设置起始时间
    if (m_startTime <= 0) {
        m_startTime = currentTime;
    }
    
    // 计算相对时间戳（相对于起始时间）
    double relativeTime = currentTime - m_startTime;
    
    // 根据参数类型解析数据
    float floatValue = 0.0f;
    
    // 获取参数定义
    ODEntry paramEntry = m_paramDictionary->getParameter(channelConfig->parameterIndex, channelConfig->parameterSubindex);
    
    if (paramEntry.index != 0) { // 找到参数定义
        // 根据参数类型解析数据
        switch (paramEntry.type) {
            case OD_TYPE_FLOAT:
                memcpy(&floatValue, &frame.Data[startByte], 4);
                break;
            case OD_TYPE_INT32:
                {
                    int32_t intValue;
                    memcpy(&intValue, &frame.Data[startByte], 4);
                    floatValue = static_cast<float>(intValue);
                }
                break;
            case OD_TYPE_UINT32:
                {
                    uint32_t uintValue;
                    memcpy(&uintValue, &frame.Data[startByte], 4);
                    floatValue = static_cast<float>(uintValue);
                }
                break;
            case OD_TYPE_INT16:
                {
                    int16_t intValue;
                    memcpy(&intValue, &frame.Data[startByte], 2);
                    floatValue = static_cast<float>(intValue);
                }
                break;
            case OD_TYPE_UINT16:
                {
                    uint16_t uintValue;
                    memcpy(&uintValue, &frame.Data[startByte], 2);
                    floatValue = static_cast<float>(uintValue);
                }
                break;
            case OD_TYPE_INT8:
                {
                    int8_t intValue;
                    memcpy(&intValue, &frame.Data[startByte], 1);
                    floatValue = static_cast<float>(intValue);
                }
                break;
            case OD_TYPE_UINT8:
                {
                    uint8_t uintValue;
                    memcpy(&uintValue, &frame.Data[startByte], 1);
                    floatValue = static_cast<float>(uintValue);
                }
                break;
            default:
                // 默认按32位整数处理
                {
                    int32_t intValue;
                    memcpy(&intValue, &frame.Data[startByte], 4);
                    floatValue = static_cast<float>(intValue);
                }
                break;
        }
    } else {
        // 未找到参数定义，使用原始数据
        int32_t paramValue;
        memcpy(&paramValue, &frame.Data[startByte], 4);
        floatValue = static_cast<float>(paramValue);
    }
    
    // 检查数据有效性
    if (qIsNaN(floatValue) || qIsInf(floatValue)) {
        qDebug() << "【多通道解析】无效数据值:" << floatValue;
        return;
    }
    
    // 生成通道名称和显示名称
    QString channelName = QString("CH%1_%2").arg(channelIndex + 1).arg(channelConfig->parameterName);
    QString displayName = QString("通道%1: %2").arg(channelIndex + 1).arg(channelConfig->parameterName);
    
    qDebug() << QString("【多通道解析】通道%1 时间:%2 值:%3 参数:%4")
                .arg(channelIndex + 1)
                .arg(relativeTime, 0, 'f', 3)
                .arg(floatValue, 0, 'f', 3)
                .arg(channelConfig->parameterName);
    
    // 发送数据点到示波器
    emit dataPointAdded(channelName, relativeTime, floatValue, displayName);
}

void DataAcquisition::parseUpdateProtocol(const VCI_CAN_OBJ &frame)
{
    try {
        // 简化逻辑：只处理参数响应帧
        if (frame.DataLen >= 4) {
            uint8_t cmdType = (frame.ID >> 4) & 0x0F;
            if (cmdType == UPDATE_CMD_RESPONSE) {
                m_responseCount++;
            }
        }
    } catch (const std::exception &e) {
        qDebug() << "解析更新协议时发生异常:" << e.what();
    }
}

void DataAcquisition::parseParameterResponse(const VCI_CAN_OBJ &frame)
{
    // 简化逻辑：只计数回复，不解析内容
    try {
        if (frame.DataLen >= 4) {
            m_responseCount++;
        }
    } catch (const std::exception &e) {
        qDebug() << "解析参数响应时发生异常:" << e.what();
    }
}

void DataAcquisition::setCANTxRx(CANTxRx* canTxRx)
{
    m_canTxRx = canTxRx;
    
    // 连接CAN通信信号
    if (m_canTxRx) {
        qDebug() << "【数据采集】连接CAN通信信号...";
        
        // 连接CAN帧接收信号
        bool connected = connect(m_canTxRx, &CANTxRx::frameReceived,
                this, &DataAcquisition::onCANFrameReceived,
                Qt::QueuedConnection);
        
        if (connected) {
            qDebug() << "【数据采集】✅ CAN通信信号连接成功";
        } else {
            qCritical() << "【数据采集】❌ CAN通信信号连接失败！";
        }
        
        // 测试：直接订阅所有CAN帧（不仅仅是数据上抛协议）
        qDebug() << "【数据采集】尝试连接所有CAN信号...";
    } else {
        qWarning() << "【数据采集】CANTxRx为空，无法连接信号";
    }
}

void DataAcquisition::onCANFrameSent(uint32_t id, const QByteArray& data)
{
    // CAN帧发送成功的回调
    QString hexData = QString(data.toHex(' ').toUpper());
    addDebugMessage(QString("CAN帧发送成功 - ID: 0x%1 数据: %2")
                    .arg(id, 0, 16)
                   .arg(hexData));
}

void DataAcquisition::requestParameters()
{
    static int callCount = 0;
    static QTime lastSecondTime = QTime::currentTime();
    static qint64 lastSendTimeMs = 0;
    
    callCount++;
    
    // 每秒统计
    QTime currentTime = QTime::currentTime();
    if (lastSecondTime.msecsTo(currentTime) >= 1000) {
        qDebug() << QString("📊 每秒统计 - 发送:%1次 接收:%2次")
                    .arg(m_requestCount)
                    .arg(m_responseCount);
        
        m_requestCount = 0;
        m_responseCount = 0;
        lastSecondTime = currentTime;
    }
    
    // 检查是否正在请求中
    if (m_isRequesting) {
        return;
    }
    
    // 使用QElapsedTimer进行精确的时间控制
    qint64 currentTimeMs = QDateTime::currentDateTime().toMSecsSinceEpoch();
    if (currentTimeMs - lastSendTimeMs < 5) { // 5ms间隔
        return;
    }
    
    // 异步发送，避免阻塞定时器
    QMetaObject::invokeMethod(this, [this]() {
        // 在下一个事件循环中发送，不阻塞当前定时器
        // 检查启用的通道数量
        QVector<ChannelConfig> enabledChannels;
        for (const auto& config : m_channelConfigs) {
            if (config.enabled) {
                enabledChannels.append(config);
            }
        }
        
        // 如果启用的通道数量为2，使用多通道读取命令
        if (enabledChannels.size() == 2) {
            sendMultiParameterRead(m_nodeIdSpinBox->value(), 
                                 enabledChannels[0].parameterIndex, enabledChannels[0].parameterSubindex,
                                 enabledChannels[1].parameterIndex, enabledChannels[1].parameterSubindex);
            m_requestCount++;
        } else {
            // 其他情况使用单通道读取命令
            for (const auto& config : enabledChannels) {
                sendParameterRead(m_nodeIdSpinBox->value(), config.parameterIndex, config.parameterSubindex);
                m_requestCount++;
            }
        }
    }, Qt::QueuedConnection);
    
    lastSendTimeMs = currentTimeMs;
}

void DataAcquisition::updateChannelConfigVisibility()
{
    qDebug() << "【通道配置】开始更新通道配置可见性，通道数量:" << m_channelCount;
    
    // 检查必要的组件是否已初始化
    if (!m_channelConfigLayout) {
        qCritical() << "【通道配置】m_channelConfigLayout为空！";
        return;
    }
    
    // 清空现有配置
    qDebug() << "【通道配置】清空现有配置...";
    QLayoutItem *item;
    while ((item = m_channelConfigLayout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }
    qDebug() << "【通道配置】现有配置已清空";
    
    // 清空配置列表
    m_categoryComboBoxes.clear();
    m_parameterComboBoxes.clear();
    m_channelLabels.clear();
    m_channelConfigs.clear();
    qDebug() << "【通道配置】配置列表已清空";
    
    // 创建4个通道配置（都显示，但根据通道数量启用）
    qDebug() << "【通道配置】开始创建4个通道配置，启用前" << m_channelCount << "个...";
    for (int i = 0; i < 4; i++) {
        qDebug() << "【通道配置】创建通道" << i << "配置...";
        
        ChannelConfig config;
        config.channelIndex = i;
        config.enabled = (i < m_channelCount); // 根据通道数量启用
        
        // 为不同通道设置不同的默认参数
        switch (i) {
        case 0: // 通道1：实际位置
            config.parameterIndex = 0x6064;
            config.parameterSubindex = 0x00;
            config.parameterName = "实际位置";
            break;
        case 1: // 通道2：实际速度
            config.parameterIndex = 0x606C;
            config.parameterSubindex = 0x00;
            config.parameterName = "实际速度";
            break;
        case 2: // 通道3：实际电流
            config.parameterIndex = 0x6072;
            config.parameterSubindex = 0x00;
            config.parameterName = "实际电流";
            break;
        case 3: // 通道4：总线电压
            config.parameterIndex = 0x6161;
            config.parameterSubindex = 0x00;
            config.parameterName = "总线电压";
            break;
        default:
            config.parameterIndex = 0x6064; // 默认参数
            config.parameterSubindex = 0x00;
            config.parameterName = "实际位置";
            break;
        }
        
        m_channelConfigs.append(config);
        qDebug() << "【通道配置】通道" << i << "ChannelConfig已创建，启用状态:" << config.enabled 
                 << "参数:" << QString("0x%1:0x%2").arg(config.parameterIndex, 4, 16, QLatin1Char('0')).arg(config.parameterSubindex, 2, 16, QLatin1Char('0'));
        
        qDebug() << "【通道配置】创建通道" << i << "控件...";
        QWidget *channelWidget = createChannelConfigWidget(i);
        qDebug() << "【通道配置】通道" << i << "控件已创建";
        
        // 根据启用状态设置控件样式
        if (channelWidget) {
            channelWidget->setEnabled(config.enabled);
            if (!config.enabled) {
                channelWidget->setStyleSheet("QWidget { color: #666666; }");
            }
        }
        
        m_channelConfigLayout->addWidget(channelWidget);
        qDebug() << "【通道配置】通道" << i << "控件已添加到布局";
    }
    qDebug() << "【通道配置】所有通道配置创建完成";
    
    // 确保所有启用的通道都有有效参数
    qDebug() << "【通道配置】确保所有启用的通道都有有效参数...";
    ensureAllChannelsHaveValidParameters();
    qDebug() << "【通道配置】updateChannelConfigVisibility完成";
}

void DataAcquisition::ensureAllChannelsHaveValidParameters()
{
    // 只处理启用的通道
    for (int i = 0; i < m_channelConfigs.size() && i < m_parameterComboBoxes.size(); i++) {
        if (m_channelConfigs[i].enabled) {
            QComboBox *paramComboBox = m_parameterComboBoxes[i];
            if (paramComboBox->count() == 0) {
                // 如果没有参数，添加默认参数
                QVariant data;
                data.setValue(QPair<uint16_t, uint8_t>(0x6064, 0x00));
                paramComboBox->addItem("0x6064.00 - 实际位置 (mm)", data);
            }
        }
    }
}

void DataAcquisition::addDebugMessage(const QString &message)
{
    if (!m_debugQueue) return;
    
    QString timestamp = QTime::currentTime().toString("hh:mm:ss.zzz");
    QString fullMessage = QString("[%1] %2").arg(timestamp).arg(message);
    
    m_debugMessages.append(fullMessage);
    
    // 限制消息数量
    while (m_debugMessages.size() > m_maxDebugMessages) {
        m_debugMessages.removeFirst();
    }
    
    // 更新显示
    m_debugQueue->setPlainText(m_debugMessages.join("\n"));
    m_debugQueue->verticalScrollBar()->setValue(m_debugQueue->verticalScrollBar()->maximum());
}

// 其他必要的函数实现
void DataAcquisition::addParameterDefinition(uint16_t index, uint8_t subindex, const QString& name,
                                           const QString& unit, double scale)
{
    // 参数定义管理
}

QString DataAcquisition::getParameterName(uint16_t index, uint8_t subindex) const
{
    if (m_paramDictionary) {
        ODEntry param = m_paramDictionary->getParameter(index, subindex);
        if (param.index != 0) {
            return param.name;
        }
    }
    return QString("参数0x%1.%2").arg(index, 4, 16, QLatin1Char('0')).arg(subindex, 2, 16, QLatin1Char('0'));
}

void DataAcquisition::parseParameterResponseThreadSafe(const VCI_CAN_OBJ &frame)
{
    // 线程安全版本
    parseParameterResponse(frame);
}

void DataAcquisition::onCANStatisticsUpdated(int sendCount, int receiveCount, double sendFreq, double receiveFreq)
{
    // CAN统计信息更新
}

void DataAcquisition::updateChannelWidgetAppearance(QWidget *channelWidget, bool enabled)
{
    if (!channelWidget) return;
    
    QString style = enabled ?
        "QWidget { background-color: #404040; border: 1px solid #666666; }" :
        "QWidget { background-color: #2a2a2a; border: 1px solid #444444; }";
    
    channelWidget->setStyleSheet(style);
}

int DataAcquisition::findParameterIndex(QComboBox *comboBox, uint16_t index, uint8_t subindex)
{
    for (int i = 0; i < comboBox->count(); i++) {
        if (!comboBox->itemData(i).isNull()) {
            QPair<uint16_t, uint8_t> param = comboBox->itemData(i).value<QPair<uint16_t, uint8_t>>();
            if (param.first == index && param.second == subindex) {
                return i;
            }
        }
    }
    return -1;
}

void DataAcquisition::updateStreamingParameters()
{
    // 更新流式参数
}

void DataAcquisition::parseStreamData(const VCI_CAN_OBJ &frame)
{
    // 解析流式数据
}

double DataAcquisition::convertParameterValue(uint16_t index, uint8_t subindex, const QByteArray& data) const
{
    // 转换参数值
    return 0.0;
}

void DataAcquisition::startStreaming(uint8_t nodeId, const QVector<QPair<uint16_t, uint8_t>>& parameters)
{
    // 开始流式传输
}

void DataAcquisition::stopStreaming(uint8_t nodeId)
{
    // 停止流式传输
}



