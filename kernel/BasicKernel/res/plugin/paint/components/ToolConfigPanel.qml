import QtQuick 2.0
import QtQuick.Controls 2.5
import CrealityUI 1.0
import "qrc:/CrealityUI"
import "qrc:/CrealityUI/qml"
import "qrc:/CrealityUI/components"

Item {
    id: root
    height: {
        if (!com)
            return 0
        let method = com.colorMethod

        if (method == 0)
            return 0 * screenScaleFactor
        else if (method == 1)
            return 30 * screenScaleFactor
        else if (method == 4)
            return 30 * screenScaleFactor
        else if (method == 2)
            return idFill.forceOpen || !idFill.checkable ? 30 : 65
        else if (method == 5)
            return 30 * screenScaleFactor
        else if (method == 3)
            return 30 * screenScaleFactor
        else 
            return 0 * screenScaleFactor

    }
    property var com
    property var forceOpenSmartFill: true

    function update() {
        idCircle.value = com.colorRadius
        idFill.checkable = com.smartFillEnable
        idFill.value = com.smartFillAngle
        idHeight.value = com.heightRange
        idGapFill.value = com.gapArea
        idSphere.value = com.sphereSize
    }

    Connections
    {
        target: com

        onColorRadiusChanged: {
            if (com.colorRadius != idCircle.value)
                idCircle.value = com.colorRadius
        }

        onSmartFillAngleChanged: {
            if (idFill.value != com.smartFillAngle)
                idFill.value = com.smartFillAngle
        }

        onSmartFillEnableChanged: {
            if (com.smartFillEnable != idFill.checkable)
                idFillcheckable = com.smartFillEnable;
        }

        onHeightRangeChanged: {
            if (com.heightRange != idHeight.value)
                idHeight.value = com.heightRange;
        }

        onGapAreaChanged: {
            if (com.gapArea != idGapFill.value)
                idGapFill.value = com.gapArea;
        }

        onSphereSizeChanged: {
            if (com.sphereSize != idSphere.value)
                idSphere.value = com.sphereSize;
        }
    }

    SliderWithSpinBox {
        id: idCircle
        anchors.fill: parent
        visible: com ? com.colorMethod == 1 : false
        title: qsTr("Pen Size")
        
        from: 0.4
        to: 8
        value: 1
        stepSize: 0.1
    
        onValueChanged: {
            if (com.colorRadius != idCircle.value)
                com.colorRadius = idCircle.value
        }
    }

    SliderWithSpinBox {
        id: idSphere
        anchors.fill: parent
        visible: com ? com.colorMethod == 4 : false
        title: qsTr("Pen Size")
        
        from: 0.1
        to: 8
        value: 1
        stepSize: 0.1
    
        onValueChanged: {
            if (com.sphereSize != idSphere.value)
                com.sphereSize = idSphere.value
        }
    }

    FillSettingsPanel {
        id: idFill
        anchors.fill: parent
        visible: com ? com.colorMethod == 2 : false
        enabled: idEnableSupport.checked
        checkable: true
        forceOpen: forceOpenSmartFill

        onValueChanged: {
            if (idFill.value != com.smartFillAngle)
                com.smartFillAngle = value;
        }

        onCheckableChanged: {
            if (com.smartFillEnable != idFill.checkable)
                com.smartFillEnable = idFill.checkable;
        }
    }

    SliderWithSpinBox {
        id: idHeight
        anchors.fill: parent
        visible: com ? com.colorMethod == 5 : false
        title: qsTr("Height Range")
        
        from: 0.1
        to: 8
        value: 0.2
        stepSize: 0.1

        onValueChanged: {
            if (com.heightRange != idHeight.value)
                com.heightRange = idHeight.value;
        }
    }

    SliderWithSpinBox {
        id: idGapFill
        anchors.fill: parent
        visible: com ? com.colorMethod == 3 : false
        title: qsTr("Gap Area")
        
        from: 0
        to: 5
        value: 1
        stepSize: 0.1
    
        onValueChanged: {
            if (com.gapArea != idGapFill.value)
                com.gapArea = idGapFill.value;
        }
    }
}