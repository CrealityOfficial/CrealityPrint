import QtQml 2.13

import QtQuick 2.13
import QtQuick.Window 2.13
import QtQuick.Layouts 1.13

import "qrc:/CrealityUI/"
import "qrc:/CrealityUI/qml/"
import "qrc:/CrealityUI/secondqml/"

QtObject {
  property var loader: Loader{
    id: idMainWindow
    active: false
    asynchronous: true
    source: "qrc:/CrealityUI/secondqml/DumpWindow.qml"
    onLoaded: {
      item.raise()
      item.show()
    }
  }

  Component.onCompleted: {
    Constants.currentTheme = defaultTheme ? defaultTheme : 0
    loader.active = true
  }
}
