import QtQuick 2.0
import QtQuick.Controls 1.4
import QtQuick.Controls 2.13 as QCC213
import QtQml.Models 2.13
import "../qml"
Rectangle{
    id: root
    width: 360
    height: 300
    property alias tableView: tableView
    property alias myModel : tableView.model
    property var myModel
    property var cellSelectedColor: "#7C7C7C"//"#C6F8FF"
    property var cellCurrentRowColor:"#424242" //"#F2F2F2"
    property var cellBackgroundColor: "#535353"//"white"


    property string checkedSrc: "qrc:/UI/photo/radio_select.png"
    property string uncheckSrc: "qrc:/UI/photo/radio_unselect.png"

    color: Constants.dialogContentBgColor
    border.width: 0
    border.color: "#535353"
    Component {
        id: itemDelegate
        //Loader 动态加载不同的组件
        Loader {
            id: itemLoader
            anchors.fill: parent
//            anchors.topMargin: 1
//            anchors.bottomMargin: 1
            visible: status === Loader.Ready
            //根据role加载相应的组件
            sourceComponent: {
                var role = styleData.role;
                if(role === "select")
                    return emptyComponent
                if (role === "order")
                    return orderComponent;
                if (role === "name")
                    return nameComponent;
                if(role === "status")
                    return statusComponent
                else
                    return manufactureComponent
            }

           // Note: 各种component需要写在loader内部。因为要访问styleData，在外部会
            //提示找不到styleData
            Component {
                id: emptyComponent
                QCC213.Button
                {
                    id : delegateBtn
                    anchors.fill: parent
                    property bool isSelected:styleData.value
//                    property bool tmpSelect : isSelected
                    Image {
                        id : btn
                        width : sourceSize.width
                        height : sourceSize.height
                        source: isSelected ?  checkedSrc: uncheckSrc
                        anchors.centerIn: parent
                    }

                    background :Rectangle
                    {
                        anchors.fill: delegateBtn
                        color: "transparent"//isSelected? "red" :"lightBlue"
                    }
                    onClicked:
                    {
//                        isSelected =!isSelected
                        tableView.model.setCheckStateByRow(styleData.row, "");
                    }
                }
            }
            Component {
                id: orderComponent
                Item{}
            }

            Component {
                id: nameComponent
                Rectangle {
                    anchors.fill: parent
                    border.width: 1
                    border.color: cellBackgroundColor
                    property bool isSelected: tableView.model.getCheckStateByRow(styleData.row)
                    color: isSelected ? cellSelectedColor :
                                        ((tableView.currentRow === styleData.row) ?
                                             cellCurrentRowColor : cellBackgroundColor)
                    StyledLabel {
                        id: orderText
                        anchors.fill: parent
                        //                        text: styleData.value ? String(styleData.value) : ""
                        text:idTextedit.text // styleData.value
                        color: parent.isSelected ? "#00BCDE" : "white"
                        x:4
                        horizontalAlignment: Text.AlignLeft
                        verticalAlignment: Text.AlignVCenter
                        visible: !idTextedit.visible
                        leftPadding: 4
                        MouseArea {
                            anchors.fill: parent
                            onDoubleClicked:
                            {
                                idTextedit.visible = true
                                idTextedit.focus = true
                            }
                        }
                    }

                    TextField
                    {
                        id : idTextedit
                        anchors.fill: parent
                        visible: false
                        text: styleData.value
                        horizontalAlignment: Text.AlignLeft
                        verticalAlignment: Text.AlignVCenter
                        x:4
                        onEditingFinished:
                        {
                            idTextedit.visible = false
                            idTextedit.focus = false
                            if (styleData.row >= 0 && styleData.value !== text) {
                                console.log("nameTextInput....")
                                tableView.model.setNameByRow(styleData.row, text);
                            }
                        }
                        onHoveredChanged:
                        {
                            if(!hovered)
                            {
                                idTextedit.visible = false
                            }
                        }
                    }

                }
            }

            Component {
                id: manufactureComponent

                Rectangle {
                    id : idRect
                    anchors.fill: parent
                    border.width: 1
                    border.color: cellBackgroundColor
                    color: isSelected ? cellSelectedColor :
                                        ((tableView.currentRow === styleData.row) ?
                                            cellCurrentRowColor : cellBackgroundColor)// "green" : "blue")

                    property bool isSelected:tableView.model.getCheckStateByRow(styleData.row) /*tableView.currentColumn === styleData.column &&
                                              tableView.currentRow === styleData.row*/
                    StyledLabel {
                        id: orderText
                        anchors.fill: parent
                        text: styleData.value
                        color: parent.isSelected ? "#00BCDE" : "white"

                        horizontalAlignment: Text.AlignLeft
                        leftPadding: 4
                        verticalAlignment: Text.AlignVCenter

                    }
                    MouseArea {
                        anchors.fill: parent
                        onPressed:
                        {
                            tableView.currentRow = styleData.row
                        }
                    }
                }
            }

            Component {
                id: statusComponent

                Rectangle {
                    id : idRect
                    anchors.fill: parent
                    border.width: 1
                    border.color: cellBackgroundColor
                    color: isSelected ? cellSelectedColor :
                                        ((tableView.currentRow === styleData.row) ?
                                            cellCurrentRowColor : cellBackgroundColor)// "green" : "blue")

                    property bool isSelected:tableView.model.getCheckStateByRow(styleData.row) /*tableView.currentColumn === styleData.column &&
                                            tableView.currentRow === styleData.row*/
                    CusProgressBar
                    {
                        id : idProgress
                        x:10
                        y:5
                        width: parent.width - 20
                        height: parent.height - 10
                        value : styleData.value
                        visible: value === 0 || value ===100 ? false : true
                    }
                    StyledLabel {
                        id: orderText
                        anchors.fill: parent
                        text: styleData.value === -1? " " + qsTr("Busy"):idProgress.value === 0 ?  " " + qsTr("Ready") : idProgress.value === 100 ? " " + qsTr("Finished")  : idProgress.value  //styleData.value
                        color: parent.isSelected ? "#00BCDE" : "white"

                        horizontalAlignment: Text.AlignLeft
                        leftPadding: 4
                        verticalAlignment: Text.AlignVCenter
                        visible: !idProgress.visible

                    }
                    MouseArea {
                        anchors.fill: parent
                        onPressed:
                        {
                            tableView.currentRow = styleData.row
                        }
                    }
                }
            }

        }
    }

    TableView{  //定义table的显示，包括定制外观
        id: tableView
        anchors.fill: parent
        focus: true
        frameVisible: false
        alternatingRowColors: true
        backgroundVisible : false
        TableViewColumn{

            role: "select";
            title: "";
            width: 20;
        }
        TableViewColumn{
            role: "name";
            title: qsTr("Name");
            width: 90;
            elideMode: Text.ElideRight;
        }
        TableViewColumn{
            role: "ipaddr";
            title: qsTr("Ip Address");
            width: 125;
            elideMode: Text.ElideRight;


        }
        TableViewColumn{
            role: "macaddr";
            title: qsTr("Mac Address");
            width: 125;
            elideMode: Text.ElideRight;
        }

        TableViewColumn{
            role: "status";
            title: qsTr("Task Status");
            width: 110;
            elideMode: Text.ElideRight;

        }
        rowDelegate: Rectangle{
            height: 30
            color: root.color
//            color: styleData.selected? root.highlight :"#C6F8FF" ? "#F2F2F2" : "white"/*root.background*/
        }
        itemDelegate: itemDelegate
        headerDelegate: Rectangle{
            implicitWidth: 10
            implicitHeight: 24
            border.color: Constants.dialogContentBgColor//"#FFFFFF"
            border.width: 1
            color: Constants.dialogContentBgColor//"white"
            Text{
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.leftMargin: 4
                anchors.right: parent.right
                anchors.rightMargin: 4
                text: styleData.value
                color: Constants.textColor//styleData.pressed ?"white" : "white"//"#333333"
                font.bold: true
            }
        }
    }

}
