import QtQuick 2.15

import CrealityUI 1.0
import "qrc:/CrealityUI"

Item {
    // initialize Constants before load ui components
    readonly property var _ : Constants

    Connections {
        target: Constants.directoryFontLoader
        onFontLoaded: {
            splash_loader.active = true
        }
    }

    Loader {
        id: splash_loader
        active: Qt.platform.os === "osx" ? true: false
        source: "qrc:/CrealityUI/qml/Splash.qml"
        onLoaded: {
            item.show()
            main_window_loader.active = true
        }
    }

    Loader {
        id: main_window_loader
        active: false
        anchors.fill: parent
        anchors.margins: frameLessView && frameLessView.isMax &&!frameLessView.isCompatible? 7 * screenScaleFactor : 0

        asynchronous: true
        sourceComponent: MainWindow2 {}

        onLoaded: {
            splash_loader.sourceComponent = null
            item.initialized = true
            console.log("main loaded")
            kernel_kernel.processCommandLine()
        }
    }
}
