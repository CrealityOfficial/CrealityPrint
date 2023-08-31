TEMPLATE = lib
TARGET = CrealityUIComponent
QT += qml quick
CONFIG += plugin c++11

TARGET = $$qtLibraryTarget($$TARGET)        #TARGET变量指定生成的目标库文件的名字，生成应用程序时即指定生成应用程序名
uri = CrealityUIComponent


CONFIG (release, debug|release){
DESTDIR =  $$OUT_PWD/release/CrealityUIComponent

copy_qmldir.target = $$OUT_PWD/../CrealityUITest/release/plugin/CrealityUIComponent/qmldir
}

CONFIG (debug, debug|release){
 DESTDIR =   $$OUT_PWD/debug/CrealityUIComponent

 copy_qmldir.target = $$OUT_PWD/../CrealityUITest/debug/plugin/CrealityUIComponent/qmldir
}

!equals(_PRO_FILE_PWD_, $$OUT_PWD) {
    copy_qmldir.depends = $$_PRO_FILE_PWD_/qmldir
    copy_qmldir.commands = $(COPY_FILE) "$$replace(copy_qmldir.depends, /, $$QMAKE_DIR_SEP)" "$$replace(copy_qmldir.target, /, $$QMAKE_DIR_SEP)"

    QMAKE_EXTRA_TARGETS += copy_qmldir
    PRE_TARGETDEPS += $$copy_qmldir.target
}

# Input
SOURCES +=         src/crealityuicomponent_plugin.cpp         src/crealityuicomponent.cpp

HEADERS +=         src/crealityuicomponent_plugin.h         src/crealityuicomponent.h

#qmldir.files = qmldir
unix {
    installPath = $$[QT_INSTALL_QML]/$$replace(uri, \., /)
    qmldir.path = $$installPath
    target.path = $$installPath
    INSTALLS += target qmldir
}

qmlsource.files += qml/*.qml
qmlsource.path =$$[QT_INSTALL_QML]/$${uri}/qml
installPath = $$[QT_INSTALL_QML]/$${uri}

win32 {
    installPath ~= s,/,\\,g
}
#qmldir.files = qmldir
qmldir.path = $$installPath
!android:INSTALLS += qmldir

!android:target.path = $$installPath
#INSTALLS += target qmlsource

RESOURCES += \
    qml.qrc

DISTFILES += \
    qmldir



