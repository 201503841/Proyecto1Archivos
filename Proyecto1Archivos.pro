QT -= gui

CONFIG += c++11 console
CONFIG -= app_bundle

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        bitmap.cpp \
        ebr.cpp \
        grupo.cpp \
        inodo.cpp \
        journal.cpp \
        main.cpp \
        mbr.cpp \
        mount.cpp \
        parti.cpp \
        superbloque.cpp \
        usuario.cpp

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    bitmap.h \
    ebr.h \
    grupo.h \
    inodo.h \
    journal.h \
    mbr.h \
    mount.h \
    parti.h \
    superbloque.h \
    usuario.h
