#include "can_types.h"
#include <QMetaType>
#include <QVector>
#include <QPair>
#include <QList>
#include <QDebug>

void registerCanTypes()
{
    static bool registered = false;
    if (registered) return;

    qDebug() << "Registering CAN types...";

    // 注册基本类型
    if (QMetaType::type("VCI_CAN_OBJ") == QMetaType::UnknownType) {
        qRegisterMetaType<VCI_CAN_OBJ>("VCI_CAN_OBJ");
        qDebug() << "Registered VCI_CAN_OBJ";
    }

    // 注册模板类型
    if (QMetaType::type("QList<VCI_CAN_OBJ>") == QMetaType::UnknownType) {
        qRegisterMetaType<QList<VCI_CAN_OBJ>>("QList<VCI_CAN_OBJ>");
        qDebug() << "Registered QList<VCI_CAN_OBJ>";
    }

    if (QMetaType::type("QVector<float>") == QMetaType::UnknownType) {
        qRegisterMetaType<QVector<float>>("QVector<float>");
        qDebug() << "Registered QVector<float>";
    }

    if (QMetaType::type("QPair<unsigned int,QVector<float>>") == QMetaType::UnknownType) {
        qRegisterMetaType<QPair<unsigned int, QVector<float>>>("QPair<unsigned int,QVector<float>>");
        qDebug() << "Registered QPair<unsigned int,QVector<float>>";
    }

    if (QMetaType::type("QVector<QPair<unsigned int,QVector<float>>>") == QMetaType::UnknownType) {
        qRegisterMetaType<QVector<QPair<unsigned int, QVector<float>>>>("QVector<QPair<unsigned int,QVector<float>>>");
        qDebug() << "Registered QVector<QPair<unsigned int,QVector<float>>>";
    }

    registered = true;
    qDebug() << "All CAN types registered successfully";
}
