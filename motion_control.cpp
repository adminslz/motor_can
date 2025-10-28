#include "motion_control.h"
#include <QSpacerItem>
#include <QDebug>

MotionControl::MotionControl(QWidget *parent)
    : QWidget(parent)
    , controlModeGroup(new QButtonGroup(this))
    , speedControlWidget(new MotionControlSpeed(this))
    , currentControlWidget(new MotionControlCurrent(this))
    , motionControlWidget(new MotionControlMotion(this))
    , positionControlWidget(new MotionControlPosition(this))  // 初始化位置控制组件
    , motorStatus(new MotorStatus(this))
{
    setupUI();
}

void MotionControl::setupUI()
{
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // 左侧 - 控制模式选择
    QWidget *leftWidget = new QWidget();
    leftWidget->setFixedWidth(300);
    leftWidget->setStyleSheet(
        "QWidget {"
        "    background-color: #3a3a3a;"
        "    border: none;"
        "}"
    );
    QVBoxLayout *leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(12, 12, 8, 12);
    leftLayout->setSpacing(12);
    setupControlModePanel(leftLayout);

    // 添加垂直分隔符
    QFrame *verticalLine1 = new QFrame();
    verticalLine1->setFrameShape(QFrame::VLine);
    verticalLine1->setFrameShadow(QFrame::Sunken);
    verticalLine1->setStyleSheet(
        "QFrame {"
        "    background-color: #555555;"
        "    border: none;"
        "    width: 1px;"
        "}"
    );

    // 中间 - 控制参数和运动控制
    QWidget *centerWidget = new QWidget();
    centerWidget->setStyleSheet(
        "QWidget {"
        "    background-color: #2d2d2d;"
        "    border: none;"
        "}"
    );
    QVBoxLayout *centerLayout = new QVBoxLayout(centerWidget);
    centerLayout->setContentsMargins(12, 12, 12, 12);

    // 添加速度控制组件
    centerLayout->addWidget(speedControlWidget);

    // 添加位置控制组件（初始隐藏）
    centerLayout->addWidget(positionControlWidget);
    positionControlWidget->setVisible(false);

    // 添加电流控制组件（初始隐藏）
    centerLayout->addWidget(currentControlWidget);
    currentControlWidget->setVisible(false);

    // 添加运控控制组件（初始隐藏）
    centerLayout->addWidget(motionControlWidget);
    motionControlWidget->setVisible(false);

    centerLayout->addStretch();

    // 添加垂直分隔符
    QFrame *verticalLine2 = new QFrame();
    verticalLine2->setFrameShape(QFrame::VLine);
    verticalLine2->setFrameShadow(QFrame::Sunken);
    verticalLine2->setStyleSheet(
        "QFrame {"
        "    background-color: #555555;"
        "    border: none;"
        "    width: 1px;"
        "}"
    );

    // 右侧 - 电机状态
    motorStatus->setFixedWidth(280);

    // 添加到主布局
    mainLayout->addWidget(leftWidget);
    mainLayout->addWidget(verticalLine1);
    mainLayout->addWidget(centerWidget, 1);
    mainLayout->addWidget(verticalLine2);
    mainLayout->addWidget(motorStatus);
}


void MotionControl::setupControlModePanel(QVBoxLayout *leftLayout)
{
    // 标题
    QLabel *titleLabel = new QLabel("控制模式");
    titleLabel->setStyleSheet(
        "QLabel {"
        "    color: #ffffff;"
        "    font-size: 30px;"
        "    font-weight: bold;"
        "    padding: 8px 0px;"
        "    border-bottom: 1px solid #555555;"
        "}"
    );
    titleLabel->setAlignment(Qt::AlignCenter);

    // 模式选择容器
    QWidget *modeContainer = new QWidget();
    modeContainer->setStyleSheet(
        "QWidget {"
        "    background-color: #454545;"
        "    border-radius: 6px;"
        "    border: 1px solid #555555;"
        "}"
    );
    QVBoxLayout *modeLayout = new QVBoxLayout(modeContainer);
    modeLayout->setSpacing(0);
    modeLayout->setContentsMargins(0, 0, 0, 0);

    // 创建控制模式按钮
    QPushButton *speedModeBtn = createModeButton("速度模式", "控制电机转速", SPEED_MODE);
    QPushButton *positionModeBtn = createModeButton("位置模式", "控制电机位置(PID+位置控制)", POSITION_MODE);  // 更新描述
    QPushButton *torqueModeBtn = createModeButton("扭矩模式", "控制电机扭矩/电流", TORQUE_MODE);
    QPushButton *motionControlModeBtn = createModeButton("运控模式", "高级运动控制(PID+位置+转速+电流)", MOTION_CONTROL_MODE);

    // 添加到布局
    modeLayout->addWidget(speedModeBtn);
    modeLayout->addWidget(createSeparator());
    modeLayout->addWidget(positionModeBtn);
    modeLayout->addWidget(createSeparator());
    modeLayout->addWidget(torqueModeBtn);
    modeLayout->addWidget(createSeparator());
    modeLayout->addWidget(motionControlModeBtn);

    // 设置默认选中
    speedModeBtn->setChecked(true);

    // 模式说明
    QWidget *descContainer = new QWidget();
    descContainer->setStyleSheet(
        "QWidget {"
        "    background-color: #454545;"
        "    border-radius: 6px;"
        "    border: 1px solid #555555;"
        "    padding: 10px;"
        "}"
    );
    QVBoxLayout *descLayout = new QVBoxLayout(descContainer);

    QLabel *descTitle = new QLabel("模式说明");
    descTitle->setStyleSheet(
        "QLabel {"
        "    color: #4fc3f7;"
        "    font-size: 25px;"
        "    font-weight: bold;"
        "    padding-bottom: 6px;"
        "}"
    );

    QLabel *descLabel = new QLabel("选择不同的控制模式来调整电机运行方式。");
    descLabel->setWordWrap(true);
    descLabel->setStyleSheet(
        "QLabel {"
        "    color: #cccccc;"
        "    font-size: 18px;"
        "    line-height: 1.4;"
        "}"
    );

    descLayout->addWidget(descTitle);
    descLayout->addWidget(descLabel);

    leftLayout->addWidget(titleLabel);
    leftLayout->addWidget(modeContainer);
    leftLayout->addWidget(descContainer);
    leftLayout->addStretch();
}

