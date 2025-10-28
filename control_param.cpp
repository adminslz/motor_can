#include "control_param.h"
#include <QTimer>
#include <QMessageBox>
#include <QTabWidget>
#include <QGroupBox>
#include <QFormLayout>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QProgressBar>
#include <QLabel>
#include <QScrollArea>
#include <QFrame>
#include <QButtonGroup>
#include <QRadioButton>
#include <QLineEdit>
#include <QPainter>
#include <QWheelEvent>
#include <cmath>
#include <cstring>
#include <QDebug>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "can_rx_tx.h"
#include "global_vars.h"

// 禁用滚轮的下划线输入框基类
class NoWheelSpinBox : public QDoubleSpinBox
{
public:
    explicit NoWheelSpinBox(QWidget *parent = nullptr) : QDoubleSpinBox(parent) {}

protected:
    void wheelEvent(QWheelEvent *event) override
    {
        event->ignore(); // 忽略滚轮事件
    }
};

class NoWheelIntSpinBox : public QSpinBox
{
public:
    explicit NoWheelIntSpinBox(QWidget *parent = nullptr) : QSpinBox(parent) {}

protected:
    void wheelEvent(QWheelEvent *event) override
    {
        event->ignore(); // 忽略滚轮事件
    }
};

// 自定义下划线数字输入框
class UnderlineSpinBox : public NoWheelSpinBox
{
public:
    explicit UnderlineSpinBox(QWidget *parent = nullptr) : NoWheelSpinBox(parent)
    {
        setFrame(false);
        setButtonSymbols(QAbstractSpinBox::NoButtons);
        setStyleSheet(
            "QDoubleSpinBox {"
            "    background: transparent;"
            "    border: none;"
            "    border-bottom: 2px solid #777;"
            "    padding: 8px 0px;"
            "    font-size: 16px;"
            "    color: #ffffff;"
            "    selection-background-color: #666;"
            "    min-width: 80px;"
            "    max-width: 120px;"
            "}"
            "QDoubleSpinBox:focus {"
            "    border-bottom: 2px solid #4CAF50;"
            "    background-color: #2a2a2a;"
            "}"
        );
    }
};

// 自定义下划线整数输入框
class UnderlineIntSpinBox : public NoWheelIntSpinBox
{
public:
    explicit UnderlineIntSpinBox(QWidget *parent = nullptr) : NoWheelIntSpinBox(parent)
    {
        setFrame(false);
        setButtonSymbols(QAbstractSpinBox::NoButtons);
        setStyleSheet(
            "QSpinBox {"
            "    background: transparent;"
            "    border: none;"
            "    border-bottom: 2px solid #777;"
            "    padding: 8px 0px;"
            "    font-size: 16px;"
            "    color: #ffffff;"
            "    selection-background-color: #666;"
            "    min-width: 80px;"
            "    max-width: 120px;"
            "}"
            "QSpinBox:focus {"
            "    border-bottom: 2px solid #4CAF50;"
            "    background-color: #2a2a2a;"
            "}"
        );
    }
};

ControlParam::ControlParam(QWidget *parent)
    : QWidget(parent)
    , m_tabWidget(nullptr)
    , m_motorEnableCheck(nullptr)
    , m_controlModeCombo(nullptr)
    , m_canIdSpin(nullptr)
    , m_autoApplyCheck(nullptr)
    , m_currentCanId(1)
    , m_autoApply(false)
{
    setupUI();
    setupConnections();
}

