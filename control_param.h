#ifndef CONTROL_PARAM_H
#define CONTROL_PARAM_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QScrollArea>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QCheckBox>
#include <QComboBox>
#include <QTabWidget>
#include <QSlider>
#include <QProgressBar>
#include <QTimer>
#include <QMap>
#include <QVector>
#include "param_dictionary.h"

class ControlParam : public QWidget
{
    Q_OBJECT

public:
    explicit ControlParam(QWidget *parent = nullptr);

    void setCanId(uint8_t canId);
    void updateParameterValue(uint16_t index, uint8_t subindex, const QVariant& value);

signals:
    void sdoWriteRequest(uint8_t nodeId, uint16_t index, uint8_t subindex, const QByteArray& data);
    void sdoReadRequest(uint8_t nodeId, uint16_t index, uint8_t subindex);

private slots:
    void onApplyParamsClicked();
    void onReadAllParamsClicked();
    void onResetParamsClicked();
    void onSaveParamsClicked();
    void onLoadParamsClicked();
    void onParameterValueChanged();
    void onMotorEnableToggled(bool enabled);
    void onControlModeChanged(int index);
    void onAutoApplyToggled(bool enabled);

private:
    void setupUI();
    void setupConnections();
    void createParameterControls();
    QWidget* createParameterWidget(const ODEntry& param);
    QWidget* createControlForParameter(const ODEntry& param);
    void sendParameterValue(const ODEntry& param, const QVariant& value);
    QString getUnitString(const ODEntry& param);
    // 辅助函数
    QString getTypeString(ODType type);
    QString getRangeString(const ODEntry& param);
    QString getDefaultValueString(const ODEntry& param);

    ParamDictionary m_paramDict;
    QTabWidget *m_tabWidget;
    QMap<QString, QWidget*> m_controlMap;

    QCheckBox *m_motorEnableCheck;
    QComboBox *m_controlModeCombo;
    QSpinBox *m_canIdSpin;
    QCheckBox *m_autoApplyCheck;

    uint8_t m_currentCanId;
    bool m_autoApply;

    // 在 ControlParam 类的 protected 部分添加
    protected:
        bool eventFilter(QObject *obj, QEvent *event) override;
};

#endif // CONTROL_PARAM_H
