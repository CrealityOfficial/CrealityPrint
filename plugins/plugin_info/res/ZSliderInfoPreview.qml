import QtQuick 2.12
import CrealityUI 1.0
import "qrc:/CrealityUI"

Item {
    y: 210 * screenScaleFactor
    width: 20 * screenScaleFactor
    height: 400 * screenScaleFactor

    parent: kernel_ui.root
    anchors.right: parent.right
    anchors.rightMargin: (65 + Constants.rightPanelWidth) * screenScaleFactor

    ZSliderRange {
        anchors.fill: parent
        visible: kernel_kernel.currentPhase == 0
        enabled: kernel_modelspace.modelNNum > 0

        from: 0
        to: plugin_info_sliderinfo.maxLayer

        stepSize: 1
        first.value: from
        second.value: to

        orientation: Qt.Vertical
        firstLabelText: "Z: " + first.value.toFixed(1)
        secondLabelText: "Z: " + second.value.toFixed(1)

        first.onMoved:
        {
            plugin_info_sliderinfo.setBottomCurrentLayer(first.value)
        }
        second.onMoved:
        {
            plugin_info_sliderinfo.setTopCurrentLayer(second.value)
        }
    }
}