void ControlParam::setupUI()
{
    // 设置窗口背景 - 黑色基调
    setStyleSheet(
        "ControlParam {"
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:1,"
        "    stop:0 #1a1a1a, stop:1 #2d2d2d);"
        "}"
    );

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    
    // 创建水平布局用于原有内容
    QHBoxLayout *contentLayout = new QHBoxLayout();
    contentLayout->setSpacing(0);
    contentLayout->setContentsMargins(0, 0, 0, 0);

    // 左侧 - 选项卡选择
    QWidget *leftWidget = new QWidget();
    leftWidget->setFixedWidth(300);  // 增加左侧宽度
    leftWidget->setStyleSheet(
        "QWidget {"
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
        "    stop:0 #0a0a0a, stop:1 #1a1a1a);"
        "    border-right: 3px solid #666;"
        "    border-radius: 0px 8px 8px 0px;"
        "}"
    );

    QVBoxLayout *leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setSpacing(5);
    leftLayout->setContentsMargins(0, 25, 0, 25);

    // 标题
    QLabel *titleLabel = new QLabel("参数设置");
    titleLabel->setStyleSheet(
        "QLabel {"
        "    color: #4fc3f7;"
        "    font-size: 24px;"
        "    font-weight: bold;"
        "    padding: 25px;"
        "    background: rgba(30, 30, 30, 0.9);"
        "    border-bottom: 3px solid #4fc3f7;"
        "    border-radius: 0px 8px 0px 0px;"
        "}"
    );
    titleLabel->setAlignment(Qt::AlignCenter);
    leftLayout->addWidget(titleLabel);

    // 创建选项卡按钮
    QButtonGroup *tabButtonGroup = new QButtonGroup(this);
    const QString tabNames[] = {"基本控制", "PID参数", "高级设置"};
    const QString tabIcons[] = {"◉", "◉", "◉"};

    for (int i = 0; i < 3; ++i) {
        QPushButton *tabButton = new QPushButton(QString("  %1  %2").arg(tabIcons[i]).arg(tabNames[i]));
        tabButton->setCheckable(true);
        tabButton->setFixedHeight(65);
        tabButton->setStyleSheet(
            "QPushButton {"
            "    background: transparent;"
            "    border: none;"
            "    color: #cccccc;"
            "    font-size: 20px;"
            "    font-weight: bold;"
            "    text-align: left;"
            "    padding: 20px 30px;"
            "    border-left: 6px solid transparent;"
            "    margin: 3px 0px;"
            "    border-radius: 0px 8px 8px 0px;"
            "}"
            "QPushButton:checked {"
            "    background: rgba(79, 195, 247, 0.3);"
            "    color: #4fc3f7;"
            "    border-left: 6px solid #4fc3f7;"
            "}"
            "QPushButton:hover {"
            "    background: rgba(79, 195, 247, 0.2);"
            "    color: #4fc3f7;"
            "    border-left: 6px solid #4fc3f7;"
            "}"
        );
        tabButtonGroup->addButton(tabButton, i);
        leftLayout->addWidget(tabButton);
    }

    leftLayout->addStretch();

    // 右侧 - 参数内容区域
    QWidget *rightWidget = new QWidget();
    rightWidget->setStyleSheet(
        "QWidget {"
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:1,"
        "    stop:0 #1a1a1a, stop:1 #2d2d2d);"
        "}"
    );

    QVBoxLayout *rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setSpacing(0);
    rightLayout->setContentsMargins(0, 0, 0, 0);

    // 创建堆叠窗口
    m_tabWidget = new QTabWidget();
    m_tabWidget->setTabPosition(QTabWidget::North);
    m_tabWidget->tabBar()->hide();
    m_tabWidget->setStyleSheet(
        "QTabWidget::pane {"
        "    border: none;"
        "    background: transparent;"
        "}"
    );

    // 创建各个分类的标签页
    for (int i = 0; i < 3; ++i) {
        QScrollArea *scrollArea = new QScrollArea();
        scrollArea->setWidgetResizable(true);
        scrollArea->setFrameShape(QFrame::NoFrame);
        scrollArea->setStyleSheet(
            "QScrollArea {"
            "    background: transparent;"
            "    border: none;"
            "}"
            "QScrollArea > QWidget > QWidget {"
            "    background: transparent;"
            "}"
        );

        QWidget *tabWidget = new QWidget();
        tabWidget->setStyleSheet("QWidget { background: transparent; }");
        QVBoxLayout *tabLayout = new QVBoxLayout(tabWidget);
        tabLayout->setSpacing(20);
        tabLayout->setContentsMargins(25, 20, 25, 20);

        m_tabWidget->addTab(scrollArea, QString::number(i));
        scrollArea->setWidget(tabWidget);
    }

    rightLayout->addWidget(m_tabWidget);

    // 底部按钮区域
    QFrame *buttonFrame = new QFrame();
    buttonFrame->setFrameStyle(QFrame::StyledPanel);
    buttonFrame->setStyleSheet(
        "QFrame {"
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
        "    stop:0 #0a0a0a, stop:1 #1a1a1a);"
        "    border-top: 3px solid #444;"
        "    border-radius: 0px;"
        "}"
    );

    QHBoxLayout *buttonLayout = new QHBoxLayout(buttonFrame);
    buttonLayout->setContentsMargins(25, 15, 25, 15);

    // 自动应用复选框
    m_autoApplyCheck = new QCheckBox("实时应用");
    m_autoApplyCheck->setStyleSheet(
        "QCheckBox {"
        "    color: #cccccc;"
        "    font-weight: bold;"
        "    font-size: 16px;"
        "    spacing: 8px;"
        "}"
        "QCheckBox::indicator {"
        "    width: 20px;"
        "    height: 20px;"
        "}"
        "QCheckBox::indicator:unchecked {"
        "    background: #555;"
        "    border: 2px solid #777;"
        "    border-radius: 4px;"
        "}"
        "QCheckBox::indicator:checked {"
        "    background: #4CAF50;"
        "    border: 2px solid #66BB6A;"
        "    border-radius: 4px;"
        "}"
    );

    QPushButton *applyBtn = new QPushButton("▷ 应用参数");
    QPushButton *readBtn = new QPushButton("↻ 读取所有");
    QPushButton *resetBtn = new QPushButton("↺ 恢复默认");
    QPushButton *saveBtn = new QPushButton("💾 保存配置");
    QPushButton *loadBtn = new QPushButton("📁 加载配置");

    // 设置按钮样式
    QString buttonStyle =
        "QPushButton {"
        "    padding: 12px 20px;"
        "    font-weight: bold;"
        "    border: 2px solid #666;"
        "    border-radius: 8px;"
        "    min-width: 120px;"
        "    font-size: 16px;"
        "    color: #ffffff;"
        "    background: #555;"
        "}"
        "QPushButton:hover {"
        "    border: 2px solid #888;"
        "    background: #666;"
        "}"
        "QPushButton:pressed {"
        "    background: #777;"
        "}";

    applyBtn->setStyleSheet(buttonStyle);
    readBtn->setStyleSheet(buttonStyle);
    resetBtn->setStyleSheet(buttonStyle);
    saveBtn->setStyleSheet(buttonStyle);
    loadBtn->setStyleSheet(buttonStyle);

    buttonLayout->addWidget(m_autoApplyCheck);
    buttonLayout->addSpacing(15);
    buttonLayout->addWidget(applyBtn);
    buttonLayout->addWidget(readBtn);
    buttonLayout->addWidget(resetBtn);
    buttonLayout->addStretch();
    buttonLayout->addWidget(loadBtn);
    buttonLayout->addWidget(saveBtn);

    rightLayout->addWidget(buttonFrame);

    // 主布局
    mainLayout->addLayout(contentLayout, 1);  // 添加内容布局
    contentLayout->addWidget(leftWidget);
    contentLayout->addWidget(rightWidget, 1);

    // 创建参数控件
    createParameterControls();

    // 连接选项卡按钮信号
    connect(tabButtonGroup, QOverload<int>::of(&QButtonGroup::buttonClicked),
            m_tabWidget, &QTabWidget::setCurrentIndex);

    // 默认选择第一个选项卡
    if (tabButtonGroup->button(0)) {
        tabButtonGroup->button(0)->setChecked(true);
    }

    // 连接按钮信号槽
    connect(applyBtn, &QPushButton::clicked, this, &ControlParam::onApplyParamsClicked);
    connect(readBtn, &QPushButton::clicked, this, &ControlParam::onReadAllParamsClicked);
    connect(resetBtn, &QPushButton::clicked, this, &ControlParam::onResetParamsClicked);
    connect(saveBtn, &QPushButton::clicked, this, &ControlParam::onSaveParamsClicked);
    connect(loadBtn, &QPushButton::clicked, this, &ControlParam::onLoadParamsClicked);
    connect(m_autoApplyCheck, &QCheckBox::toggled, this, &ControlParam::onAutoApplyToggled);
}

