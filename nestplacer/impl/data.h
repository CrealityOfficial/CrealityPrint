#ifndef _DATA_H
#define _DATA_H
#include <clipper3r/clipper.hpp>
#include "nestplacer/nestplacer.h"

namespace nestplacer {

    struct TransMatrix
    {
        Clipper3r::cInt x;
        Clipper3r::cInt y;
        double rotation;

        TransMatrix()
        {
            x = 0;
            y = 0;
            rotation = 0.;
        }

        TransMatrix(Clipper3r::cInt _x, Clipper3r::cInt _y, double _angle)
        {
            x = _x;
            y = _y;
            rotation = _angle;
        }

        void merge(TransMatrix mat)
        {
            double r = mat.rotation * M_PIf / 180;
            double c = cos(r);
            double s = sin(r);
            int x_blk = c * x - s * y;
            int y_blk = s * x + c * y;
            x = x_blk + mat.x;
            y = y_blk + mat.y;
            rotation += mat.rotation;
        }
    };

    struct NestParaCInt
    {
        Clipper3r::cInt workspaceW; //平台宽
        Clipper3r::cInt workspaceH;
        Clipper3r::cInt modelsDist;
        PlaceType packType;
        bool parallel;
        StartPoint sp;
        int rotationStep;
        ccglobal::Tracer* tracer = nullptr;

        NestParaCInt()
        {
            packType = PlaceType::CENTER_TO_SIDE;
            modelsDist = 0;
            workspaceW = 0;
            workspaceH = 0;
            parallel = true;
            sp = StartPoint::NULLTYPE;
            rotationStep = 8;
        }

        NestParaCInt(Clipper3r::cInt w, Clipper3r::cInt h, Clipper3r::cInt dist, PlaceType type, bool _parallel, StartPoint _sp, int _rotationStep = 8)
        {
            workspaceW = w;
            workspaceH = h;
            modelsDist = dist;
            packType = type;
            parallel = _parallel;
            sp = _sp;
            rotationStep = _rotationStep;
        }
    };

    struct NestParaToCInt {
        Clipper3r::cInt workspaceW; //平台宽
        Clipper3r::cInt workspaceH;
        Clipper3r::cInt modelsDist;
        Clipper3r::cInt edgeDist;
        PlaceType packType;
        bool parallel;
        StartPoint sp;
        float rotationAngle;

        NestParaToCInt()
        {
            packType = PlaceType::CENTER_TO_SIDE;
            modelsDist = 0;
            edgeDist = 0;
            workspaceW = 0;
            workspaceH = 0;
            parallel = true;
            sp = StartPoint::NULLTYPE;
            rotationAngle = 20.0f;
        }

        NestParaToCInt(Clipper3r::cInt w, Clipper3r::cInt h, Clipper3r::cInt dist, Clipper3r::cInt offset, PlaceType type, bool _parallel, StartPoint _sp, float _rotationAngle = 20.0f)
        {
            workspaceW = w;
            workspaceH = h;
            modelsDist = dist;
            edgeDist = offset;
            packType = type;
            parallel = _parallel;
            sp = _sp;
            rotationAngle = _rotationAngle;
        }
    };
}
#endif // _DATA_H