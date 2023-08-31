import QtQuick 2.7
import QtQuick.Controls 2.3
import QtQuick.Scene3D 2.0

Item {
    id : idControl
    // width: 200/*80*/
    // height: 200/*80*/
    //color: "blue"
    signal setViewShow(var posion, var upvector)

    readonly property real margin: 12

    function cameraChaged(position, upVector)
    {
        idView3D.setViewChange(position, upVector)
    }

    function resetViewer3D()
    {
        idView3D.restView()
    }

    function setLanguage(langType)
    {
        idView3D.setlanguage(langType)
    }

    function setTheme(themeType)
    {
        idView3D.setTheme(themeType)
    }

    Scene3D {
        id: scene3d
        anchors.fill: parent
        //anchors.margins: 10
        focus: true
        aspects: ['input', 'logic']
        cameraAspectRatioMode: Scene3D.AutomaticAspectRatio
        hoverEnabled: true
        antialiasing: true

        Viewer3D {
            id: idView3D
            onViewChanged:
            {
                setViewShow(posion, upvector)
            }
        }
    }
}
