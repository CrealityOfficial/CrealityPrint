#include "paintzseam.h"
#include "linearAlg2D.h"
#include "communication/MeshGroup.h"
#include "settings/ZSeamConfig.h"
#include "pathPlanning/PathOrderOptimizer.h"
#include "communication/sliceDataStorage.h"

namespace cura52
{
	paintzseam::paintzseam(SliceDataStorage* _storage, const size_t _total_layers)
		:storage(_storage),
		total_layers(_total_layers)
	{

	}

	void paintzseam::paint()
	{
		markerZSeam(storage->zSeamPoints, ExtrusionJunction::PAINT);
		generateZSeam();
	}

	void paintzseam::intercept()
	{
		markerZSeam(storage->interceptSeamPoints, ExtrusionJunction::INTERCEPT);
	}

	void paintzseam::markerZSeam(std::vector<ZseamDrawlayer>& _layersPoints, ExtrusionJunction::paintFlag _flag)
	{
		for (int layer_nr = 0; layer_nr < total_layers; layer_nr++)
		{
			int icount = 0;
			for (SliceMeshStorage& mesh : storage->meshes)
			{
				for (SliceLayerPart& part : mesh.layers[layer_nr].parts)
				{
					for (VariableWidthLines& path : part.wall_toolpaths)
					{
						for (ExtrusionLine& line : path)
						{
							if (line.inset_idx == 0 && line.is_closed)
							{
								for (ZseamDrawPoint& aPoint : _layersPoints[layer_nr].ZseamLayers)
								{
									if (aPoint.flag)//已经标记过的涂抹点，跳过
									{
										continue;
									}

									coord_t minPPdis = std::numeric_limits<coord_t>::max();
									int minPPidex=getDisPtoJunctions(aPoint.start, line.junctions, minPPdis);
									if (minPPdis < 1000)//涂抹点到轮廓点的最短距离
									{
										line.junctions[minPPidex].flag = _flag;
										aPoint.flag = true;
									}
									else
									{

										int realsize = line.junctions[0] == line.junctions[line.junctions.size() - 1] ? line.junctions.size() - 1 : line.junctions.size();
										int preIdx = (minPPidex + realsize - 1) % realsize;
										int nextIdx = (minPPidex + 1) % realsize;

										coord_t pre_dis = getDistFromSeg(aPoint.start, line.junctions[minPPidex].p, line.junctions[preIdx].p);
										coord_t next_dis = getDistFromSeg(aPoint.start, line.junctions[minPPidex].p, line.junctions[nextIdx].p);
										if (pre_dis >= 1000 && next_dis >= 1000)//距离大于1000，则认为涂抹点和轮廓没有匹配上，跳过
										{
											continue;
										}

										if (pre_dis < next_dis)
										{
											Point addPoint = getDisPtoSegment(aPoint.start, line.junctions[minPPidex].p, line.junctions[preIdx].p);
											if (std::abs(addPoint.X - line.junctions[minPPidex].p.X) < 500 && std::abs(addPoint.Y - line.junctions[minPPidex].p.Y) < 500)
											{
												line.junctions[minPPidex].flag = _flag;
												aPoint.flag = true;
												continue;
											}
											ExtrusionJunction addEJ = line.junctions.at(preIdx);
											addEJ.p = addPoint;
											addEJ.flag = _flag;
											aPoint.flag = true;
											if (minPPidex == 0)
											{
												minPPidex = preIdx + 1;
											}
											line.junctions.insert(line.junctions.begin() + minPPidex, addEJ);
										}
										else
										{
											Point addPoint = getDisPtoSegment(aPoint.start, line.junctions[minPPidex].p, line.junctions[nextIdx].p);
											if (std::abs(addPoint.X - line.junctions[minPPidex].p.X) < 500 && std::abs(addPoint.Y - line.junctions[minPPidex].p.Y) < 500)
											{
												line.junctions[minPPidex].flag = _flag;
												aPoint.flag = true;
												continue;
											}
											ExtrusionJunction addEJ = line.junctions.at(nextIdx);
											addEJ.p = addPoint;
											addEJ.flag = _flag;
											aPoint.flag = true;
											if (nextIdx == 0)
											{
												nextIdx = minPPidex + 1;
											}
											line.junctions.insert(line.junctions.begin() + nextIdx, addEJ);
										}

									}
								}
							}
						}
					}
				}
			}
		}
	}

