#include "slicepolygonbuilder.h"
#include "utilconst.h"
#include "slicehelper.h"
#include "trimesh2/TriMesh.h"

#include <fstream>

namespace topomesh
{
	SlicePolygonBuilder::SlicePolygonBuilder()
	{

	}

	SlicePolygonBuilder::~SlicePolygonBuilder()
	{

	}

	void SlicePolygonBuilder::makePolygon(Polygons* polygons, Polygons* openPolygons)
	{
		size_t segment_size = segments.size();
		if (segment_size > 0) visitFlags.resize(segment_size, false);
		for (size_t start_segment_idx = 0; start_segment_idx < segment_size; start_segment_idx++)
		{
			if (!visitFlags[start_segment_idx])
			{
				ClipperLib::Path poly;
				poly.push_back(segments[start_segment_idx].start);

				bool closed = false;
				for (int segment_idx = start_segment_idx; segment_idx != -1; )
				{
					const SlicerSegment& segment = segments[segment_idx];
					poly.push_back(segment.end);

					visitFlags[segment_idx] = true;
					segment_idx = getNextSegmentIdx(segment, start_segment_idx);
					if (segment_idx == static_cast<int>(start_segment_idx))
					{ // polyon is closed
						closed = true;
						break;
					}
				}

				if (closed)
				{
					polygons->add(poly);
				}
				else
				{
					// polygon couldn't be closed
					openPolygons->add(poly);
				}
			}
		}
	}

	coord_t interpolate_my(const coord_t x, const coord_t x0, const coord_t x1, const coord_t y0, const coord_t y1)
	{
		const coord_t dx_01 = x1 - x0;
		coord_t num = (y1 - y0) * (x - x0);
		num += num > 0 ? dx_01 / 2 : -dx_01 / 2; // add in offset to round result
		return y0 + num / dx_01;
	}

	void project2D(const Point3& p0, const Point3& p1, const Point3& p2, const coord_t z, SlicerSegment& seg)
	{
		if (seg.faceIndex == -1)
		{
			seg.start.X = interpolate_my(z, p0.z, p1.z, p0.x, p1.x);
			seg.start.Y = interpolate_my(z, p0.z, p1.z, p0.y, p1.y);
		}
		else
		{
			seg.start.X = seg.end.X;
			seg.start.Y = seg.end.Y;
		}
		seg.end.X = interpolate_my(z, p0.z, p2.z, p0.x, p2.x);
		seg.end.Y = interpolate_my(z, p0.z, p2.z, p0.y, p2.y);

	}

	bool zCrossFace(trimesh::TriMesh* mesh, SliceHelper* helper, int faceId, int z, SlicerSegment& s)
	{
		const trimesh::TriMesh::Face& face = mesh->faces[faceId];

		// get all vertices represented as 3D point
		Point3 p0 = Point3(MM2INT(mesh->vertices[face[0]].x), MM2INT(mesh->vertices[face[0]].y), MM2INT(mesh->vertices[face[0]].z));
		Point3 p1 = Point3(MM2INT(mesh->vertices[face[1]].x), MM2INT(mesh->vertices[face[1]].y), MM2INT(mesh->vertices[face[1]].z));
		Point3 p2 = Point3(MM2INT(mesh->vertices[face[2]].x), MM2INT(mesh->vertices[face[2]].y), MM2INT(mesh->vertices[face[2]].z));

		int end_edge_idx = -1;

		if (p0.z < z && p1.z >= z && p2.z >= z)
		{
			project2D(p0, p2, p1, z, s);
			if (p1.z == z)
			{
				if (s.end != s.start)
				{
					s.end.X = p1.x;
					s.end.Y = p1.y;
					s.endOtherFaceIdx = -1;
					return true;
				}
				else
				{
					return false;
				}
			}
			else
			{
				end_edge_idx = 0;
			}
		}
		else if (p0.z > z && p1.z < z && p2.z < z)
		{
			project2D(p0, p1, p2, z, s);
			end_edge_idx = 2;
		}
		else if (p1.z < z && p0.z >= z && p2.z >= z)
		{
			project2D(p1, p0, p2, z, s);
			if (p2.z == z)
			{
				if (s.end != s.start)
				{
					s.end.X = p2.x;
					s.end.Y = p2.y;
					s.endOtherFaceIdx = -1;
					return true;
				}
				else
				{
					return false;
				}
			}
			else
			{
				end_edge_idx = 1;
			}
		}
		else if (p1.z > z && p0.z < z && p2.z < z)
		{
			project2D(p1, p2, p0, z, s);
			end_edge_idx = 0;
		}
		else if (p2.z < z && p1.z >= z && p0.z >= z)
		{
			project2D(p2, p1, p0, z, s);
			if (p0.z == z)
			{
				if (s.end != s.start)
				{
					s.end.X = p0.x;
					s.end.Y = p0.y;
					s.endOtherFaceIdx = -1;
					return true;
				}
				else
				{
					return false;
				}
			}
			else
			{
				end_edge_idx = 2;
			}
		}
		else if (p2.z > z && p1.z < z && p0.z < z)
		{
			project2D(p2, p0, p1, z, s);
			end_edge_idx = 1;
		}
		else
		{
			return false;
			//Not all cases create a segment, because a point of a face could create just a dot, and two touching faces
			//  on the slice would create two segments
		}

		s.faceIndex = faceId;
		if (end_edge_idx != -1)
		{
			s.endOtherFaceIdx = helper->faces[faceId].connected_face_index[end_edge_idx];
		}
		return true;
	}

