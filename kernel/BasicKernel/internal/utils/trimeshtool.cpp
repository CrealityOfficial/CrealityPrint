#include "trimeshtool.h"
#include <math.h>

#define ARC_PER_BLOCK 5

namespace creative_kernel
{
    float getAngelOfTwoVector(const trimesh::vec& pt1, const trimesh::vec& pt2, const trimesh::vec& c)
    {
        float theta = atan2(pt1.y - c.y, pt1.x - c.x) - atan2(pt2.y - c.y, pt2.x - c.x);
        if (theta > M_PIf)
            theta -= 2 * M_PIf;
        if (theta < -M_PIf)
            theta += 2 * M_PIf;

        theta = theta * 180.0 / M_PIf;
        if (theta < 0)
        {
            theta = 360 + theta;
        }
        return theta;
    }

    void getDevidePoint(const trimesh::vec& p0, const trimesh::vec& p1,
        std::vector<trimesh::vec>& out, float theta, bool clockwise)
    {
        //int x = 1, y = 2;//旋转的点
        //int dx = 1, dy = 1;//被绕着旋转的点

        int count = theta > ARC_PER_BLOCK ? theta / ARC_PER_BLOCK : theta / 2;
        int angle = theta > ARC_PER_BLOCK ? ARC_PER_BLOCK : theta / 2;
        //int count = theta / ARC_PER_BLOCK;
        //int angle = ARC_PER_BLOCK;
        out.reserve(count);
        for (int i = 1; i < count; i++)
        {
            //if (clockwise)
            //{
            //    angle = -15 * i;
            //}
            //else
            //{
            //    angle = 15 * i;
            //}
            ////int angle = 45 * i;//逆时针
            ////int angle = -15 * i;//顺时针
            //trimesh::vec _out = p0;
            //_out.x = (p0.x - p1.x) * cos(angle * PI / 180) - (p0.y - p1.y) * sin(angle * PI / 180) + p1.x;
            //_out.y = (p0.y - p1.y) * cos(angle * PI / 180) + (p0.x - p1.x) * sin(angle * PI / 180) + p1.y;
            trimesh::vec _out = p0;
            if (clockwise)
            {
                angle = ARC_PER_BLOCK * i;
                _out.x = (p1.x - p0.x) * cos(angle * M_PIf / 180) + (p1.y - p0.y) * sin(angle * M_PIf / 180) + p0.x;
                _out.y = (p1.y - p0.y) * cos(angle * M_PIf / 180) - (p1.x - p0.x) * sin(angle * M_PIf / 180) + p0.y;
            }
            else
            {
                angle = ARC_PER_BLOCK * i;

                _out.x = (p1.x - p0.x) * cos(angle * M_PIf / 180) - (p1.y - p0.y) * sin(angle * M_PIf / 180) + p0.x;
                _out.y = (p1.y - p0.y) * cos(angle * M_PIf / 180) + (p1.x - p0.x) * sin(angle * M_PIf / 180) + p0.y;

            }

            out.push_back(_out);
        }
    }
}