// 获取参数单位的辅助函数
QString ControlParam::getUnitString(const ODEntry& param)
{
    // 根据参数名称或索引判断单位
    QString paramName = param.name.toLower();

    if (paramName.contains("current") || paramName.contains("电流")) {
        return "A";
    } else if (paramName.contains("voltage") || paramName.contains("电压")) {
        return "V";
    } else if (paramName.contains("speed") || paramName.contains("速度")) {
        return "rpm";
    } else if (paramName.contains("position") || paramName.contains("位置")) {
        return "°";
    } else if (paramName.contains("torque") || paramName.contains("扭矩")) {
        return "N·m";
    } else if (paramName.contains("time") || paramName.contains("时间")) {
        return "s";
    } else if (paramName.contains("frequency") || paramName.contains("频率")) {
        return "Hz";
    } else if (paramName.contains("temperature") || paramName.contains("温度")) {
        return "°C";
    } else if (paramName.contains("ratio") || paramName.contains("比率")) {
        return "%";
    }

    return ""; // 没有单位
}

void ControlParam::createParameterControls()
{
    // 为每个分类创建参数控件
    for (int category = 0; category < 3; ++category) {
        QScrollArea *scrollArea = qobject_cast<QScrollArea*>(m_tabWidget->widget(category));
        if (!scrollArea) continue;

        QWidget *tabWidget = scrollArea->widget();
        QVBoxLayout *tabLayout = qobject_cast<QVBoxLayout*>(tabWidget->layout());
        if (!tabLayout) continue;

        // 清空现有布局
        QLayoutItem *child;
        while ((child = tabLayout->takeAt(0)) != nullptr) {
            if (child->widget()) {
                delete child->widget();
            }
            delete child;
        }

        // 获取该分类的所有参数
        QVector<ODEntry> params = m_paramDict.getParametersByCategory(category);

        if (params.isEmpty()) {
            QLabel *emptyLabel = new QLabel("该分类暂无参数");
            emptyLabel->setStyleSheet("QLabel { color: #888; font-size: 18px; text-align: center; }");
            emptyLabel->setAlignment(Qt::AlignCenter);
            tabLayout->addWidget(emptyLabel);
            continue;
        }

        // 按分组名称创建GroupBox
        QMap<QString, QVector<ODEntry>> groupedParams;
        for (const auto& param : params) {
            groupedParams[param.groupName].append(param);
        }

        // 为每个组创建GroupBox
        for (auto it = groupedParams.begin(); it != groupedParams.end(); ++it) {
            QGroupBox *groupBox = new QGroupBox(it.key());
            groupBox->setStyleSheet(
                "QGroupBox {"
                "    font-weight: bold;"
                "    font-size: 18px;"
                "    color: #dddddd;"
                "    margin-top: 12px;"
                "    border: 2px solid #555;"
                "    border-radius: 10px;"
                "    padding-top: 16px;"
                "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
                "    stop:0 rgba(70, 70, 70, 0.8), stop:1 rgba(90, 90, 90, 0.6));"
                "}"
                "QGroupBox::title {"
                "    subcontrol-origin: margin;"
                "    left: 16px;"
                "    padding: 0 12px 0 12px;"
                "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
                "    stop:0 #666, stop:1 #777);"
                "    color: #ffffff;"
                "    border-radius: 6px;"
                "}"
            );

            QVBoxLayout *groupLayout = new QVBoxLayout(groupBox);
            groupLayout->setSpacing(12);
            groupLayout->setContentsMargins(20, 25, 20, 20);

            for (const auto& param : it.value()) {
                QWidget *paramWidget = createParameterWidget(param);
                if (paramWidget) {
                    groupLayout->addWidget(paramWidget);

                    // 存储控件映射 - 只存储控制参数
                    QString key = QString("%1-%2").arg(param.index, 4, 16, QChar('0'))
                                                 .arg(param.subindex, 2, 16, QChar('0'));

                    QWidget *control = paramWidget->findChild<QDoubleSpinBox*>();
                    if (!control) control = paramWidget->findChild<QSpinBox*>();
                    if (!control) control = paramWidget->findChild<QCheckBox*>();
                    if (!control) control = paramWidget->findChild<QComboBox*>();
                    if (control) {
                        m_controlMap[key] = control;
                    }

                    // 保存特殊控件指针
                    if (param.index == 0x60D4 && param.subindex == 0x00) {
                        m_motorEnableCheck = paramWidget->findChild<QCheckBox*>();
                    } else if (param.index == 0x6042 && param.subindex == 0x00) {
                        m_controlModeCombo = paramWidget->findChild<QComboBox*>();
                    } else if (param.index == 0x60D2 && param.subindex == 0x00) {
                        m_canIdSpin = paramWidget->findChild<QSpinBox*>();
                    }
                }
            }

            tabLayout->addWidget(groupBox);
        }

        tabLayout->addStretch();
    }
}