	void paintzseam::generateZSeam()
	{
		std::vector<Point> PreZseamPoints;
		std::vector<Point> curZseamPoints;
		for (int layer_nr = 0; layer_nr < total_layers; layer_nr++)
		{
			for (SliceMeshStorage& mesh : storage->meshes)
			{
				for (SliceLayerPart& part : mesh.layers[layer_nr].parts)
				{
					for (VariableWidthLines& path : part.wall_toolpaths)
					{
						for (ExtrusionLine& line : path)
						{
							if (line.inset_idx == 0 && line.is_closed)
							{
								if (layer_nr == 0)
								{
									float minCornerAngle = M_PI;
									int minCorner_idx = -1;//最尖角索引
									for (int n = 0; n < line.junctions.size() - 1; n++)
									{
										if (line.junctions[n].flag == ExtrusionJunction::PAINT)
										{
											//int preIdx = (n - 1 + line.junctions.size()) % line.junctions.size();
											//int nextIdx = (n + 1) % line.junctions.size();
											int realsize = line.junctions[0] == line.junctions[line.junctions.size() - 1] ? line.junctions.size() - 1 : line.junctions.size();
											int preIdx = (n + realsize - 1) % realsize;
											int nextIdx = (n + 1) % realsize;
											float pab = getAngleLeft(line.junctions[preIdx].p, line.junctions[n].p, line.junctions[nextIdx].p);
											if (minCornerAngle >= pab)
											{
												minCornerAngle = pab;
												minCorner_idx = n;
											}
										}
									}
									if (minCorner_idx != -1)
									{
										line.start_idx = minCorner_idx;
										curZseamPoints.push_back(line.junctions[minCorner_idx].p);
									}
								}
								else
								{
									coord_t minPPdis = std::numeric_limits<coord_t>::max();
									int minPPidex = -1;
									Point shortest_point;
									for (int n = 0; n < line.junctions.size() - 1; n++)
									{
										if (line.junctions[n].flag == ExtrusionJunction::PAINT)
										{
											for (Point& apoint : PreZseamPoints)
											{
												coord_t ppdis = vSize(line.junctions[n].p - apoint);
												if (minPPdis > ppdis /*&& ppdis<3000*/)
												{
													minPPdis = ppdis;
													minPPidex = n;//最短距离
													shortest_point = apoint;
												}
											}
										}
									}

									float minAngle = M_PI;
									float shortestAngle = M_PI;
									int sharpCorner_idx = -1;
									for (int n = 0; n < line.junctions.size() - 1; n++)
									{
										if (line.junctions[n].flag== ExtrusionJunction::PAINT)
										{
											//int preIdx = (n - 1 + line.junctions.size()) % line.junctions.size();
											//int nextIdx = (n + 1) % line.junctions.size();
											int realsize = line.junctions[0] == line.junctions[line.junctions.size() - 1] ? line.junctions.size() - 1 : line.junctions.size();
											int preIdx = (n + realsize - 1) % realsize;
											int nextIdx = (n + 1) % realsize;
											float pab = getAngleLeft(line.junctions[preIdx].p, line.junctions[n].p, line.junctions[nextIdx].p);
											if (minAngle >= pab)//最尖角
											{
												minAngle = pab;
												sharpCorner_idx = n;
											}
											if (minPPidex == n)
											{
												shortestAngle = pab;
											}
										}
									}

									if ((std::abs(shortestAngle - minAngle) < (M_PI / 6) || minAngle > (M_PI * 3 / 4)) && minPPidex >= 0)
									{
										int realsize = line.junctions[0] == line.junctions[line.junctions.size() - 1] ? line.junctions.size() - 1 : line.junctions.size();
										int preIdx = (minPPidex + realsize - 1) % realsize;
										int nextIdx = (minPPidex + 1) % realsize;
										if (line.junctions[preIdx].flag == ExtrusionJunction::PAINT && line.junctions[nextIdx].flag== ExtrusionJunction::PAINT)
										{
											coord_t pre_dis = getDistFromSeg(shortest_point, line.junctions[minPPidex].p, line.junctions[preIdx].p);
											coord_t next_dis = getDistFromSeg(shortest_point, line.junctions[minPPidex].p, line.junctions[nextIdx].p);
											if (pre_dis < next_dis)
											{
												Point addPoint = getDisPtoSegment(shortest_point, line.junctions[minPPidex].p, line.junctions[preIdx].p);
												if (addPoint.X == line.junctions[minPPidex].p.X && addPoint.Y == line.junctions[minPPidex].p.Y)
												{
													line.start_idx = minPPidex;
													curZseamPoints.push_back(line.junctions[minPPidex].p);
													continue;
												}
												ExtrusionJunction addEJ = line.junctions.at(preIdx);
												addEJ.p = addPoint;
												addEJ.flag = ExtrusionJunction::PAINT;
												if (minPPidex == 0)
												{
													minPPidex = preIdx + 1;
												}
												line.junctions.insert(line.junctions.begin() + minPPidex, addEJ);
												line.start_idx = minPPidex;
												curZseamPoints.push_back(line.junctions[minPPidex].p);
											}
											else
											{
												Point addPoint = getDisPtoSegment(shortest_point, line.junctions[minPPidex].p, line.junctions[nextIdx].p);
												if (addPoint.X == line.junctions[minPPidex].p.X && addPoint.Y == line.junctions[minPPidex].p.Y)
												{
													line.start_idx = minPPidex;
													curZseamPoints.push_back(line.junctions[minPPidex].p);
													continue;
												}
												ExtrusionJunction addEJ = line.junctions.at(nextIdx);
												addEJ.p = addPoint;
												addEJ.flag = ExtrusionJunction::PAINT;
												if (nextIdx == 0)
												{
													nextIdx = minPPidex + 1;
												}
												line.junctions.insert(line.junctions.begin() + nextIdx, addEJ);
												line.start_idx = nextIdx;
												curZseamPoints.push_back(line.junctions[nextIdx].p);
											}
										}
										else if (line.junctions[preIdx].flag== ExtrusionJunction::PAINT)
										{
											Point addPoint = getDisPtoSegment(shortest_point, line.junctions[minPPidex].p, line.junctions[preIdx].p);
											if (addPoint.X == line.junctions[minPPidex].p.X && addPoint.Y == line.junctions[minPPidex].p.Y)
											{
												line.start_idx = minPPidex;
												curZseamPoints.push_back(line.junctions[minPPidex].p);
												continue;
											}
											ExtrusionJunction addEJ = line.junctions.at(preIdx);
											addEJ.p = addPoint;
											addEJ.flag = ExtrusionJunction::PAINT;
											if (minPPidex == 0)
											{
												minPPidex = preIdx + 1;
											}
											line.junctions.insert(line.junctions.begin() + minPPidex, addEJ);
											line.start_idx = minPPidex;
											curZseamPoints.push_back(line.junctions[minPPidex].p);
										}
										else if (line.junctions[nextIdx].flag== ExtrusionJunction::PAINT)
										{
											Point addPoint = getDisPtoSegment(shortest_point, line.junctions[minPPidex].p, line.junctions[nextIdx].p);
											if (addPoint.X == line.junctions[minPPidex].p.X && addPoint.Y == line.junctions[minPPidex].p.Y)
											{
												line.start_idx = minPPidex;
												curZseamPoints.push_back(line.junctions[minPPidex].p);
												continue;
											}
											ExtrusionJunction addEJ = line.junctions.at(nextIdx);
											addEJ.p = addPoint;
											addEJ.flag = ExtrusionJunction::PAINT;
											if (nextIdx == 0)
											{
												nextIdx = minPPidex + 1;
											}
											line.junctions.insert(line.junctions.begin() + nextIdx, addEJ);
											line.start_idx = nextIdx;
											curZseamPoints.push_back(line.junctions[nextIdx].p);
										}
										else
										{
											line.start_idx = minPPidex;
											curZseamPoints.push_back(line.junctions[minPPidex].p);
										}
									}
									else
									{
										if (sharpCorner_idx != -1)
										{
											line.start_idx = sharpCorner_idx;
											curZseamPoints.push_back(line.junctions[sharpCorner_idx].p);
										}
									}

								}
							}

						}

					}

				}

			}
			PreZseamPoints = curZseamPoints;
			curZseamPoints.clear();
		}
	}