	void SlicePolygonBuilder::sliceOneLayer_dst(SliceHelper* helper, int z, Polygons* polygons, Polygons* openPolygons)
	{
		trimesh::TriMesh* meshSrc = helper->meshSrc;
		const std::vector<Point2>& faceRanges = helper->faceRanges;
		int faceNum = meshSrc->faces.size();

		if (faceNum > 0) visitFlags.resize(faceNum, false);
		for (int i = 0; i < faceNum; ++i)
		{
			if ((z < faceRanges.at(i).x) || (z > faceRanges.at(i).y))
				continue;
			if (visitFlags[i]) continue;
			bool bFirstSegment = true;
			bool bClosed = false;
			SlicerSegment s;
			ClipperLib::Path poly;
			poly.reserve(100);
			for (int faceid = i; faceid != -1; )
			{
				if (visitFlags[faceid]) //开轮廓
				{
					break;
				}
				if (zCrossFace(meshSrc, helper, faceid, z, s))
				{
					if (bFirstSegment) poly.push_back(s.start);
					poly.push_back(s.end);
					visitFlags[faceid] = true;

					faceid = s.endOtherFaceIdx;
					if (bFirstSegment) bFirstSegment = false;
				}
				else
				{
					break; //开轮廓
				}

				if (faceid == i) //闭轮廓
				{
					bClosed = true;
					break;
				}
			}
			if (bClosed)
			{
				ClipperLib::CleanPolygon(poly);
				polygons->add(poly);
			}
			else if (!poly.empty())
			{////开轮廓不能在这里清理轮廓，否则在后续开轮廓闭合时会出错
				openPolygons->add(poly);
			}
		}
	}

	int SlicePolygonBuilder::tryFaceNextSegmentIdx(const SlicerSegment& segment,
		const int face_idx, const size_t start_segment_idx)
	{
		decltype(face_idx_to_segment_idx.begin()) it;
		auto it_end = face_idx_to_segment_idx.end();
		it = face_idx_to_segment_idx.find(face_idx);
		if (it != it_end)
		{
			const int segment_idx = (*it).second;
            Point p0 = segment.end;
            p0.X = DLP_S_UM(p0.X);
            p0.Y = DLP_S_UM(p0.Y);
			Point p1 = segments[segment_idx].start;
            p1.X = DLP_S_UM(p1.X);
            p1.Y = DLP_S_UM(p1.Y);
            Point diff = p0 - p1;
			if (shorterThen(diff, largest_neglected_gap_first_phase))
			{
				if (segment_idx == static_cast<int>(start_segment_idx))
				{
					return start_segment_idx;
				}
				if (visitFlags[segment_idx])
				{
					return -1;
				}
				return segment_idx;
			}
		}

		return -1;
	}

	int SlicePolygonBuilder::getNextSegmentIdx(const SlicerSegment& segment, const size_t start_segment_idx)
	{
		int next_segment_idx = -1;

		const bool segment_ended_at_edge = segment.endVertex == nullptr;
		if (segment_ended_at_edge)
		{
			const int face_to_try = segment.endOtherFaceIdx;
			if (face_to_try == -1)
			{
				return -1;
			}
			return tryFaceNextSegmentIdx(segment, face_to_try, start_segment_idx);
		}
		else
		{
			// segment ended at vertex

			const std::vector<uint32_t>& faces_to_try = segment.endVertex->connected_faces;
			for (int face_to_try : faces_to_try)
			{
				const int result_segment_idx =
					tryFaceNextSegmentIdx(segment, face_to_try, start_segment_idx);
				if (result_segment_idx == static_cast<int>(start_segment_idx))
				{
					return start_segment_idx;
				}
				else if (result_segment_idx != -1)
				{
					// not immediately returned since we might still encounter the start_segment_idx
					next_segment_idx = result_segment_idx;
				}
			}
		}

		return next_segment_idx;
	}

	void SlicePolygonBuilder::write(std::fstream& out)
	{
		for (size_t j = 0; j < segments.size(); ++j)
		{
			const SlicerSegment& segment = segments.at(j);
			out << segment.start << " " << segment.endOtherFaceIdx << std::endl;
			out << segment.start << " " << segment.end << std::endl;
		}
		for (const std::pair<int, int>& p : face_idx_to_segment_idx)
		{
			out << p.first << " " << p.second << std::endl;
		}
	}

    double dot(const Point& A, const Point& B, const Point& P)      //向量的内积
    {
        Point AB = B-A;
        Point AP = P -A;
        return AB.X* AP.X + AB.Y * AP.Y;
    }

    double cross(const Point& A, const Point& B, const Point& P)  //向量的外积
    {
        Point AB = B- A;
        Point AP = P-A;
        return AB.X* AP.Y - AB.Y * AP.X;
    }

    double dis2(const Point& a, const Point& b)           //两点间的距离的平方
    {
        return (b.X - a.X) * (b.X - a.X) + (b.Y - a.Y) * (b.Y - a.Y);

    }

    int dir(const Point& A, const Point& B, const Point& P)     //点与线段方位判定
    {
        if (cross(A, B, P) > 0)  return -1;
        else if (cross(A, B, P) < 0) return 1;
        else if (dot(A, B, P) < 0) return -2;
        else if (dot(A, B, P) >= 0)
        {
            if (dis2(A, B) < dis2(A, P)) return 2;
            else return 0;
        }
        return 0;
    }

    double disLine(const Point& A, const Point& B, const Point& P)    //点P到直线AB的距离
    {
        return fabs(cross(A, B, P)) / sqrt(dis2(A, B));
    }