QWidget* ControlParam::createParameterWidget(const ODEntry& param)
{
    QWidget *paramWidget = new QWidget();
    paramWidget->setStyleSheet("QWidget { background: transparent; }");

    QGridLayout *layout = new QGridLayout(paramWidget);
    layout->setVerticalSpacing(8);
    layout->setHorizontalSpacing(15);
    layout->setContentsMargins(8, 8, 8, 8);

    // 第一行：参数名称和输入控件
    QLabel *nameLabel = new QLabel(param.name + ":");
    nameLabel->setStyleSheet(
        "QLabel {"
        "    font-weight: bold;"
        "    font-size: 18px;"
        "    color: #dddddd;"
        "    padding: 4px 0px;"
        "}"
    );

    QWidget *control = createControlForParameter(param);

    // 获取单位
    QString unit = getUnitString(param);
    QLabel *unitLabel = nullptr;

    if (!unit.isEmpty()) {
        unitLabel = new QLabel(unit);
        unitLabel->setStyleSheet(
            "QLabel {"
            "    font-size: 14px;"
            "    color: #aaaaaa;"
            "    padding: 4px 0px;"
            "    margin-left: 5px;"
            "}"
        );
    }

    // 第二行：详细信息
    QWidget *infoWidget = new QWidget();
    infoWidget->setStyleSheet("QWidget { background: rgba(70, 70, 70, 0.5); border-radius: 5px; }");
    QHBoxLayout *infoLayout = new QHBoxLayout(infoWidget);
    infoLayout->setSpacing(20);
    infoLayout->setContentsMargins(12, 6, 12, 6);

    // 索引信息
    QString indexText = QString("索引: 0x%1-%2")
                       .arg(param.index, 4, 16, QChar('0'))
                       .arg(param.subindex, 2, 16, QChar('0'));
    QLabel *indexLabel = new QLabel(indexText);
    indexLabel->setStyleSheet("QLabel { color: #aaaaaa; font-size: 14px; }");

    // 数据类型
    QString typeText = getTypeString(param.type);
    QLabel *typeLabel = new QLabel("类型: " + typeText);
    typeLabel->setStyleSheet("QLabel { color: #aaaaaa; font-size: 14px; }");

    // 范围信息
    QString rangeText = getRangeString(param);
    QLabel *rangeLabel = new QLabel("范围: " + rangeText);
    rangeLabel->setStyleSheet("QLabel { color: #aaaaaa; font-size: 14px; }");

    // 默认值
    QString defaultText = getDefaultValueString(param);
    QLabel *defaultLabel = new QLabel("默认: " + defaultText);
    defaultLabel->setStyleSheet("QLabel { color: #aaaaaa; font-size: 14px; }");

    infoLayout->addWidget(indexLabel);
    infoLayout->addWidget(typeLabel);
    infoLayout->addWidget(rangeLabel);
    infoLayout->addWidget(defaultLabel);
    infoLayout->addStretch();

    // 第一行布局：名称 + 输入框 + 单位
    layout->addWidget(nameLabel, 0, 0);
    layout->addWidget(control, 0, 1);
    if (unitLabel) {
        layout->addWidget(unitLabel, 0, 2);
    }

    // 第二行：详细信息
    layout->addWidget(infoWidget, 1, 0, 1, unitLabel ? 3 : 2);

    return paramWidget;
}

