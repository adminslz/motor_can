QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport

CONFIG += c++11 moc
#TARGET = CANDemo
# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    can_init.cpp \
    can_rx_tx.cpp \
    can_types.cpp \
    canthread.cpp \
    can_communication_thread.cpp \
    control_param.cpp \
    data_acquisition.cpp \
    global_vars.cpp \
    main.cpp \
    mainwindow.cpp \
    motion_contr_current.cpp \
    motion_contr_mentionctr.cpp \
    motion_contr_position.cpp \
    motion_contr_speed.cpp \
    motion_control.cpp \
    motor_debug.cpp \
    motor_param.cpp \
    motor_status.cpp \
    param_dictionary.cpp

HEADERS += \
    ControlCAN.h \
    can_init.h \
    can_rx_tx.h \
    can_types.h \
    canthread.h \
    can_communication_thread.h \
    control_param.h \
    data_acquisition.h \
    global_vars.h \
    mainwindow.h \
    motion_contr_current.h \
    motion_contr_mentionctr.h \
    motion_contr_speed.h \
    motion_control.h \
    motor_contr_position.h \
    motor_debug.h \
    motor_param.h \
    motor_status.h \
    param_dictionary.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/./ -lControlCAN
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/./ -lControlCAN

INCLUDEPATH += $$PWD/.
DEPENDPATH += $$PWD/.

win32: LIBS += -lControlCAN

RESOURCES += \
    pic.qrc
