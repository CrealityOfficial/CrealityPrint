import QtQuick 2.0
import QtQuick 2.10
import QtQuick.Controls 2.0
import Qt.labs.platform 1.1
import QtQuick.Shapes 1.12
import Qt.labs.calendar 1.0
import Qt3D.Core 2.0
import Qt3D.Render 2.0
import Qt3D.Input 2.0
import Qt3D.Extras 2.0
import CrealityUI 1.0
import QtQuick.Shapes 1.13
import QtQml 2.13
import QtMultimedia 5.12
import QtCharts 2.0
import "qrc:/CrealityUI"
Item {
    property var com

    function execute()
    {

    }
    ClonePanel {

        control:com

    }

    MessageDialog {
          id:messagebox
          buttons: MessageDialog.Ok
          text: qsTr("The document has been modified.")
      }
}