QPushButton* MotionControl::createModeButton(const QString &title, const QString &description, int modeId)
{
    QPushButton *button = new QPushButton(title);
    button->setCheckable(true);
    button->setAutoExclusive(true);

    // 设置固定高度
    button->setMinimumHeight(45);
    button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    // 设置样式
    button->setStyleSheet(
        "QPushButton {"
        "    background-color: #454545;"
        "    color: #ffffff;"
        "    font-size: 25px;"
        "    font-weight: bold;"
        "    text-align: left;"
        "    padding: 12px 15px;"
        "    border: none;"
        "    border-radius: 0px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #505050;"
        "}"
        "QPushButton:checked {"
        "    background-color: #2d2d2d;"
        "    color: #4fc3f7;"
        "    border-left: 4px solid #4fc3f7;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #3a3a3a;"
        "}"
    );

    // 设置工具提示
    button->setToolTip(description);

    // 添加到按钮组
    controlModeGroup->addButton(button, modeId);

    // 连接信号槽
    connect(button, &QPushButton::clicked, [this, button]() {
        onControlModeChanged(controlModeGroup->id(button));
    });

    return button;
}

QFrame* MotionControl::createSeparator()
{
    QFrame *separator = new QFrame();
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);
    separator->setStyleSheet(
        "QFrame {"
        "    background-color: #555555;"
        "    border: none;"
        "    height: 1px;"
        "    margin: 0px;"
        "}"
    );
    return separator;
}

void MotionControl::updateControlParams(int mode)
{
    // 根据模式显示/隐藏对应的参数区域
    switch (mode) {
    case SPEED_MODE:
        speedControlWidget->setVisible(true);
        positionControlWidget->setVisible(false);
        currentControlWidget->setVisible(false);
        motionControlWidget->setVisible(false);
        break;
    case POSITION_MODE:
        speedControlWidget->setVisible(false);
        positionControlWidget->setVisible(true);  // 在位置模式下显示位置控制
        currentControlWidget->setVisible(false);
        motionControlWidget->setVisible(false);
        break;
    case TORQUE_MODE:
        speedControlWidget->setVisible(false);
        positionControlWidget->setVisible(false);
        currentControlWidget->setVisible(true);
        motionControlWidget->setVisible(false);
        break;
    case MOTION_CONTROL_MODE:
        speedControlWidget->setVisible(false);
        positionControlWidget->setVisible(false);
        currentControlWidget->setVisible(false);
        motionControlWidget->setVisible(true);
        break;
    }
}

QPushButton* MotionControl::createControlButton(const QString &text, const QString &color)
{
    QPushButton *button = new QPushButton(text);
    button->setMinimumHeight(40);
    button->setStyleSheet(
        QString(
            "QPushButton {"
            "    background-color: %1;"
            "    color: white;"
            "    border: none;"
            "    border-radius: 4px;"
            "    padding: 10px 8px;"
            "    font-weight: bold;"
            "    font-size: 13px;"
            "    min-height: 40px;"
            "}"
            "QPushButton:hover {"
            "    background-color: %2;"
            "}"
            "QPushButton:pressed {"
            "    background-color: %3;"
            "}"
        ).arg(color)
         .arg(adjustColor(color, 1.2))
         .arg(adjustColor(color, 0.8))
    );
    return button;
}

QString MotionControl::adjustColor(const QString &color, double factor)
{
    if (color == "#f44336") return "#d32f2f";
    if (color == "#4caf50") return "#388e3c";
    if (color == "#2196f3") return "#1976d2";
    if (color == "#ff9800") return "#f57c00";
    if (color == "#9c27b0") return "#7b1fa2";
    if (color == "#795548") return "#5d4037";
    return "#607d8b";
}

void MotionControl::onControlModeChanged(int mode)
{
    // 根据选择的控制模式更新界面
    updateControlParams(mode);

    switch (mode) {
    case SPEED_MODE:
        qDebug() << "切换到速度模式";
        break;
    case POSITION_MODE:
        qDebug() << "切换到位置模式(PID+位置控制)";
        break;
    case TORQUE_MODE:
        qDebug() << "切换到扭矩模式（电流控制）";
        break;
    case MOTION_CONTROL_MODE:
        qDebug() << "切换到运控模式（PID+位置+转速+电流控制）";
        break;
    }
}