    void tryConnectPath(Polygons& open_polylines, int cell_size, bool needReverse = false)
    {
        //获取开多边形的起始点
        std::vector<SlicerSegment> openSegs;
        for (const ClipperLib::Path& path : open_polylines.paths)
        {
            if (path.size() < 2)
                continue;
            SlicerSegment seg;
            seg.start = path.front();
            seg.start.X = DLP_S_UM(seg.start.X);
            seg.start.Y = DLP_S_UM(seg.start.Y);
            seg.end = path.back();
            seg.end.X = DLP_S_UM(seg.end.X);
            seg.end.Y = DLP_S_UM(seg.end.Y);
            openSegs.push_back(seg);
        }

        //segment: a-->b   c-->d
        //connect: a-->b-->c-->d    c-->d-->a-->b   Positive 
        //connect: a-->b-->d-->c    c-->d-->b-->a   negative
        int i = 0;
        while (i < openSegs.size())
        {
            SlicerSegment& seg1 = openSegs.at(i);
            if (seg1.addedToPolygon)
            {
                i++;
                continue;
            }

            int j = 0;
            while (j < openSegs.size())
            {
                if (open_polylines.paths[0].size() > 350)
                {
                    int test = 0;
                }


                SlicerSegment& seg2 = openSegs.at(j);
                if (i == j || seg2.addedToPolygon)
                {
                    j++;
                    continue;
                }

                Point diff = seg1.end - seg2.start;
                bool bdiff = shorterThen(diff, cell_size);
                if (bdiff)
                { //a-->b-->c-->d 
                    open_polylines.paths[i].insert(open_polylines.paths[i].end(), open_polylines.paths[j].begin(), open_polylines.paths[j].end());
                    open_polylines.paths[j].clear();
                    seg2.addedToPolygon = true;
                    seg1.end = seg2.end;

                    j = 0;
                    continue;
                }
                Point diff1 = seg2.end - seg1.start;
                bool bdiff1 = shorterThen(diff1, cell_size);
                if (bdiff1)
                { //c-->d-->a-->b
                    open_polylines.paths[i].insert(open_polylines.paths[i].begin(), open_polylines.paths[j].begin(), open_polylines.paths[j].end());
                    open_polylines.paths[j].clear();
                    seg2.addedToPolygon = true;
                    seg1.start = seg2.start;

                    j = 0;
                    continue;
                }
                Point diff2 = seg2.end - seg1.end;
                bool bdiff2 = shorterThen(diff2, cell_size);
                if (bdiff2)
                { //a-->b-->d-->c
                    std::reverse(open_polylines.paths[j].begin(), open_polylines.paths[j].end());
                    open_polylines.paths[i].insert(open_polylines.paths[i].end(), open_polylines.paths[j].begin(), open_polylines.paths[j].end());
                    open_polylines.paths[j].clear();
                    seg2.addedToPolygon = true;
                    seg1.end = seg2.start;

                    j = 0;
                    continue;
                }

                Point diff3 = seg2.end - seg1.end;
                bool bdiff3 = shorterThen(diff3, cell_size);
                if (bdiff3)
                { //c-->d-->b-->a
                    std::reverse(open_polylines.paths[i].begin(), open_polylines.paths[i].end());
                    open_polylines.paths[i].insert(open_polylines.paths[i].begin(), open_polylines.paths[j].begin(), open_polylines.paths[j].end());
                    open_polylines.paths[j].clear();
                    seg2.addedToPolygon = true;
                    seg1.end = seg1.start;
                    seg1.start = seg2.start;


                    j = 0;
                    continue;
                }

                j++;
            }
            i++;
        }

        for (auto it = open_polylines.paths.begin(); it != open_polylines.paths.end(); )
        {
            if (it->empty())
            {
                it = open_polylines.paths.erase(it);
            }
            else
                it++;
        }
    }

