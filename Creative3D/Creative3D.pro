QT += qml quick widgets 3dcore 3drender 3dextras 3dinput 3dlogic 3dquick 3drender-private concurrent

#CONFIG += c++11 qtquickcompiler
CONFIG += c++11
SOURCES += src/main.cpp


#RESOURCES += qml.qrc
INCLUDEPATH += src ../Scence3D/src ../plugins/include
# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Additional import path used to resolve QML modules just for Qt Quick Designer
QML_DESIGNER_IMPORT_PATH =

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += \
    QT_DEPRECATED_WARNINGS \
    QT_MESSAGELOGCONTEXT
# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

# Default rules for deployment.


win32:CONFIG (release, debug|release){
LIBS += -L$$OUT_PWD/../../lib/release -lscence3d
DESTDIR = ./../../bin/Release/
}
win32:CONFIG (debug, debug|release){
 LIBS += -L$$OUT_PWD/../../lib/debug -lscence3d
 DESTDIR = ./../../bin/Debug/
}



HEADERS +=

RESOURCES += \
    ../plugins/CrealityUI/qml.qrc \
    scence3d.qrc \
    translations.qrc

