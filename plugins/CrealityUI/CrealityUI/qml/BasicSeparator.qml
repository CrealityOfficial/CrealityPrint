import QtQuick 2.0

Item {
    id : control
    property var firstHieght: 2
    property var secondHeight: 1
    property var firstWidtth : parent.width
    property var secondWidth: parent.width
    Rectangle {
        width:control.width > control.height ?  control.width : 2
        height: control.width > control.height ?  2 : control.height
        color: Constants.splitLineColor
        Rectangle {
        width: control.width > control.height ? control.width : 1
        height: control.width > control.height ? 1 : control.height
        color: Constants.splitLineColor
        }
    }
}