    void tryConnectPath1(Polygons& open_polylines, int cell_size, bool needReverse = false)
    {
        //获取开多边形的起始点
        std::vector<SlicerSegment> openSegs;
        for (const ClipperLib::Path& path : open_polylines.paths)
        {
            if (path.size() < 2)
                continue;
            SlicerSegment seg;
            seg.start = path.front();
            seg.end = path.back();
            openSegs.push_back(seg);
        }

        for (size_t i = 0; i < openSegs.size(); i++)
        {
            SlicerSegment& seg1 = openSegs.at(i);

            for (size_t j = 0; j < openSegs.size(); j++)
            {
                SlicerSegment& seg2 = openSegs.at(j);
                if (i == j || seg2.addedToPolygon || seg1.addedToPolygon || open_polylines.paths[i].empty())
                    continue;

                Point diff = seg1.end - seg2.start;
                diff.X = DLP_S_UM(diff.X);
                diff.Y = DLP_S_UM(diff.Y);
                bool bdiff = shorterThen(diff, cell_size);
                if (bdiff)
                {
                    open_polylines.paths[i].insert(open_polylines.paths[i].end(), open_polylines.paths[j].begin(), open_polylines.paths[j].end());
                    open_polylines.paths[j].clear();
                    seg2.addedToPolygon = true;
                    seg1.end = open_polylines.paths[i].back();
                    break;
                }
                else if (needReverse) //处理反序
                {
                    Point diffR1 = seg1.start - seg2.end;
                    Point diffR2 = seg1.end - seg2.end;
                    Point diffR3 = seg1.start - seg2.start;
                    diffR1.X = DLP_S_UM(diffR1.X);
                    diffR1.Y = DLP_S_UM(diffR1.Y);
                    diffR2.X = DLP_S_UM(diffR2.X);
                    diffR2.Y = DLP_S_UM(diffR2.Y);
                    diffR3.X = DLP_S_UM(diffR3.X);
                    diffR3.Y = DLP_S_UM(diffR3.Y);
                    bool bdiffR1 = shorterThen(diffR1, cell_size);
                    bool bdiffR2 = shorterThen(diffR2, cell_size);
                    bool bdiffR3 = shorterThen(diffR3, cell_size);
                    if (bdiffR1)
                    {
                        open_polylines.paths[i].insert(open_polylines.paths[i].begin(), open_polylines.paths[j].begin(), open_polylines.paths[j].end());
                        open_polylines.paths[j].clear();
                        seg2.addedToPolygon = true;
                        seg1.end = open_polylines.paths[i].back();
                        break;
                    }
                    else if (bdiffR2)
                    {
                        if (open_polylines.paths[i].size() > open_polylines.paths[j].size())
                        {
                            std::reverse(open_polylines.paths[j].begin(), open_polylines.paths[j].end());
                            open_polylines.paths[i].insert(open_polylines.paths[i].end(), open_polylines.paths[j].begin(), open_polylines.paths[j].end());
                        }
                        else
                        {
                            std::reverse(open_polylines.paths[i].begin(), open_polylines.paths[i].end());
                            open_polylines.paths[i].insert(open_polylines.paths[i].begin(), open_polylines.paths[j].begin(), open_polylines.paths[j].end());
                        }

                        open_polylines.paths[j].clear();
                        seg2.addedToPolygon = true;
                        seg1.end = open_polylines.paths[i].back();
                        break;
                    }
                    else if (bdiffR3)
                    {
                        if (open_polylines.paths[i].size() > open_polylines.paths[j].size())
                        {
                            std::reverse(open_polylines.paths[j].begin(), open_polylines.paths[j].end());
                            open_polylines.paths[i].insert(open_polylines.paths[i].begin(), open_polylines.paths[j].begin(), open_polylines.paths[j].end());
                        }
                        else
                        {
                            std::reverse(open_polylines.paths[i].begin(), open_polylines.paths[i].end());
                            open_polylines.paths[i].insert(open_polylines.paths[i].end(), open_polylines.paths[j].begin(), open_polylines.paths[j].end());
                        }

                        open_polylines.paths[j].clear();
                        seg2.addedToPolygon = true;
                        seg1.end = open_polylines.paths[i].back();
                        break;
                    }
                }
            }
        }

        for (auto it = open_polylines.paths.begin(); it != open_polylines.paths.end(); )
        {
            if (it->empty())
            {
                it = open_polylines.paths.erase(it);
            }
            else
                it++;
        }
    }

    // 返回值：点C到线段AB的最近距离；nearestPoint是线段AB上距离点C最近的点
    float PointToLineSegment(const Point& C, const Point& A, const Point& B, Point& nearestPoint)
    {
        Point ac = C - A;
        Point ab = B - A;//线段所在直线向量

        float f = ac.X * ab.X + ac.Y * ab.Y;
        float f2 = ab.X * ab.X + ab.Y * ab.Y;
        if (f < 0)//C点在AB上的投影位于A的左侧    不在线段AB的范围内
        {
            nearestPoint = A;
            return sqrt(dis2(C, A));
        }
        else if (f > f2)//C点在AB上的投影位于B的右侧     不在线段AB的范围内
        {
            nearestPoint = B;
            return sqrt(dis2(C, B));
        }
        else//C点在AB上的投影位于线段AB内部
        {
            float abLen2 = dis2(A, B);//ab长度的平方
            nearestPoint = A;
            float epsilon = 0.000001f;
            if (abLen2 - 0.0f > epsilon)//ab长度不等于0，亦即A、B两点不重合
            {
                nearestPoint += ab * (f / abLen2);//(f/abLen2)是指的倍数关系
            }
            return sqrt(dis2(nearestPoint, C));
        }
        return 0.0f;
    }

    //包含端点
    bool tryConnectSegmentSE(Point& A1, Point& A2, Point& B1, Point& B2, Point& ps1, Point& ps2)
    {
        ////快速排斥实验
        //if ((A1.X > A2.X ? A1.X : A2.X) < (B1.X < B2.X ? B1.X : B2.X) ||
        //    (A1.Y > A2.Y ? A1.Y : A2.Y) < (B1.Y < B2.Y ? B1.Y : B2.Y) ||
        //    (B1.X > B2.X ? B1.X : B2.X) < (A1.X < A2.X ? A1.X : A2.X) ||
        //    (B1.Y > B2.Y ? B1.Y : B2.Y) < (A1.Y < A2.Y ? A1.Y : A2.Y))
        //{
        //    return false;
        //}

        if (dir(A1, A2, B1) * dir(A1, A2, B2) <= 0 && dir(B1, B2, A1) * dir(B1, B2, A2) <= 0)
        {//判断有无交点
            double t = disLine(A1, A2, B1) / (disLine(A1, A2, B1) + disLine(A1, A2, B2));

            ClipperLib::IntPoint B1B2 = B2 - B1;
            ClipperLib::IntPoint p = { B1.X + (ClipperLib::cInt)(B1B2.X * t), B1.Y + (ClipperLib::cInt)(B1B2.Y * t) };
            ps1 = p;
            ps2 = p;

            //判断交点有效
            if (std::min(A1.X, A2.X) <= p.X && p.X <= std::max(A1.X, A2.X)
                && std::min(A1.Y, A2.Y) <= p.Y && p.Y <= std::max(A1.Y, A2.Y))
            {
                return true;
            }
        }
        else
        {
            int cellSize = largest_neglected_gap_first_phase * 10;
            Point nearestPoint;
            //float len1 = PointToLineSegment(A1, B1, B2, nearestPoint);
            //float len2 = PointToLineSegment(A2, B1, B2, nearestPoint);
            //float len3 = PointToLineSegment(B1, A1, A2, nearestPoint);
            //float len4 = PointToLineSegment(B2, A1, A2, nearestPoint);
            if (PointToLineSegment(A1, B1, B2, nearestPoint) <= cellSize)
            {
                ps1 = A1;
                ps2 = nearestPoint;
                return true;
            }
            else if (PointToLineSegment(A2, B1, B2, nearestPoint) <= cellSize)
            {
                ps1 = A2;
                ps2 = nearestPoint;
                return true;
            }
            else if (PointToLineSegment(B1, A1, A2, nearestPoint) <= cellSize)
            {
                ps1 = nearestPoint;
                ps2 = B1;
                return true;
            }
            else if (PointToLineSegment(B2, A1, A2, nearestPoint) <= cellSize)
            {
                ps1 = nearestPoint;
                ps2 = B2;
                return true;
            }
        }
        return false;
    }

