import QtQuick.Controls 2.12
import QtQuick 2.0
import QtQuick.Controls 1.4
import QtQml.Models 2.13

import QtQuick.Controls.Styles 1.4
import ".."
import "../qml"
BasicGroupBox {
    id: control
    implicitWidth: 260
    implicitHeight:280
    property alias model : idBasicTreeView.model
    signal addModel()
    signal delModels(var indexs)
    signal selectModels(var indexs)
    signal centerAndLayout()
	signal labelShow(var bLabel)
    signal uploadModel()
    signal uploadModelLocalFile()
    visible: width > 10 ? true:false
    property var listRowCount: 0
    property var bLabelShow : true
    function rowTest()
    {
        var rows = idBasicTreeView.model.rowCount()
        if(rows !== 0) 
            bLabelShow =false
        else 
            bLabelShow = true
        
        labelShow(bLabelShow)//pass to sliceBtn
    }
    function updateCheckboxStatus()
    {
        listRowCount = idBasicTreeView.model.rowCount()
        var selectRow = idBasicTreeView.selection.selectedIndexes
        var nCount = 0;
        for(var xxx in selectRow)
        {
            nCount ++ ;
        }
        idCheckBox.lockCheckChanged = true
        if(nCount === listRowCount)
        {
            idCheckBox.checked =true
        }
        else
        {
            idCheckBox.checked = false
        }
        idCheckBox.lockCheckChanged = false

    }

    MouseArea//by TCJ
    {
        anchors.fill: parent
        onWheel:
        {
            //console.log("modelList wheel===")
            control.focus = true
        }
    }

    StyleCheckBox
    {
       id :idCheckBox
       property var lockCheckChanged: false
        y: -32
        x: control.width - 66
        width: 42/*40*/
        height: 20
        text: qsTr("All")
        visible: control.visible
        onCheckedChanged:
        {
            if(lockCheckChanged)return ;
            if(checked)
            {
                var rows = idBasicTreeView.model.rowCount()
                idBasicTreeView.selection.clearSelection()
                for(var row = 0;row < rows;row++)//select all model
                {
                    idBasicTreeView.selection.setCurrentIndex(idBasicTreeView.model.index(row,1),ItemSelectionModel.Select)
                }
            }
            else
            {
                idBasicTreeView.selection.clearSelection()
            }
        }
    }
    Grid
    {
        id: grid_wrapper
        rows: 2
        width: parent.width
        height: parent.height
        spacing: 10
        ModeListTreeView
        {
            id: idBasicTreeView
            width: parent.width
            height: parent.height -68
            verticalScrollBarPolicy: Qt.ScrollBarAlwaysOff
            vpolicy:listRowCount < 6 ? ScrollBar.AlwaysOff : ScrollBar.AlwaysOn
            Rectangle
            {
               anchors.fill: parent
               border.width: 1
               border.color: Constants.rectBorderColor //"#333333"
               color: "transparent"
                StyledLabel
                {
                    x: 5
                    y: 5
                    id : idInfoShow
                    text : qsTr("You have not imported the model,please click Import")
                    width : parent.width-20//contentWidth
                    height :parent.height-20//contentHeight
                    horizontalAlignment:Text.AlignHCenter
                    verticalAlignment:Qt.AlignVCenter//Text.AlignVCenter
                    wrapMode: Text.WordWrap//换行
                    visible : bLabelShow//idBasicTreeView.model.rowCount() > 0 ? false : true 
                }
            }

            style:TreeViewStyle
            {
                backgroundColor:  Constants.treeBackgroundColor//"#424242"
                // itemDelegate:Item{
                // // color: "transparent" //背景设置透明，不然在选中行的时候会出现选中颜色就一半的情况
                //     id:idTreeDelegat
                //     Text {
                //             color: Constants.textColor
                //             elide: styleData.elideMode
                //             text: styleData.value
                //         }
                // }
                rowDelegate:Rectangle{
                    id: rowDel
                    color: styleData.selected ? Constants.treeItemColor_pressed :Constants.treeItemColor
                    height: 20
                }
            }
            headerVisible: false

            TableViewColumn {
                   title: "Order"
                   role: "title"
                   width: 160
            }

            itemDelegate: Item {
                       height: 20
                       Text {
                           height: contentHeight//parent.height
                           anchors.verticalCenter: parent.verticalCenter
                           text: /*(styleData.value.indentation + 1)+ "、" +*/ styleData.value.text
                           font.pointSize: Constants.labelFontPointSize_10
                           color: Constants.textColor
                           font.family: Constants.labelFontFamily
                       }
            }

            selection: ItemSelectionModel {
				objectName:"selectModelSelection"
                model: idBasicTreeView.model
                onSelectionChanged:{
						selectModels(idBasicTreeView.selection.selectedIndexes)
						updateCheckboxStatus()
                }
            }
//            ScrollBar.vertical: ScrollBar { }
            selectionMode:SelectionMode.ExtendedSelection
            //myModel: objModel2
        }


        Grid
        {
            id: grid_wrapper2
            width: parent.width
            height: 64
            rows: 2
            spacing: 10
            Row{
                spacing: 10
                BasicButton/*BasicComButton*/
                {
                    id: idImportBtn
                    width: /*idBasicTreeView.width*/(idBasicTreeView.width - 10 ) /2
                    height:28
                    text: qsTr("Import Model")
                    defaultBtnBgColor : bLabelShow ? "#1E9BE2" : Constants.buttonColor
                    hoveredBtnBgColor: bLabelShow ? "#1EB6E2" : Constants.hoveredColor
                    btnBorderW: bLabelShow ? 0 : 1
                    btnTextColor: bLabelShow? "white": Constants.textColor
                    onSigButtonClicked:
                    {
                        addModel();
                    }
                    btnRadius:3
                    visible: control.width > 10 ?true : false
                    //strTooptip : qsTr(" Import Model ")
                }
                 BasicButton/*BasicComButton*/
                {
                    id: idMiddleSort
                    height : 28
                    width: (idBasicTreeView.width - 10 ) /2/*idBasicTreeView.width*/
                    text: qsTr("Auto arrange")
                    enabled: !bLabelShow
                    btnRadius:3
        //               enabledIconSource: "qrc:/UI/photo/midsortBtn.png"
        //               pressedIconSource: "qrc:/UI/photo/midsortBtn.png"
                    onSigButtonClicked:
                    {
                        centerAndLayout()
                    }
                    visible: control.width > 10 ?true : false
                    //strTooptip : qsTr(" Middle And Sort ")
            //           anchors.left: control.left
                }
            }
            
           Row
           {
               spacing: 10
               BasicDialogButton{
                   id: idUploadBtn
                   width: 70
                   height: 28
                   btnRadius: 3
                   btnBorderW: 1
                   borderColor: Constants.rectBorderColor//"#2B2B2B"
                   defaultBtnBgColor: Constants.buttonColor//"#454545"
                   hoveredBtnBgColor: Constants.dialogBtnHoveredColor//"#5D5D5D"
                   pressedIconSource: btnHovred ? "qrc:/UI/photo/upload_model_img_h.png" : Constants.uploadModelImg
                   strTooptip: qsTr("Upload platform model to CrealityCloud")
                    enabled: {
                        var selectRows = idBasicTreeView.selection.selectedIndexes
                        var nCount = 0;
                        for(var xxx in selectRows)
                        {
                            nCount ++ ;
                        }
                        if(nCount > 0)                       
                        {
                            return true
                        }else{
                            return false
                        }
                    }
                    onSigButtonClicked:
                    {
                        uploadModel()
                    }
                   visible: control.width > 10 ?true : false
               }

                BasicDialogButton{
                   id: idUploadModelFileBtn
                   width: 70
                   height: 28
                   btnRadius: 3
                   btnBorderW: 1
                   borderColor: Constants.rectBorderColor//"#2B2B2B"
                   defaultBtnBgColor: Constants.buttonColor//"#454545"
                   hoveredBtnBgColor: Constants.dialogBtnHoveredColor//"#5D5D5D"
                   pressedIconSource: btnHovred ? "qrc:/UI/photo/upload_model_local_img_h.png" : Constants.uploadLocalModelImg
                    strTooptip: qsTr("Upload local model to CrealityCloud")
                    onSigButtonClicked:
                    {
                        uploadModelLocalFile()
                    }
                   visible: control.width > 10 ?true : false
               }

                BasicDialogButton{
                   id: idDeleteBtn
                   width: 70
                   height: 28
                   btnRadius: 3
                   btnBorderW: 1
                   borderColor: Constants.rectBorderColor//"#2B2B2B"
                   defaultBtnBgColor: Constants.buttonColor//"#454545"
                   hoveredBtnBgColor: Constants.dialogBtnHoveredColor//"#5D5D5D"
                   pressedIconSource: btnHovred ? "qrc:/UI/photo/delete_model_img_h.png" : Constants.deleteModelImg
                   strTooptip: qsTr("Delete Model")
                    enabled: {
                        var selectRows = idBasicTreeView.selection.selectedIndexes
                        var nCount = 0;
                        for(var xxx in selectRows)
                        {
                            nCount ++ ;
                        }
                        if(nCount > 0)                       
                        {
                            return true
                        }else{
                            return false
                        }
                    }
                    onSigButtonClicked:
                    {
                        var selectRows = idBasicTreeView.selection.selectedIndexes
                        var nCount = 0;
                        for(var xxx in selectRows)
                        {
                            nCount ++ ;
                        }
                        if(nCount > 0)                       
                        {
                            delModels(selectRows)
                        }
                    }
                   visible: control.width > 10 ?true : false
               }

             }          
        }
    }
    Connections{
            target: idBasicTreeView.model
            onAddNewItem:function(index){
                idBasicTreeView.expand(index)
            }
            onRowChanged://add or delete
            {               
                rowTest()
            }
    }

}
