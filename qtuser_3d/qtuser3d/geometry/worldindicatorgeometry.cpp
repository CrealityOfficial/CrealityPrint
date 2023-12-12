#include "worldindicatorgeometry.h"
#include <Qt3DRender/QBuffer>
#include "qtuser3d/geometry/geometrycreatehelper.h"

namespace qtuser_3d
{
    Qt3DRender::QGeometry* createWorldIndicatorGeometry()
    {
        int  ifront = 1 << 0;
        int  iback = 1 << 1;
        int  ileft = 1 << 2;
        int  iright = 1 << 3;
        int  ibottom = 1 << 4;
        int  itop = 1 << 5;

        float front = ifront;
        float back = iback;
        float left = ileft;
        float right = iright;
        float bottom = ibottom;
        float top = itop;

        float k = 0.15f;

        float uvstep = 1.0f / 6.0f;

        float vertices[] = {

            // positions        // normals             // texture coords  //face index

            //front
            -0.5f,  -0.5f,   0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,    front + bottom + left,
            -0.5f + k,-0.5f,   0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,    front + bottom + left,
            -0.5f + k,-0.5f + k, 0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,    front + bottom + left,
            -0.5f + k,-0.5f + k, 0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,    front + bottom + left,
            -0.5f,  -0.5f + k, 0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,    front + bottom + left,
            -0.5f,  -0.5f,   0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,    front + bottom + left,

            -0.5f + k, -0.5f,    0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,    front + bottom,
             0.5f - k, -0.5f,    0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,    front + bottom,
             0.5f - k,  -0.5f + k, 0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,    front + bottom,
             0.5f - k,  -0.5f + k, 0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,    front + bottom,
            -0.5f + k,  -0.5f + k, 0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,    front + bottom,
            -0.5f + k, -0.5f,    0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,    front + bottom,


            0.5f - k,  -0.5f,    0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,    front + bottom + right,
            0.5f,    -0.5f,    0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,    front + bottom + right,
            0.5f,    -0.5f + k,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,    front + bottom + right,
            0.5f,    -0.5f + k,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,    front + bottom + right,
            0.5f - k,  -0.5f + k,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,    front + bottom + right,
            0.5f - k,  -0.5f,    0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,    front + bottom + right,

            -0.5f,   -0.5f + k,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,    front + left,
            -0.5f + k, -0.5f + k,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,    front + left,
            -0.5f + k,  0.5f - k,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,    front + left,
            -0.5f + k,  0.5f - k,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,    front + left,
            -0.5f,    0.5f - k,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,    front + left,
            -0.5f,   -0.5f + k,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,    front + left,

            -0.5f + k, -0.5f + k,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,    front,
            0.5f - k,  -0.5f + k,  0.5f,  0.0f,  0.0f,  1.0f,  uvstep,  0.0f,    front,
            0.5f - k,  0.5f - k,   0.5f,  0.0f,  0.0f,  1.0f,  uvstep,  1.0f,    front,
            0.5f - k,  0.5f - k,   0.5f,  0.0f,  0.0f,  1.0f,  uvstep,  1.0f,    front,
            -0.5f + k, 0.5f - k,   0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,    front,
            -0.5f + k, -0.5f + k,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,    front,

            0.5f - k, -0.5f + k,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,    front + right,
            0.5f,   -0.5f + k,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,    front + right,
            0.5f,    0.5f - k,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,    front + right,
            0.5f,    0.5f - k,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,    front + right,
            0.5f - k,  0.5f - k,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,    front + right,
            0.5f - k, -0.5f + k,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,    front + right,



            -0.5f,    0.5f - k,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,    top + front + left,
            -0.5f + k,  0.5f - k,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,    top + front + left,
            -0.5f + k,  0.5f,    0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,    top + front + left,
            -0.5f + k,  0.5f,    0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,    top + front + left,
            -0.5f,    0.5f,    0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,    top + front + left,
            -0.5f,    0.5f - k,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,    top + front + left,

            -0.5f + k,  0.5f - k,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,    top + front,
             0.5f - k,  0.5f - k,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,    top + front,
             0.5f - k,  0.5f,    0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,    top + front,
             0.5f - k,  0.5f,    0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,    top + front,
            -0.5f + k,  0.5f,    0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,    top + front,
            -0.5f + k,  0.5f - k,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,    top + front,


            0.5f - k, 0.5f - k,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,    top + front + right,
             0.5f,  0.5f - k,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,    top + front + right,
             0.5f,  0.5f,    0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,    top + front + right,
             0.5f,  0.5f,    0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,    top + front + right,
            0.5f - k, 0.5f,    0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,    top + front + right,
            0.5f - k, 0.5f - k,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,    top + front + right,


            //back
            -0.5f,  -0.5f,   -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,      back + bottom + left,
            -0.5f + k,-0.5f + k, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,  back + bottom + left,
            -0.5f + k,-0.5f,   -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,    back + bottom + left,
            -0.5f + k,-0.5f + k, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,  back + bottom + left,
            -0.5f,  -0.5f,   -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,      back + bottom + left,
            -0.5f,  -0.5f + k, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,    back + bottom + left,

            -0.5f + k, -0.5f,    -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,      back + bottom,
             0.5f - k, -0.5f + k,  -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,    back + bottom,
             0.5f - k, -0.5f,    -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,      back + bottom,
             0.5f - k, -0.5f + k,  -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,    back + bottom,
            -0.5f + k, -0.5f,    -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,      back + bottom,
            -0.5f + k, -0.5f + k,  -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,    back + bottom,


            0.5f - k, -0.5f,    -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,    back + bottom + right,
             0.5f,  -0.5f + k,  -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,    back + bottom + right,
             0.5f,  -0.5f,    -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,      back + bottom + right,
             0.5f,  -0.5f + k,  -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,    back + bottom + right,
            0.5f - k, -0.5f,    -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,    back + bottom + right,
            0.5f - k, -0.5f + k,  -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,  back + bottom + right,

            -0.5f,   -0.5f + k,  -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,     back + left,
            -0.5f + k,  0.5f - k,  -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,   back + left,
            -0.5f + k, -0.5f + k,  -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,   back + left,
            -0.5f + k,  0.5f - k,  -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,   back + left,
            -0.5f,   -0.5f + k,  -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,     back + left,
            -0.5f,    0.5f - k,  -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,     back + left,

            -0.5f + k, -0.5f + k,  -0.5f,  0.0f,  0.0f, -1.0f,  uvstep * 2,  0.0f,    back,
            0.5f - k,  0.5f - k,   -0.5f,  0.0f,  0.0f, -1.0f,  uvstep,  1.0f,    back,
            0.5f - k,  -0.5f + k,  -0.5f,  0.0f,  0.0f, -1.0f,  uvstep,  0.0f,    back,
            0.5f - k,  0.5f - k,   -0.5f,  0.0f,  0.0f, -1.0f,  uvstep,  1.0f,    back,
            -0.5f + k, -0.5f + k,  -0.5f,  0.0f,  0.0f, -1.0f,  uvstep * 2,  0.0f,    back,
            -0.5f + k, 0.5f - k,   -0.5f,  0.0f,  0.0f, -1.0f,  uvstep * 2,  1.0f,    back,

            0.5f - k, -0.5f + k, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,    back + right,
            0.5f,   0.5f - k,  -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,    back + right,
            0.5f,   -0.5f + k, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,    back + right,
            0.5f,   0.5f - k,  -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,    back + right,
            0.5f - k, -0.5f + k, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,    back + right,
            0.5f - k, 0.5f - k,  -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,    back + right,



            -0.5f,   0.5f - k, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,    back + top + left,
            -0.5f + k, 0.5f,   -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,    back + top + left,
            -0.5f + k, 0.5f - k, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,  back + top + left,
            -0.5f + k, 0.5f,   -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,    back + top + left,
            -0.5f,   0.5f - k, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,    back + top + left,
            -0.5f,   0.5f,   -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,      back + top + left,

            -0.5f + k,  0.5f - k, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,  back + top,
             0.5f - k,  0.5f,   -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,    back + top,
             0.5f - k,  0.5f - k, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,  back + top,
             0.5f - k,  0.5f,   -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,    back + top,
            -0.5f + k,  0.5f - k, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,  back + top,
            -0.5f + k,  0.5f,   -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,    back + top,

            0.5f - k, 0.5f - k,  -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,    back + top + right,
            0.5f,   0.5f,    -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,        back + top + right,
            0.5f,   0.5f - k,  -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,      back + top + right,
            0.5f,   0.5f,    -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,        back + top + right,
            0.5f - k, 0.5f - k,  -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,    back + top + right,
            0.5f - k, 0.5f,    -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,      back + top + right,


            //left
            -0.5f,  -0.5f + k,  -0.5f + k, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,    left + bottom + back,
            -0.5f,  -0.5f + k,  -0.5f,   -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,      left + bottom + back,
            -0.5f,  -0.5f,    -0.5f,   -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,        left + bottom + back,
            -0.5f,  -0.5f,    -0.5f,   -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,        left + bottom + back,
            -0.5f,  -0.5f,    -0.5f + k, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,      left + bottom + back,
            -0.5f,  -0.5f + k,  -0.5f + k, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,    left + bottom + back,

            -0.5f,  -0.5f + k, 0.5f - k,   -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,    left + bottom,
            -0.5f,  -0.5f + k, -0.5f + k,  -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,    left + bottom,
            -0.5f,  -0.5f,   -0.5f + k,  -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,      left + bottom,
            -0.5f,  -0.5f,   -0.5f + k,  -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,      left + bottom,
            -0.5f,  -0.5f,   0.5f - k,   -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,      left + bottom,
            -0.5f,  -0.5f + k, 0.5f - k,   -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,    left + bottom,

            -0.5f,  -0.5f + k, 0.5f,    -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,    left + bottom + front,
            -0.5f,  -0.5f + k, 0.5f - k,  -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  left + bottom + front,
            -0.5f,  -0.5f,   0.5f - k,  -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,    left + bottom + front,
            -0.5f,  -0.5f,   0.5f - k,  -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,    left + bottom + front,
            -0.5f,  -0.5f,   0.5f,    -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,      left + bottom + front,
            -0.5f,  -0.5f + k, 0.5f,    -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,    left + bottom + front,


            -0.5f,  0.5f - k,  -0.5f + k, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,    left + back,
            -0.5f,  0.5f - k, -0.5f,    -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,      left + back,
            -0.5f, -0.5f + k, -0.5f,    -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,      left + back,
            -0.5f, -0.5f + k, -0.5f,    -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,      left + back,
            -0.5f, -0.5f + k,  -0.5f + k, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,    left + back,
            -0.5f,  0.5f - k,  -0.5f + k, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,    left + back,


            -0.5f,  0.5f - k,  0.5f - k, -1.0f,  0.0f,  0.0f,  uvstep * 3,  1.0f,    left,
            -0.5f,  0.5f - k, -0.5f + k, -1.0f,  0.0f,  0.0f,  uvstep * 2,  1.0f,    left,
            -0.5f, -0.5f + k, -0.5f + k, -1.0f,  0.0f,  0.0f,  uvstep * 2,  0.0f,    left,
            -0.5f, -0.5f + k, -0.5f + k, -1.0f,  0.0f,  0.0f,  uvstep * 2,  0.0f,    left,
            -0.5f, -0.5f + k,  0.5f - k, -1.0f,  0.0f,  0.0f,  uvstep * 3,  0.0f,    left,
            -0.5f,  0.5f - k,  0.5f - k, -1.0f,  0.0f,  0.0f,  uvstep * 3,  1.0f,    left,


            -0.5f,  0.5f - k,  0.5f,   -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,      left + front,
            -0.5f,  0.5f - k,  0.5f - k, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,    left + front,
            -0.5f, -0.5f + k,  0.5f - k, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,    left + front,
            -0.5f, -0.5f + k,  0.5f - k, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,    left + front,
            -0.5f, -0.5f + k,  0.5f,   -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,      left + front,
            -0.5f,  0.5f - k,  0.5f,   -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,      left + front,

            -0.5f,  0.5f,  -0.5f + k,  -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,    left + top + back,
            -0.5f,  0.5f,  -0.5f,    -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,      left + top + back,
            -0.5f, 0.5f - k, -0.5f,    -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,    left + top + back,
            -0.5f, 0.5f - k, -0.5f,    -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,    left + top + back,
            -0.5f, 0.5f - k, -0.5f + k,  -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  left + top + back,
            -0.5f,  0.5f,  -0.5f + k,  -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,    left + top + back,

            -0.5f,  0.5f,  0.5f - k,  -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,    left + top,
            -0.5f,  0.5f, -0.5f + k,  -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,    left + top,
            -0.5f, 0.5f - k, -0.5f + k, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  left + top,
            -0.5f, 0.5f - k, -0.5f + k, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  left + top,
            -0.5f, 0.5f - k,  0.5f - k, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  left + top,
            -0.5f,  0.5f,  0.5f - k,  -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,    left + top,

            -0.5f,  0.5f,  0.5f,   -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,      left + top + front,
            -0.5f,  0.5f,  0.5f - k, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,    left + top + front,
            -0.5f, 0.5f - k, 0.5f - k, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  left + top + front,
            -0.5f, 0.5f - k, 0.5f - k, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  left + top + front,
            -0.5f, 0.5f - k, 0.5f,   -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,    left + top + front,
            -0.5f,  0.5f,  0.5f,   -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,      left + top + front,


            //right
            0.5f,  -0.5f + k, 0.5f,        1.0f,  0.0f,  0.0f,  0.0f,  0.0f,    right + bottom + front,
            0.5f,  -0.5f,     0.5f - k,    1.0f,  0.0f,  0.0f,  0.0f,  0.0f,    right + bottom + front,
            0.5f,  -0.5f + k, 0.5f - k,    1.0f,  0.0f,  0.0f,  0.0f,  0.0f,    right + bottom + front,
            0.5f,  -0.5f,     0.5f - k,    1.0f,  0.0f,  0.0f,  0.0f,  0.0f,    right + bottom + front,
            0.5f,  -0.5f + k, 0.5f,        1.0f,  0.0f,  0.0f,  0.0f,  0.0f,    right + bottom + front,
            0.5f,  -0.5f,     0.5f,        1.0f,  0.0f,  0.0f,  0.0f,  0.0f,    right + bottom + front,


            0.5f,  -0.5f + k, 0.5f - k,    1.0f,  0.0f,  0.0f,  0.0f,  0.0f,    right + bottom,
            0.5f,  -0.5f,   -0.5f + k,   1.0f,  0.0f,  0.0f,  0.0f,  0.0f,      right + bottom,
            0.5f,  -0.5f + k, -0.5f + k,   1.0f,  0.0f,  0.0f,  0.0f,  0.0f,    right + bottom,
            0.5f,  -0.5f,   -0.5f + k,   1.0f,  0.0f,  0.0f,  0.0f,  0.0f,      right + bottom,
            0.5f,  -0.5f + k, 0.5f - k,    1.0f,  0.0f,  0.0f,  0.0f,  0.0f,    right + bottom,
            0.5f,  -0.5f,   0.5f - k,    1.0f,  0.0f,  0.0f,  0.0f,  0.0f,      right + bottom,

            0.5f,  -0.5f + k,  -0.5f + k, 1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  right + bottom + back,
            0.5f,  -0.5f,    -0.5f,   1.0f,  0.0f,  0.0f,  0.0f,  0.0f,      right + bottom + back,
            0.5f,  -0.5f + k,  -0.5f,   1.0f,  0.0f,  0.0f,  0.0f,  0.0f,    right + bottom + back,
            0.5f,  -0.5f,    -0.5f,   1.0f,  0.0f,  0.0f,  0.0f,  0.0f,      right + bottom + back,
            0.5f,  -0.5f + k,  -0.5f + k, 1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  right + bottom + back,
            0.5f,  -0.5f,    -0.5f + k, 1.0f,  0.0f,  0.0f,  0.0f,  0.0f,    right + bottom + back,

            0.5f,  0.5f - k,  0.5f,     1.0f,  0.0f,  0.0f,  0.0f,  0.0f,      right + front,
            0.5f, -0.5f + k,  0.5f - k,   1.0f,  0.0f,  0.0f,  0.0f,  0.0f,    right + front,
            0.5f,  0.5f - k,  0.5f - k,   1.0f,  0.0f,  0.0f,  0.0f,  0.0f,    right + front,
            0.5f, -0.5f + k,  0.5f - k,   1.0f,  0.0f,  0.0f,  0.0f,  0.0f,    right + front,
            0.5f,  0.5f - k,  0.5f,     1.0f,  0.0f,  0.0f,  0.0f,  0.0f,      right + front,
            0.5f, -0.5f + k,  0.5f,     1.0f,  0.0f,  0.0f,  0.0f,  0.0f,      right + front,


            0.5f,  0.5f - k,  0.5f - k,   1.0f,  0.0f,  0.0f,  uvstep * 3,  1.0f,    right,
            0.5f, -0.5f + k, -0.5f + k,   1.0f,  0.0f,  0.0f,  uvstep * 4,  0.0f,    right,
            0.5f, 0.5f - k, -0.5f + k,    1.0f, 0.0f,    0.0f, uvstep * 4,  1.0f,    right,
            0.5f, -0.5f + k, -0.5f + k,   1.0f,  0.0f,  0.0f,  uvstep * 4,  0.0f,    right,
            0.5f,  0.5f - k,  0.5f - k,   1.0f,  0.0f,  0.0f,  uvstep * 3,  1.0f,    right,
            0.5f, -0.5f + k, 0.5f - k,    1.0f, 0.0f,    0.0f, uvstep * 3,  0.0f,    right,

            0.5f,  0.5f - k,  -0.5f + k,   1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  right + back,
            0.5f, -0.5f + k, -0.5f,      1.0f,  0.0f,  0.0f,  0.0f,  0.0f,    right + back,
            0.5f,  0.5f - k, -0.5f,      1.0f,  0.0f,  0.0f,  0.0f,  0.0f,    right + back,
            0.5f, -0.5f + k, -0.5f,      1.0f,  0.0f,  0.0f,  0.0f,  0.0f,    right + back,
            0.5f,  0.5f - k,  -0.5f + k,   1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  right + back,
            0.5f, -0.5f + k,  -0.5f + k,   1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  right + back,

            0.5f,  0.5f,  0.5f,      1.0f,  0.0f,  0.0f,  0.0f,  0.0f,       right + top + front,
            0.5f, 0.5f - k, 0.5f - k,    1.0f,  0.0f,  0.0f,  0.0f,  0.0f,   right + top + front,
            0.5f,  0.5f,  0.5f - k,    1.0f,  0.0f,  0.0f,  0.0f,  0.0f,     right + top + front,
            0.5f, 0.5f - k, 0.5f - k,    1.0f,  0.0f,  0.0f,  0.0f,  0.0f,   right + top + front,
            0.5f,  0.5f,  0.5f,      1.0f,  0.0f,  0.0f,  0.0f,  0.0f,       right + top + front,
            0.5f, 0.5f - k, 0.5f,      1.0f,  0.0f,  0.0f,  0.0f,  0.0f,     right + top + front,


            0.5f,  0.5f,  0.5f - k,     1.0f,  0.0f,  0.0f,  0.0f,  0.0f,    right + top,
            0.5f, 0.5f - k, -0.5f + k,    1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  right + top,
            0.5f,  0.5f, -0.5f + k,     1.0f,  0.0f,  0.0f,  0.0f,  0.0f,    right + top,
            0.5f, 0.5f - k, -0.5f + k,    1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  right + top,
            0.5f,  0.5f,  0.5f - k,     1.0f,  0.0f,  0.0f,  0.0f,  0.0f,    right + top,
            0.5f, 0.5f - k,  0.5f - k,    1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  right + top,

            0.5f,  0.5f,  -0.5f + k,    1.0f,  0.0f,  0.0f,  0.0f,  0.0f,    right + top + back,
            0.5f, 0.5f - k, -0.5f,      1.0f,  0.0f,  0.0f,  0.0f,  0.0f,    right + top + back,
            0.5f,  0.5f, -0.5f,       1.0f,  0.0f,  0.0f,  0.0f,  0.0f,      right + top + back,
            0.5f, 0.5f - k, -0.5f,      1.0f,  0.0f,  0.0f,  0.0f,  0.0f,    right + top + back,
            0.5f,  0.5f,  -0.5f + k,    1.0f,  0.0f,  0.0f,  0.0f,  0.0f,    right + top + back,
            0.5f, 0.5f - k,  -0.5f + k,   1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  right + top + back,

            //bottom
            -0.5f,   -0.5f,  -0.5f,    0.0f, -1.0f,  0.0f,  0.0f,  0.0f,    bottom + left + back,
            -0.5f + k, -0.5f,  -0.5f,    0.0f, -1.0f,  0.0f,  0.0f,  0.0f,    bottom + left + back,
            -0.5f + k, -0.5f,  -0.5f + k,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,    bottom + left + back,
            -0.5f + k, -0.5f,  -0.5f + k,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,    bottom + left + back,
            -0.5f,   -0.5f,  -0.5f + k,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,    bottom + left + back,
            -0.5f,   -0.5f,  -0.5f,    0.0f, -1.0f,  0.0f,  0.0f,  0.0f,    bottom + left + back,

            -0.5f + k, -0.5f,  -0.5f,    0.0f, -1.0f,  0.0f,  0.0f,  0.0f,    bottom + back,
            0.5f - k,  -0.5f,  -0.5f,    0.0f, -1.0f,  0.0f,  0.0f,  0.0f,    bottom + back,
            0.5f - k,  -0.5f,  -0.5f + k,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,    bottom + back,
            0.5f - k,  -0.5f,  -0.5f + k,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,    bottom + back,
            -0.5f + k, -0.5f,  -0.5f + k,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,    bottom + back,
            -0.5f + k, -0.5f,  -0.5f,    0.0f, -1.0f,  0.0f,  0.0f,  0.0f,    bottom + back,

            0.5f - k, -0.5f,  -0.5f,    0.0f, -1.0f,  0.0f,  0.0f,  0.0f,    bottom + back + right,
            0.5f,   -0.5f,  -0.5f,    0.0f, -1.0f,  0.0f,  0.0f,  0.0f,    bottom + back + right,
            0.5f,   -0.5f,  -0.5f + k,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,    bottom + back + right,
            0.5f,   -0.5f,  -0.5f + k,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,    bottom + back + right,
            0.5f - k, -0.5f,  -0.5f + k,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,    bottom + back + right,
            0.5f - k, -0.5f,  -0.5f,    0.0f, -1.0f,  0.0f,  0.0f,  0.0f,    bottom + back + right,

            -0.5f, -0.5f,   -0.5f + k,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,    bottom + left,
            -0.5f + k, -0.5f, -0.5f + k,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,    bottom + left,
            -0.5f + k, -0.5f,  0.5f - k,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,    bottom + left,
            -0.5f + k, -0.5f,  0.5f - k,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,    bottom + left,
            -0.5f, -0.5f,    0.5f - k,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,    bottom + left,
            -0.5f, -0.5f,   -0.5f + k,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,    bottom + left,

            -0.5f + k, -0.5f,  -0.5f + k, 0.0f, -1.0f,  0.0f,  uvstep * 4,  0.0f,    bottom,
            0.5f - k,  -0.5f,  -0.5f + k, 0.0f, -1.0f,  0.0f,  uvstep * 5,  0.0f,    bottom,
            0.5f - k,  -0.5f,  0.5f - k,  0.0f, -1.0f,  0.0f,  uvstep * 5,  1.0f,    bottom,
            0.5f - k,  -0.5f,  0.5f - k,  0.0f, -1.0f,  0.0f,  uvstep * 5,  1.0f,    bottom,
            -0.5f + k, -0.5f,  0.5f - k,  0.0f, -1.0f,  0.0f,  uvstep * 4,  1.0f,    bottom,
            -0.5f + k, -0.5f,  -0.5f + k, 0.0f, -1.0f,  0.0f,  uvstep * 4,  0.0f,    bottom,

            0.5f - k, -0.5f, -0.5f + k,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,    bottom + right,
            0.5f,   -0.5f, -0.5f + k,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,    bottom + right,
            0.5f,   -0.5f,  0.5f - k,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,    bottom + right,
            0.5f,   -0.5f,  0.5f - k,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,    bottom + right,
            0.5f - k, -0.5f,  0.5f - k,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,    bottom + right,
            0.5f - k, -0.5f, -0.5f + k,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,    bottom + right,


            -0.5f,   -0.5f,  0.5f - k,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,    bottom + left + front,
            -0.5f + k, -0.5f,  0.5f - k,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,    bottom + left + front,
            -0.5f + k, -0.5f,  0.5f,    0.0f, -1.0f,  0.0f,  0.0f,  0.0f,    bottom + left + front,
            -0.5f + k, -0.5f,  0.5f,    0.0f, -1.0f,  0.0f,  0.0f,  0.0f,    bottom + left + front,
            -0.5f,   -0.5f,  0.5f,    0.0f, -1.0f,  0.0f,  0.0f,  0.0f,    bottom + left + front,
            -0.5f,   -0.5f,  0.5f - k,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,    bottom + left + front,


            -0.5f + k, -0.5f,  0.5f - k, 0.0f, -1.0f,  0.0f,  0.0f,  0.0f,    bottom + front,
            0.5f - k,  -0.5f,  0.5f - k, 0.0f, -1.0f,  0.0f,  0.0f,  0.0f,    bottom + front,
            0.5f - k,  -0.5f,  0.5f,   0.0f, -1.0f,  0.0f,  0.0f,  0.0f,    bottom + front,
            0.5f - k,  -0.5f,  0.5f,   0.0f, -1.0f,  0.0f,  0.0f,  0.0f,    bottom + front,
            -0.5f + k, -0.5f,  0.5f,   0.0f, -1.0f,  0.0f,  0.0f,  0.0f,    bottom + front,
            -0.5f + k, -0.5f,  0.5f - k, 0.0f, -1.0f,  0.0f,  0.0f,  0.0f,    bottom + front,

            0.5f - k, -0.5f,  0.5f - k,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,    bottom + front + right,
            0.5f,   -0.5f,  0.5f - k,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,    bottom + front + right,
            0.5f,   -0.5f,  0.5f,    0.0f, -1.0f,  0.0f,  0.0f,  0.0f,    bottom + front + right,
            0.5f,   -0.5f,  0.5f,    0.0f, -1.0f,  0.0f,  0.0f,  0.0f,    bottom + front + right,
            0.5f - k, -0.5f,  0.5f,    0.0f, -1.0f,  0.0f,  0.0f,  0.0f,    bottom + front + right,
            0.5f - k, -0.5f,  0.5f - k,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,    bottom + front + right,


            //top
            -0.5f,    0.5f,  0.5f - k,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,    top + left + front,
            -0.5f + k,  0.5f,  0.5f,    0.0f,  1.0f,  0.0f,  0.0f,  0.0f,    top + left + front,
            -0.5f + k,  0.5f,  0.5f - k,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,    top + left + front,
            -0.5f + k,  0.5f,  0.5f,    0.0f,  1.0f,  0.0f,  0.0f,  0.0f,    top + left + front,
            -0.5f,    0.5f,  0.5f - k,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,    top + left + front,
            -0.5f,    0.5f,  0.5f,    0.0f,  1.0f,  0.0f,  0.0f,  0.0f,    top + left + front,

            -0.5f + k, 0.5f,   0.5f - k,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,    top + front,
            0.5f - k,  0.5f,   0.5f,    0.0f,  1.0f,  0.0f,  0.0f,  0.0f,    top + front,
            0.5f - k,  0.5f,   0.5f - k,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,    top + front,
            0.5f - k,  0.5f,   0.5f,    0.0f,  1.0f,  0.0f,  0.0f,  0.0f,    top + front,
            -0.5f + k, 0.5f,   0.5f - k,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,    top + front,
            -0.5f + k, 0.5f,   0.5f,    0.0f,  1.0f,  0.0f,  0.0f,  0.0f,    top + front,

            0.5f - k, 0.5f,  0.5f - k,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,    top + front + right,
            0.5f,   0.5f,  0.5f,    0.0f,  1.0f,  0.0f,  0.0f,  0.0f,    top + front + right,
            0.5f,   0.5f,  0.5f - k,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,    top + front + right,
            0.5f,   0.5f,  0.5f,    0.0f,  1.0f,  0.0f,  0.0f,  0.0f,    top + front + right,
            0.5f - k, 0.5f,  0.5f - k,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,    top + front + right,
            0.5f - k, 0.5f,  0.5f,    0.0f,  1.0f,  0.0f,  0.0f,  0.0f,    top + front + right,


            -0.5f,   0.5f,  -0.5f + k,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,    top + left,
            -0.5f + k, 0.5f,  0.5f - k,   0.0f,  1.0f,  0.0f,  0.0f,  0.0f,    top + left,
            -0.5f + k, 0.5f,  -0.5f + k,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,    top + left,
            -0.5f + k, 0.5f,  0.5f - k,   0.0f,  1.0f,  0.0f,  0.0f,  0.0f,    top + left,
            -0.5f,   0.5f,  -0.5f + k,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,    top + left,
            -0.5f,   0.5f,  0.5f - k,   0.0f,  1.0f,  0.0f,  0.0f,  0.0f,    top + left,

            -0.5f + k, 0.5f,  -0.5f + k,  0.0f,  1.0f,  0.0f,  uvstep * 5,  1.0f,    top,
            0.5f - k,  0.5f,  0.5f - k,   0.0f,  1.0f,  0.0f,  1.0f,  0.0f,    top,
            0.5f - k,  0.5f,  -0.5f + k,  0.0f,  1.0f,  0.0f,  1.0f,  1.0f,    top,
            0.5f - k,  0.5f,  0.5f - k,   0.0f,  1.0f,  0.0f,  1.0f,  0.0f,    top,
            -0.5f + k, 0.5f,  -0.5f + k,  0.0f,  1.0f,  0.0f,  uvstep * 5,  1.0f,    top,
            -0.5f + k, 0.5f,  0.5f - k,   0.0f,  1.0f,  0.0f,  uvstep * 5,  0.0f,    top,

            0.5f - k, 0.5f,  -0.5f + k,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,    top + right,
            0.5f,   0.5f,  0.5f - k,   0.0f,  1.0f,  0.0f,  0.0f,  0.0f,    top + right,
            0.5f,   0.5f,  -0.5f + k,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,    top + right,
            0.5f,   0.5f,  0.5f - k,   0.0f,  1.0f,  0.0f,  0.0f,  0.0f,    top + right,
            0.5f - k, 0.5f,  -0.5f + k,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,    top + right,
            0.5f - k, 0.5f,  0.5f - k,   0.0f,  1.0f,  0.0f,  0.0f,  0.0f,    top + right,

            -0.5f,    0.5f,  -0.5f,    0.0f,  1.0f,  0.0f,  0.0f,  0.0f,    top + left + back,
            -0.5f + k,  0.5f,  -0.5f + k,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,    top + left + back,
            -0.5f + k,  0.5f,  -0.5f,    0.0f,  1.0f,  0.0f,  0.0f,  0.0f,    top + left + back,
            -0.5f + k,  0.5f,  -0.5f + k,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,    top + left + back,
            -0.5f,    0.5f,  -0.5f,    0.0f,  1.0f,  0.0f,  0.0f,  0.0f,    top + left + back,
            -0.5f,    0.5f,  -0.5f + k,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,    top + left + back,

            -0.5f + k, 0.5f,  -0.5f,   0.0f,  1.0f,  0.0f,  0.0f,  0.0f,    top + back,
            0.5f - k,  0.5f,  -0.5f + k, 0.0f,  1.0f,  0.0f,  0.0f,  0.0f,    top + back,
            0.5f - k,  0.5f,  -0.5f,   0.0f,  1.0f,  0.0f,  0.0f,  0.0f,    top + back,
            0.5f - k,  0.5f,  -0.5f + k, 0.0f,  1.0f,  0.0f,  0.0f,  0.0f,    top + back,
            -0.5f + k, 0.5f,  -0.5f,   0.0f,  1.0f,  0.0f,  0.0f,  0.0f,    top + back,
            -0.5f + k, 0.5f,  -0.5f + k, 0.0f,  1.0f,  0.0f,  0.0f,  0.0f,    top + back,

            0.5f - k, 0.5f,  -0.5f,    0.0f,  1.0f,  0.0f,  0.0f,  0.0f,    top + back + right,
            0.5f,   0.5f,  -0.5f + k,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,    top + back + right,
            0.5f,   0.5f,  -0.5f,    0.0f,  1.0f,  0.0f,  0.0f,  0.0f,    top + back + right,
            0.5f,   0.5f,  -0.5f + k,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,    top + back + right,
            0.5f - k, 0.5f,  -0.5f,    0.0f,  1.0f,  0.0f,  0.0f,  0.0f,    top + back + right,
            0.5f - k, 0.5f,  -0.5f + k,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,    top + back + right,
        };

        int vertSize = sizeof(vertices) / sizeof(float) / 9;
        int stride = 9 * sizeof(float);
        QByteArray* bytes = new QByteArray((const char*)&vertices[0], sizeof(vertices));

        Qt3DRender::QBuffer* qBuffer = new Qt3DRender::QBuffer(Qt3DRender::QBuffer::VertexBuffer);
        qBuffer->setData(*bytes);
        delete bytes;

        Qt3DRender::QAttribute* position = new Qt3DRender::QAttribute(qBuffer, Qt3DRender::QAttribute::defaultPositionAttributeName(), Qt3DRender::QAttribute::Float, 3, vertSize, 0, stride);
        Qt3DRender::QAttribute* normal = new Qt3DRender::QAttribute(qBuffer, Qt3DRender::QAttribute::defaultNormalAttributeName(), Qt3DRender::QAttribute::Float, 3, vertSize, 3 * sizeof(float), stride);
        Qt3DRender::QAttribute* texCoord = new Qt3DRender::QAttribute(qBuffer, Qt3DRender::QAttribute::defaultTextureCoordinateAttributeName(), Qt3DRender::QAttribute::Float, 2, vertSize, 6 * sizeof(float), stride);
        Qt3DRender::QAttribute* dirId = new Qt3DRender::QAttribute(qBuffer, "facesIndex", Qt3DRender::QAttribute::Float, 1, vertSize, 8 * sizeof(float), stride);

        Qt3DRender::QGeometry* geo = GeometryCreateHelper::create(nullptr, position, normal, texCoord, dirId);
        return geo;
    }

}