    class polyStartEnd
    {
    public:
        int curPathIndex = -1;

        int startIndex = -1;
        int startOtherIndex = -1;
        Point start = Point(0,0);
        Point startOther = Point(0, 0);
        int startNextPathIndex = -1;

        int endIndex = -1;
        int endOtherIndex = -1;;
        Point end = Point(0, 0);
        Point endOther = Point(0, 0);
        int endNextPathIndex = -1;
    };

    bool pointInSegment(const Point& A1, const Point& A2, const Point& p)
    {
        if (std::min(A1.X, A2.X) <= p.X && p.X <= std::max(A1.X, A2.X)
            && std::min(A1.Y, A2.Y) <= p.Y && p.Y <= std::max(A1.Y, A2.Y))
        {
            float len1 = std::sqrt(dis2(A1.X, A2.X) + dis2(A1.Y, A2.Y));
            float len2 = std::sqrt(dis2(p.X, A1.X) + dis2(p.Y, A1.Y)) + std::sqrt(dis2(p.X, A2.X) + dis2(p.Y, A2.Y));
            if (fabs(len1 - len2) < 0.1)
            {
                //if ((A1.X != p.X && A1.Y != p.Y) && (A2.X != p.X && A2.Y != p.Y))
                {
                    return true;
                }
            }
        }
        return false;
    }

    //严格检测不包含端点
    bool tryConnectSegment(const Point& A1, const Point& A2
        , const Point& B1, const Point& B2
        , Point& ps1, Point& ps2)
    {
        //快速排斥实验
        if ((A1.X > A2.X ? A1.X : A2.X) < (B1.X < B2.X ? B1.X : B2.X) ||
            (A1.Y > A2.Y ? A1.Y : A2.Y) < (B1.Y < B2.Y ? B1.Y : B2.Y) ||
            (B1.X > B2.X ? B1.X : B2.X) < (A1.X < A2.X ? A1.X : A2.X) ||
            (B1.Y > B2.Y ? B1.Y : B2.Y) < (A1.Y < A2.Y ? A1.Y : A2.Y))
        {
            return false;
        }

        if (dir(A1, A2, B1) * dir(A1, A2, B2) <= 0 && dir(B1, B2, A1) * dir(B1, B2, A2) <= 0)
        {//判断有无交点
            double t = disLine(A1, A2, B1) / (disLine(A1, A2, B1) + disLine(A1, A2, B2));

            Point B1B2 = B2-B1;
            Point p = { B1.X + (ClipperLib::cInt)(B1B2.X * t), B1.Y + (ClipperLib::cInt)(B1B2.Y * t) };
            ps1 = p;
            ps2 = p;
            if (pointInSegment(A1, A2, p) && pointInSegment(B1, B2, p))
                return true;
        }
        else
        {
            //cout << min({ disMin(A1, A2, B1), disMin(A1, A2, B2), disMin(B1, B2, A1),disMin(B1,B2,A2) }) << endl;
            int cellSize = largest_neglected_gap_first_phase * 2;
            Point nearestPoint;

            if (PointToLineSegment(A1, B1, B2, nearestPoint) <= cellSize)
            {
                ps1 = A1;
                ps2 = nearestPoint;
                return true;
            }
            else if (PointToLineSegment(A2, B1, B2, nearestPoint) <= cellSize)
            {
                ps1 = A2;
                ps2 = nearestPoint;
                return true;
            }
            else if (PointToLineSegment(B1, A1, A2, nearestPoint) <= cellSize)
            {
                ps1 = nearestPoint;
                ps2 = B1;
                return true;
            }
            else if (PointToLineSegment(B2, A1, A2, nearestPoint) <= cellSize)
            {
                ps1 = nearestPoint;
                ps2 = B2;
                return true;
            }

        }
        return false;
    }

    bool findintersectStart(ClipperLib::Path& path, ClipperLib::Path& pathOther, polyStartEnd& ployStartEnd)
    {
        //查找起始点 //超过一半舍弃
        for (size_t k = 0; k < path.size() / 2; k++)
        {
            Point A1 = path[k];
            Point A2 = path[(k + 1) % path.size()];
            A1.X = DLP_S_UM(A1.X);
            A1.Y = DLP_S_UM(A1.Y);
            A2.X = DLP_S_UM(A2.X);
            A2.Y = DLP_S_UM(A2.Y);
            for (size_t m = 0; m < pathOther.size() - 1; m++)
            {
                Point B1 = pathOther[m];
                Point B2 = pathOther[(m + 1) % pathOther.size()];
                B1.X = DLP_S_UM(B1.X);
                B1.Y = DLP_S_UM(B1.Y);
                B2.X = DLP_S_UM(B2.X);
                B2.Y = DLP_S_UM(B2.Y);
                if (tryConnectSegment(A1, A2, B1, B2, ployStartEnd.start, ployStartEnd.startOther))
                {
                    ployStartEnd.start.X = MM2INT(ployStartEnd.start.X);
                    ployStartEnd.start.Y = MM2INT(ployStartEnd.start.Y);
                    ployStartEnd.startOther.X = MM2INT(ployStartEnd.startOther.X);
                    ployStartEnd.startOther.Y = MM2INT(ployStartEnd.startOther.Y);

                    ployStartEnd.startIndex = k;
                    ployStartEnd.startOtherIndex = m;
                    return true;
                }
            }
        }

        return false;
    }