	coord_t paintzseam::getDistFromSeg(const Point& p, const Point& Seg_start, const Point& Seg_end)
	{
		float pab = LinearAlg2D::getAngleLeft(p, Seg_start, Seg_end);
		float pba = LinearAlg2D::getAngleLeft(p, Seg_end, Seg_start);
		if (pab > M_PI / 2 && pab < 3 * M_PI / 2) {
			return vSize(p - Seg_start);
		}
		else if (pba > M_PI / 2 && pba < 3 * M_PI / 2) {
			return vSize(p - Seg_end);
		}
		else {
			return LinearAlg2D::getDistFromLine(p, Seg_start, Seg_end);
		}
	}

	Point paintzseam::getDisPtoSegment(Point& apoint, Point& Segment_start, Point& Segment_end)
	{
		double A = Segment_end.Y - Segment_start.Y;     //y2-y1
		double B = Segment_start.X - Segment_end.X;     //x1-x2;
		double C = Segment_end.X * Segment_start.Y - Segment_start.X * Segment_end.Y;     //x2*y1-x1*y2
		if (A * A + B * B < 1e-13) {
			return Segment_start;   //Segment_start与Segment_end重叠
		}
		else {
			double x = (B * B * apoint.X - A * B * apoint.Y - A * C) / (A * A + B * B);
			double y = (-A * B * apoint.X + A * A * apoint.Y - B * C) / (A * A + B * B);
			Point fpoint = Point();
			fpoint.X = x;
			fpoint.Y = y;

			if (pointOnSegment(fpoint, Segment_start, Segment_end))
			{
				return fpoint;
			}

			if (vSize(Point(Segment_start.X - apoint.X, Segment_start.Y - apoint.Y)) < vSize(Point(Segment_end.X - apoint.X, Segment_end.Y - apoint.Y)))
			{
				return Segment_start;
			}
			else
			{
				return Segment_end;
			}
		}
	}

