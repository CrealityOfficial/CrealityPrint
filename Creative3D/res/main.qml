import QtQuick 2.15

import CrealityUI 1.0
import "qrc:/CrealityUI"

Item {
    // initialize Constants before load ui components
    readonly property var _ : Constants

    Loader {
        id: splash_loader
        asynchronous: true
        source: "qrc:/CrealityUI/qml/Splash.qml"
        onLoaded: {
            item.show()
        }
    }

    Loader {
        id: main_window_loader

        anchors.fill: parent
        anchors.margins: frameLessView && frameLessView.isMax ? 7 * screenScaleFactor : 0

        asynchronous: true
        sourceComponent: MainWindow2 {}

        onLoaded: {
            splash_loader.sourceComponent = null
            item.initialized = true
            console.log("main loaded")
            kernel_kernel.processCommandLine()
            if(!kernel_kernel.blTestEnabled())
                autoSaveProject.startAutoSave()
        }
    }
}