QWidget* ControlParam::createControlForParameter(const ODEntry& param)
{
    // 只读参数创建标签显示
    if (!param.writable) {
        QLabel *label = new QLabel("只读");
        label->setStyleSheet(
            "QLabel {"
            "    color: #888888;"
            "    font-style: italic;"
            "    padding: 10px 15px;"
            "    background: rgba(100, 100, 100, 0.5);"
            "    border-radius: 6px;"
            "    min-width: 80px;"
            "    font-size: 14px;"
            "    text-align: center;"
            "}"
        );
        label->setProperty("paramIndex", param.index);
        label->setProperty("paramSubindex", param.subindex);
        return label;
    }

    switch (param.type) {
    case OD_TYPE_FLOAT: {
        UnderlineSpinBox *spinBox = new UnderlineSpinBox();
        spinBox->setRange(param.minVal.toDouble(), param.maxVal.toDouble());
        spinBox->setValue(param.defaultValue.toDouble());

        // 根据参数范围设置精度
        double range = param.maxVal.toDouble() - param.minVal.toDouble();
        if (range < 1.0) {
            spinBox->setDecimals(4);
            spinBox->setSingleStep(0.0001);
        } else if (range < 10.0) {
            spinBox->setDecimals(3);
            spinBox->setSingleStep(0.001);
        } else if (range < 100.0) {
            spinBox->setDecimals(2);
            spinBox->setSingleStep(0.01);
        } else {
            spinBox->setDecimals(1);
            spinBox->setSingleStep(0.1);
        }

        spinBox->setProperty("paramIndex", param.index);
        spinBox->setProperty("paramSubindex", param.subindex);

        // 安装事件过滤器以捕获回车键
        spinBox->installEventFilter(this);

        connect(spinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                this, &ControlParam::onParameterValueChanged);
        return spinBox;
    }

    case OD_TYPE_INT16:
    case OD_TYPE_UINT16: {
        UnderlineIntSpinBox *spinBox = new UnderlineIntSpinBox();
        spinBox->setRange(param.minVal.toInt(), param.maxVal.toInt());
        spinBox->setValue(param.defaultValue.toInt());

        spinBox->setProperty("paramIndex", param.index);
        spinBox->setProperty("paramSubindex", param.subindex);

        // 安装事件过滤器以捕获回车键
        spinBox->installEventFilter(this);

        connect(spinBox, QOverload<int>::of(&QSpinBox::valueChanged),
                this, &ControlParam::onParameterValueChanged);
        return spinBox;
    }

    case OD_TYPE_UINT8: {
        UnderlineIntSpinBox *spinBox = new UnderlineIntSpinBox();
        spinBox->setRange(param.minVal.toInt(), param.maxVal.toInt());
        spinBox->setValue(param.defaultValue.toInt());

        spinBox->setProperty("paramIndex", param.index);
        spinBox->setProperty("paramSubindex", param.subindex);

        // 安装事件过滤器以捕获回车键
        spinBox->installEventFilter(this);

        connect(spinBox, QOverload<int>::of(&QSpinBox::valueChanged),
                this, &ControlParam::onParameterValueChanged);
        return spinBox;
    }

    case OD_TYPE_BOOLEAN: {
        QCheckBox *checkBox = new QCheckBox();
        checkBox->setChecked(param.defaultValue.toBool());

        checkBox->setStyleSheet(
            "QCheckBox {"
            "    spacing: 12px;"
            "    font-weight: bold;"
            "    font-size: 14px;"
            "    color: #dddddd;"
            "}"
            "QCheckBox::indicator {"
            "    width: 20px;"
            "    height: 20px;"
            "    border: 2px solid #777;"
            "    border-radius: 4px;"
            "    background: #555;"
            "}"
            "QCheckBox::indicator:checked {"
            "    background: #4CAF50;"
            "    border: 2px solid #66BB6A;"
            "}"
            "QCheckBox::indicator:checked:hover {"
            "    background: #66BB6A;"
            "    border: 2px solid #81C784;"
            "}"
        );

        checkBox->setProperty("paramIndex", param.index);
        checkBox->setProperty("paramSubindex", param.subindex);
        connect(checkBox, &QCheckBox::toggled,
                this, &ControlParam::onParameterValueChanged);
        return checkBox;
    }

    default: {
        QLabel *label = new QLabel("不支持的类型");
        label->setStyleSheet(
            "QLabel {"
            "    color: #ff6b6b;"
            "    padding: 10px 15px;"
            "    background: rgba(255, 107, 107, 0.2);"
            "    border-radius: 6px;"
            "    font-size: 14px;"
            "}"
        );
        return label;
    }
    }
}

