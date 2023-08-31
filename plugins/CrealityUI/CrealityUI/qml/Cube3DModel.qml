import QtQuick 2.7
import Qt3D.Core 2.1
import Qt3D.Render 2.9
import Qt3D.Extras 2.9

import Qt3D.Input 2.1
import "qrc:/CrealityUI"

Entity {
    id: gridEntity
    signal changeShowPos(var type, var changeid) //1:点  2：边  3：面
    property var languageType
    function setlangType(langType)
    {
        console.log("setlangType~~~~~~~~" + langType)
        languageType = langType

        idTextureImage1.source = languageType == 0 ? Constants.cube3DTopImg : Constants.cube3DTopImg_C
        idTextureImage2.source = languageType == 0 ? Constants.cube3DFrontImg : Constants.cube3DFrontImg_C
        idTextureImage3.source = languageType == 0 ? Constants.cube3DBottomImg : Constants.cube3DBottomImg_C
        idTextureImage4.source = languageType == 0 ? Constants.cube3DBackImg : Constants.cube3DBackImg_C
        idTextureImage5.source = languageType == 0 ? Constants.cube3DLeftkImg : Constants.cube3DLeftkImg_C
        idTextureImage6.source = languageType == 0 ? Constants.cube3DLeftkRight : Constants.cube3DLeftkRight_C
    }

    function setThemeType(themeType)
    {
        vertexMaterial1.ambient = Constants.cube3DAmbientColor
        vertexMaterial2.ambient = Constants.cube3DAmbientColor
        vertexMaterial3.ambient = Constants.cube3DAmbientColor
        vertexMaterial4.ambient = Constants.cube3DAmbientColor
        vertexMaterial5.ambient = Constants.cube3DAmbientColor
        vertexMaterial6.ambient = Constants.cube3DAmbientColor
        vertexMaterial7.ambient = Constants.cube3DAmbientColor
        vertexMaterial8.ambient = Constants.cube3DAmbientColor

        vertexMaterialEdge1.ambient = Constants.cube3DAmbientColor
        vertexMaterialEdge2.ambient = Constants.cube3DAmbientColor
        vertexMaterialEdge3.ambient = Constants.cube3DAmbientColor
        vertexMaterialEdge4.ambient = Constants.cube3DAmbientColor
        vertexMaterialEdge5.ambient = Constants.cube3DAmbientColor
        vertexMaterialEdge6.ambient = Constants.cube3DAmbientColor
        vertexMaterialEdge7.ambient = Constants.cube3DAmbientColor
        vertexMaterialEdge8.ambient = Constants.cube3DAmbientColor
        vertexMaterialEdge9.ambient = Constants.cube3DAmbientColor
        vertexMaterialEdge10.ambient = Constants.cube3DAmbientColor
        vertexMaterialEdge11.ambient = Constants.cube3DAmbientColor
        vertexMaterialEdge12.ambient = Constants.cube3DAmbientColor

        idTextureImage1.source = languageType == 0 ? Constants.cube3DTopImg : Constants.cube3DTopImg_C
        idTextureImage2.source = languageType == 0 ? Constants.cube3DFrontImg : Constants.cube3DFrontImg_C
        idTextureImage3.source = languageType == 0 ? Constants.cube3DBottomImg : Constants.cube3DBottomImg_C
        idTextureImage4.source = languageType == 0 ? Constants.cube3DBackImg : Constants.cube3DBackImg_C
        idTextureImage5.source = languageType == 0 ? Constants.cube3DLeftkImg : Constants.cube3DLeftkImg_C
        idTextureImage6.source = languageType == 0 ? Constants.cube3DLeftkRight : Constants.cube3DLeftkRight_C
    }

    //尖角1 start
    Entity {
        id: vertexEntity1
        components: [
            PhongMaterial {
                id: vertexMaterial1
                //环境光
                ambient: Constants.cube3DAmbientColor
                //漫反射光
                diffuse: "#C8C8C8"
                //镜面高光
                specular: "#010101"
                //高光半径
                shininess:5
            },

            ObjectPicker{
                id: vertexSpherePicker1
                hoverEnabled: true
                dragEnabled: true
                onClicked:{
                    if (pick.button === PickEvent.LeftButton)
                    {
                        console.log("~~~~~~~~~~~~~~~~~~")
                        console.log("vertexSpherePicker1 at:triangleIndex =  " + pick.triangleIndex)
                        changeShowPos(1, 1)
                    }
                }
                onEntered: {
                    console.log("vertexSpherePicker1 onEntered")
                    vertexMaterial1.ambient = Constants.cube3DAmbientColor_Entered
                    vertexMaterial1.diffuse = Constants.cube3DAmbientColor_Entered
                    vertexMaterial1.specular = Constants.cube3DAmbientColor_Entered
                }
                onExited: {
                    console.log("vertexSpherePicker1 onExited")
                    vertexMaterial1.ambient = Constants.cube3DAmbientColor
                    vertexMaterial1.diffuse = "#C8C8C8"
                    vertexMaterial1.specular = "#010101"
                }

            },

            GeometryRenderer {
                id: vertexGeometryRenderer1
                // 利用GeometryRenderer提供的基本类型(line)来进行几何图形的绘制
                primitiveType: GeometryRenderer.Triangles//GeometryRenderer.Lines //GeometryRenderer.TriangleStrip //
                // 几何数据的坐标
                geometry: Geometry {
                    Attribute {
                        id: vertexPosition1
                        attributeType: Attribute.VertexAttribute
                        vertexBaseType: Attribute.Float // 定义顶点的数据类型
                        vertexSize: 3 // 顶点大小
                        count: 0
                        name: defaultPositionAttributeName
                        buffer: Buffer {
                            type: Buffer.VertexBuffer
                            data: {
                                function createCube(attribute)
                                {
                                    var vertexCount = 2*3;
                                    var f32a = new Float32Array(vertexCount*3);
                                    //1
                                    f32a[0] = -4;
                                    f32a[1] = 4;
                                    f32a[2] = 4;

                                    f32a[3] = -3;
                                    f32a[4] = 4;
                                    f32a[5] = 3;

                                    f32a[6] = -4;
                                    f32a[7] = 4;
                                    f32a[8] = 3;

                                    //2
                                    f32a[9] = -4;
                                    f32a[10] = 4;
                                    f32a[11] = 4;

                                    f32a[12] = -3;
                                    f32a[13] = 4;
                                    f32a[14] = 4;

                                    f32a[15] = -3;
                                    f32a[16] = 4;
                                    f32a[17] = 3;

                                    attribute.count = vertexCount
                                    return f32a;

                                }
                                return createCube(vertexPosition1)
                            }
                        }
                    }
                    Attribute {
                        id: vertexNormal1
                        attributeType: Attribute.VertexAttribute
                        vertexBaseType: Attribute.Float // 定义顶点的数据类型
                        vertexSize: 3 // 顶点大小
                        count: 0
                        name: defaultNormalAttributeName
                        buffer: Buffer {
                            type: Buffer.VertexBuffer
                            data: {
                                function createCube(attribute)
                                {
                                    var vertexCount = 2*3;
                                    var f32a = new Float32Array(vertexCount*3);
                                    //3
                                    f32a[0] = 0;
                                    f32a[1] = 1;
                                    f32a[2] = 0;

                                    f32a[3] = 0;
                                    f32a[4] = 1;
                                    f32a[5] = 0;

                                    f32a[6] = 0;
                                    f32a[7] = 1;
                                    f32a[8] = 0;

                                    //4
                                    f32a[9] = 0;
                                    f32a[10] = 1;
                                    f32a[11] = 0;

                                    f32a[12] = 0;
                                    f32a[13] = 1;
                                    f32a[14] = 0;

                                    f32a[15] = 0;
                                    f32a[16] = 1;
                                    f32a[17] = 0;

                                    attribute.count = vertexCount
                                    return f32a;

                                }
                                return createCube(vertexNormal1)
                            }
                        }
                    }
                }
            }
        ]
    }


    Transform {
            id: cuboidTransform1_2
            translation:Qt.vector3d(0, 7, 0)
            rotation: fromAxisAndAngle(Qt.vector3d(1, 0 , 0), 90)
        }
    Entity {
        id: vertexEntity1_2
        components: [ vertexGeometryRenderer1, vertexMaterial1, vertexSpherePicker1, cuboidTransform1_2 ]
    }

    Transform {
            id: cuboidTransform1_3
            translation:Qt.vector3d(0, 7, 0)
            rotation: fromAxisAndAngle(Qt.vector3d(0, 0 , 1), 90)
        }
    Entity {
        id: vertexEntity1_3
        components: [ vertexGeometryRenderer1, vertexMaterial1, vertexSpherePicker1, cuboidTransform1_3 ]
    }
    //尖角1 end


    //尖角2 start
    Entity {
        id: vertexEntity2
        components: [
            GeometryRenderer {
                id: vertexGeometryRenderer2
                // 利用GeometryRenderer提供的基本类型(line)来进行几何图形的绘制
                primitiveType: GeometryRenderer.Triangles//GeometryRenderer.Lines //GeometryRenderer.TriangleStrip //
                // 几何数据的坐标
                geometry: Geometry {
                    Attribute {
                        id: vertexPosition2
                        attributeType: Attribute.VertexAttribute
                        vertexBaseType: Attribute.Float // 定义顶点的数据类型
                        vertexSize: 3 // 顶点大小
                        count: 0
                        name: defaultPositionAttributeName
                        buffer: Buffer {
                            type: Buffer.VertexBuffer
                            data: {
                                function createCube(attribute)
                                {
                                    var vertexCount = 2*3;
                                    var f32a = new Float32Array(vertexCount*3);
                                    //5
                                    f32a[0] = -4;
                                    f32a[1] = 4;
                                    f32a[2] = -3;

                                    f32a[3] = -3;
                                    f32a[4] = 4;
                                    f32a[5] = -4;

                                    f32a[6] = -4;
                                    f32a[7] = 4;
                                    f32a[8] = -4;

                                    //6
                                    f32a[9] = -4;
                                    f32a[10] = 4;
                                    f32a[11] = -3;

                                    f32a[12] = -3;
                                    f32a[13] = 4;
                                    f32a[14] = -3;

                                    f32a[15] = -3;
                                    f32a[16] = 4;
                                    f32a[17] = -4;

                                    attribute.count = vertexCount
                                    return f32a;

                                }
                                return createCube(vertexPosition2)
                            }
                        }
                    }
                    Attribute {
                        id: vertexNormal2
                        attributeType: Attribute.VertexAttribute
                        vertexBaseType: Attribute.Float // 定义顶点的数据类型
                        vertexSize: 3 // 顶点大小
                        count: 0
                        name: defaultNormalAttributeName
                        buffer: Buffer {
                            type: Buffer.VertexBuffer
                            data: {
                                function createCube(attribute)
                                {
                                    var vertexCount = 2*3;
                                    var f32a = new Float32Array(vertexCount*3);
                                    //3
                                    f32a[0] = 0;
                                    f32a[1] = 1;
                                    f32a[2] = 0;

                                    f32a[3] = 0;
                                    f32a[4] = 1;
                                    f32a[5] = 0;

                                    f32a[6] = 0;
                                    f32a[7] = 1;
                                    f32a[8] = 0;

                                    //4
                                    f32a[9] = 0;
                                    f32a[10] = 1;
                                    f32a[11] = 0;

                                    f32a[12] = 0;
                                    f32a[13] = 1;
                                    f32a[14] = 0;

                                    f32a[15] = 0;
                                    f32a[16] = 1;
                                    f32a[17] = 0;

                                    attribute.count = vertexCount
                                    return f32a;

                                }
                                return createCube(vertexNormal2)
                            }
                        }
                    }
                }
            },

            PhongMaterial {
                id: vertexMaterial2
                //环境光
                ambient:Constants.cube3DAmbientColor
                //漫反射光
                diffuse: "#C8C8C8"
                //镜面高光
                specular: "#010101"
                //高光半径
                shininess:5
            },

            ObjectPicker{
                id: vertexSpherePicker2
                hoverEnabled: true
                dragEnabled: true
                onClicked:{
                    if (pick.button === PickEvent.LeftButton)
                    {
                        console.log("~~~~~~~~~~~~~~~~~~")
                        console.log("vertexSpherePicker2 at:triangleIndex =  " + pick.triangleIndex)
                        changeShowPos(1, 2)
                    }
                }
                onEntered: {
                    console.log("vertexSpherePicker2 onEntered")
                    vertexMaterial2.ambient = Constants.cube3DAmbientColor_Entered
                    vertexMaterial2.diffuse = Constants.cube3DAmbientColor_Entered
                    vertexMaterial2.specular = Constants.cube3DAmbientColor_Entered
                }
                onExited: {
                    console.log("vertexSpherePicker2 onExited")
                    vertexMaterial2.ambient = Constants.cube3DAmbientColor
                    vertexMaterial2.diffuse = "#C8C8C8"
                    vertexMaterial2.specular = "#010101"
                }
            }
        ]
    }

    Transform {
            id: cuboidTransform2_2
            translation:Qt.vector3d(0, 7, 0)
            rotation: fromAxisAndAngle(Qt.vector3d(1, 0 , 0), -90)
        }
    Entity {
        id: vertexEntity2_2
        components: [ vertexGeometryRenderer2, vertexMaterial2, vertexSpherePicker2, cuboidTransform2_2 ]
    }

    Transform {
            id: cuboidTransform2_3
            translation:Qt.vector3d(0, 7, 0)
            rotation: fromAxisAndAngle(Qt.vector3d(0, 0 , 1), 90)
        }
    Entity {
        id: vertexEntity2_3
        components: [ vertexGeometryRenderer2, vertexMaterial2, vertexSpherePicker2, cuboidTransform2_3 ]
    }
    //尖角2 end

    //尖角3 start
    Entity {
        id: vertexEntity3
        components: [
            GeometryRenderer {
                id: vertexGeometryRenderer3
                // 利用GeometryRenderer提供的基本类型(line)来进行几何图形的绘制
                primitiveType: GeometryRenderer.Triangles//GeometryRenderer.Lines //GeometryRenderer.TriangleStrip //
                // 几何数据的坐标
                geometry: Geometry {
                    Attribute {
                        id: vertexPosition3
                        attributeType: Attribute.VertexAttribute
                        vertexBaseType: Attribute.Float // 定义顶点的数据类型
                        vertexSize: 3 // 顶点大小
                        count: 0
                        name: defaultPositionAttributeName
                        buffer: Buffer {
                            type: Buffer.VertexBuffer
                            data: {
                                function createCube(attribute)
                                {
                                    var vertexCount = 2*3;
                                    var f32a = new Float32Array(vertexCount*3);
                                    //9
                                    f32a[0] = 3;
                                    f32a[1] = 4;
                                    f32a[2] = -3;

                                    f32a[3] = 4;
                                    f32a[4] = 4;
                                    f32a[5] = -4;

                                    f32a[6] = 3;
                                    f32a[7] = 4;
                                    f32a[8] = -4;

                                    //10
                                    f32a[9] = 3;
                                    f32a[10] = 4;
                                    f32a[11] = -3;

                                    f32a[12] = 4;
                                    f32a[13] = 4;
                                    f32a[14] = -3;

                                    f32a[15] = 4;
                                    f32a[16] = 4;
                                    f32a[17] = -4;

                                    attribute.count = vertexCount
                                    return f32a;

                                }
                                return createCube(vertexPosition3)
                            }
                        }
                    }
                    Attribute {
                        id: vertexNormal3
                        attributeType: Attribute.VertexAttribute
                        vertexBaseType: Attribute.Float // 定义顶点的数据类型
                        vertexSize: 3 // 顶点大小
                        count: 0
                        name: defaultNormalAttributeName
                        buffer: Buffer {
                            type: Buffer.VertexBuffer
                            data: {
                                function createCube(attribute)
                                {
                                    var vertexCount = 2*3;
                                    var f32a = new Float32Array(vertexCount*3);
                                    //3
                                    f32a[0] = 0;
                                    f32a[1] = 1;
                                    f32a[2] = 0;

                                    f32a[3] = 0;
                                    f32a[4] = 1;
                                    f32a[5] = 0;

                                    f32a[6] = 0;
                                    f32a[7] = 1;
                                    f32a[8] = 0;

                                    //4
                                    f32a[9] = 0;
                                    f32a[10] = 1;
                                    f32a[11] = 0;

                                    f32a[12] = 0;
                                    f32a[13] = 1;
                                    f32a[14] = 0;

                                    f32a[15] = 0;
                                    f32a[16] = 1;
                                    f32a[17] = 0;

                                    attribute.count = vertexCount
                                    return f32a;

                                }
                                return createCube(vertexNormal3)
                            }
                        }
                    }
                }
            },

            PhongMaterial {
                id: vertexMaterial3
                //环境光
                ambient:Constants.cube3DAmbientColor
                //漫反射光
                diffuse: "#C8C8C8"
                //镜面高光
                specular: "#010101"
                //高光半径
                shininess:5
            },

            ObjectPicker{
                id: vertexSpherePicker3
                hoverEnabled: true
                dragEnabled: true
                onClicked:{
                    if (pick.button === PickEvent.LeftButton)
                    {
                        console.log("~~~~~~~~~~~~~~~~~~")
                        console.log("vertexSpherePicker3 at:triangleIndex =  " + pick.triangleIndex)
                        changeShowPos(1, 3)
                    }
                }
                onEntered: {
                    //console.log("vertexSpherePicker3 onEntered")
                    vertexMaterial3.ambient = Constants.cube3DAmbientColor_Entered
                    vertexMaterial3.diffuse = Constants.cube3DAmbientColor_Entered
                    vertexMaterial3.specular = Constants.cube3DAmbientColor_Entered
                }
                onExited: {
                    //console.log("vertexSpherePicker3 onExited")
                    vertexMaterial3.ambient = Constants.cube3DAmbientColor
                    vertexMaterial3.diffuse = "#C8C8C8"
                    vertexMaterial3.specular = "#010101"
                }
            }
        ]
    }

    Transform {
            id: cuboidTransform3_2
            translation:Qt.vector3d(0, 7, 0)
            rotation: fromAxisAndAngle(Qt.vector3d(1, 0 , 0), -90)
        }
    Entity {
        id: vertexEntity3_2
        components: [ vertexGeometryRenderer3, vertexMaterial3, vertexSpherePicker3, cuboidTransform3_2 ]
    }

    Transform {
            id: cuboidTransform3_3
            translation:Qt.vector3d(0, 7, 0)
            rotation: fromAxisAndAngle(Qt.vector3d(0, 0 , 1), -90)
        }
    Entity {
        id: vertexEntity3_3
        components: [ vertexGeometryRenderer3, vertexMaterial3, vertexSpherePicker3, cuboidTransform3_3 ]
    }
    //尖角3 end

    //尖角4 start
    Entity {
        id: vertexEntity4
        components: [
            GeometryRenderer {
                id: vertexGeometryRenderer4
                // 利用GeometryRenderer提供的基本类型(line)来进行几何图形的绘制
                primitiveType: GeometryRenderer.Triangles//GeometryRenderer.Lines //GeometryRenderer.TriangleStrip //
                // 几何数据的坐标
                geometry: Geometry {
                    Attribute {
                        id: vertexPosition4
                        attributeType: Attribute.VertexAttribute
                        vertexBaseType: Attribute.Float // 定义顶点的数据类型
                        vertexSize: 3 // 顶点大小
                        count: 0
                        name: defaultPositionAttributeName
                        buffer: Buffer {
                            type: Buffer.VertexBuffer
                            data: {
                                function createCube(attribute)
                                {
                                    var vertexCount = 2*3;
                                    var f32a = new Float32Array(vertexCount*3);
                                    //13
                                    f32a[0] = 3;
                                    f32a[1] = 4;
                                    f32a[2] = 4;

                                    f32a[3] = 4;
                                    f32a[4] = 4;
                                    f32a[5] = 3;

                                    f32a[6] = 3;
                                    f32a[7] = 4;
                                    f32a[8] = 3;

                                    //14
                                    f32a[9] = 3;
                                    f32a[10] = 4;
                                    f32a[11] = 4;

                                    f32a[12] = 4;
                                    f32a[13] = 4;
                                    f32a[14] = 4;

                                    f32a[15] = 4;
                                    f32a[16] = 4;
                                    f32a[17] = 3;

                                    attribute.count = vertexCount
                                    return f32a;

                                }
                                return createCube(vertexPosition4)
                            }
                        }
                    }
                    Attribute {
                        id: vertexNormal4
                        attributeType: Attribute.VertexAttribute
                        vertexBaseType: Attribute.Float // 定义顶点的数据类型
                        vertexSize: 3 // 顶点大小
                        count: 0
                        name: defaultNormalAttributeName
                        buffer: Buffer {
                            type: Buffer.VertexBuffer
                            data: {
                                function createCube(attribute)
                                {
                                    var vertexCount = 2*3;
                                    var f32a = new Float32Array(vertexCount*3);
                                    //3
                                    f32a[0] = 0;
                                    f32a[1] = 1;
                                    f32a[2] = 0;

                                    f32a[3] = 0;
                                    f32a[4] = 1;
                                    f32a[5] = 0;

                                    f32a[6] = 0;
                                    f32a[7] = 1;
                                    f32a[8] = 0;

                                    //4
                                    f32a[9] = 0;
                                    f32a[10] = 1;
                                    f32a[11] = 0;

                                    f32a[12] = 0;
                                    f32a[13] = 1;
                                    f32a[14] = 0;

                                    f32a[15] = 0;
                                    f32a[16] = 1;
                                    f32a[17] = 0;

                                    attribute.count = vertexCount
                                    return f32a;

                                }
                                return createCube(vertexNormal4)
                            }
                        }
                    }
                }
            },

            PhongMaterial {
                id: vertexMaterial4
                //环境光
                ambient:Constants.cube3DAmbientColor
                //漫反射光
                diffuse: "#C8C8C8"
                //镜面高光
                specular: "#010101"
                //高光半径
                shininess:5
            },

            ObjectPicker{
                id: vertexSpherePicker4
                hoverEnabled: true
                dragEnabled: true
                onClicked:{
                    if (pick.button === PickEvent.LeftButton)
                    {
                        console.log("~~~~~~~~~~~~~~~~~~")
                        console.log("vertexSpherePicker4 at:triangleIndex =  " + pick.triangleIndex)
                        changeShowPos(1, 4)
                    }
                }
                onEntered: {
                    //console.log("vertexSpherePicker4 onEntered")
                    vertexMaterial4.ambient = Constants.cube3DAmbientColor_Entered
                    vertexMaterial4.diffuse = Constants.cube3DAmbientColor_Entered
                    vertexMaterial4.specular = Constants.cube3DAmbientColor_Entered
                }
                onExited: {
                    //console.log("vertexSpherePicker4 onExited")
                    vertexMaterial4.ambient = Constants.cube3DAmbientColor
                    vertexMaterial4.diffuse = "#C8C8C8"
                    vertexMaterial4.specular = "#010101"
                }
            }
        ]
    }

    Transform {
            id: cuboidTransform4_2
            translation:Qt.vector3d(0, 7, 0)
            rotation: fromAxisAndAngle(Qt.vector3d(1, 0 , 0), 90)
        }
    Entity {
        id: vertexEntity4_2
        components: [ vertexGeometryRenderer4, vertexMaterial4, vertexSpherePicker4, cuboidTransform4_2 ]
    }

    Transform {
            id: cuboidTransform4_3
            translation:Qt.vector3d(0, 7, 0)
            rotation: fromAxisAndAngle(Qt.vector3d(0, 0 , 1), -90)
        }
    Entity {
        id: vertexEntity4_3
        components: [ vertexGeometryRenderer4, vertexMaterial4, vertexSpherePicker4, cuboidTransform4_3 ]
    }
    //尖角4 end

    //尖角5 start
    Entity {
        id: vertexEntity5
        components: [
            PhongMaterial {
                id: vertexMaterial5
                //环境光
                ambient:Constants.cube3DAmbientColor
                //漫反射光
                diffuse: "#C8C8C8"
                //镜面高光
                specular: "#010101"
                //高光半径
                shininess:5
            },

            ObjectPicker{
                id: vertexSpherePicker5
                hoverEnabled: true
                dragEnabled: true
                onClicked:{
                    if (pick.button === PickEvent.LeftButton)
                    {
                        console.log("~~~~~~~~~~~~~~~~~~")
                        console.log("vertexSpherePicker5 at:triangleIndex =  " + pick.triangleIndex)
                        changeShowPos(1, 5)
                    }
                }
                onEntered: {
                    vertexMaterial5.ambient = Constants.cube3DAmbientColor_Entered
                    vertexMaterial5.diffuse = Constants.cube3DAmbientColor_Entered
                    vertexMaterial5.specular = Constants.cube3DAmbientColor_Entered
                }
                onExited: {
                    vertexMaterial5.ambient = Constants.cube3DAmbientColor
                    vertexMaterial5.diffuse = "#C8C8C8"
                    vertexMaterial5.specular = "#010101"
                }

            },

            GeometryRenderer {
                id: vertexGeometryRenderer5
                // 利用GeometryRenderer提供的基本类型(line)来进行几何图形的绘制
                primitiveType: GeometryRenderer.Triangles//GeometryRenderer.Lines //GeometryRenderer.TriangleStrip //
                // 几何数据的坐标
                geometry: Geometry {
                    Attribute {
                        id: vertexPosition5
                        attributeType: Attribute.VertexAttribute
                        vertexBaseType: Attribute.Float // 定义顶点的数据类型
                        vertexSize: 3 // 顶点大小
                        count: 0
                        name: defaultPositionAttributeName
                        buffer: Buffer {
                            type: Buffer.VertexBuffer
                            data: {
                                function createCube(attribute)
                                {
                                    var vertexCount = 2*3;
                                    var f32a = new Float32Array(vertexCount*3);
                                    //1
                                    f32a[0] = -3;
                                    f32a[1] = -4;
                                    f32a[2] = 3;

                                    f32a[3] = -3;
                                    f32a[4] = -4;
                                    f32a[5] = 4;

                                    f32a[6] = -4;
                                    f32a[7] = -4;
                                    f32a[8] = 4;

                                    //2
                                    f32a[9] = -4;
                                    f32a[10] = -4;
                                    f32a[11] = 3;

                                    f32a[12] = -3;
                                    f32a[13] = -4;
                                    f32a[14] = 3;

                                    f32a[15] = -4;
                                    f32a[16] = -4;
                                    f32a[17] = 4;

                                    attribute.count = vertexCount
                                    return f32a;

                                }
                                return createCube(vertexPosition5)
                            }
                        }
                    }
                    Attribute {
                        id: vertexNormal5
                        attributeType: Attribute.VertexAttribute
                        vertexBaseType: Attribute.Float // 定义顶点的数据类型
                        vertexSize: 3 // 顶点大小
                        count: 0
                        name: defaultNormalAttributeName
                        buffer: Buffer {
                            type: Buffer.VertexBuffer
                            data: {
                                function createCube(attribute)
                                {
                                    var vertexCount = 2*3;
                                    var f32a = new Float32Array(vertexCount*3);
                                    //3
                                    f32a[0] = 0;
                                    f32a[1] = -1;
                                    f32a[2] = 0;

                                    f32a[3] = 0;
                                    f32a[4] = -1;
                                    f32a[5] = 0;

                                    f32a[6] = 0;
                                    f32a[7] = -1;
                                    f32a[8] = 0;

                                    //4
                                    f32a[9] = 0;
                                    f32a[10] = -1;
                                    f32a[11] = 0;

                                    f32a[12] = 0;
                                    f32a[13] = -1;
                                    f32a[14] = 0;

                                    f32a[15] = 0;
                                    f32a[16] = -1;
                                    f32a[17] = 0;

                                    attribute.count = vertexCount
                                    return f32a;

                                }
                                return createCube(vertexNormal5)
                            }
                        }
                    }
                }
            }
        ]
    }


    Transform {
            id: cuboidTransform5_2
            translation:Qt.vector3d(0, -7, 0)
            rotation: fromAxisAndAngle(Qt.vector3d(1, 0 , 0), 270)
        }
    Entity {
        id: vertexEntity5_2
        components: [ vertexGeometryRenderer5, vertexMaterial5, vertexSpherePicker5, cuboidTransform5_2 ]
    }

    Transform {
            id: cuboidTransform5_3
            translation:Qt.vector3d(0, -7, 0)
            rotation: fromAxisAndAngle(Qt.vector3d(0, 0 , 1), 270)
        }
    Entity {
        id: vertexEntity5_3
        components: [ vertexGeometryRenderer5, vertexMaterial5, vertexSpherePicker5, cuboidTransform5_3 ]
    }
    //尖角5 end

    //尖角6 start
    Entity {
        id: vertexEntity6
        components: [
            GeometryRenderer {
                id: vertexGeometryRenderer6
                // 利用GeometryRenderer提供的基本类型(line)来进行几何图形的绘制
                primitiveType: GeometryRenderer.Triangles//GeometryRenderer.Lines //GeometryRenderer.TriangleStrip //
                // 几何数据的坐标
                geometry: Geometry {
                    Attribute {
                        id: vertexPosition6
                        attributeType: Attribute.VertexAttribute
                        vertexBaseType: Attribute.Float // 定义顶点的数据类型
                        vertexSize: 3 // 顶点大小
                        count: 0
                        name: defaultPositionAttributeName
                        buffer: Buffer {
                            type: Buffer.VertexBuffer
                            data: {
                                function createCube(attribute)
                                {
                                    var vertexCount = 2*3;
                                    var f32a = new Float32Array(vertexCount*3);
                                    //5
                                    f32a[0] = -3;
                                    f32a[1] = -4;
                                    f32a[2] = -4;

                                    f32a[3] = -3;
                                    f32a[4] = -4;
                                    f32a[5] = -3;

                                    f32a[6] = -4;
                                    f32a[7] = -4;
                                    f32a[8] = -3;

                                    //6
                                    f32a[9] = -4;
                                    f32a[10] = -4;
                                    f32a[11] = -4;

                                    f32a[12] = -3;
                                    f32a[13] = -4;
                                    f32a[14] = -4;

                                    f32a[15] = -4;
                                    f32a[16] = -4;
                                    f32a[17] = -3;

                                    attribute.count = vertexCount
                                    return f32a;

                                }
                                return createCube(vertexPosition6)
                            }
                        }
                    }
                    Attribute {
                        id: vertexNormal6
                        attributeType: Attribute.VertexAttribute
                        vertexBaseType: Attribute.Float // 定义顶点的数据类型
                        vertexSize: 3 // 顶点大小
                        count: 0
                        name: defaultNormalAttributeName
                        buffer: Buffer {
                            type: Buffer.VertexBuffer
                            data: {
                                function createCube(attribute)
                                {
                                    var vertexCount = 2*3;
                                    var f32a = new Float32Array(vertexCount*3);
                                    //3
                                    f32a[0] = 0;
                                    f32a[1] = -1;
                                    f32a[2] = 0;

                                    f32a[3] = 0;
                                    f32a[4] = -1;
                                    f32a[5] = 0;

                                    f32a[6] = 0;
                                    f32a[7] = -1;
                                    f32a[8] = 0;

                                    //4
                                    f32a[9] = 0;
                                    f32a[10] = -1;
                                    f32a[11] = 0;

                                    f32a[12] = 0;
                                    f32a[13] = -1;
                                    f32a[14] = 0;

                                    f32a[15] = 0;
                                    f32a[16] = -1;
                                    f32a[17] = 0;

                                    attribute.count = vertexCount
                                    return f32a;

                                }
                                return createCube(vertexNormal6)
                            }
                        }
                    }
                }
            },

            PhongMaterial {
                id: vertexMaterial6
                //环境光
                ambient:Constants.cube3DAmbientColor
                //漫反射光
                diffuse: "#C8C8C8"
                //镜面高光
                specular: "#010101"
                //高光半径
                shininess:5
            },

            ObjectPicker{
                id: vertexSpherePicker6
                hoverEnabled: true
                dragEnabled: true
                onClicked:{
                    if (pick.button === PickEvent.LeftButton)
                    {
                        console.log("~~~~~~~~~~~~~~~~~~")
                        console.log("vertexSpherePicker2 at:triangleIndex =  " + pick.triangleIndex)
                        changeShowPos(1, 6)
                    }
                }
                onEntered: {
                    vertexMaterial6.ambient = Constants.cube3DAmbientColor_Entered
                    vertexMaterial6.diffuse = Constants.cube3DAmbientColor_Entered
                    vertexMaterial6.specular = Constants.cube3DAmbientColor_Entered
                }
                onExited: {
                    vertexMaterial6.ambient = Constants.cube3DAmbientColor
                    vertexMaterial6.diffuse = "#C8C8C8"
                    vertexMaterial6.specular = "#010101"
                }
            }
        ]
    }

    Transform {
            id: cuboidTransform6_2
            translation:Qt.vector3d(0, -7, 0)
            rotation: fromAxisAndAngle(Qt.vector3d(1, 0 , 0), -270)
        }
    Entity {
        id: vertexEntity6_2
        components: [ vertexGeometryRenderer6, vertexMaterial6, vertexSpherePicker6, cuboidTransform6_2 ]
    }

    Transform {
            id: cuboidTransform6_3
            translation:Qt.vector3d(0, -7, 0)
            rotation: fromAxisAndAngle(Qt.vector3d(0, 0 , 1), 270)
        }
    Entity {
        id: vertexEntity6_3
        components: [ vertexGeometryRenderer6, vertexMaterial6, vertexSpherePicker6, cuboidTransform6_3 ]
    }
    //尖角6 end

    //尖角7 start
    Entity {
        id: vertexEntity7
        components: [
            GeometryRenderer {
                id: vertexGeometryRenderer7
                // 利用GeometryRenderer提供的基本类型(line)来进行几何图形的绘制
                primitiveType: GeometryRenderer.Triangles//GeometryRenderer.Lines //GeometryRenderer.TriangleStrip //
                // 几何数据的坐标
                geometry: Geometry {
                    Attribute {
                        id: vertexPosition7
                        attributeType: Attribute.VertexAttribute
                        vertexBaseType: Attribute.Float // 定义顶点的数据类型
                        vertexSize: 3 // 顶点大小
                        count: 0
                        name: defaultPositionAttributeName
                        buffer: Buffer {
                            type: Buffer.VertexBuffer
                            data: {
                                function createCube(attribute)
                                {
                                    var vertexCount = 2*3;
                                    var f32a = new Float32Array(vertexCount*3);
                                    //9
                                    f32a[0] = 4;
                                    f32a[1] = -4;
                                    f32a[2] = -4;

                                    f32a[3] = 4;
                                    f32a[4] = -4;
                                    f32a[5] = -3;

                                    f32a[6] = 3;
                                    f32a[7] = -4;
                                    f32a[8] = -3;

                                    //10
                                    f32a[9] = 3;
                                    f32a[10] = -4;
                                    f32a[11] = -4;

                                    f32a[12] = 4;
                                    f32a[13] = -4;
                                    f32a[14] = -4;

                                    f32a[15] = 3;
                                    f32a[16] = -4;
                                    f32a[17] = -3;

                                    attribute.count = vertexCount
                                    return f32a;

                                }
                                return createCube(vertexPosition7)
                            }
                        }
                    }
                    Attribute {
                        id: vertexNormal7
                        attributeType: Attribute.VertexAttribute
                        vertexBaseType: Attribute.Float // 定义顶点的数据类型
                        vertexSize: 3 // 顶点大小
                        count: 0
                        name: defaultNormalAttributeName
                        buffer: Buffer {
                            type: Buffer.VertexBuffer
                            data: {
                                function createCube(attribute)
                                {
                                    var vertexCount = 2*3;
                                    var f32a = new Float32Array(vertexCount*3);
                                    //3
                                    f32a[0] = 0;
                                    f32a[1] = -1;
                                    f32a[2] = 0;

                                    f32a[3] = 0;
                                    f32a[4] = -1;
                                    f32a[5] = 0;

                                    f32a[6] = 0;
                                    f32a[7] = -1;
                                    f32a[8] = 0;

                                    //4
                                    f32a[9] = 0;
                                    f32a[10] = -1;
                                    f32a[11] = 0;

                                    f32a[12] = 0;
                                    f32a[13] = -1;
                                    f32a[14] = 0;

                                    f32a[15] = 0;
                                    f32a[16] = -1;
                                    f32a[17] = 0;

                                    attribute.count = vertexCount
                                    return f32a;

                                }
                                return createCube(vertexNormal7)
                            }
                        }
                    }
                }
            },

            PhongMaterial {
                id: vertexMaterial7
                //环境光
                ambient:Constants.cube3DAmbientColor
                //漫反射光
                diffuse: "#C8C8C8"
                //镜面高光
                specular: "#010101"
                //高光半径
                shininess:5
            },

            ObjectPicker{
                id: vertexSpherePicker7
                hoverEnabled: true
                dragEnabled: true
                onClicked:{
                    if (pick.button === PickEvent.LeftButton)
                    {
                        console.log("~~~~~~~~~~~~~~~~~~")
                        console.log("vertexSpherePicker7 at:triangleIndex =  " + pick.triangleIndex)
                        changeShowPos(1, 7)
                    }
                }
                onEntered: {
                    vertexMaterial7.ambient = Constants.cube3DAmbientColor_Entered
                    vertexMaterial7.diffuse = Constants.cube3DAmbientColor_Entered
                    vertexMaterial7.specular = Constants.cube3DAmbientColor_Entered
                }
                onExited: {
                    vertexMaterial7.ambient = Constants.cube3DAmbientColor
                    vertexMaterial7.diffuse = "#C8C8C8"
                    vertexMaterial7.specular = "#010101"
                }
            }
        ]
    }

    Transform {
            id: cuboidTransform7_2
            translation:Qt.vector3d(0, -7, 0)
            rotation: fromAxisAndAngle(Qt.vector3d(1, 0 , 0), -270)
        }
    Entity {
        id: vertexEntity7_2
        components: [ vertexGeometryRenderer7, vertexMaterial7, vertexSpherePicker7, cuboidTransform7_2 ]
    }

    Transform {
            id: cuboidTransform7_3
            translation:Qt.vector3d(0, -7, 0)
            rotation: fromAxisAndAngle(Qt.vector3d(0, 0 , 1), -270)
        }
    Entity {
        id: vertexEntity7_3
        components: [ vertexGeometryRenderer7, vertexMaterial7, vertexSpherePicker7, cuboidTransform7_3 ]
    }
    //尖角7 end

    //尖角8 start
    Entity {
        id: vertexEntity8
        components: [
            GeometryRenderer {
                id: vertexGeometryRenderer8
                // 利用GeometryRenderer提供的基本类型(line)来进行几何图形的绘制
                primitiveType: GeometryRenderer.Triangles//GeometryRenderer.Lines //GeometryRenderer.TriangleStrip //
                // 几何数据的坐标
                geometry: Geometry {
                    Attribute {
                        id: vertexPosition8
                        attributeType: Attribute.VertexAttribute
                        vertexBaseType: Attribute.Float // 定义顶点的数据类型
                        vertexSize: 3 // 顶点大小
                        count: 0
                        name: defaultPositionAttributeName
                        buffer: Buffer {
                            type: Buffer.VertexBuffer
                            data: {
                                function createCube(attribute)
                                {
                                    var vertexCount = 2*3;
                                    var f32a = new Float32Array(vertexCount*3);
                                    //13
                                    f32a[0] = 4;
                                    f32a[1] = -4;
                                    f32a[2] = 3;

                                    f32a[3] = 4;
                                    f32a[4] = -4;
                                    f32a[5] = 4;

                                    f32a[6] = 3;
                                    f32a[7] = -4;
                                    f32a[8] = 4;

                                    //14
                                    f32a[9] = 3;
                                    f32a[10] = -4;
                                    f32a[11] = 3;

                                    f32a[12] = 4;
                                    f32a[13] = -4;
                                    f32a[14] = 3;

                                    f32a[15] = 3;
                                    f32a[16] = -4;
                                    f32a[17] = 4;

                                    attribute.count = vertexCount
                                    return f32a;

                                }
                                return createCube(vertexPosition8)
                            }
                        }
                    }
                    Attribute {
                        id: vertexNormal8
                        attributeType: Attribute.VertexAttribute
                        vertexBaseType: Attribute.Float // 定义顶点的数据类型
                        vertexSize: 3 // 顶点大小
                        count: 0
                        name: defaultNormalAttributeName
                        buffer: Buffer {
                            type: Buffer.VertexBuffer
                            data: {
                                function createCube(attribute)
                                {
                                    var vertexCount = 2*3;
                                    var f32a = new Float32Array(vertexCount*3);
                                    //3
                                    f32a[0] = 0;
                                    f32a[1] = -1;
                                    f32a[2] = 0;

                                    f32a[3] = 0;
                                    f32a[4] = -1;
                                    f32a[5] = 0;

                                    f32a[6] = 0;
                                    f32a[7] = -1;
                                    f32a[8] = 0;

                                    //4
                                    f32a[9] = 0;
                                    f32a[10] = -1;
                                    f32a[11] = 0;

                                    f32a[12] = 0;
                                    f32a[13] = -1;
                                    f32a[14] = 0;

                                    f32a[15] = 0;
                                    f32a[16] = -1;
                                    f32a[17] = 0;

                                    attribute.count = vertexCount
                                    return f32a;

                                }
                                return createCube(vertexNormal8)
                            }
                        }
                    }
                }
            },

            PhongMaterial {
                id: vertexMaterial8
                //环境光
                ambient:Constants.cube3DAmbientColor
                //漫反射光
                diffuse: "#C8C8C8"
                //镜面高光
                specular: "#010101"
                //高光半径
                shininess:5
            },

            ObjectPicker{
                id: vertexSpherePicker8
                hoverEnabled: true
                dragEnabled: true
                onClicked:{
                    if (pick.button === PickEvent.LeftButton)
                    {
                        console.log("~~~~~~~~~~~~~~~~~~")
                        console.log("vertexSpherePicker8 at:triangleIndex =  " + pick.triangleIndex)
                        changeShowPos(1, 8)
                    }
                }
                onEntered: {
                    vertexMaterial8.ambient = Constants.cube3DAmbientColor_Entered
                    vertexMaterial8.diffuse = Constants.cube3DAmbientColor_Entered
                    vertexMaterial8.specular = Constants.cube3DAmbientColor_Entered
                }
                onExited: {
                    vertexMaterial8.ambient = Constants.cube3DAmbientColor
                    vertexMaterial8.diffuse = "#C8C8C8"
                    vertexMaterial8.specular = "#010101"
                }
            }
        ]
    }

    Transform {
            id: cuboidTransform8_2
            translation:Qt.vector3d(0, -7, 0)
            rotation: fromAxisAndAngle(Qt.vector3d(1, 0 , 0), 270)
        }
    Entity {
        id: vertexEntity8_2
        components: [ vertexGeometryRenderer8, vertexMaterial8, vertexSpherePicker8, cuboidTransform8_2 ]
    }

    Transform {
            id: cuboidTransform8_3
            translation:Qt.vector3d(0, -7, 0)
            rotation: fromAxisAndAngle(Qt.vector3d(0, 0 , 1), -270)
        }
    Entity {
        id: vertexEntity8_3
        components: [ vertexGeometryRenderer8, vertexMaterial8, vertexSpherePicker8, cuboidTransform8_3 ]
    }
    //尖角8 end

    //边1 start
    Entity {
        id: vertexEntityEdge1
        components: [
            PhongMaterial {
                id: vertexMaterialEdge1
                //环境光
                ambient:Constants.cube3DAmbientColor
                //漫反射光
                diffuse: "#C8C8C8"
                //镜面高光
                specular: "#010101"
                //高光半径
                shininess:5
            },

            ObjectPicker{
                id: vertexSpherePickerEdge1
                hoverEnabled: true
                dragEnabled: true
                onClicked:{
                    if (pick.button === PickEvent.LeftButton)
                    {
                        console.log("~~~~~~~~~~~~~~~~~~")
                        console.log("vertexSpherePickerEdge1 at:triangleIndex =  " + pick.triangleIndex)
                        changeShowPos(2, 1)
                    }
                }
                onEntered: {
                    vertexMaterialEdge1.ambient = Constants.cube3DAmbientColor_Entered
                    vertexMaterialEdge1.diffuse = Constants.cube3DAmbientColor_Entered
                    vertexMaterialEdge1.specular = Constants.cube3DAmbientColor_Entered
                }
                onExited: {
                    vertexMaterialEdge1.ambient = Constants.cube3DAmbientColor
                    vertexMaterialEdge1.diffuse = "#C8C8C8"
                    vertexMaterialEdge1.specular = "#010101"
                }

            },

            GeometryRenderer {
                id: vertexGeometryRendererEdge1
                // 利用GeometryRenderer提供的基本类型(line)来进行几何图形的绘制
                primitiveType: GeometryRenderer.Triangles//GeometryRenderer.Lines //GeometryRenderer.TriangleStrip //
                // 几何数据的坐标
                geometry: Geometry {
                    Attribute {
                        id: vertexPositionEdge1
                        attributeType: Attribute.VertexAttribute
                        vertexBaseType: Attribute.Float // 定义顶点的数据类型
                        vertexSize: 3 // 顶点大小
                        count: 0
                        name: defaultPositionAttributeName
                        buffer: Buffer {
                            type: Buffer.VertexBuffer
                            data: {
                                function createCube(attribute)
                                {
                                    var vertexCount = 2*3;
                                    var f32a = new Float32Array(vertexCount*3);
                                    //3
                                    f32a[0] = -4;
                                    f32a[1] = 4;
                                    f32a[2] = 3;

                                    f32a[3] = -3;
                                    f32a[4] = 4;
                                    f32a[5] = -3;

                                    f32a[6] = -4;
                                    f32a[7] = 4;
                                    f32a[8] = -3;

                                    //4
                                    f32a[9] = -4;
                                    f32a[10] = 4;
                                    f32a[11] = 3;

                                    f32a[12] = -3;
                                    f32a[13] = 4;
                                    f32a[14] = 3;

                                    f32a[15] = -3;
                                    f32a[16] = 4;
                                    f32a[17] = -3;

                                    attribute.count = vertexCount
                                    return f32a;

                                }
                                return createCube(vertexPositionEdge1)
                            }
                        }
                    }

                    Attribute {
                        id: vertexNormalEdge1
                        attributeType: Attribute.VertexAttribute
                        vertexBaseType: Attribute.Float // 定义顶点的数据类型
                        vertexSize: 3 // 顶点大小
                        count: 0
                        name: defaultNormalAttributeName
                        buffer: Buffer {
                            type: Buffer.VertexBuffer
                            data: {
                                function createCube(attribute)
                                {
                                    var vertexCount = 2*3;
                                    var f32a = new Float32Array(vertexCount*3);
                                    //3
                                    f32a[0] = 0;
                                    f32a[1] = 1;
                                    f32a[2] = 0;

                                    f32a[3] = 0;
                                    f32a[4] = 1;
                                    f32a[5] = 0;

                                    f32a[6] = 0;
                                    f32a[7] = 1;
                                    f32a[8] = 0;

                                    //4
                                    f32a[9] = 0;
                                    f32a[10] = 1;
                                    f32a[11] = 0;

                                    f32a[12] = 0;
                                    f32a[13] = 1;
                                    f32a[14] = 0;

                                    f32a[15] = 0;
                                    f32a[16] = 1;
                                    f32a[17] = 0;

                                    attribute.count = vertexCount
                                    return f32a;

                                }
                                return createCube(vertexNormalEdge1)
                            }
                        }
                    }
                }
            }
        ]
    }


    Transform {
            id: cuboidTransformEdge1_2
            translation:Qt.vector3d(0, 7, 0)
            rotation: fromAxisAndAngle(Qt.vector3d(0, 0 , 1), 90)
        }
    Entity {
        id: vertexEntityEdge1_2
        components: [ vertexGeometryRendererEdge1, vertexMaterialEdge1, vertexSpherePickerEdge1, cuboidTransformEdge1_2 ]
    }
    //边1 end

    //边2 start
    PhongMaterial {
        id: vertexMaterialEdge2
        //环境光
        ambient:Constants.cube3DAmbientColor
        //漫反射光
        diffuse: "#C8C8C8"
        //镜面高光
        specular: "#010101"
        //高光半径
        shininess:5
    }

    ObjectPicker{
        id: vertexSpherePickerEdge2
        hoverEnabled: true
        dragEnabled: true
        onClicked:{
            if (pick.button === PickEvent.LeftButton)
            {
                console.log("~~~~~~~~~~~~~~~~~~")
                console.log("vertexSpherePickerEdge3 at:triangleIndex =  " + pick.triangleIndex)
                changeShowPos(2, 2)
            }
        }
        onEntered: {
            vertexMaterialEdge2.ambient = Constants.cube3DAmbientColor_Entered
            vertexMaterialEdge2.diffuse = Constants.cube3DAmbientColor_Entered
            vertexMaterialEdge2.specular = Constants.cube3DAmbientColor_Entered
        }
        onExited: {
            vertexMaterialEdge2.ambient = Constants.cube3DAmbientColor
            vertexMaterialEdge2.diffuse = "#C8C8C8"
            vertexMaterialEdge2.specular = "#010101"
        }

    }
    Transform {
            id: cuboidTransformEdge2
            //translation:Qt.vector3d(0, 7, 0)
            rotation: fromAxisAndAngle(Qt.vector3d(0, 0 , 1), 180)
        }
    Entity {
        id: vertexEntityEdge2
        components: [ vertexGeometryRendererEdge1, vertexMaterialEdge2, vertexSpherePickerEdge2, cuboidTransformEdge2 ]
    }

    Transform {
            id: cuboidTransformEdge2_2
            translation:Qt.vector3d(0, -7, 0)
            rotation: fromAxisAndAngle(Qt.vector3d(0, 0 , 1), 270)
        }
    Entity {
        id: vertexEntityEdge2_2
        components: [ vertexGeometryRendererEdge1, vertexMaterialEdge2, vertexSpherePickerEdge2, cuboidTransformEdge2_2 ]
    }
    //边2 end

    //边3 start
    PhongMaterial {
        id: vertexMaterialEdge3
        //环境光
        ambient:Constants.cube3DAmbientColor
        //漫反射光
        diffuse: "#C8C8C8"
        //镜面高光
        specular: "#010101"
        //高光半径
        shininess:5
    }

    ObjectPicker{
        id: vertexSpherePickerEdge3
        hoverEnabled: true
        dragEnabled: true
        onClicked:{
            if (pick.button === PickEvent.LeftButton)
            {
                console.log("~~~~~~~~~~~~~~~~~~")
                console.log("vertexSpherePickerEdge3 at:triangleIndex =  " + pick.triangleIndex)
                changeShowPos(2, 3)
            }
        }
        onEntered: {
            vertexMaterialEdge3.ambient = Constants.cube3DAmbientColor_Entered
            vertexMaterialEdge3.diffuse = Constants.cube3DAmbientColor_Entered
            vertexMaterialEdge3.specular = Constants.cube3DAmbientColor_Entered
        }
        onExited: {
            vertexMaterialEdge3.ambient = Constants.cube3DAmbientColor
            vertexMaterialEdge3.diffuse = "#C8C8C8"
            vertexMaterialEdge3.specular = "#010101"
        }

    }
    Transform {
            id: cuboidTransformEdge3
            translation:Qt.vector3d(7, 0, 0)
            //rotation: fromAxisAndAngle(Qt.vector3d(0, 0 , 1), 180)
        }
    Entity {
        id: vertexEntityEdge3
        components: [ vertexGeometryRendererEdge1, vertexMaterialEdge3, vertexSpherePickerEdge3, cuboidTransformEdge3 ]
    }

    Transform {
            id: cuboidTransformEdge3_2
            //translation:Qt.vector3d(0, -7, 0)
            rotation: fromAxisAndAngle(Qt.vector3d(0, 0 , 1), 270)
        }
    Entity {
        id: vertexEntityEdge3_2
        components: [ vertexGeometryRendererEdge1, vertexMaterialEdge3, vertexSpherePickerEdge3, cuboidTransformEdge3_2 ]
    }
    //边3 end

    //边4 start
    PhongMaterial {
        id: vertexMaterialEdge4
        //环境光
        ambient:Constants.cube3DAmbientColor
        //漫反射光
        diffuse: "#C8C8C8"
        //镜面高光
        specular: "#010101"
        //高光半径
        shininess:5
    }

    ObjectPicker{
        id: vertexSpherePickerEdge4
        hoverEnabled: true
        dragEnabled: true
        onClicked:{
            if (pick.button === PickEvent.LeftButton)
            {
                console.log("~~~~~~~~~~~~~~~~~~")
                console.log("vertexSpherePickerEdge3 at:triangleIndex =  " + pick.triangleIndex)
                changeShowPos(2, 4)
            }
        }
        onEntered: {
            vertexMaterialEdge4.ambient = Constants.cube3DAmbientColor_Entered
            vertexMaterialEdge4.diffuse = Constants.cube3DAmbientColor_Entered
            vertexMaterialEdge4.specular = Constants.cube3DAmbientColor_Entered
        }
        onExited: {
            vertexMaterialEdge4.ambient = Constants.cube3DAmbientColor
            vertexMaterialEdge4.diffuse = "#C8C8C8"
            vertexMaterialEdge4.specular = "#010101"
        }

    }
    Transform {
            id: cuboidTransformEdge4
            translation:Qt.vector3d(-7, 0, 0)
            rotation: fromAxisAndAngle(Qt.vector3d(0, 0 , 1), 180)
        }
    Entity {
        id: vertexEntityEdge4
        components: [ vertexGeometryRendererEdge1, vertexMaterialEdge4, vertexSpherePickerEdge4, cuboidTransformEdge4 ]
    }

    Transform {
            id: cuboidTransformEdge4_2
            //translation:Qt.vector3d(0, -7, 0)
            rotation: fromAxisAndAngle(Qt.vector3d(0, 0 , 1), 90)
        }
    Entity {
        id: vertexEntityEdge4_2
        components: [ vertexGeometryRendererEdge1, vertexMaterialEdge4, vertexSpherePickerEdge4, cuboidTransformEdge4_2 ]
    }
    //边4 end

    //边5 start
    Entity {
        id: vertexEntityEdge5
        components: [
            PhongMaterial {
                id: vertexMaterialEdge5
                //环境光
                ambient:Constants.cube3DAmbientColor
                //漫反射光
                diffuse: "#C8C8C8"
                //镜面高光
                specular: "#010101"
                //高光半径
                shininess:5
            },

            ObjectPicker{
                id: vertexSpherePickerEdge5
                hoverEnabled: true
                dragEnabled: true
                onClicked:{
                    if (pick.button === PickEvent.LeftButton)
                    {
                        console.log("~~~~~~~~~~~~~~~~~~")
                        console.log("vertexSpherePickerEdge5 at:triangleIndex =  " + pick.triangleIndex)
                        changeShowPos(2, 5)
                    }
                }
                onEntered: {
                    vertexMaterialEdge5.ambient = Constants.cube3DAmbientColor_Entered
                    vertexMaterialEdge5.diffuse = Constants.cube3DAmbientColor_Entered
                    vertexMaterialEdge5.specular = Constants.cube3DAmbientColor_Entered
                }
                onExited: {
                    vertexMaterialEdge5.ambient = Constants.cube3DAmbientColor
                    vertexMaterialEdge5.diffuse = "#C8C8C8"
                    vertexMaterialEdge5.specular = "#010101"
                }

            },

            GeometryRenderer {
                id: vertexGeometryRendererEdge5
                // 利用GeometryRenderer提供的基本类型(line)来进行几何图形的绘制
                primitiveType: GeometryRenderer.Triangles//GeometryRenderer.Lines //GeometryRenderer.TriangleStrip //
                // 几何数据的坐标
                geometry: Geometry {
                    Attribute {
                        id: vertexPositionEdge5
                        attributeType: Attribute.VertexAttribute
                        vertexBaseType: Attribute.Float // 定义顶点的数据类型
                        vertexSize: 3 // 顶点大小
                        count: 0
                        name: defaultPositionAttributeName
                        buffer: Buffer {
                            type: Buffer.VertexBuffer
                            data: {
                                function createCube(attribute)
                                {
                                    var vertexCount = 2*3;
                                    var f32a = new Float32Array(vertexCount*3);
                                    //7
                                    f32a[0] = -3;
                                    f32a[1] = 4;
                                    f32a[2] = -3;

                                    f32a[3] = 3;
                                    f32a[4] = 4;
                                    f32a[5] = -4;

                                    f32a[6] = -3;
                                    f32a[7] = 4;
                                    f32a[8] = -4;

                                    //8
                                    f32a[9] = -3;
                                    f32a[10] = 4;
                                    f32a[11] = -3;

                                    f32a[12] = 3;
                                    f32a[13] = 4;
                                    f32a[14] = -3;

                                    f32a[15] = 3;
                                    f32a[16] = 4;
                                    f32a[17] = -4;

                                    attribute.count = vertexCount
                                    return f32a;

                                }
                                return createCube(vertexPositionEdge5)
                            }
                        }
                    }

                    Attribute {
                        id: vertexNormalEdge5
                        attributeType: Attribute.VertexAttribute
                        vertexBaseType: Attribute.Float // 定义顶点的数据类型
                        vertexSize: 3 // 顶点大小
                        count: 0
                        name: defaultNormalAttributeName
                        buffer: Buffer {
                            type: Buffer.VertexBuffer
                            data: {
                                function createCube(attribute)
                                {
                                    var vertexCount = 2*3;
                                    var f32a = new Float32Array(vertexCount*3);
                                    //7
                                    f32a[0] = 0;
                                    f32a[1] = 1;
                                    f32a[2] = 0;

                                    f32a[3] = 0;
                                    f32a[4] = 1;
                                    f32a[5] = 0;

                                    f32a[6] = 0;
                                    f32a[7] = 1;
                                    f32a[8] = 0;

                                    //8
                                    f32a[9] = 0;
                                    f32a[10] = 1;
                                    f32a[11] = 0;

                                    f32a[12] = 0;
                                    f32a[13] = 1;
                                    f32a[14] = 0;

                                    f32a[15] = 0;
                                    f32a[16] = 1;
                                    f32a[17] = 0;

                                    attribute.count = vertexCount
                                    return f32a;

                                }
                                return createCube(vertexNormalEdge5)
                            }
                        }
                    }
                }
            }
        ]
    }


    Transform {
            id: cuboidTransformEdge5_2
            translation:Qt.vector3d(0, 7, 0)
            rotation: fromAxisAndAngle(Qt.vector3d(1, 0 , 0), 270)
        }
    Entity {
        id: vertexEntityEdge5_2
        components: [ vertexGeometryRendererEdge5, vertexMaterialEdge5, vertexSpherePickerEdge5, cuboidTransformEdge5_2 ]
    }
    //边5 end

    //边6 start
    PhongMaterial {
        id: vertexMaterialEdge6
        //环境光
        ambient:Constants.cube3DAmbientColor
        //漫反射光
        diffuse: "#C8C8C8"
        //镜面高光
        specular: "#010101"
        //高光半径
        shininess:5
    }

    ObjectPicker{
        id: vertexSpherePickerEdge6
        hoverEnabled: true
        dragEnabled: true
        onClicked:{
            if (pick.button === PickEvent.LeftButton)
            {
                console.log("~~~~~~~~~~~~~~~~~~")
                console.log("vertexSpherePickerEdge6 at:triangleIndex =  " + pick.triangleIndex)
                changeShowPos(2, 6)
            }
        }
        onEntered: {
            vertexMaterialEdge6.ambient = Constants.cube3DAmbientColor_Entered
            vertexMaterialEdge6.diffuse = Constants.cube3DAmbientColor_Entered
            vertexMaterialEdge6.specular = Constants.cube3DAmbientColor_Entered
        }
        onExited: {
            vertexMaterialEdge6.ambient = Constants.cube3DAmbientColor
            vertexMaterialEdge6.diffuse = "#C8C8C8"
            vertexMaterialEdge6.specular = "#010101"
        }

    }
    Transform {
            id: cuboidTransformEdge6
            //translation:Qt.vector3d(-7, 0, 0)
            rotation: fromAxisAndAngle(Qt.vector3d(1, 0 , 0), 180)
        }
    Entity {
        id: vertexEntityEdge6
        components: [ vertexGeometryRendererEdge5, vertexMaterialEdge6, vertexSpherePickerEdge6, cuboidTransformEdge6 ]
    }

    Transform {
            id: cuboidTransformEdge6_2
            translation:Qt.vector3d(0, -7, 0)
            rotation: fromAxisAndAngle(Qt.vector3d(1, 0 , 0), 90)
        }
    Entity {
        id: vertexEntityEdge6_2
        components: [ vertexGeometryRendererEdge5, vertexMaterialEdge6, vertexSpherePickerEdge6, cuboidTransformEdge6_2 ]
    }
    //边6 end

    //边7 start
    PhongMaterial {
        id: vertexMaterialEdge7
        //环境光
        ambient:Constants.cube3DAmbientColor
        //漫反射光
        diffuse: "#C8C8C8"
        //镜面高光
        specular: "#010101"
        //高光半径
        shininess:5
    }

    ObjectPicker{
        id: vertexSpherePickerEdge7
        hoverEnabled: true
        dragEnabled: true
        onClicked:{
            if (pick.button === PickEvent.LeftButton)
            {
                console.log("~~~~~~~~~~~~~~~~~~")
                console.log("vertexSpherePickerEdge6 at:triangleIndex =  " + pick.triangleIndex)
                changeShowPos(2, 7)
            }
        }
        onEntered: {
            vertexMaterialEdge7.ambient = Constants.cube3DAmbientColor_Entered
            vertexMaterialEdge7.diffuse = Constants.cube3DAmbientColor_Entered
            vertexMaterialEdge7.specular = Constants.cube3DAmbientColor_Entered
        }
        onExited: {
            vertexMaterialEdge7.ambient = Constants.cube3DAmbientColor
            vertexMaterialEdge7.diffuse = "#C8C8C8"
            vertexMaterialEdge7.specular = "#010101"
        }

    }
    Transform {
            id: cuboidTransformEdge7
            translation:Qt.vector3d(0, 0, -7)
            rotation: fromAxisAndAngle(Qt.vector3d(1, 0 , 0), 180)
        }
    Entity {
        id: vertexEntityEdge7
        components: [ vertexGeometryRendererEdge5, vertexMaterialEdge7, vertexSpherePickerEdge7, cuboidTransformEdge7 ]
    }

    Transform {
            id: cuboidTransformEdge7_2
            //translation:Qt.vector3d(0, -7, 0)
            rotation: fromAxisAndAngle(Qt.vector3d(1, 0 , 0), 270)
        }
    Entity {
        id: vertexEntityEdge7_2
        components: [ vertexGeometryRendererEdge5, vertexMaterialEdge7, vertexSpherePickerEdge7, cuboidTransformEdge7_2 ]
    }
    //边7 end

    //边8 start
    PhongMaterial {
        id: vertexMaterialEdge8
        //环境光
        ambient:Constants.cube3DAmbientColor
        //漫反射光
        diffuse: "#C8C8C8"
        //镜面高光
        specular: "#010101"
        //高光半径
        shininess:5
    }

    ObjectPicker{
        id: vertexSpherePickerEdge8
        hoverEnabled: true
        dragEnabled: true
        onClicked:{
            if (pick.button === PickEvent.LeftButton)
            {
                console.log("~~~~~~~~~~~~~~~~~~")
                console.log("vertexSpherePickerEdge6 at:triangleIndex =  " + pick.triangleIndex)
                changeShowPos(2, 8)
            }
        }
        onEntered: {
            vertexMaterialEdge8.ambient = Constants.cube3DAmbientColor_Entered
            vertexMaterialEdge8.diffuse = Constants.cube3DAmbientColor_Entered
            vertexMaterialEdge8.specular = Constants.cube3DAmbientColor_Entered
        }
        onExited: {
            vertexMaterialEdge8.ambient = Constants.cube3DAmbientColor
            vertexMaterialEdge8.diffuse = "#C8C8C8"
            vertexMaterialEdge8.specular = "#010101"
        }

    }
    Transform {
            id: cuboidTransformEdge8
            translation:Qt.vector3d(0, 0, 7)
            //rotation: fromAxisAndAngle(Qt.vector3d(1, 0 , 0), 180)
        }
    Entity {
        id: vertexEntityEdge8
        components: [ vertexGeometryRendererEdge5, vertexMaterialEdge8, vertexSpherePickerEdge8, cuboidTransformEdge8 ]
    }

    Transform {
            id: cuboidTransformEdge8_2
            //translation:Qt.vector3d(0, -7, 0)
            rotation: fromAxisAndAngle(Qt.vector3d(1, 0 , 0), 90)
        }
    Entity {
        id: vertexEntityEdge8_2
        components: [ vertexGeometryRendererEdge5, vertexMaterialEdge8, vertexSpherePickerEdge8, cuboidTransformEdge8_2 ]
    }
    //边8 end

    //边9 start
    Entity {
        id: vertexEntityEdge9
        components: [
            PhongMaterial {
                id: vertexMaterialEdge9
                //环境光
                ambient:Constants.cube3DAmbientColor
                //漫反射光
                diffuse: "#C8C8C8"
                //镜面高光
                specular: "#010101"
                //高光半径
                shininess:5
            },

            ObjectPicker{
                id: vertexSpherePickerEdge9
                hoverEnabled: true
                dragEnabled: true
                onClicked:{
                    if (pick.button === PickEvent.LeftButton)
                    {
                        console.log("~~~~~~~~~~~~~~~~~~")
                        console.log("vertexSpherePickerEdge9 at:triangleIndex =  " + pick.triangleIndex)
                        changeShowPos(2, 9)
                    }
                }
                onEntered: {
                    vertexMaterialEdge9.ambient = Constants.cube3DAmbientColor_Entered
                    vertexMaterialEdge9.diffuse = Constants.cube3DAmbientColor_Entered
                    vertexMaterialEdge9.specular = Constants.cube3DAmbientColor_Entered
                }
                onExited: {
                    vertexMaterialEdge9.ambient = Constants.cube3DAmbientColor
                    vertexMaterialEdge9.diffuse = "#C8C8C8"
                    vertexMaterialEdge9.specular = "#010101"
                }

            },

            GeometryRenderer {
                id: vertexGeometryRendererEdge9
                // 利用GeometryRenderer提供的基本类型(line)来进行几何图形的绘制
                primitiveType: GeometryRenderer.Triangles//GeometryRenderer.Lines //GeometryRenderer.TriangleStrip //
                // 几何数据的坐标
                geometry: Geometry {
                    Attribute {
                        id: vertexPositionEdge9
                        attributeType: Attribute.VertexAttribute
                        vertexBaseType: Attribute.Float // 定义顶点的数据类型
                        vertexSize: 3 // 顶点大小
                        count: 0
                        name: defaultPositionAttributeName
                        buffer: Buffer {
                            type: Buffer.VertexBuffer
                            data: {
                                function createCube(attribute)
                                {
                                    var vertexCount = 2*3;
                                    var f32a = new Float32Array(vertexCount*3);
                                    //7
                                    f32a[0] = 4;
                                    f32a[1] = -3;
                                    f32a[2] = 4;

                                    f32a[3] = 4;
                                    f32a[4] = 3;
                                    f32a[5] = 4;

                                    f32a[6] = 3;
                                    f32a[7] = 3;
                                    f32a[8] = 4;

                                    //8
                                    f32a[9] = 3;
                                    f32a[10] = -3;
                                    f32a[11] = 4;

                                    f32a[12] = 4;
                                    f32a[13] = -3;
                                    f32a[14] = 4;

                                    f32a[15] = 3;
                                    f32a[16] = 3;
                                    f32a[17] = 4;

                                    attribute.count = vertexCount
                                    return f32a;

                                }
                                return createCube(vertexPositionEdge9)
                            }
                        }
                    }
                    Attribute {
                        id: vertexNormalEdge9
                        attributeType: Attribute.VertexAttribute
                        vertexBaseType: Attribute.Float // 定义顶点的数据类型
                        vertexSize: 3 // 顶点大小
                        count: 0
                        name: defaultNormalAttributeName
                        buffer: Buffer {
                            type: Buffer.VertexBuffer
                            data: {
                                function createCube(attribute)
                                {
                                    var vertexCount = 2*3;
                                    var f32a = new Float32Array(vertexCount*3);
                                    //3
                                    f32a[0] = 0;
                                    f32a[1] = 0;
                                    f32a[2] = 1;

                                    f32a[3] = 0;
                                    f32a[4] = 0;
                                    f32a[5] = 1;

                                    f32a[6] = 0;
                                    f32a[7] = 0;
                                    f32a[8] = 1;

                                    //4
                                    f32a[9] = 0;
                                    f32a[10] = 0;
                                    f32a[11] = 1;

                                    f32a[12] = 0;
                                    f32a[13] = 0;
                                    f32a[14] = 1;

                                    f32a[15] = 0;
                                    f32a[16] = 0;
                                    f32a[17] = 1;

                                    attribute.count = vertexCount
                                    return f32a;

                                }
                                return createCube(vertexNormalEdge9)
                            }
                        }
                    }
                }
            }
        ]
    }


    Transform {
            id: cuboidTransformEdge9_2
            translation:Qt.vector3d(0, 0, 7)
            rotation: fromAxisAndAngle(Qt.vector3d(0, 1 , 0), 90)
        }
    Entity {
        id: vertexEntityEdge9_2
        components: [ vertexGeometryRendererEdge9, vertexMaterialEdge9, vertexSpherePickerEdge9, cuboidTransformEdge9_2 ]
    }
    //边9 end

    //边10 start
    PhongMaterial {
        id: vertexMaterialEdge10
        //环境光
        ambient:Constants.cube3DAmbientColor
        //漫反射光
        diffuse: "#C8C8C8"
        //镜面高光
        specular: "#010101"
        //高光半径
        shininess:5
    }

    ObjectPicker{
        id: vertexSpherePickerEdge10
        hoverEnabled: true
        dragEnabled: true
        onClicked:{
            if (pick.button === PickEvent.LeftButton)
            {
                console.log("~~~~~~~~~~~~~~~~~~")
                console.log("vertexSpherePickerEdge6 at:triangleIndex =  " + pick.triangleIndex)
                changeShowPos(2, 10)
            }
        }
        onEntered: {
            vertexMaterialEdge10.ambient = Constants.cube3DAmbientColor_Entered
            vertexMaterialEdge10.diffuse = Constants.cube3DAmbientColor_Entered
            vertexMaterialEdge10.specular = Constants.cube3DAmbientColor_Entered
        }
        onExited: {
            vertexMaterialEdge10.ambient = Constants.cube3DAmbientColor
            vertexMaterialEdge10.diffuse = "#C8C8C8"
            vertexMaterialEdge10.specular = "#010101"
        }

    }
    Transform {
            id: cuboidTransformEdge10
            //translation:Qt.vector3d(0, 0, 7)
            rotation: fromAxisAndAngle(Qt.vector3d(0, 1 , 0), 180)
        }
    Entity {
        id: vertexEntityEdge10
        components: [ vertexGeometryRendererEdge9, vertexMaterialEdge10, vertexSpherePickerEdge10, cuboidTransformEdge10 ]
    }

    Transform {
            id: cuboidTransformEdge10_2
            translation:Qt.vector3d(0, 0, -7)
            rotation: fromAxisAndAngle(Qt.vector3d(0, 1 , 0), 270)
        }
    Entity {
        id: vertexEntityEdge10_2
        components: [ vertexGeometryRendererEdge9, vertexMaterialEdge10, vertexSpherePickerEdge10, cuboidTransformEdge10_2 ]
    }
    //边10 end

    //边11 start
    PhongMaterial {
        id: vertexMaterialEdge11
        //环境光
        ambient:Constants.cube3DAmbientColor
        //漫反射光
        diffuse: "#C8C8C8"
        //镜面高光
        specular: "#010101"
        //高光半径
        shininess:5
    }

    ObjectPicker{
        id: vertexSpherePickerEdge11
        hoverEnabled: true
        dragEnabled: true
        onClicked:{
            if (pick.button === PickEvent.LeftButton)
            {
                console.log("~~~~~~~~~~~~~~~~~~")
                console.log("vertexSpherePickerEdge6 at:triangleIndex =  " + pick.triangleIndex)
                changeShowPos(2, 11)
            }
        }
        onEntered: {
            vertexMaterialEdge11.ambient = Constants.cube3DAmbientColor_Entered
            vertexMaterialEdge11.diffuse = Constants.cube3DAmbientColor_Entered
            vertexMaterialEdge11.specular = Constants.cube3DAmbientColor_Entered
        }
        onExited: {
            vertexMaterialEdge11.ambient = Constants.cube3DAmbientColor
            vertexMaterialEdge11.diffuse = "#C8C8C8"
            vertexMaterialEdge11.specular = "#010101"
        }

    }
    Transform {
            id: cuboidTransformEdge11
            translation:Qt.vector3d(7, 0, 0)
            rotation: fromAxisAndAngle(Qt.vector3d(0, 1 , 0), 180)
        }
    Entity {
        id: vertexEntityEdge11
        components: [ vertexGeometryRendererEdge9, vertexMaterialEdge11, vertexSpherePickerEdge11, cuboidTransformEdge11 ]
    }

    Transform {
            id: cuboidTransformEdge11_2
            //translation:Qt.vector3d(0, 0, -7)
            rotation: fromAxisAndAngle(Qt.vector3d(0, 1 , 0), 90)
        }
    Entity {
        id: vertexEntityEdge11_2
        components: [ vertexGeometryRendererEdge9, vertexMaterialEdge11, vertexSpherePickerEdge11, cuboidTransformEdge11_2 ]
    }
    //边11 end

    //边12 start
    PhongMaterial {
        id: vertexMaterialEdge12
        //环境光
        ambient:Constants.cube3DAmbientColor
        //漫反射光
        diffuse: "#C8C8C8"
        //镜面高光
        specular: "#010101"
        //高光半径
        shininess:5
    }

    ObjectPicker{
        id: vertexSpherePickerEdge12
        hoverEnabled: true
        dragEnabled: true
        onClicked:{
            if (pick.button === PickEvent.LeftButton)
            {
                console.log("~~~~~~~~~~~~~~~~~~")
                console.log("vertexSpherePickerEdge6 at:triangleIndex =  " + pick.triangleIndex)
                changeShowPos(2, 12)
            }
        }
        onEntered: {
            vertexMaterialEdge12.ambient = Constants.cube3DAmbientColor_Entered
            vertexMaterialEdge12.diffuse = Constants.cube3DAmbientColor_Entered
            vertexMaterialEdge12.specular = Constants.cube3DAmbientColor_Entered
        }
        onExited: {
            vertexMaterialEdge12.ambient = Constants.cube3DAmbientColor
            vertexMaterialEdge12.diffuse = "#C8C8C8"
            vertexMaterialEdge12.specular = "#010101"
        }

    }
    Transform {
            id: cuboidTransformEdge12
            translation:Qt.vector3d(-7, 0, 0)
            //rotation: fromAxisAndAngle(Qt.vector3d(0, 1 , 0), 180)
        }
    Entity {
        id: vertexEntityEdge12
        components: [ vertexGeometryRendererEdge9, vertexMaterialEdge12, vertexSpherePickerEdge12, cuboidTransformEdge12 ]
    }

    Transform {
            id: cuboidTransformEdge12_2
            //translation:Qt.vector3d(0, 0, -7)
            rotation: fromAxisAndAngle(Qt.vector3d(0, 1 , 0), 270)
        }
    Entity {
        id: vertexEntityEdge12_2
        components: [ vertexGeometryRendererEdge9, vertexMaterialEdge12, vertexSpherePickerEdge12, cuboidTransformEdge12_2 ]
    }
    //边12 end

    //面1 start
    Entity {
        id: vertexEntitySurface1
        components: [
//            PhongMaterial {
//                id: vertexMaterialSurface1
//                //环境光
//                ambient:"yellow"
//                //漫反射光
//                diffuse: "yellow"
//                //镜面高光
//                specular: "yellow"
//                //高光半径
//                shininess:5
//            },

            Material {
                id: vertexMaterialSurface1
                effect: Effect {
                    techniques: Technique {
                        graphicsApiFilter {
                            api: GraphicsApiFilter.OpenGL
                            profile: GraphicsApiFilter.CoreProfile
                            majorVersion: 3
                            minorVersion: 1
                        }
                        filterKeys: FilterKey{
                            name: "renderingStyle"
                            value: "forward"
                        }
                        renderPasses: RenderPass {

                            shaderProgram: ShaderProgram {
                                id: gl3Shader
                                vertexShaderCode:   loadSource("qrc:/UI/shaders/gl3/showImg.vert")
                                fragmentShaderCode: loadSource("qrc:/UI/shaders/gl3/showImg.frag")
                            }
                        }
                    }
                }
                parameters: Parameter {
                    name: "diffuseTexture"
                    value: Texture2D {
                        generateMipMaps: true
                        minificationFilter: Texture.Linear
                        magnificationFilter: Texture.Linear
                        wrapMode {
                            x: WrapMode.Repeat
                            y: WrapMode.Repeat
                        }
                        TextureImage {
                            id: idTextureImage1
                            mipLevel: 0
                            source: languageType == 0 ? Constants.cube3DTopImg : Constants.cube3DTopImg_C
                        }
                    }
                }
            },

            ObjectPicker{
                id: vertexSpherePickerSurface1
                hoverEnabled: true
                dragEnabled: true
                onClicked:{
                    if (pick.button === PickEvent.LeftButton)
                    {
                        console.log("~~~~~~~~~~~~~~~~~~")
                        console.log("vertexMaterialSurface1 at:triangleIndex =  " + pick.triangleIndex)
                        changeShowPos(3, 1)
                    }
                }
                onEntered: {
                    idTextureImage1.source = languageType == 0 ? "qrc:/UI/images/top_s.png" : "qrc:/UI/images/top_s_C.png"
                }
                onExited: {
                    idTextureImage1.source = languageType == 0 ? Constants.cube3DTopImg : Constants.cube3DTopImg_C
                }

            },

            GeometryRenderer {
                id: vertexGeometryRendererSurface1
                // 利用GeometryRenderer提供的基本类型(line)来进行几何图形的绘制
                primitiveType: GeometryRenderer.Triangles//GeometryRenderer.Lines //GeometryRenderer.TriangleStrip //
                // 几何数据的坐标
                geometry: Geometry {
                    boundingVolumePositionAttribute: vertexPositionSurface1
                    Attribute {
                        id: vertexPositionSurface1
                        attributeType: Attribute.VertexAttribute
                        vertexBaseType: Attribute.Float // 定义顶点的数据类型
                        vertexSize: 3 // 顶点大小
                        count: 6
                        name: "vertexPosition"
                        byteOffset: 0
                        byteStride: 3 * 4
                        buffer: Buffer {
                            type: Buffer.VertexBuffer
                            data: {
                                function createCube(attribute)
                                {
                                    var vertexCount = 2*3;
                                    var f32a = new Float32Array(vertexCount*3);
                                    //17
                                    f32a[0] = -3;
                                    f32a[1] = 4;
                                    f32a[2] = 3;

                                    f32a[3] = 3;
                                    f32a[4] = 4;
                                    f32a[5] = -3;

                                    f32a[6] = -3;
                                    f32a[7] = 4;
                                    f32a[8] = -3;

                                    //18
                                    f32a[9] = -3;
                                    f32a[10] = 4;
                                    f32a[11] = 3;

                                    f32a[12] = 3;
                                    f32a[13] = 4;
                                    f32a[14] = 3;

                                    f32a[15] = 3;
                                    f32a[16] = 4;
                                    f32a[17] = -3;

                                    attribute.count = vertexCount
                                    return f32a;

                                }
                                return createCube(vertexPositionSurface1)
                            }
                        }
                    }
                    Attribute {
                        id: vertexPositionSurface1_texCoord
                        attributeType: Attribute.VertexAttribute
                        vertexBaseType: Attribute.Float // 定义顶点的数据类型
                        vertexSize: 2 // 顶点大小
                        count: 6
                        name: "vertexTexCoord"
                        byteOffset: 0
                        byteStride: 2 * 4
                        buffer: Buffer {
                            type: Buffer.VertexBuffer
                            data: {
                                function createCube(attribute)
                                {
                                    var vertexCount = 2*3;
                                    var f32a = new Float32Array(vertexCount*2);
                                    //17
                                    f32a[0] = 0.0;
                                    f32a[1] = 1.0;

                                    f32a[2] = 1.0;
                                    f32a[3] = 0.0;

                                    f32a[4] = 0.0;
                                    f32a[5] = 0.0;

                                    //18
                                    f32a[6] = 0.0;
                                    f32a[7] = 1.0;

                                    f32a[8] = 1.0;
                                    f32a[9] = 1.0;

                                    f32a[10] = 1.0;
                                    f32a[11] = 0.0;

                                    attribute.count = vertexCount
                                    return f32a;

                                }
                                return createCube(vertexPositionSurface1_texCoord)
                            }
                        }
                    }
                }
            }
        ]
    }
    //面1 end

    //面2 start
//    PhongMaterial {
//        id: vertexMaterialSurface2
//        //环境光
//        ambient:"hotpink"
//        //漫反射光
//        diffuse: "hotpink"
//        //镜面高光
//        specular: "hotpink"
//        //高光半径
//        shininess:5
//    }

    Material {
        id: vertexMaterialSurface2
        effect: Effect {
            techniques: Technique {
                graphicsApiFilter {
                    api: GraphicsApiFilter.OpenGL
                    profile: GraphicsApiFilter.CoreProfile
                    majorVersion: 3
                    minorVersion: 1
                }
                filterKeys: FilterKey{
                    name: "renderingStyle"
                    value: "forward"
                }
                renderPasses: RenderPass {

                    shaderProgram: ShaderProgram {
                        id: gl3Shader2
                        vertexShaderCode:   loadSource("qrc:/UI/shaders/gl3/showImg.vert")
                        fragmentShaderCode: loadSource("qrc:/UI/shaders/gl3/showImg.frag")
                    }
                }
            }
        }
        parameters: Parameter {
            name: "diffuseTexture"
            value: Texture2D {
                generateMipMaps: true
                minificationFilter: Texture.Linear
                magnificationFilter: Texture.Linear
                wrapMode {
                    x: WrapMode.Repeat
                    y: WrapMode.Repeat
                }
                TextureImage {
                    id: idTextureImage2
                    mipLevel: 0
                    source: languageType == 0 ? Constants.cube3DFrontImg : Constants.cube3DFrontImg_C
                }
            }
        }
    }

    ObjectPicker{
        id: vertexSpherePickerSurface2
        hoverEnabled: true
        dragEnabled: true
        onClicked:{
            if (pick.button === PickEvent.LeftButton)
            {
                changeShowPos(3, 2)
            }
        }
        onEntered: {
            idTextureImage2.source = languageType == 0 ? "qrc:/UI/images/front_s.png" : "qrc:/UI/images/front_s_C.png"
        }
        onExited: {
            idTextureImage2.source = languageType == 0 ? Constants.cube3DFrontImg : Constants.cube3DFrontImg_C
        }

    }
    Transform {
            id: cuboidTransformSurface2
            //translation:Qt.vector3d(-7, 0, 0)
            rotation: fromAxisAndAngle(Qt.vector3d(1, 0 , 0), 90)
        }
    Entity {
        id: vertexEntitySurface2
        components: [ vertexGeometryRendererSurface1, vertexMaterialSurface2, vertexSpherePickerSurface2, cuboidTransformSurface2 ]
    }
    //面2 end

    //面3 start
//    PhongMaterial {
//        id: vertexMaterialSurface3
//        //环境光
//        ambient:"indigo"
//        //漫反射光
//        diffuse: "indigo"
//        //镜面高光
//        specular: "indigo"
//        //高光半径
//        shininess:5
//    }
    Material {
        id: vertexMaterialSurface3
        effect: Effect {
            techniques: Technique {
                graphicsApiFilter {
                    api: GraphicsApiFilter.OpenGL
                    profile: GraphicsApiFilter.CoreProfile
                    majorVersion: 3
                    minorVersion: 1
                }
                filterKeys: FilterKey{
                    name: "renderingStyle"
                    value: "forward"
                }
                renderPasses: RenderPass {

                    shaderProgram: ShaderProgram {
                        id: gl3Shader3
                        vertexShaderCode:   loadSource("qrc:/UI/shaders/gl3/showImg.vert")
                        fragmentShaderCode: loadSource("qrc:/UI/shaders/gl3/showImg.frag")
                    }
                }
            }
        }
        parameters: Parameter {
            name: "diffuseTexture"
            value: Texture2D {
                generateMipMaps: true
                minificationFilter: Texture.Linear
                magnificationFilter: Texture.Linear
                wrapMode {
                    x: WrapMode.Repeat
                    y: WrapMode.Repeat
                }
                TextureImage {
                    id: idTextureImage3
                    mipLevel: 0
                    source: languageType == 0 ? Constants.cube3DBottomImg : Constants.cube3DBottomImg_C
                }
            }
        }
    }

    ObjectPicker{
        id: vertexSpherePickerSurface3
        hoverEnabled: true
        dragEnabled: true
        onClicked:{
            if (pick.button === PickEvent.LeftButton)
            {
                changeShowPos(3, 3)
            }
        }
        onEntered: {
            idTextureImage3.source = languageType == 0 ? "qrc:/UI/images/bottom_s.png" : "qrc:/UI/images/bottom_s_C.png"
        }
        onExited: {
            idTextureImage3.source = languageType == 0 ? Constants.cube3DBottomImg : Constants.cube3DBottomImg_C
        }

    }
    Transform {
            id: cuboidTransformSurface3
            //translation:Qt.vector3d(-7, 0, 0)
            rotation: fromAxisAndAngle(Qt.vector3d(1, 0 , 0), 180)
        }
    Entity {
        id: vertexEntitySurface3
        components: [ vertexGeometryRendererSurface1, vertexMaterialSurface3, vertexSpherePickerSurface3, cuboidTransformSurface3 ]
    }
    //面3 end

    //面4 start
//    PhongMaterial {
//        id: vertexMaterialSurface4
//        //环境光
//        ambient:"lightgrey"
//        //漫反射光
//        diffuse: "lightgrey"
//        //镜面高光
//        specular: "lightgrey"
//        //高光半径
//        shininess:5
//    }

    Material {
        id: vertexMaterialSurface4
        effect: Effect {
            techniques: Technique {
                graphicsApiFilter {
                    api: GraphicsApiFilter.OpenGL
                    profile: GraphicsApiFilter.CoreProfile
                    majorVersion: 3
                    minorVersion: 1
                }
                filterKeys: FilterKey{
                    name: "renderingStyle"
                    value: "forward"
                }
                renderPasses: RenderPass {

                    shaderProgram: ShaderProgram {
                        id: gl3Shader4
                        vertexShaderCode:   loadSource("qrc:/UI/shaders/gl3/showImg.vert")
                        fragmentShaderCode: loadSource("qrc:/UI/shaders/gl3/showImg.frag")
                    }
                }
            }
        }
        parameters: Parameter {
            name: "diffuseTexture"
            value: Texture2D {
                generateMipMaps: true
                minificationFilter: Texture.Linear
                magnificationFilter: Texture.Linear
                wrapMode {
                    x: WrapMode.Repeat
                    y: WrapMode.Repeat
                }
                TextureImage {
                    id: idTextureImage4
                    mipLevel: 0
                    source: languageType == 0 ? Constants.cube3DBackImg : Constants.cube3DBackImg_C
                }
            }
        }
    }

    ObjectPicker{
        id: vertexSpherePickerSurface4
        hoverEnabled: true
        dragEnabled: true
        onClicked:{
            if (pick.button === PickEvent.LeftButton)
            {
                changeShowPos(3, 4)
            }
        }
        onEntered: {
            idTextureImage4.source = languageType == 0 ? "qrc:/UI/images/back_s.png" : "qrc:/UI/images/back_s_C.png"
        }
        onExited: {
            idTextureImage4.source = languageType == 0 ? Constants.cube3DBackImg : Constants.cube3DBackImg_C
        }

    }
    Transform {
            id: cuboidTransformSurface4
            //translation:Qt.vector3d(-7, 0, 0)
            rotation: fromAxisAndAngle(Qt.vector3d(1, 0 , 0), 270)
        }
    Entity {
        id: vertexEntitySurface4
        components: [ vertexGeometryRendererSurface1, vertexMaterialSurface4, vertexSpherePickerSurface4, cuboidTransformSurface4 ]
    }
    //面4 end

    //面5 start
//    PhongMaterial {
//        id: vertexMaterialSurface5
//        //环境光
//        ambient:"teal"
//        //漫反射光
//        diffuse: "teal"
//        //镜面高光
//        specular: "teal"
//        //高光半径
//        shininess:5
//    }

    Material {
        id: vertexMaterialSurface5
        effect: Effect {
            techniques: Technique {
                graphicsApiFilter {
                    api: GraphicsApiFilter.OpenGL
                    profile: GraphicsApiFilter.CoreProfile
                    majorVersion: 3
                    minorVersion: 1
                }
                filterKeys: FilterKey{
                    name: "renderingStyle"
                    value: "forward"
                }
                renderPasses: RenderPass {

                    shaderProgram: ShaderProgram {
                        id: gl3Shader5
                        vertexShaderCode:   loadSource("qrc:/UI/shaders/gl3/showImg.vert")
                        fragmentShaderCode: loadSource("qrc:/UI/shaders/gl3/showImg.frag")
                    }
                }
            }
        }
        parameters: Parameter {
            name: "diffuseTexture"
            value: Texture2D {
                generateMipMaps: true
                minificationFilter: Texture.Linear
                magnificationFilter: Texture.Linear
                wrapMode {
                    x: WrapMode.Repeat
                    y: WrapMode.Repeat
                }
                TextureImage {
                    id: idTextureImage5
                    mipLevel: 0
                    source: languageType == 0 ? Constants.cube3DLeftkImg : Constants.cube3DLeftkImg_C
                }
            }
        }
    }

    ObjectPicker{
        id: vertexSpherePickerSurface5
        hoverEnabled: true
        dragEnabled: true
        onClicked:{
            if (pick.button === PickEvent.LeftButton)
            {
                changeShowPos(3, 5)
            }
        }
        onEntered: {
            idTextureImage5.source = languageType == 0 ? "qrc:/UI/images/left_s.png" : "qrc:/UI/images/left_s_C.png"
        }
        onExited: {
            idTextureImage5.source = languageType == 0 ? Constants.cube3DLeftkImg : Constants.cube3DLeftkImg_C
        }

    }
    Transform {
            id: cuboidTransformSurface5
            //translation:Qt.vector3d(-7, 0, 0)
            rotation: fromAxisAndAngle(Qt.vector3d(0, 0 , 1), 90)
        }
    Entity {
        id: vertexEntitySurface5
        components: [ vertexGeometryRendererSurface1, vertexMaterialSurface5, vertexSpherePickerSurface5, cuboidTransformSurface5 ]
    }
    //面5 end

    //面6 start
//    PhongMaterial {
//        id: vertexMaterialSurface6
//        //环境光
//        ambient:"tomato"
//        //漫反射光
//        diffuse: "tomato"
//        //镜面高光
//        specular: "tomato"
//        //高光半径
//        shininess:6
//    }

    Material {
        id: vertexMaterialSurface6
        effect: Effect {
            techniques: Technique {
                graphicsApiFilter {
                    api: GraphicsApiFilter.OpenGL
                    profile: GraphicsApiFilter.CoreProfile
                    majorVersion: 3
                    minorVersion: 1
                }
                filterKeys: FilterKey{
                    name: "renderingStyle"
                    value: "forward"
                }
                renderPasses: RenderPass {

                    shaderProgram: ShaderProgram {
                        id: gl3Shader6
                        vertexShaderCode:   loadSource("qrc:/UI/shaders/gl3/showImg.vert")
                        fragmentShaderCode: loadSource("qrc:/UI/shaders/gl3/showImg.frag")
                    }
                }
            }
        }
        parameters: Parameter {
            name: "diffuseTexture"
            value: Texture2D {
                generateMipMaps: true
                minificationFilter: Texture.Linear
                magnificationFilter: Texture.Linear
                wrapMode {
                    x: WrapMode.Repeat
                    y: WrapMode.Repeat
                }
                TextureImage {
                    id: idTextureImage6
                    mipLevel: 0
                    source: languageType == 0 ? Constants.cube3DLeftkRight : Constants.cube3DLeftkRight_C
                }
            }
        }
    }

    ObjectPicker{
        id: vertexSpherePickerSurface6
        hoverEnabled: true
        dragEnabled: true
        onClicked:{
            if (pick.button === PickEvent.LeftButton)
            {
                changeShowPos(3, 6)
            }
        }
        onEntered: {
            idTextureImage6.source = languageType == 0 ? "qrc:/UI/images/right_s.png" : "qrc:/UI/images/right_s_C.png"
        }
        onExited: {
            idTextureImage6.source = languageType == 0 ? Constants.cube3DLeftkRight : Constants.cube3DLeftkRight_C
        }

    }
    Transform {
            id: cuboidTransformSurface6
            //translation:Qt.vector3d(-7, 0, 0)
            rotation: fromAxisAndAngle(Qt.vector3d(0, 0 , 1), 270)
        }
    Entity {
        id: vertexEntitySurface6
        components: [ vertexGeometryRendererSurface1, vertexMaterialSurface6, vertexSpherePickerSurface6, cuboidTransformSurface6 ]
    }
    //面6 end
}
