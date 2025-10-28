#ifndef PARAM_DICTIONARY_H
#define PARAM_DICTIONARY_H

#include <QObject>
#include <QVector>
#include <QVariant>

// 对应您的OD_type_t
enum ODType {
    OD_TYPE_NULL = 0x00,
    OD_TYPE_BOOLEAN = 0x01,
    OD_TYPE_INT8 = 0x02,
    OD_TYPE_UINT8 = 0x03,
    OD_TYPE_INT16 = 0x04,
    OD_TYPE_UINT16 = 0x05,
    OD_TYPE_INT32 = 0x06,
    OD_TYPE_UINT32 = 0x07,
    OD_TYPE_FLOAT = 0x08
};

// 对应您的OD_entry_t
struct ODEntry {
    uint16_t index;
    uint8_t subindex;
    ODType type;
    QString name;
    QString unit;
    QVariant minVal;
    QVariant maxVal;
    QVariant defaultValue;
    bool readable;
    bool writable;
    int tabCategory; // 0:基本控制, 1:PID参数, 2:高级设置, 3:监控参数
    QString groupName; // 分组名称
};

class ParamDictionary : public QObject
{
    Q_OBJECT

public:
    explicit ParamDictionary(QObject *parent = nullptr);

    QVector<ODEntry> getParametersByCategory(int category) const;
    QVector<ODEntry> getAllParameters() const;
    ODEntry getParameter(uint16_t index, uint8_t subindex = 0) const;

private:
    void initializeDictionary();
    QVector<ODEntry> m_dictionary;
};

#endif // PARAM_DICTIONARY_H