bool ControlParam::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);

        // 检查是否是回车键或小键盘回车键
        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            // 检查对象是否是输入框
            if (QDoubleSpinBox *spinBox = qobject_cast<QDoubleSpinBox*>(obj)) {
                uint16_t index = spinBox->property("paramIndex").toUInt();
                uint8_t subindex = spinBox->property("paramSubindex").toUInt();
                double value = spinBox->value();

                ODEntry param = m_paramDict.getParameter(index, subindex);

                // 弹出确认提示框
                int ret = QMessageBox::question(this, "确认下发参数",
                    QString("是否下发参数?\n\n"
                           "参数名称: %1\n"
                           "参数值: %2\n"
                           "索引: 0x%3-%4")
                    .arg(param.name)
                    .arg(value)
                    .arg(index, 4, 16, QChar('0'))
                    .arg(subindex, 2, 16, QChar('0')),
                    QMessageBox::Yes | QMessageBox::No,
                    QMessageBox::No);

                if (ret == QMessageBox::Yes) {
                    sendParameterValue(param, value);
                    qDebug() << "回车键下发参数:" << param.name << "值:" << value;

                    // 发送成功后显示成功提示
                    QMessageBox::information(this, "下发成功",
                        QString("参数下发成功:\n%1\n值: %2")
                        .arg(param.name)
                        .arg(value));
                } else {
                    qDebug() << "用户取消下发参数:" << param.name;
                }
                return true;
            }
            else if (QSpinBox *spinBox = qobject_cast<QSpinBox*>(obj)) {
                uint16_t index = spinBox->property("paramIndex").toUInt();
                uint8_t subindex = spinBox->property("paramSubindex").toUInt();
                int value = spinBox->value();

                ODEntry param = m_paramDict.getParameter(index, subindex);

                // 弹出确认提示框
                int ret = QMessageBox::question(this, "确认下发参数",
                    QString("是否下发参数?\n\n"
                           "参数名称: %1\n"
                           "参数值: %2\n"
                           "索引: 0x%3-%4")
                    .arg(param.name)
                    .arg(value)
                    .arg(index, 4, 16, QChar('0'))
                    .arg(subindex, 2, 16, QChar('0')),
                    QMessageBox::Yes | QMessageBox::No,
                    QMessageBox::No);

                if (ret == QMessageBox::Yes) {
                    sendParameterValue(param, value);
                    qDebug() << "回车键下发参数:" << param.name << "值:" << value;

                    // 发送成功后显示成功提示
                    QMessageBox::information(this, "下发成功",
                        QString("参数下发成功:\n%1\n值: %2")
                        .arg(param.name)
                        .arg(value));
                } else {
                    qDebug() << "用户取消下发参数:" << param.name;
                }
                return true;
            }
            else if (QCheckBox *checkBox = qobject_cast<QCheckBox*>(obj)) {
                uint16_t index = checkBox->property("paramIndex").toUInt();
                uint8_t subindex = checkBox->property("paramSubindex").toUInt();
                bool value = checkBox->isChecked();

                ODEntry param = m_paramDict.getParameter(index, subindex);

                // 弹出确认提示框
                int ret = QMessageBox::question(this, "确认下发参数",
                    QString("是否下发参数?\n\n"
                           "参数名称: %1\n"
                           "参数值: %2\n"
                           "索引: 0x%3-%4")
                    .arg(param.name)
                    .arg(value ? "true" : "false")
                    .arg(index, 4, 16, QChar('0'))
                    .arg(subindex, 2, 16, QChar('0')),
                    QMessageBox::Yes | QMessageBox::No,
                    QMessageBox::No);

                if (ret == QMessageBox::Yes) {
                    sendParameterValue(param, value);
                    qDebug() << "回车键下发参数:" << param.name << "值:" << value;

                    // 发送成功后显示成功提示
                    QMessageBox::information(this, "下发成功",
                        QString("参数下发成功:\n%1\n值: %2")
                        .arg(param.name)
                        .arg(value ? "true" : "false"));
                } else {
                    qDebug() << "用户取消下发参数:" << param.name;
                }
                return true;
            }
        }
    }

    return QWidget::eventFilter(obj, event);
}

QString ControlParam::getTypeString(ODType type)
{
    switch (type) {
    case OD_TYPE_FLOAT: return "float";
    case OD_TYPE_INT16: return "int16";
    case OD_TYPE_UINT16: return "uint16";
    case OD_TYPE_UINT8: return "uint8";
    case OD_TYPE_BOOLEAN: return "bool";
    default: return "unknown";
    }
}

QString ControlParam::getRangeString(const ODEntry& param)
{
    switch (param.type) {
    case OD_TYPE_FLOAT:
        return QString("%1 ~ %2").arg(param.minVal.toDouble()).arg(param.maxVal.toDouble());
    case OD_TYPE_INT16:
    case OD_TYPE_UINT16:
    case OD_TYPE_UINT8:
        return QString("%1 ~ %2").arg(param.minVal.toInt()).arg(param.maxVal.toInt());
    case OD_TYPE_BOOLEAN:
        return "0 ~ 1";
    default:
        return "N/A";
    }
}

