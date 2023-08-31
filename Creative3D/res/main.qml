import QtQuick 2.0
import CrealityUI 1.0
import "qrc:/CrealityUI"
QtObject {
      id: root
      property var crealityVersion: ""
      property QtObject $splashScreen:Splash {
          }
      property var loader: Loader{
          id:idMainWindow
          asynchronous: true
          source: "qrc:/scence3d/res/MainWindow.qml"
          active: false
          onLoaded: {
              $splashScreen.visible = false
              idMainWindow.item.showWizardDlg()
          }
      }
      Component.onCompleted:{
          loader.active = true;
      }
  }
