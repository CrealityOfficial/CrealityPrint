import QtQuick 2.0
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.3
import QtQml 2.13

import ".."
import "../qml"
BasicGroupBox{
    property var smodel

    ParameterContext {
        id : idParameterContext
    }
    function update(model,settings)
    {

        idParameterContext.settings = settings
        idParameterContext.currentProfile = model
        smodel = model

    }
    defaultBgColor: "transparent"
    textBgColor: Constants.custom_tabview_panel_color
    width: 800* screenScaleFactor
    implicitHeight:isExpand ? contentItem.implicitHeight + 30* screenScaleFactor : 40* screenScaleFactor
    state:"canExpend"
    backRadius: 5
    borderWidth: 1
    borderColor: Constants.dialogItemRectBgBorderColor
    contentItem: ColumnLayout{
        anchors.fill: parent
        anchors.margins: 10 * screenScaleFactor
        spacing: 2 * screenScaleFactor
        Repeater{
            id: retractRep1
            model: smodel
            delegate: gCom.itemRow
        }

        Item{
            Layout.fillHeight: true
        }
    }

    ParameterGeneralCom{
        id:gCom
        parameterContext: idParameterContext
    }
}