QString ControlParam::getDefaultValueString(const ODEntry& param)
{
    switch (param.type) {
    case OD_TYPE_FLOAT:
        return QString::number(param.defaultValue.toDouble(), 'f', 3);
    case OD_TYPE_INT16:
    case OD_TYPE_UINT16:
    case OD_TYPE_UINT8:
        return QString::number(param.defaultValue.toInt());
    case OD_TYPE_BOOLEAN:
        return param.defaultValue.toBool() ? "true" : "false";
    default:
        return "N/A";
    }
}

void ControlParam::setupConnections()
{
    if (m_motorEnableCheck) {
        connect(m_motorEnableCheck, &QCheckBox::toggled, this, &ControlParam::onMotorEnableToggled);
    }
    if (m_controlModeCombo) {
        connect(m_controlModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &ControlParam::onControlModeChanged);
    }
}

void ControlParam::setCanId(uint8_t canId)
{
    m_currentCanId = canId;
    if (m_canIdSpin) {
        m_canIdSpin->blockSignals(true);
        m_canIdSpin->setValue(canId);
        m_canIdSpin->blockSignals(false);
    }
}

void ControlParam::updateParameterValue(uint16_t index, uint8_t subindex, const QVariant& value)
{
    QString key = QString("%1-%2").arg(index, 4, 16, QChar('0'))
                                 .arg(subindex, 2, 16, QChar('0'));

    if (m_controlMap.contains(key)) {
        QWidget *control = m_controlMap[key];

        // 阻塞信号防止循环触发
        if (QDoubleSpinBox *spinBox = qobject_cast<QDoubleSpinBox*>(control)) {
            spinBox->blockSignals(true);
            spinBox->setValue(value.toDouble());
            spinBox->blockSignals(false);
        } else if (QSpinBox *spinBox = qobject_cast<QSpinBox*>(control)) {
            spinBox->blockSignals(true);
            spinBox->setValue(value.toInt());
            spinBox->blockSignals(false);
        } else if (QCheckBox *checkBox = qobject_cast<QCheckBox*>(control)) {
            checkBox->blockSignals(true);
            checkBox->setChecked(value.toBool());
            checkBox->blockSignals(false);
        }
    }
}

void ControlParam::onApplyParamsClicked()
{
    // 应用所有可写参数的当前值
    int appliedCount = 0;
    for (auto it = m_controlMap.begin(); it != m_controlMap.end(); ++it) {
        QWidget *control = it.value();
        uint16_t index = control->property("paramIndex").toUInt();
        uint8_t subindex = control->property("paramSubindex").toUInt();

        ODEntry param = m_paramDict.getParameter(index, subindex);
        if (param.writable) {
            QVariant value;
            if (QDoubleSpinBox *spinBox = qobject_cast<QDoubleSpinBox*>(control)) {
                value = spinBox->value();
            } else if (QSpinBox *spinBox = qobject_cast<QSpinBox*>(control)) {
                value = spinBox->value();
            } else if (QCheckBox *checkBox = qobject_cast<QCheckBox*>(control)) {
                value = checkBox->isChecked();
            }

            sendParameterValue(param, value);
            appliedCount++;
        }
    }

    QMessageBox::information(this, "提示", QString("已应用 %1 个参数到电机").arg(appliedCount));
}

void ControlParam::onReadAllParamsClicked()
{
    // 读取所有参数
    QVector<ODEntry> allParams = m_paramDict.getAllParameters();
    int readCount = 0;
    for (const auto& param : allParams) {
        if (param.readable) {
            emit sdoReadRequest(m_currentCanId, param.index, param.subindex);
            readCount++;
        }
    }

    QMessageBox::information(this, "提示", QString("已发送 %1 个参数读取请求").arg(readCount));
}

void ControlParam::onResetParamsClicked()
{
    int ret = QMessageBox::question(this, "确认", "确定要恢复默认参数吗？");
    if (ret == QMessageBox::Yes) {
        // 重置所有参数到默认值
        int resetCount = 0;
        for (auto it = m_controlMap.begin(); it != m_controlMap.end(); ++it) {
            QWidget *control = it.value();
            uint16_t index = control->property("paramIndex").toUInt();
            uint8_t subindex = control->property("paramSubindex").toUInt();

            ODEntry param = m_paramDict.getParameter(index, subindex);
            QVariant defaultValue = param.defaultValue;

            // 阻塞信号防止触发发送
            if (QDoubleSpinBox *spinBox = qobject_cast<QDoubleSpinBox*>(control)) {
                spinBox->blockSignals(true);
                spinBox->setValue(defaultValue.toDouble());
                spinBox->blockSignals(false);
                resetCount++;
            } else if (QSpinBox *spinBox = qobject_cast<QSpinBox*>(control)) {
                spinBox->blockSignals(true);
                spinBox->setValue(defaultValue.toInt());
                spinBox->blockSignals(false);
                resetCount++;
            } else if (QCheckBox *checkBox = qobject_cast<QCheckBox*>(control)) {
                checkBox->blockSignals(true);
                checkBox->setChecked(defaultValue.toBool());
                checkBox->blockSignals(false);
                resetCount++;
            }
        }

        QMessageBox::information(this, "提示", QString("已重置 %1 个参数为默认值").arg(resetCount));
    }
}

