/****************************************************************************
**
** Copyright (C) 2015 Klaralvdalens Datakonsult AB (KDAB).
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt3D module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.0
import QtQuick.Window 2.0


Item {
    visible: true
    width: 1024
    height: 800

    //color: "#FAFAFA"
    function repaint(){
        canvas.clean();
        canvas.requestPaint()
    }
    function clear() {
       canvas.clean();
       canvas.requestPaint();
    }
    Canvas {
        id: canvas
        width: parent.width
        height: parent.height
        anchors.centerIn: parent
        antialiasing: true
        property int wgrid: 30
        function clean()
        {
            if(visible)
            {
                var ctx = getContext("2d");
                ctx.reset();
            }
        }
        MouseArea {
                                //parent: scrollCanvas
                                anchors.fill: canvas
                                propagateComposedEvents: true


                                onPressed:{
                                    mouse.accepted = false
                                }
                                onPositionChanged:{mouse.accepted = false}
                                onReleased:{mouse.accepted = false}
                            }
        onPaint: {
            var ctx = getContext("2d")
            ctx.save()
            var zero = Qt.point(0,height)//Qt.point(width/2,height/2)//
            ctx.lineWidth = 0.2756410256410256
            ctx.strokeStyle = '#5a5a5a'
            ctx.lineDashOffset=0
            //ctx.globalAlpha=0.16
            ctx.beginPath()
            var nrows = width/wgrid;
            ctx.moveTo(zero);
 
            for(var i=1; i<nrows+1; i++)
            {
                var p1 = Qt.point(zero.x+wgrid*i,zero.y)//
                var p2 = Qt.point(p1.x,p1.y-height)
                ctx.moveTo(p1.x,p1.y);
                ctx.lineTo(p2.x,p2.y);              
            }
            var ncols = height/wgrid
             for(var j=1; j < ncols+1; j++){
                 p1 = Qt.point(zero.x,zero.y-wgrid*j)//
                 p2 = Qt.point(p1.x+width,p1.y)
                 ctx.moveTo(p1.x,p1.y);
                 ctx.lineTo(p2.x,p2.y);               
            }
            /*for(var i=1; i < nrows/2+1; i++){
                var p1 = Qt.point(zero.x+wgrid*i,zero.y)//
                var p2 = Qt.point(p1.x,p1.y+height/2)
                ctx.moveTo(p1.x,p1.y);
                ctx.lineTo(p2.x,p2.y);
                ctx.lineTo(p2.x,p2.y-height);
                var p3 = Qt.point(zero.x-wgrid*i,zero.y)//
                var p4 = Qt.point(p3.x,p3.y+height/2)
                ctx.moveTo(p3.x,p3.y);
                ctx.lineTo(p4.x,p4.y);
                ctx.lineTo(p4.x,p4.y-height);
            }*/

           /*var ncols = height/wgrid
             for(var j=1; j < ncols+1; j++){
                 p1 = Qt.point(zero.x,zero.y+wgrid*j)//
                 p2 = Qt.point(p1.x+width/2,p1.y)
                 ctx.moveTo(p1.x,p1.y);
                 ctx.lineTo(p2.x,p2.y);
                 ctx.lineTo(p2.x-width,p2.y);
                 p3 = Qt.point(zero.x,zero.y-wgrid*j)/
                 p4 = Qt.point(p3.x+width/2,p3.y)
                 ctx.moveTo(p3.x,p3.y);
                 ctx.lineTo(p4.x,p4.y);
                 ctx.lineTo(p4.x-width,p4.y);
            }*/

            ctx.closePath()
            ctx.stroke()

//draw x
             ctx.save()
             ctx.lineWidth = 2//1
            ctx.strokeStyle = "#FF1D38"
             ctx.font = " 4px sans-serif";

            ctx.setLineDash([5,5])
            ctx.beginPath()
            ctx.moveTo(zero.x,zero.y)
            ctx.lineTo(width, zero.y);
             ctx.closePath()
             ctx.stroke()
            //ctx.moveTo(width, zero.y)
            /* ctx.setLineDash([3,9])
             ctx.beginPath()
             ctx.moveTo(zero.x,zero.y)
            ctx.lineTo(0, zero.y);
            //ctx.fillText("10" , 10 , 50);
            ctx.closePath()
            ctx.stroke()
            ctx.restore()*/

//draw y
             ctx.lineWidth = 2//1
            ctx.strokeStyle = "#22AC38"
            ctx.font = " 4px sans-serif";
             //ctx.save()
             ctx.setLineDash([5,5])
             ctx.beginPath()
             ctx.moveTo(zero.x,zero.y)
             ctx.lineTo(zero.x, 0);
             ctx.closePath()
             ctx.stroke();

             /*ctx.setLineDash([3,9])
             ctx.beginPath()
             ctx.moveTo(zero.x,zero.y)
             ctx.lineTo(zero.x, height);
             ctx.closePath()
             ctx.stroke();*/

//draw origin
             ctx.beginPath()
             ctx.fillStyle = "#4c4c4c"
             ctx.moveTo(zero.x,zero.y)
             ctx.arc(zero.x,zero.y,5,0,2*Math.PI,true);
             ctx.stroke();
             ctx.fill();
             //ctx.restore()
//draw text
             ctx.font = " 12px sans-serif";
             ctx.fillStyle = "#22AC38"
             var itxt=0;
             for(var i=wgrid; i<height; i=i+wgrid)
             {
                 itxt=itxt+10
                 if(itxt<100)
                 {
                    //ctx.fillText("-"+itxt,zero.x-25,zero.y+i+6)
                    ctx.fillText(""+itxt,zero.x+10,zero.y-i+6)
                 }else{
                     //ctx.fillText("-"+itxt,zero.x-30,zero.y+i+6)
                     ctx.fillText(""+itxt,zero.x+10,zero.y-i+6)
                 }

             }

             ctx.fillStyle = "#FF1D38"
             itxt=0;
             for(var i=wgrid; i<width; i=i+wgrid)
             {
                 itxt=itxt+10
                 if(itxt<100)
                 {
                    ctx.fillText(""+itxt,zero.x+i-8,zero.y-10)
                    //ctx.fillText("-"+itxt,zero.x-i-8-4,zero.y+15)
                 }else{
                     ctx.fillText(""+itxt,zero.x+i-8,zero.y-10)
                     //ctx.fillText("-"+itxt,zero.x-i-8-4,zero.y+15)
                 }

             }
             ctx.restore();

        }

    }

}
