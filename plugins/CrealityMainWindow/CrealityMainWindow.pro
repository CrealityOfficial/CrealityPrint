QT += quick virtualkeyboard

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        main.cpp

RESOURCES += qml.qrc

#copy qmldir
CONFIG (release, debug|release){
copy_qmldir.target = $$OUT_PWD/release/plugin/CrealityUIComponent/qmldir
}

CONFIG (debug, debug|release){
copy_qmldir.target = $$OUT_PWD/debug/plugin/CrealityUIComponent/qmldir
}

!equals(_PRO_FILE_PWD_, $$OUT_PWD) {
    copy_qmldir.depends = $$_PRO_FILE_PWD_/../CrealityUIComponent/qmldir
    copy_qmldir.commands = $(COPY_FILE) "$$replace(copy_qmldir.depends, /, $$QMAKE_DIR_SEP)" "$$replace(copy_qmldir.target, /, $$QMAKE_DIR_SEP)"

    QMAKE_EXTRA_TARGETS += copy_qmldir
    PRE_TARGETDEPS += $$copy_qmldir.target
}
#copy qmldir

#copy dll
CONFIG (release, debug|release){
 copy_dll.depends = $$OUT_PWD/../CrealityUIComponent/release/CrealityUIComponent/CrealityUIComponent.dll
 copy_dll.target = $$OUT_PWD/release/plugin/CrealityUIComponent/CrealityUIComponentd.dll
}

CONFIG (debug, debug|release){
copy_dll.depends = $$OUT_PWD/../CrealityUIComponent/debug/CrealityUIComponent/CrealityUIComponentd.dll
copy_dll.target = $$OUT_PWD/debug/plugin/CrealityUIComponent/CrealityUIComponentd.dll
}

!equals(_PRO_FILE_PWD_, $$OUT_PWD) {
    copy_dll.commands = $(COPY_FILE) "$$replace(copy_dll.depends, /, $$QMAKE_DIR_SEP)" "$$replace(copy_dll.target, /, $$QMAKE_DIR_SEP)"
    QMAKE_EXTRA_TARGETS += copy_dll
    PRE_TARGETDEPS += $$copy_dll.target
}
#copy dll

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH = $$OUT_PWD/debug/plugin

# Additional import path used to resolve QML modules just for Qt Quick Designer
QML_DESIGNER_IMPORT_PATH =

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