	bool paintzseam::pointOnSegment(Point p, Point Segment_start, Point Segment_end)
	{
		if (p.X >= std::min(Segment_start.X, Segment_end.X) &&
			p.X <= std::max(Segment_start.X, Segment_end.X) &&
			p.Y >= std::min(Segment_start.Y, Segment_end.Y) &&
			p.Y <= std::max(Segment_start.Y, Segment_end.Y))
		{
			if ((Segment_end.X - Segment_start.X) * (p.Y - Segment_start.Y) == (p.X - Segment_start.X) * (Segment_end.Y - Segment_start.Y))
			{
				return true;
			}
		}
		return false;
	}

	float paintzseam::getAngleLeft(const Point& a, const Point& b, const Point& c)
	{
		float angle = LinearAlg2D::getAngleLeft(a, b, c);
		if (angle > M_PI)
		{
			angle = 2 * M_PI - angle;
		}
		return angle;
	}

	int paintzseam::getDisPtoJunctions(Point& aPoint, std::vector<ExtrusionJunction>& junctions, coord_t& minPPdis)
	{
		int minPPidex = -1;
		int icount = junctions[0] == junctions[junctions.size() - 1] ? junctions.size() - 1 : junctions.size();
		for (int n = 0; n < icount; n++)
		{
			coord_t ppdis = vSize(junctions[n].p - aPoint);
			if (minPPdis > ppdis)
			{
				minPPdis = ppdis;
				minPPidex = n;
			}
		}
		return minPPidex;
	}