    bool findintersectEnd(const ClipperLib::Path& path, const ClipperLib::Path& pathOther, polyStartEnd& ployStartEnd)
    {
        //查找起始点
        for (int k = path.size() - 2; k >= path.size() / 2; k--)
        {
            Point A1 = path[k];
            Point A2 = path[(k + 1) % path.size()];
            A1.X = DLP_S_UM(A1.X);
            A1.Y = DLP_S_UM(A1.Y);
            A2.X = DLP_S_UM(A2.X);
            A2.Y = DLP_S_UM(A2.Y);
            for (int m = 0; m < pathOther.size() - 1; m++)
            {
                Point B1 = pathOther[m];
                Point B2 = pathOther[(m + 1) % pathOther.size()];
                B1.X = DLP_S_UM(B1.X);
                B1.Y = DLP_S_UM(B1.Y);
                B2.X = DLP_S_UM(B2.X);
                B2.Y = DLP_S_UM(B2.Y);

                if (tryConnectSegment(A1, A2, B1, B2, ployStartEnd.end, ployStartEnd.endOther))
                {
                    ployStartEnd.end.X = MM2INT(ployStartEnd.end.X);
                    ployStartEnd.end.Y = MM2INT(ployStartEnd.end.Y);
                    ployStartEnd.endOther.X = MM2INT(ployStartEnd.endOther.X);
                    ployStartEnd.endOther.Y = MM2INT(ployStartEnd.endOther.Y);

                    ployStartEnd.endIndex = k;
                    ployStartEnd.endOtherIndex = m;
                    return true;
                }
            }
        }

        return false;
    }

    void addTopaths(ClipperLib::Path& path, polyStartEnd& pse, ClipperLib::Path& pathresult)
    {
        if (pse.endIndex > pse.startIndex && pse.startIndex >= 0 && pse.endIndex < path.size())
        {
            if (path[pse.startIndex % path.size()].X != pse.start.X || path[pse.startIndex % path.size()].Y != pse.start.Y)
            {
                pathresult.push_back(pse.start);
            }

            for (size_t j = pse.startIndex + 1; j < pse.endIndex; j++)
            {
                pathresult.push_back(path[j]);
            }
            if (path[pse.endIndex % path.size()].X != pse.end.X || path[pse.endIndex % path.size()].Y != pse.end.Y)
                pathresult.push_back(pse.end);
        }
    }

    void trySplitConnected(Polygons& open_polylines, Polygons& connected, int cell_size)
    {
        for (ClipperLib::Path& path : open_polylines.paths)
        {
            if (path.size() > 3)
            {
                //查找起始点 //超过一半舍弃
                Point A1 = path[0];
                Point A2 = path[1];
                A1.X = DLP_S_UM(A1.X);
                A1.Y = DLP_S_UM(A1.Y);
                A2.X = DLP_S_UM(A2.X);
                A2.Y = DLP_S_UM(A2.Y);

                int end = 0;
                for (int i = path.size() - 2; i > path.size() / 2; i--)
                {
                    Point B1 = path[i];
                    Point B2 = path[(i + 1) % path.size()];
                    Point p1;
                    Point p2;
                    B1.X = DLP_S_UM(B1.X);
                    B1.Y = DLP_S_UM(B1.Y);
                    B2.X = DLP_S_UM(B2.X);
                    B2.Y = DLP_S_UM(B2.Y);
                    if (tryConnectSegmentSE(A1, A2, B1, B2, p1, p2))
                    {
                        ClipperLib::Path pathconnect;
                        pathconnect.insert(pathconnect.end(), path.begin(), path.begin() + i + 2);
                        if (pathconnect.size() > 3)
                        {
                            connected.add(pathconnect);
                        }

                    }
                }

                //检测结束
                Point A3 = path[path.size() - 1];
                Point A4 = path[path.size() - 2];
                A3.X = DLP_S_UM(A3.X);
                A3.Y = DLP_S_UM(A3.Y);
                A4.X = DLP_S_UM(A4.X);
                A4.Y = DLP_S_UM(A4.Y);
                for (int i = 0; i < path.size() / 2; i++)
                {
                    Point B3 = path[i];
                    Point B4 = path[(i + 1) % path.size()];
                    Point p1;
                    Point p2;
                    B3.X = DLP_S_UM(B3.X);
                    B3.Y = DLP_S_UM(B3.Y);
                    B4.X = DLP_S_UM(B4.X);
                    B4.Y = DLP_S_UM(B4.Y);
                    if (tryConnectSegmentSE(A3, A4, B3, B4, p1, p2))
                    {
                        ClipperLib::Path pathconnect;
                        pathconnect.insert(pathconnect.end(), path.begin() + i, path.end());
                        if (pathconnect.size() > 3)
                        {
                            connected.add(pathconnect);
                            break;
                        }
                    }
                }
            }
        }
    }