void ControlParam::onSaveParamsClicked()
{
    QString fileName = QFileDialog::getSaveFileName(this, "保存参数配置", "", "JSON Files (*.json)");
    if (!fileName.isEmpty()) {
        // 这里可以实现保存参数到JSON文件的功能
        QMessageBox::information(this, "提示", "参数配置已保存到文件: " + fileName);
    }
}

void ControlParam::onLoadParamsClicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, "加载参数配置", "", "JSON Files (*.json)");
    if (!fileName.isEmpty()) {
        // 这里可以实现从JSON文件加载参数的功能
        QMessageBox::information(this, "提示", "参数配置已从文件加载: " + fileName);
    }
}

void ControlParam::onParameterValueChanged()
{
    QWidget *senderWidget = qobject_cast<QWidget*>(sender());
    if (!senderWidget) return;

    uint16_t index = senderWidget->property("paramIndex").toUInt();
    uint8_t subindex = senderWidget->property("paramSubindex").toUInt();

    ODEntry param = m_paramDict.getParameter(index, subindex);
    if (!param.writable) return;

    QVariant value;
    if (QDoubleSpinBox *spinBox = qobject_cast<QDoubleSpinBox*>(senderWidget)) {
        value = spinBox->value();
    } else if (QSpinBox *spinBox = qobject_cast<QSpinBox*>(senderWidget)) {
        value = spinBox->value();
    } else if (QCheckBox *checkBox = qobject_cast<QCheckBox*>(senderWidget)) {
        value = checkBox->isChecked();
    }

    // 如果开启自动应用，实时发送参数变化
    if (m_autoApply) {
        sendParameterValue(param, value);
    }
}

void ControlParam::onAutoApplyToggled(bool enabled)
{
    m_autoApply = enabled;
    if (enabled) {
        QMessageBox::information(this, "提示", "已开启实时应用模式");
    } else {
        QMessageBox::information(this, "提示", "已关闭实时应用模式");
    }
}

void ControlParam::onMotorEnableToggled(bool enabled)
{
    if (enabled) {
        QMessageBox::information(this, "提示", "电机已使能");
    } else {
        QMessageBox::information(this, "提示", "电机已失能");
    }

    // 发送电机使能命令
    ODEntry param = m_paramDict.getParameter(0x60D4, 0x00);
    sendParameterValue(param, enabled ? 1 : 0);
}

void ControlParam::onControlModeChanged(int index)
{
    const QString modes[] = {"MIT模式", "位置模式", "速度模式", "电流模式"};
    if (index >= 0 && index < 4) {
        QMessageBox::information(this, "模式切换", QString("已切换到%1").arg(modes[index]));

        // 发送模式切换命令
        ODEntry param = m_paramDict.getParameter(0x6042, 0x00);
        sendParameterValue(param, index);
    }
}

void ControlParam::sendParameterValue(const ODEntry& param, const QVariant& value)
{
    QByteArray data;

    switch (param.type) {
    case OD_TYPE_FLOAT: {
        float floatValue = value.toFloat();
        data = QByteArray(reinterpret_cast<const char*>(&floatValue), sizeof(float));
        break;
    }
    case OD_TYPE_INT16: {
        int16_t intValue = value.toInt();
        data = QByteArray(reinterpret_cast<const char*>(&intValue), sizeof(int16_t));
        break;
    }
    case OD_TYPE_UINT16: {
        uint16_t uintValue = value.toUInt();
        data = QByteArray(reinterpret_cast<const char*>(&uintValue), sizeof(uint16_t));
        break;
    }
    case OD_TYPE_UINT8: {
        uint8_t uintValue = value.toUInt();
        data = QByteArray(reinterpret_cast<const char*>(&uintValue), sizeof(uint8_t));
        break;
    }
    case OD_TYPE_BOOLEAN: {
        uint8_t boolValue = value.toBool() ? 1 : 0;
        data = QByteArray(reinterpret_cast<const char*>(&boolValue), sizeof(uint8_t));
        break;
    }
    default:
        return;
    }

    // 使用CAN通信发送参数数据
    if (g_canTxRx && g_canTxRx->isDeviceReady()) {
        bool success = g_canTxRx->sendParameterData(m_currentCanId, param.index, param.subindex, data);

        if (success) {
            qDebug() << "参数下发成功:" << param.name << "值:" << value.toString()
                     << "索引:" << QString::number(param.index, 16)
                     << "子索引:" << QString::number(param.subindex, 16);
        } else {
            qDebug() << "参数下发失败:" << param.name;
            QMessageBox::warning(this, "发送失败",
                QString("参数 %1 下发失败\n%2")
                .arg(param.name)
                .arg(g_canTxRx->getLastError()));
        }
    } else {
        qDebug() << "CAN设备未就绪，无法发送参数";
        QMessageBox::warning(this, "设备未就绪", "CAN设备未初始化，无法发送参数");
    }
}
