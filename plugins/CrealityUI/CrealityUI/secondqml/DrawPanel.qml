import QtQuick 2.0
import "../qml"
MouseArea {
            id:roiRectArea
            anchors.fill: parent
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
            property var currentSharp:DrawPanel.SharpType.NONE
			property var ctrlSelArray:[]
			property var isLine: false
			property var zCnt: 1
			
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

            function clearSelectedStatus()
            {
                for(var i = 0; i < ctrlSelArray.length; i++)
                {
                    ctrlSelArray[i].selected = false;
                }
                ctrlSelArray = [];
            }

            function createSharp(x,y,width,height,sharp)
            {
                var component;
                if(sharp===DrawPanel.SharpType.RECT)
                {
                    component = Qt.createComponent("qrc:/CrealityUI/qml/DragRect.qml");
                }
                if(sharp===DrawPanel.SharpType.ELIPSE)
                {
                    component = Qt.createComponent("qrc:/CrealityUI/qml/DragEllipse.qml");
                }
                if(sharp===DrawPanel.SharpType.IMAGE)
                {
                    component = Qt.createComponent("qrc:/CrealityUI/qml/DragImage.qml");
                }
                if(sharp===DrawPanel.SharpType.PATH)
                {
                    component = Qt.createComponent("qrc:/CrealityUI/qml/DragPath.qml");
                }
                if(sharp===DrawPanel.SharpType.TEXT)
                {
                    component = Qt.createComponent("qrc:/CrealityUI/qml/DragText.qml");
                }

                if (component.status === Component.Ready)
                {
                    var rectobj;
                    if(currentSharp!==DrawPanel.SharpType.TEXT)
                    {
                        rectobj =component.createObject(this, {x: x, y: y});
                        
                    }else{
                        rectobj =component.createObject(this, {x: x, y: y,width:width,height:height});
                    }
                    //console.log(component.status)
                    
                    rectobj.objectChanged.connect(this.objectChanged)
                    rectobj.select.connect(this.onRectSelected)
                    rectobj.ctrlSelect.connect(this.onCtrSelectArray)
                    rectobj.selected=true
					rectobj.z = zCnt
					zCnt++
                    return rectobj;
                }
            }
            function onRectSelected(sender){
                console.log("onRectSelected ...")
                clearSelectedStatus();
                if(currentSel)
                {
                    currentSel.selected=false
                }
                sender.selected=true
                currentSel=sender;
                ctrlSelArray.push(currentSel)
                selectChanged(currentSel)
                verifyFocus()
                //console.log(sender.selected);
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
					case Qt.ShiftModifier:
						isLine = true
						break;
					default:
						isLine = false
						break;
                }
				
				
                if(currentSel)
                {
                    currentSel.selected=false
                }
                clearSelectedMul()
                clearSelectedStatus()
                if(currentSharp!==DrawPanel.SharpType.NONE)
                {
                    currentSel = createSharp(pressX,pressY,1,1,currentSharp)
                }

                //selRect.x=pressX
                //selRect.y=pressY
            }

            onReleased: {
                releaseX = mouse.x
                releaseY = mouse.y
                rectWidth = releaseX
                widthRect = Math.abs(releaseX - pressX)
                rectHeight = releaseY
                heightRect = Math.abs(releaseY - pressY)
                if(currentSharp!==DrawPanel.SharpType.NONE)
                 {
                    if(currentSel)
                    {
                        if(currentSharp===DrawPanel.SharpType.PATH)
                        {
                            widthRect = currentSel.maxX-currentSel.minX
                            rectHeight = currentSel.maxY-currentSel.minY

                            currentSel.width=widthRect
                            currentSel.height=rectHeight
                            currentSel.x=currentSel.x+currentSel.minX
                            currentSel.y=currentSel.y+currentSel.minY
							currentSel.shapeX = currentSel.shapeX-currentSel.minX
							currentSel.shapeY = currentSel.shapeY-currentSel.minY
							
                            //currentSel.reCal()

                        }
                        if(currentSharp!==DrawPanel.SharpType.TEXT && currentSharp!==DrawPanel.SharpType.PATH)
                        {
                            if(rectHeight<=1 || widthRect<=1)
                            {
                                currentSel.destroy()
                                return;
                            }
                        }

                        currentSel.opacity = 1
                        addRect(currentSel,currentSharp)
                        selectChanged(currentSel)
                    }
                }

            }

            onPositionChanged: {
                releaseX = mouse.x
                releaseY = mouse.y
                widthRect = Math.abs(releaseX - pressX)
                heightRect = Math.abs(releaseY - pressY)
                if(currentSharp!==DrawPanel.SharpType.TEXT && currentSharp!==DrawPanel.SharpType.PATH)
                {
                    if(currentSel.selected)
                    {
                        currentSel.width=widthRect
                        currentSel.height=heightRect
						if(releaseX - pressX < 0)
						{
							currentSel.x= releaseX
						}
						if(releaseY - pressY < 0)
						{
							currentSel.y=releaseY
						}
						
                    }
                }
                if(currentSharp===DrawPanel.SharpType.PATH)
                {
                    if(currentSel)
                    {
                        var pathx=releaseX-currentSel.x;
                        var pathy=releaseY-currentSel.y;
                        if(pathx>currentSel.maxX)
                        {
                            currentSel.maxX=pathx;
                        }
                        if(pathy>currentSel.maxY)
                        {
                            currentSel.maxY=pathy;
                        }
                        if(pathx<currentSel.minX)
                        {
                            currentSel.minX=pathx;
                        }
                        if(pathy<currentSel.minY)
                        {
                            currentSel.minY=pathy;
                        }
						
						if(isLine)
						{
							if(pathx > 0)
							{
								currentSel.maxX=pathx;
							}
							else
							{
								currentSel.minX=pathx;
							}
							
							if(pathy > 0)
							{
								currentSel.maxY=pathy;
							}
							else
							{
								currentSel.minY=pathy;
							}
							
							if(currentSel.getPathcnt() == 0)
							{
								currentSel.addPath(pressX-currentSel.x,pressY-currentSel.y)
							}
							else
							{
								currentSel.setEndPos(pathx,pathy)
							}
						}
						else
						{
							currentSel.addPath(pathx,pathy)
						}
                    }
                }

            }


        }