    void insertClosedPloy(Polygons& polylines, Polygons& open_polylines)
    {
        std::vector<bool> needAdd(polylines.paths.size(), false);
        for (int i = 0; i < polylines.paths.size(); i++)
        {
            Polygons poly;
            poly.add(polylines.paths[i]);
            for (int j = 0; j < open_polylines.paths.size(); j++)
            {
                Polygons polyOpen;
                polyOpen.add(open_polylines.paths[j]);
                bool border_resultMin = false;
                bool border_resultMax = false;
                if (poly.inside(polyOpen.min(), border_resultMin) || poly.inside(polyOpen.max(), border_resultMax))
                {
                    needAdd[i] = true;
                    break;
                }
                else if (border_resultMin || border_resultMax)
                {
                    needAdd[i] = true;
                    break;
                }

                if (polyOpen.inside(poly.min(), border_resultMin) || polyOpen.inside(poly.max(), border_resultMax))
                {
                    needAdd[i] = true;
                    break;
                }
                else if (border_resultMin || border_resultMax)
                {
                    needAdd[i] = true;
                    break;
                }
            }
        }

        for (int i = 0; i < needAdd.size(); i++)
        {
            if (needAdd[i])
            {
                open_polylines.paths.push_back(polylines.paths[i]);
            }
        }
    }

    void tryConnectEnd(Polygons& polylines,Polygons& open_polylines, int cell_size, ClipperLib::Path& intersectPoints)
    {
        //加入闭合多边形 //相关的闭合多边形
        insertClosedPloy(polylines, open_polylines);

        //从端点开始查找最近的点
        std::vector<std::vector<polyStartEnd>> polyStartEnds;
        for (size_t i = 0; i < open_polylines.paths.size(); i++)
        {
            ClipperLib::Path& path = open_polylines.paths[i];
            polyStartEnds.push_back(std::vector<polyStartEnd >());
            bool bstart = false;
            bool bend = false;
            for (size_t j = 0; j < open_polylines.paths.size(); j++)
            {
                if (i == j)
                    continue;

                ClipperLib::Path & pathOther = open_polylines.paths[j];

                polyStartEnd ployStartEnd;
                ployStartEnd.curPathIndex = i;
                bstart = findintersectStart(path, pathOther, ployStartEnd);
                bend = findintersectEnd(path, pathOther, ployStartEnd);
                if (bstart)
                {
                    intersectPoints.push_back(ployStartEnd.start);
                    intersectPoints.push_back(ployStartEnd.startOther);
                    ployStartEnd.startNextPathIndex = j;
                }

                if (bend)
                {
                    intersectPoints.push_back(ployStartEnd.end);
                    intersectPoints.push_back(ployStartEnd.endOther);
                    ployStartEnd.endNextPathIndex = j;
                }

                if (bstart || bend)
                    polyStartEnds.back().push_back(ployStartEnd);

            }
        }

        for (size_t i = 0; i < polyStartEnds.size(); i++)
        {
            std::vector<polyStartEnd>& polys = polyStartEnds[i];
            bool open = true;
            if (true)
            {
                for (auto poly : polys)
                {
                    if (poly.startIndex != -1 && poly.endIndex != -1)
                    {
                        open = false;
                    }
                }
                if (open)
                {
                    polyStartEnd newPoly;
                    newPoly.curPathIndex = i;
                    for (auto poly : polys)
                    {
                        if (poly.startIndex != -1)
                        {
                            if (newPoly.startIndex == -1 || newPoly.startIndex > poly.startIndex)
                            {
                                newPoly.start = poly.start;
                                newPoly.startIndex = poly.startIndex;
                                newPoly.startNextPathIndex = poly.startNextPathIndex;
                                newPoly.startOther = poly.startOther;
                                newPoly.startOtherIndex = poly.startOtherIndex;
                            }
                        }

                        if (poly.end != -1)
                        {
                            if (newPoly.endIndex == -1 || newPoly.endIndex < poly.endIndex)
                            {
                                newPoly.end = poly.end;
                                newPoly.endIndex = poly.endIndex;
                                newPoly.endNextPathIndex = poly.endNextPathIndex;
                                newPoly.endOther = poly.endOther;
                                newPoly.endOtherIndex = poly.endOtherIndex;
                            }
                        }
                    }
                    if (newPoly.startNextPathIndex >= 0 && newPoly.endNextPathIndex >= 0)
                    {
                        polys.push_back(newPoly);
                    }
                }
            }
        }

        long long count = 1;
        for (auto poly : polyStartEnds)
        {
            count *= (poly.size()>0 ? poly.size() : 1);
        }

        //首尾连接
        Polygons result;
        for (int i = 0; i < open_polylines.paths.size(); i++)
        {
            ClipperLib::Path& path = open_polylines.paths[i];
            for (int j = 0; j < polyStartEnds[i].size(); j++)
            {
                polyStartEnd& pse = polyStartEnds[i][j];
                ClipperLib::Path pathresult;
                addTopaths(path, pse, pathresult);

                //两个轮廓闭合的情况
                if (pse.startNextPathIndex == pse.endNextPathIndex)
                {
                    ClipperLib::Path& pathOther = open_polylines.paths[pse.startNextPathIndex % open_polylines.paths.size()];
                    if (pse.endOtherIndex < pse.startOtherIndex && pse.endOtherIndex >= 0)
                    {
                        if (pathOther[pse.endOtherIndex %pathOther.size()].X != pse.endOther.X || pathOther[pse.endOtherIndex %pathOther.size()].Y != pse.endOther.Y)
                            pathresult.push_back(pse.endOther);
                        for (int k = pse.endOtherIndex + 1; k < pse.startOtherIndex; k++)
                        {
                            pathresult.push_back(pathOther[k % pathOther.size()]);
                        }
                        if (pathOther[pse.startOtherIndex %pathOther.size()].X != pse.startOther.X || pathOther[pse.startOtherIndex %pathOther.size()].Y != pse.startOther.Y)
                            pathresult.push_back(pse.startOther);

                        result.paths.push_back(pathresult);
                    }
                }
                else if (pse.startNextPathIndex >= 0 && pse.endNextPathIndex >= 0)
                {
                    int nextIndex = pse.endNextPathIndex;
                    int curIndex = i;
                    polyStartEnd curpoly = pse;

                    long long cnt = count * 5;
                    while (cnt-- > 0)
                    {
                        ClipperLib::Path& pathOther = open_polylines.paths[nextIndex %open_polylines.paths.size()];
                        polyStartEnd PolySE;
                        PolySE.startIndex = curpoly.endOtherIndex;
                        PolySE.start = curpoly.endOther;

                        std::vector<polyStartEnd>& polys = polyStartEnds[nextIndex %polyStartEnds.size()];
                        bool hasConnect = false;
                        for (auto poly : polys)
                        {
                            if (poly.startNextPathIndex >= 0 && poly.endNextPathIndex >= 0)
                            {
                                if (poly.endNextPathIndex == i && pse.startNextPathIndex == poly.endNextPathIndex)
                                {
                                    PolySE.endIndex = pse.startOtherIndex;
                                    PolySE.end = pse.startOther;
                                    curpoly = pse;
                                    hasConnect = false;
                                    addTopaths(pathOther, PolySE, pathresult);

                                    result.paths.push_back(pathresult);
                                    break;
                                }
                                else if (poly.startNextPathIndex == curIndex)
                                {
                                    PolySE.endIndex = poly.endIndex;
                                    PolySE.end = poly.end;
                                    curpoly = poly;
                                    
                                    addTopaths(pathOther, PolySE, pathresult);
                                    curIndex = nextIndex;
                                    nextIndex = poly.endNextPathIndex;
                                    if (nextIndex != i)
                                        hasConnect = true;
                                    else
                                        result.paths.push_back(pathresult);
                                    break;
                                }
                            }
                        }
                        if (!hasConnect)
                        {
                            break;
                        }
                    }
                }
            }
        }
        result = result.unionPolygons();
        open_polylines.paths.swap(result.paths);

    }

