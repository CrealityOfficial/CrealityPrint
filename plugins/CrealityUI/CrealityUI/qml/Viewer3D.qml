import QtQuick 2.7
import Qt3D.Core 2.1
import Qt3D.Render 2.1
import Qt3D.Extras 2.1
import Qt3D.Input 2.1
import QtQuick 2.2 as QQ2

Entity {
    id: rootEntity

    property var isFromCpp: false
    signal viewChanged(var posion, var upvector)
    function setViewChange(viewDirection, upVector)
    {
        isFromCpp = true
        var dirtemp = Qt.vector3d(viewDirection.x,viewDirection.y,viewDirection.z).normalized()
        mainCamera.position = Qt.vector3d(dirtemp.x*30, dirtemp.z*30, dirtemp.y*-1*30)
        //mainCamera.position = Qt.vector3d(position.x/500*30, position.z/500*30, position.y*-1/500*30)
        mainCamera.upVector = Qt.vector3d(upVector.x, upVector.z, upVector.y*-1)
    }

    function restView()
    {
        mainCamera.position = Qt.vector3d(0, 20, 20)//Qt.vector3d(0, 0, 30)
        mainCamera.upVector = Qt.vector3d(0, 0.8, -0.8)//Qt.vector3d(0, 1, 0)
        viewChanged(mainCamera.position, mainCamera.upVector)
    }

    function setAnimationTarget(targetValue)
    {
        idAnimation.from = mainCamera.position
        idAnimation.to = targetValue
        idAnimation.start()
    }

    function setlanguage(langType)
    {
        idCube3D.setlangType(langType)
    }

    function setTheme(themeType)
    {
        idCube3D.setThemeType(themeType)
    }

    Camera {
        id: mainCamera
        projectionType: CameraLens.PerspectiveProjection
        fieldOfView: 45
        nearPlane: 0.01
        farPlane: 1000.0
        position: Qt.vector3d(0, 20, 20)
        upVector: Qt.vector3d(0, 0.8, -0.8)
        viewCenter: Qt.vector3d(0.0, 0.0, 0.0)
        onPositionChanged:
        {
            if(!isFromCpp)
            {
                viewChanged(mainCamera.position, mainCamera.upVector)
            }
        }
    }

    ViewControler {
        id:viewControler
        camera: mainCamera
        linearSpeed:50.0
        lookSpeed:180.0
        onSigViewControl:
        {
            isFromCpp = false
        }
    }

    components: [
        RenderSettings {
            activeFrameGraph: ForwardRenderer {
                camera: mainCamera
                clearColor: "transparent"
            }
            pickingSettings.pickMethod: PickingSettings.TrianglePicking
        },

        DirectionalLight{
            id: idligt
            worldDirection: Qt.vector3d(-1*mainCamera.position.x, -1*mainCamera.position.y, -1*mainCamera.position.z)
         },

        InputSettings { }
    ]

    Cube3DModel {
        id: idCube3D
        enabled:true
        onChangeShowPos:
        {
            isFromCpp = false
            if(type === 1 && changeid === 1)
            {
                console.log("onChangeShowPos 1 1")
                //mainCamera.position = Qt.vector3d(-16.0, 16.0, 16)
                setAnimationTarget( Qt.vector3d(-16.0, 16.0, 16))
                mainCamera.upVector = Qt.vector3d(0.4, 0.8, -0.4)
            }
            else if(type === 1 && changeid === 2)
            {
                console.log("onChangeShowPos 1 2")
                //mainCamera.position = Qt.vector3d(-16.0, 16.0, -16)
                setAnimationTarget( Qt.vector3d(-16.0, 16.0, -16))
                mainCamera.upVector = Qt.vector3d(0.4, 0.8, 0.4)
            }
            else if(type === 1 && changeid === 3)
            {
                console.log("onChangeShowPos 1 3")
                //mainCamera.position = Qt.vector3d(16.0, 16.0, -16)
                setAnimationTarget( Qt.vector3d(16.0, 16.0, -16))
                mainCamera.upVector = Qt.vector3d(-0.4, 0.8, 0.4)
            }
            else if(type === 1 && changeid === 4)
            {
                console.log("onChangeShowPos 1 4")
                //mainCamera.position = Qt.vector3d(16.0, 16.0, 16)
                setAnimationTarget( Qt.vector3d(16.0, 16.0, 16))
                mainCamera.upVector = Qt.vector3d(-0.4, 0.8, -0.4)
            }
            else if(type === 1 && changeid === 5)
            {
                console.log("onChangeShowPos 1 5")
                //mainCamera.position = Qt.vector3d(-16.0, -16.0, 16)
                setAnimationTarget( Qt.vector3d(-16.0, -16.0, 16))
                mainCamera.upVector = Qt.vector3d(-0.4, 0.8, 0.4)
            }
            else if(type === 1 && changeid === 6)
            {
                console.log("onChangeShowPos 1 6")
                //mainCamera.position = Qt.vector3d(-16.0, -16.0, -16)
                setAnimationTarget( Qt.vector3d(-16.0, -16.0, -16))
                mainCamera.upVector = Qt.vector3d(-0.4, 0.8, -0.4)
            }
            else if(type === 1 && changeid === 7)
            {
                console.log("onChangeShowPos 1 7")
                //mainCamera.position = Qt.vector3d(16.0, -16.0, -16)
                setAnimationTarget( Qt.vector3d(16.0, -16.0, -16))
                mainCamera.upVector = Qt.vector3d(0.4, 0.8, -0.4)
            }
            else if(type === 1 && changeid === 8)
            {
                console.log("onChangeShowPos 1 8")
                //mainCamera.position = Qt.vector3d(16.0, -16.0, 16)
                setAnimationTarget( Qt.vector3d(16.0, -16.0, 16))
                mainCamera.upVector = Qt.vector3d(0.4, 0.8, 0.4)
            }
            else if(type === 2 && changeid === 1)
            {
                console.log("onChangeShowPos 2 1")
                //mainCamera.position = Qt.vector3d(-20, 20, 0)
                setAnimationTarget( Qt.vector3d(-20, 20, 0))
                mainCamera.upVector = Qt.vector3d(0.8, 0.8, 0)
            }
            else if(type === 2 && changeid === 2)
            {
                console.log("onChangeShowPos 2 2")
                //mainCamera.position = Qt.vector3d(20, -20, 0)
                setAnimationTarget( Qt.vector3d(20, -20, 0))
                mainCamera.upVector = Qt.vector3d(0.8, 0.8, 0)
            }
            else if(type === 2 && changeid === 3)
            {
                console.log("onChangeShowPos 2 3")
                //mainCamera.position = Qt.vector3d(20, 20, 0)
                setAnimationTarget( Qt.vector3d(20, 20, 0))
                mainCamera.upVector = Qt.vector3d(-0.8, 0.8, 0)
            }
            else if(type === 2 && changeid === 4)
            {
                console.log("onChangeShowPos 2 4")
                //mainCamera.position = Qt.vector3d(-20, -20, 0)
                setAnimationTarget( Qt.vector3d(-20, -20, 0))
                mainCamera.upVector = Qt.vector3d(-0.8, 0.8, 0)
            }
            else if(type === 2 && changeid === 5)
            {
                console.log("onChangeShowPos 2 5")
                //mainCamera.position = Qt.vector3d(0, 20, -20)
                setAnimationTarget( Qt.vector3d(0, 20, -20))
                mainCamera.upVector = Qt.vector3d(0, 0.8, 0.8)
            }
            else if(type === 2 && changeid === 6)
            {
                console.log("onChangeShowPos 2 6")
                //mainCamera.position = Qt.vector3d(0, -20, 20)
                setAnimationTarget( Qt.vector3d(0, -20, 20))
                mainCamera.upVector = Qt.vector3d(0, 0.8, 0.8)
            }
            else if(type === 2 && changeid === 7)
            {
                console.log("onChangeShowPos 2 7")
                //mainCamera.position = Qt.vector3d(0, -20, -20)
                setAnimationTarget( Qt.vector3d(0, -20, -20))
                mainCamera.upVector = Qt.vector3d(0, 0.8, -0.8)
            }
            else if(type === 2 && changeid === 8)
            {
                console.log("onChangeShowPos 2 8")
                //mainCamera.position = Qt.vector3d(0, 20, 20)
                setAnimationTarget( Qt.vector3d(0, 20, 20))
                mainCamera.upVector = Qt.vector3d(0, 0.8, -0.8)
            }
            else if(type === 2 && changeid === 9)
            {
                console.log("onChangeShowPos 2 9")
                //mainCamera.position = Qt.vector3d(20, 0, 20)
                setAnimationTarget( Qt.vector3d(20, 0, 20))
                mainCamera.upVector = Qt.vector3d(0, 1, 0)
            }
            else if(type === 2 && changeid === 10)
            {
                console.log("onChangeShowPos 2 10")
                //mainCamera.position = Qt.vector3d(-20, 0, -20)
                setAnimationTarget( Qt.vector3d(-20, 0, -20))
                mainCamera.upVector = Qt.vector3d(0, 1, 0)
            }
            else if(type === 2 && changeid === 11)
            {
                console.log("onChangeShowPos 2 11")
                //mainCamera.position = Qt.vector3d(20, 0, -20)
                setAnimationTarget( Qt.vector3d(20, 0, -20))
                mainCamera.upVector = Qt.vector3d(0, 1, 0)
            }
            else if(type === 2 && changeid === 12)
            {
                console.log("onChangeShowPos 2 12")
                //mainCamera.position = Qt.vector3d(-20, 0, 20)
                setAnimationTarget( Qt.vector3d(-20, 0, 20))
                mainCamera.upVector = Qt.vector3d(0, 1, 0)
            }
            else if(type === 3 && changeid === 1)
            {
                console.log("onChangeShowPos 3 1")
                //mainCamera.position = Qt.vector3d(0, 30, 0)
                setAnimationTarget( Qt.vector3d(0, 30, 0))
                mainCamera.upVector = Qt.vector3d(0, 0, -1)
            }
            else if(type === 3 && changeid === 2)
            {
                console.log("onChangeShowPos 3 2")
                //mainCamera.position = Qt.vector3d(0, 0, 30)
                setAnimationTarget( Qt.vector3d(0, 0, 30))
                mainCamera.upVector = Qt.vector3d(0, 1, 0)
            }
            else if(type === 3 && changeid === 3)
            {
                console.log("onChangeShowPos 3 3")
                //mainCamera.position = Qt.vector3d(0, -30, 0)
                setAnimationTarget( Qt.vector3d(0, -30, 0))
                mainCamera.upVector = Qt.vector3d(0, 0, -1)
            }
            else if(type === 3 && changeid === 4)
            {
                console.log("onChangeShowPos 3 4")
                //mainCamera.position = Qt.vector3d(0, 0, -30)
                setAnimationTarget( Qt.vector3d(0, 0, -30))
                mainCamera.upVector = Qt.vector3d(0, 1, 0)
            }
            else if(type === 3 && changeid === 5)
            {
                console.log("onChangeShowPos 3 5")
                //mainCamera.position = Qt.vector3d(-30, 0, 0)
                setAnimationTarget( Qt.vector3d(-30, 0, 0))
                mainCamera.upVector = Qt.vector3d(0, 1, 0)
            }
            else if(type === 3 && changeid === 6)
            {
                console.log("onChangeShowPos 3 6")
                //mainCamera.position = Qt.vector3d(30, 0, 0)
                setAnimationTarget( Qt.vector3d(30, 0, 0))
                mainCamera.upVector = Qt.vector3d(0, 1, 0)
            }
            else
            {
            console.log("error param")
            }
            //viewChanged(mainCamera.position, mainCamera.upVector)
        }
    }

    QQ2.PropertyAnimation {
        id: idAnimation
        target: mainCamera
        property: "position"
        duration: 500
        from: 0
        to: 360

        loops: 1//QQ2.Animation.Infinite
        running: false
    }
}