	void paintzseam::processZSeam(MeshGroup* mesh_group, AngleDegrees& z_seam_min_angle_diff, AngleDegrees& z_seam_max_angle, coord_t& wall_line_width_0, coord_t& wall_line_count)
	{
		//点P到线段ab 最短的距离的点result
		auto getProjection = [](const Point& p, const Point& a, const Point& b, Point& result)
		{
			Point base = b - a;
			if (vSize2(base) == 0)
			{
				return false;
			}
			float pab = LinearAlg2D::getAngleLeft(p, a, b);
			float pba = LinearAlg2D::getAngleLeft(p, b, a);
			if (pab > M_PI / 2 && pab < 3 * M_PI / 2) {
				return false;
			}
			else if (pba > M_PI / 2 && pba < 3 * M_PI / 2) {
				return false;
			}
			else {
				double r = dot(p - a, base) / (float)vSize2(base);
				result = a + base * r;
				if (vSize2(result - a) > MM2_2INT(3.0))
					return false;
			}
			return true;
		};

		auto getDistFromSeg = [](const Point& p, const Point& a, const Point& b)
		{
			float pab = LinearAlg2D::getAngleLeft(p, a, b);
			float pba = LinearAlg2D::getAngleLeft(p, b, a);
			if (pab > M_PI / 2 && pab < 3 * M_PI / 2) {
				return vSize(p - a);
			}
			else if (pba > M_PI / 2 && pba < 3 * M_PI / 2) {
				return vSize(p - b);
			}
			else {
				return LinearAlg2D::getDistFromLine(p, a, b);
			}
		};

		//点P距离path轮廓最近的点的索引
		auto closestPointToIdx = [&](Polygon path, Point p, coord_t& bestDist)
		{
			int ret = -1;
			bestDist = std::numeric_limits<coord_t>::max();
			int size = (int)path.size();
			for (int n = 0; n < size; n++)
			{
				Point cur_position = (*path)[n];
				Point next_position = (*path)[(n + 1 + size) % size];
				coord_t v_ab_dis = getDistFromSeg(p, cur_position, next_position);
				if (v_ab_dis < bestDist)
				{
					ret = n;
					bestDist = v_ab_dis;
				}
			}
			return vSize2((*path)[ret] - p) < vSize2((*path)[(ret + 1 + size) % size] - p) ? ret : (ret + 1 + size) % size;
		};


		//轮廓pts的点距离轮廓path最近的点的索引
		auto closestPtIdx = [&](Polygon path, std::vector<Point> pts, Point& nearest_pt, coord_t dist_limit)
		{
			coord_t dis_min = std::numeric_limits<coord_t>::max();
			int closestPtIdx = -1;
			for (const Point& pt : pts)
			{
				coord_t dis = std::numeric_limits<coord_t>::max();
				int idx = closestPointToIdx(path, pt, dis);
				if (dis < dis_min && dis < dist_limit)
				{
					dis_min = dis;
					closestPtIdx = idx;
					nearest_pt = pt;
				}
			}
			return  closestPtIdx;
		};

		//起始点是否悬空，如果悬空则顺序取下一个点
		auto getSupportedVertex = [](Polygons below_outline, ExtrusionLine wall, int start_idx)
		{
			if (below_outline.empty() || start_idx < 0)
			{
				return start_idx;
			}

			int curr_idx = start_idx;

			while (true)
			{
				const Point& vertex = cura52::make_point(wall[curr_idx]);
				if (below_outline.inside(vertex, true) && wall[curr_idx].flag != ExtrusionJunction::INTERCEPT)
				{
					// vertex isn't above air so it's OK to use
					return curr_idx;
				}

				if (++curr_idx >= wall.size())
				{
					curr_idx = 0;
				}

				if (curr_idx == start_idx)
				{
					// no vertices are supported so just return the original index
					return start_idx;
				}
			}
		};

		auto findClosestPointToIdx = [&](Polygon path, Point p, coord_t dist_limit, coord_t dist_limit_max)
		{
			int ret = -1;
			int size = (int)path.size();
			for (int n = 0; n < size; n++)
			{
				Point cur_position = (*path)[n];
				Point next_position = (*path)[(n + 1 + size) % size];
				coord_t v_ab_dis = getDistFromSeg(p, cur_position, next_position);
				if (v_ab_dis < dist_limit_max && v_ab_dis > dist_limit)
				{
					ret = n;
				}
			}
			if (ret < 0)
			{
				return ret;
			}
			return vSize2((*path)[ret] - p) < vSize2((*path)[(ret + 1 + size) % size] - p) ? ret : (ret + 1 + size) % size;
		};
		auto findClosest = [&](Polygon path, std::vector<Point> pts, coord_t dist_limit, coord_t dist_limit_max)
		{
			int closestPtIdx = -1;
			for (const Point& pt : pts)
			{
				return  findClosestPointToIdx(path, pt, dist_limit, dist_limit_max);
			}
			return  closestPtIdx;
		};

		auto minLength = [&](std::vector<Point>& pts, ExtrusionLine& line, Point& nearest_pt, int index, coord_t dist_limit, coord_t dist_limit_max)
		{
			int closestIdx = -1;
			for (const Point& pt : pts)
			{
				coord_t length = vSize(pt - nearest_pt);
				if (length <= dist_limit)
				{
					return findClosest(line.toRealPolygon(), pts, dist_limit, dist_limit_max);
				}
			}
			return  closestIdx;
		};

		//MeshGroup* mesh_group = application->currentGroup();
		//AngleDegrees z_seam_min_angle_diff = mesh_group->settings.get<AngleDegrees>("z_seam_min_angle_diff");
		//AngleDegrees z_seam_max_angle = mesh_group->settings.get<AngleDegrees>("z_seam_max_angle");
		//coord_t wall_line_width_0 = mesh_group->settings.get<coord_t>("wall_line_width_0");
		//coord_t wall_line_count = mesh_group->settings.get<coord_t>("wall_line_count");

		std::vector<Point> last_layer_start_pt;//上一层的所有轮廓起始点坐标
		for (int layer_nr = 0; layer_nr < total_layers; layer_nr++)
		{
			std::vector<Point> current_layer_start_pt;//当前层的所有轮廓起始点坐标
			Polygons mesh_last_layer_outline;
			for (SliceMeshStorage& mesh : storage->meshes)
			{
				if (layer_nr > 0)
				{
					for (SliceLayerPart part : mesh.layers[layer_nr - 1].parts)
					{
						mesh_last_layer_outline.add(part.outline);
					}
					//计算非悬空区域
					mesh_last_layer_outline = mesh_last_layer_outline.offset(0.5 * wall_line_width_0);
				}
				ZSeamConfig z_seam_config;
				if (mesh.isPrinted()) //"normal" meshes with walls, skin, infill, etc. get the traditional part ordering based on the z-seam settings.
				{
					z_seam_config = ZSeamConfig(mesh.settings.get<EZSeamType>("z_seam_type"), mesh.getZSeamHint(), mesh.settings.get<EZSeamCornerPrefType>("z_seam_corner"), wall_line_width_0 * 2);
				}
				for (SliceLayerPart& part : mesh.layers[layer_nr].parts)
				{
					for (VariableWidthLines& lines : part.wall_toolpaths)//wall_toolpaths 0 外壁，1,2,3...内壁
					{
						PathOrderOptimizer<const ExtrusionLine*> part_order_optimizer(Point(), z_seam_config);
						std::vector<Point> last_layer_Line_startPoint;//上一层的轮廓外壁起始点坐标
						for (const ExtrusionLine& line : lines)
						{
							if (line.inset_idx == 0 && line.is_closed)
							{
								part_order_optimizer.addPolygon(&line);
								Point nearest_pt;
								part_order_optimizer.last_layer_start_idx.push_back(closestPtIdx(line.toRealPolygon(), last_layer_start_pt, nearest_pt, MM2INT(5.0)));
								part_order_optimizer.vvctpaintFlag.push_back(line.toFlag());
								last_layer_Line_startPoint.push_back(nearest_pt);
							}
						}
						part_order_optimizer.bFound = std::vector(part_order_optimizer.last_layer_start_idx.size(), true);
						part_order_optimizer.z_seam_max_angle = z_seam_max_angle * M_PI / 180;
						part_order_optimizer.z_seam_min_angle_diff = z_seam_min_angle_diff * M_PI / 180;

						std::vector<int> start_idx;
						part_order_optimizer.findZSeam(start_idx);
						int idx = 0;
						for (ExtrusionLine& line : lines)
						{
							if (line.inset_idx == 0 && line.is_closed)
							{
								line.start_idx = getSupportedVertex(mesh_last_layer_outline, line, start_idx[idx]);
								if (line.start_idx != -1 && !part_order_optimizer.bFound[idx] )
								{
									Polygon _path = line.toPolygon();
									Point lastLayerNearestZSeam = last_layer_Line_startPoint[idx];
									Point pre_pt = _path[(line.start_idx - 1 + _path.size()) % _path.size()];
									Point cur_pt = _path[line.start_idx];
									Point next_pt = _path[(line.start_idx + 1) % _path.size()];
									Point result;
									if (getProjection(lastLayerNearestZSeam, cur_pt, pre_pt, result))
									{
										ExtrusionJunction new_pt = ExtrusionJunction(result, line.junctions[line.start_idx].w, line.junctions[line.start_idx].perimeter_index, line.junctions[line.start_idx].overhang_distance);
										line.junctions.insert(line.junctions.begin() + line.start_idx, new_pt);
									}
									else if (getProjection(lastLayerNearestZSeam, cur_pt, next_pt, result))
									{
										ExtrusionJunction new_pt = ExtrusionJunction(result, line.junctions[line.start_idx].w, line.junctions[line.start_idx].perimeter_index, line.junctions[line.start_idx].overhang_distance);
										line.junctions.insert(line.junctions.begin() + line.start_idx + 1, new_pt);
										line.start_idx++;
									}
								}

								//同层Z缝点的距离检测，如果在Z缝距离在 [wall_line_width_0*3, wall_line_width_0 * (wall_line_count + 1)]范围内，则替换为最近的点
								int index = minLength(current_layer_start_pt, line, line.junctions[line.start_idx].p, line.start_idx, wall_line_width_0 * 3, wall_line_width_0 * (wall_line_count + 1));
								if (index >= 0 && index < line.junctions.size())
								{
									line.start_idx = index;
								}
								current_layer_start_pt.push_back(line.junctions[line.start_idx].p);
								idx++;
							}
							else//内壁起始点，选择离当前层所有起始点 最近的点
							{
								Point nearest_pt;
								line.start_idx = closestPtIdx(line.toRealPolygon(), current_layer_start_pt, nearest_pt, wall_line_width_0 * (wall_line_count + 1));
							}
						}
					}
				}
			}
			last_layer_start_pt.swap(current_layer_start_pt);
		}
	}

}


