import QtQuick 2.0
MouseArea {
    id:roiRectArea
    anchors.fill: parent
    propagateComposedEvents: true   //允许鼠标穿透
    property  var pressX:0
    property  var pressY:0
    property var rulersSize: 20
    property  var releaseX
    property  var releaseY
    property  var rectWidth
    property  var rectHeight
    property  var widthRect
    property  var heightRect
    property var currentSel
    property var currentSharp:DrawPanel.SharpType.TEXT
    property var ctrlSelArray:[]
    property var allArray: []
    property var isControlModifier: false       //通过ctrl+左键 添加文字
    property var zCnt: 1

    property bool hasWordSelect: true

    signal addRect(var obj, var sharpType)
    signal selectChanged(var obj)
    signal verifyFocus()
    signal selectedMul(var array)
    signal clearSelectedMul()
    signal objectChanged(var obj, var oldX, var oldY, var oldWidth, var oldHeight, var oldRotation, var newX, var newY, var newWidth, var newHeight, var newRotation)
    enum SharpType {
        NONE,
        RECT,
        ELIPSE,
        IMAGE,
        PATH,
        TEXT
    }
    function checkSelected()
    {
        for(var i = 0; i < allArray.length; i++)
        {
            if(allArray[i].selected)
            {
                return true
            }
        }
        return false
    }
    function clearSelectedStatus()
    {
        for(var i = 0; i < ctrlSelArray.length; i++)
        {
            ctrlSelArray[i].selected = false;
        }
        ctrlSelArray = [];

    }
    function selectAll()
    {
        clearSelectedStatus()
        for(var i = 0; i < allArray.length; i++)
        {
            allArray[i].selected = true;
            ctrlSelArray.push(allArray[i])
        }
    }

    function deleteSelectObject() {
      if (ctrlSelArray.length > 0) {
        for (var i = 0; i < allArray.length; i++) {
          if (ctrlSelArray.length > i) {
            if (ctrlSelArray[i].selected) {
              ctrlSelArray[i].destroy()
              allArray.pop(ctrlSelArray[i])
            }
          }
        }

        ctrlSelArray = [];
        return
      }

      if (currentSel && currentSel.selected) {
        currentSel.destroy()
        currentSel = null;
        return
      }
    }

    function createDragText(x,y,width,height)
    {
        createSharp(x,y,width,height,DrawPanel.SharpType.TEXT)
    }
    function createSharp(x,y,width,height,sharp)
    {
        var component;
        if(sharp===DrawPanel.SharpType.TEXT)
        {
            component = Qt.createComponent("qrc:/letter/letter/DragText.qml");
        }

        if (component.status === Component.Ready)
        {
            var rectobj;
            rectobj =component.createObject(this, {x: x, y: y,width:width,height:height,
                                                textConfig: gTextPara,
                                                fontsize:gTextPara.textSize	,
                                                objectName:"drawText",
                                                visible:roiRectArea.visible},
                                                );
            rectobj.objectChanged.connect(this.objectChanged)
            rectobj.select.connect(this.onRectSelected)
            rectobj.ctrlSelect.connect(this.onCtrSelectArray)
            rectobj.textChanged.connect(function slotText(){return gTextPara.textName = rectobj.text})
            rectobj.fontsizeChanged.connect(function(){gTextPara.textSize = rectobj.fontsize})
            rectobj.selected=true
            rectobj.z = zCnt
            currentSel = rectobj
            ctrlSelArray.push(rectobj)
            allArray.push(rectobj)
            zCnt++
            selectChanged(currentSel)
            return rectobj;
        }
    }
    function onRectSelected(sender){
        clearSelectedStatus();
        if(currentSel)
        {
            currentSel.selected=false
        }
        sender.selected=true
        currentSel=sender;
        ctrlSelArray.push(currentSel)
        selectChanged(currentSel)
        hasWordSelect = checkSelected()

        verifyFocus()
    }

    function onCtrSelectArray(ctrlSender)
    {
        if(ctrlSender.selected)
        {
            ctrlSender.selected = false
            ctrlSelArray.pop(ctrlSender)
        }
        else
        {
            ctrlSelArray.push(ctrlSender)
        }

        onRectMulSelected(ctrlSelArray)
    }

    function onRectMulSelected(objArray)
    {
        clearSelectedStatus()
        for(var i = 0; i < objArray.length; i++)
        {
            objArray[i].selected = true;
            ctrlSelArray.push(objArray[i])
        }
        selectedMul(objArray);
    }

    onPressed: {
        verifyFocus()
        pressX = mouse.x
        pressY = mouse.y

        switch(mouse.modifiers){
            case Qt.ControlModifier :

                 isControlModifier = true
                break;
            default:
                isControlModifier = false
                break;
        }


        if(currentSel)
        {
            currentSel.selected=false
        }
        clearSelectedMul()
        clearSelectedStatus()

        if(isControlModifier && allArray.length ===0)
            currentSel = createSharp(pressX,pressY,1,1,currentSharp)
        //鼠标穿透的效果

        hasWordSelect = checkSelected()
        mouse.accepted = false

    }

    onReleased: {
        releaseX = mouse.x
        releaseY = mouse.y
        rectWidth = releaseX
        widthRect = Math.abs(releaseX - pressX)
        rectHeight = releaseY
        heightRect = Math.abs(releaseY - pressY)
        //鼠标穿透后release不会进来
//        mouse.accepted = false
    }

    onPositionChanged: {
        releaseX = mouse.x
        releaseY = mouse.y
        widthRect = Math.abs(releaseX - pressX)
        heightRect = Math.abs(releaseY - pressY)
    }
}
