import QtQuick 2.0
import QtQuick.Controls 2.5
ProgressBar{
    id: control
    from : 0
    to : 100
//    property var value: 10
    padding: 2
    background: Rectangle {
        anchors.fill: parent
        color: "#e6e6e6"
        radius: 3
    }

    contentItem: Item{
         anchors.fill: parent
         Rectangle {
            width: control.visualPosition * parent.width
            height: parent.height
            radius: 2
            color: "#17a81a"
        }

        Text {
            id: text1
            color: "blue"

            text: control.value + "%" //Math.floor(control.value * 100) + "%"
            z: 2
            anchors.centerIn: parent
        }
    }


//    Rectangle
//    {
//        id:progressBar
//        anchors.fill: parent
//        color: Constants.statusBarColor
//        Rectangle
//        {
//            id: idProgress
//            height:parent.height
//            width: value / 100 * parent.width
//            color:"#FFCC00"//"#00cae2"
//            anchors.left: parent.left
//        }

//    }
//    Timer {
//        interval: 250
//        repeat: true
//        running: true
//        onTriggered: {
//            if (control.value < 99)
//            {
//                control.value += 1
//            }
//            else {
//                text1.text = 100 + "%"
//                stop();
//            }
//       }
//  }
}
