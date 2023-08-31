import QtQuick 2.9
import QtQuick.Controls 2.2
//import TaoQuick 1.0
Item {
    id: homeItem
    anchors.centerIn: parent
//    Label {
//        text: qsTr("TaoQuick provides a set of controls that can be used to build complete interfaces in Qt Quick.")
//        horizontalAlignment: Label.AlignHCenter
//        verticalAlignment: Label.AlignVCenter
//        wrapMode: Label.Wrap
//        anchors.centerIn: parent
//    }
    property ListModel wizardModel: ListModel {
        //ListElement {
        //    name: "TitleBar"
        //    descript: "my company logo"//"drag change window pos, double click change window state"
        //    targetObjectName: "blankItem"
        //    arrowType: Qt.UpArrow
        //}
        ListElement {
            name: "Menu Bar"
            descript: "1. You can click open file, open project, close, save as, etc. 
2. The toolbar can set language, preferences, etc. 
3. Click 'help' for more information."
            targetObjectName: "mybasicmenubar"
			descriptWidth:590 
			descriptHeight: 190
			descriptOffset:-40
            arrowType: Qt.UpArrow
        }

        ListElement {
            name: "Control Buttons"
            descript: "1. Window maximizing.
2. Window minimizing.
3. Close window"
            targetObjectName: "controlButtonsRow"
			descriptWidth:270 
			descriptHeight: 190
			descriptOffset:-100
            arrowType: Qt.UpArrow
        }
		
		ListElement {
            name: "Machine Selection"
            descript: "1. Add Printer: select printer type and add.
2. Manage Printers: manage the added printers, such as deleting and modifying parameters."
            targetObjectName: "selectMachine"
			descriptWidth:425 
			descriptHeight: 190
			descriptOffset:-100
            arrowType: Qt.UpArrow
        }
		
        //ListElement {
        //    name: "Skin Button"
        //    descript: "switch theme"
        //    targetObjectName: "skinBtn"
        //    arrowType: Qt.UpArrow
        //}


        //ListElement {
        //    name: "Operate Button"
        //    descript: "Some Model Operate Button"
        //    targetObjectName: "leftToolObj"
        //    arrowType: Qt.LeftArrow
        //}
        ListElement {
            name: "Right Pane"
            descript: "1. Model List: displays the list of model files.
2. Parameter Configuration: set slice related parameters.
3. Start Slice: slicing the model."
            targetObjectName: "RightInfoPanel"
			descriptWidth:420
			descriptHeight: 200
			descriptOffset:-40
            arrowType: Qt.RightArrow
        }
//        ListElement {
//            name: "Drawer Button"
//            descript: "show or hide left pane"
//            targetObjectName: "menuBtn"
//            arrowType: Qt.LeftArrow
//        }
//        ListElement {
//            name: "Search Input"
//            descript: "search content"
//            targetObjectName: "searchInput"
//            arrowType: Qt.LeftArrow
//        }
//        ListElement {
//            name: "Home Button"
//            descript: "go back home page"
//            targetObjectName: "homeBtn"
//            arrowType: Qt.LeftArrow
//        }
//        ListElement {
//            name: "Content List"
//            descript: "switch content"
//            targetObjectName: "contentListView"
//            arrowType: Qt.LeftArrow
//        }
//        ListElement {
//            name: "Content Pane"
//            descript: "show current selected content by list"
//            targetObjectName: "contentRect"
//            arrowType: Qt.RightArrow
//        }
        ListElement {
            name: "Wizard Buttons"
            descript: "Explain the general function of the software"
            targetObjectName: "wizardBtn"
			descriptWidth:270 
			descriptHeight: 190
			descriptOffset:-40
            arrowType: Qt.UpArrow
        }

    }
}