    void SlicePolygonBuilder::connectOpenPolylines(Polygons& polygons, Polygons& open_polylines,ClipperLib::Path& intersectPoints)
    {
        Polygons open_polylinesOrigin = open_polylines;
        if (open_polylinesOrigin.empty())
        {
            return;
        }
        constexpr bool allow_reverse = false;
        constexpr coord_t cell_size = largest_neglected_gap_first_phase * 2;

        //线段连接成多边形
        tryConnectPath(open_polylinesOrigin, largest_neglected_gap_first_phase);

        //提取自身闭合的部分
        Polygons connected;
        trySplitConnected(open_polylinesOrigin, connected, largest_neglected_gap_first_phase);

        //合并闭合多边形
        if (!connected.paths.empty())
        {
            polygons.paths.insert(polygons.paths.end(), connected.paths.begin(), connected.paths.end());
        }

        //尝试首尾相连
        tryConnectEnd(polygons, open_polylinesOrigin, cell_size, intersectPoints);

        //合并闭合多边形
        if (!open_polylinesOrigin.paths.empty())
        {
            polygons.paths.insert(polygons.paths.end(), open_polylinesOrigin.paths.begin(), open_polylinesOrigin.paths.end());
        }

        //polygons = polygons.unionPolygons();

    }

    void SlicePolygonBuilder::connectOpenPolylines(Polygons& polygons, Polygons& openPolygons)
    {
        //线段连接成多边形
        tryConnectPath(openPolygons, largest_neglected_gap_first_phase);

        //再次线段连接成多边形(包含反向线段检测)
        constexpr bool allow_reverse = false;
        constexpr coord_t cell_size = largest_neglected_gap_first_phase * 2;
        if (openPolygons.paths.size())
        {
            tryConnectPath(openPolygons, cell_size, true);
        }

        //提取自身闭合的部分
        trySplitConnected(openPolygons, polygons, largest_neglected_gap_first_phase);
    }

    void SlicePolygonBuilder::removeSamePoint(Polygons& openPolygons)
    {
        std::vector<std::pair<int, int>> pathNum(openPolygons.paths.size());
        for (int i = 0; i < openPolygons.paths.size(); i++)
        {
            pathNum[i].first = openPolygons.paths[i].size();
            pathNum[i].second = i;
        }

        for (size_t i = 0; i < pathNum.size(); i++)
        {
            for (int j = i + 1; j < pathNum.size(); j++)
            {
                if (pathNum[i].first > pathNum[j].first)
                {
                    std::pair<int, int> temp = std::pair<int, int>(pathNum[i]);
                    pathNum[i] = pathNum[j];
                    pathNum[j] = temp;
                }
            }
        }

        std::vector<bool> needDelete(pathNum.size(), false);
        for (size_t i = 0; i < pathNum.size(); i++)
        {
            if (needDelete[i])
            {
                continue;
            }
            auto& pathi = openPolygons.paths.at(pathNum[i].second);
            for (int j = i + 1; j < pathNum.size(); j++)
            {
                if (needDelete[j])
                {
                    continue;
                }

                auto& pathj = openPolygons.paths.at(pathNum[j].second);
                Polygons polygons2;
                polygons2.add(pathj);
                polygons2 = polygons2.offsetPolyLine(10);
                bool inside = true;
                for (auto& p : pathi)
                {
                    if (!polygons2.inside(p, true))
                    {
                        inside = false;
                        break;
                    }
                }

                if (inside)
                {
                    needDelete[i] = true;
                    break;
                }
            }
        }

        Polygons _openPolygons;
        for (int i = 0; i < pathNum.size(); i++)
        {
            if (!needDelete[i])
            {
                _openPolygons.add(openPolygons.paths[pathNum[i].second]);
            }
        }

        openPolygons = _openPolygons;
    }

}