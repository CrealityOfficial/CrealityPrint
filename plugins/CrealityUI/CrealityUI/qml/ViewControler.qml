import Qt3D.Core 2.0
import Qt3D.Render 2.0
import Qt3D.Input 2.0

Entity{
    id: root
    property Camera camera;
    property real dt: 0.001
    property real linearSpeed: 1
    property real lookSpeed: 500
    property real zoomLimit: 0.16
    property bool mouseEnabled: true
    signal sigViewControl()
    function setEnabled(isEnable)
    {
        mouseEnabled = isEnable;
        console.log("--------------"+isEnable)
    }
    MouseDevice {
        id: mouseDevice
        sensitivity: 0.001 // Make it more smooth
    }

    MouseHandler {
        id: mh
        readonly property vector3d upVect: Qt.vector3d(0, 1, 0)
        property point lastPos;
        property real pan;
        property real tilt;
        sourceDevice: mouseDevice

        onPanChanged: root.camera.panAboutViewCenter(pan, upVect);
        onTiltChanged: root.camera.tiltAboutViewCenter(tilt);

        onPressed: {
            lastPos = Qt.point(mouse.x, mouse.y);
        }
        onPositionChanged: {
            sigViewControl()
            if(!mouseEnabled)
            {
                return;
            }
            // You can change the button as you like for rotation or translation
            if (mouse.buttons === 2){ // Right button for rotation
                pan = -(mouse.x - lastPos.x) * dt * lookSpeed;
                tilt = (mouse.y - lastPos.y) * dt * lookSpeed;
            } else if (mouse.buttons === 1) { // Left button for translate
                return;
//                var rx = -(mouse.x - lastPos.x) * dt * linearSpeed;
//                var ry = (mouse.y - lastPos.y) * dt * linearSpeed;
//                camera.translate(Qt.vector3d(rx, ry, 0))
//                console.log(linearSpeed)
            } else if (mouse.buttons === 3) { // Left & Right button for zoom
                ry = (mouse.y - lastPos.y) * dt * linearSpeed
                zoom(ry)
            }

            lastPos = Qt.point(mouse.x, mouse.y)
        }
        onWheel: {
            //zoom(wheel.angleDelta.y * dt * linearSpeed)
        }

        function zoom(ry) {
            if (ry > 0 && zoomDistance(camera.position, camera.viewCenter) < zoomLimit) {
                return
            }

            camera.translate(Qt.vector3d(0, 0, ry), Camera.DontTranslateViewCenter)
        }

        function zoomDistance(posFirst, posSecond) {
            return posSecond.minus(posFirst).length()
        }
    }
}
