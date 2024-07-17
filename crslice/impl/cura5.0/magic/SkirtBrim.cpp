// Copyright (c) 2022 Ultimaker B.V.
// CuraEngine is released under the terms of the AGPLv3 or higher

#include "SkirtBrim.h"

#include "communication/sliceDataStorage.h"
#include "communication/slicecontext.h"

#include "types/Ratio.h"

#include "utils/Simplify.h" //Simplifying the brim/skirt at every inset.

#define MINAREA 100


namespace cura52
{

	void SkirtBrim::getFirstLayerOutline(SliceDataStorage& storage, const size_t primary_line_count, const bool is_skirt, Polygons& first_layer_outline, bool is_autoBrim)
	{
		const ExtruderTrain& train = storage.application->currentGroup()->settings.get<ExtruderTrain&>("skirt_brim_extruder_nr");
		const ExtruderTrain& support_infill_extruder = storage.application->currentGroup()->settings.get<ExtruderTrain&>("support_infill_extruder_nr");
		const bool external_only = is_skirt || train.settings.get<bool>("brim_outside_only"); // Whether to include holes or not. Skirt doesn't have any holes.
		const LayerIndex layer_nr = 0;
		if (is_skirt)
		{
			constexpr bool include_support = true;
			constexpr bool include_prime_tower = true;
			first_layer_outline = storage.getLayerOutlines(layer_nr, include_support, include_prime_tower, external_only);
			first_layer_outline = first_layer_outline.approxConvexHull();
		}
		else
		{ // add brim underneath support by removing support where there's brim around the model
			constexpr bool include_support = false; // Include manually below.
			constexpr bool include_prime_tower = false; // Include manually below.
			constexpr bool external_outlines_only = false; // Remove manually below.
			constexpr bool for_brim = true;
			if (is_autoBrim)
			{
				first_layer_outline = storage.getLayerOutlines(layer_nr, include_support, include_prime_tower, true, false);
				return;
			}
			else
			{
				first_layer_outline = storage.getLayerOutlines(layer_nr, include_support, include_prime_tower, external_outlines_only, for_brim);
			}

			first_layer_outline = first_layer_outline.unionPolygons(); // To guard against overlapping outlines, which would produce holes according to the even-odd rule.
			Polygons first_layer_empty_holes;
			if (external_only)
			{
				first_layer_empty_holes = first_layer_outline.getEmptyHoles();
				first_layer_outline = first_layer_outline.removeEmptyHoles();

				Polygons resultPolygons;
				for (ClipperLib::Paths::iterator it = first_layer_outline.begin(); it != first_layer_outline.end(); it++)
				{
					if (it == first_layer_outline.begin())
					{
						resultPolygons.add(*it);
					}
					else
					{
						Polygons tempPolygons;
						tempPolygons.add(*it);
						Polygons ret;
						ClipperLib::Clipper clipper(clipper_init);
						clipper.AddPaths(resultPolygons.paths, ClipperLib::ptSubject, true);
						clipper.AddPaths(tempPolygons.paths, ClipperLib::ptClip, true);
						clipper.Execute(ClipperLib::ctUnion, ret.paths, ClipperLib::PolyFillType::pftEvenOdd, ClipperLib::PolyFillType::pftEvenOdd);
						resultPolygons = ret;
					}
				}
				first_layer_outline.clear();
				first_layer_outline = resultPolygons;
			}
			if (storage.support.generated && primary_line_count > 0 && !storage.support.supportLayers.empty())
			{ // remove model-brim from support
				SupportLayer& support_layer = storage.support.supportLayers[0];
				if (support_infill_extruder.settings.get<bool>("brim_replaces_support"))
				{
					// avoid gap in the middle
					//    V
					//  +---+     +----+
					//  |+-+|     |+--+|
					//  || ||     ||[]|| > expand to fit an extra brim line
					//  |+-+|     |+--+|
					//  +---+     +----+
					const coord_t primary_extruder_skirt_brim_line_width = train.settings.get<coord_t>("skirt_brim_line_width") * train.settings.get<Ratio>("initial_layer_line_width_factor");
					Polygons model_brim_covered_area = first_layer_outline.offset(primary_extruder_skirt_brim_line_width * (primary_line_count + primary_line_count % 2),
						ClipperLib::jtRound); // always leave a gap of an even number of brim lines, so that it fits if it's generating brim from both sides
					if (external_only)
					{ // don't remove support within empty holes where no brim is generated.
						model_brim_covered_area.add(first_layer_empty_holes);
					}
					AABB model_brim_covered_area_boundary_box(model_brim_covered_area);
					support_layer.excludeAreasFromSupportInfillAreas(model_brim_covered_area, model_brim_covered_area_boundary_box);

					// If the gap between the model and the BP is small enough, support starts with the interface instead, so remove it there as well:
					support_layer.support_roof = support_layer.support_roof.difference(model_brim_covered_area);
				}
				for (const SupportInfillPart& support_infill_part : support_layer.support_infill_parts)
				{
					first_layer_outline.add(support_infill_part.outline);
				}
				first_layer_outline.add(support_layer.support_bottom);
				first_layer_outline.add(support_layer.support_roof);
			}
			if (storage.primeTower.enabled && !train.settings.get<bool>("prime_tower_brim_enable"))
			{
				first_layer_outline.add(storage.primeTower.outer_poly_first_layer); // don't remove parts of the prime tower, but make a brim for it
			}
		}
		constexpr coord_t join_distance = 20;
		first_layer_outline = first_layer_outline.offset(join_distance).offset(-join_distance); // merge adjacent models into single polygon
		constexpr coord_t smallest_line_length = 200;
		constexpr coord_t largest_error_of_removed_point = 50;
		first_layer_outline = Simplify(smallest_line_length, largest_error_of_removed_point, 0).polygon(first_layer_outline);
		if (first_layer_outline.size() == 0)
		{
			LOGE("Couldn't generate skirt / brim! No polygons on first layer.");
		}
	}

	// BBS: properties of an expolygon
	struct ExPolyProp
	{
		double aera = 0;
		ClipperLib::DoublePoint  centroid;
		ClipperLib::DoublePoint  secondMomentOfAreaRespectToCentroid;

	};

	// center of mass ����
	// source: https://en.wikipedia.org/wiki/Centroid
	ClipperLib::IntPoint centroid(ClipperLib::Path aPath)
	{
		double area_sum = 0.;
		ClipperLib::IntPoint  c(0., 0.);
		if (aPath.size() >= 3) {
			ClipperLib::IntPoint p1 = aPath.back();
			for (const ClipperLib::IntPoint& p : aPath) {
				ClipperLib::IntPoint p2 = p;
				double a = p1.X * p2.Y - p1.Y * p2.X; //cross(p1, p2);
				area_sum += a;
				c += (p1 + p2) * a;
				p1 = p2;
			}
		}
		return Point(c / (3. * area_sum));
	}

	// BBS: second moment of area of a polygon
	bool compSecondMoment3(ClipperLib::Path poly, ClipperLib::DoublePoint& sm)
	{
		//if (ClipperLib::Orientation(poly))
	//          ClipperLib::ReversePath(poly);

		sm = ClipperLib::DoublePoint(0., 0.);
		if (poly.size() >= 3) {
			Point p1 = poly.back();
			for (const Point& p : poly)
			{
				Point p2 = p;
				double a = p1.X * p2.Y - p1.Y * p2.X;
				ClipperLib::DoublePoint temp((p1.Y * p1.Y + p1.Y * p2.Y + p2.Y * p2.Y) * a / 12, (p1.X * p1.X + p1.X * p2.X + p2.X * p2.X) * a / 12);
				sm.X += temp.X;
				sm.Y += temp.Y;
				p1 = p2;
			}
			return true;
		}
		return false;
	}

	// BBS: second moment of area of an expolyon
	bool compSecondMoment2(const ClipperLib::Path& expoly, ExPolyProp& expolyProp, ClipperLib::Paths& aPolysHolePaths, ClipperLib::DoublePoint& secondMomentOfAreaRespectToCentroid)
	{
		double aera = ClipperLib::Area(expoly);//expoly.contour.area();
		Point cent = centroid(expoly) * aera;
		ClipperLib::DoublePoint sm(0.0, 0.0);
		if (!compSecondMoment3(expoly, sm))
			return false;

		for (auto& hole : aPolysHolePaths)
		{
			double a = ClipperLib::Area(hole);
			aera += ClipperLib::Area(hole);
			cent += centroid(hole) * a;
			ClipperLib::DoublePoint  smh(0.0, 0.0);
			if (compSecondMoment3(hole, smh))
			{
				//sm += -smh;
				sm.X += -smh.X;
				sm.Y += -smh.Y;
			}

		}

		cent = cent / aera;
		sm.X -= cent.Y * cent.Y * aera;
		sm.Y -= cent.X * cent.X * aera;
		expolyProp.aera = aera;
		expolyProp.centroid = cent;
		expolyProp.secondMomentOfAreaRespectToCentroid = sm;
		secondMomentOfAreaRespectToCentroid = sm;
		return true;
	}


	// BBS: second moment of area of expolygons
	bool compSecondMoment(const ClipperLib::Paths& expolys, double& smExpolysX, double& smExpolysY, ClipperLib::Paths& aExpolysHole)
	{
		if (expolys.empty()) return false;
		//std::vector<ExPolyProp> props;
		std::vector<ExPolyProp> props;
		ClipperLib::DoublePoint secondMomentOfAreaRespectToCentroid;
		for (const ClipperLib::Path& expoly : expolys)
		{
			ExPolyProp prop;
			if (compSecondMoment2(expoly, prop, aExpolysHole, secondMomentOfAreaRespectToCentroid))
				props.push_back(prop);
		}
		if (props.empty())
			return false;
		double totalArea = 0.;
		ClipperLib::DoublePoint staticMoment(0., 0.);
		for (const ExPolyProp& prop : props)
		{
			totalArea += prop.aera;
			staticMoment.X += prop.centroid.X * prop.aera;
			staticMoment.Y += prop.centroid.Y * prop.aera;
		}
		double totalCentroidX = staticMoment.X / totalArea;
		double totalCentroidY = staticMoment.Y / totalArea;

		smExpolysX = 0;
		smExpolysY = 0;
		for (const ExPolyProp& prop : props)
		{
			double deltaX = prop.centroid.X - totalCentroidX;
			double deltaY = prop.centroid.Y - totalCentroidY;
			smExpolysX += secondMomentOfAreaRespectToCentroid.X + prop.aera * deltaY * deltaY;
			smExpolysY += secondMomentOfAreaRespectToCentroid.Y + prop.aera * deltaX * deltaX;
		}

		return true;
	}

	double getMaxSpeed(SliceDataStorage& storage)
	{
		double objMaxSpeed = -1.;
		const Settings& mesh_group_settings = storage.application->currentGroup()->settings;
		if (mesh_group_settings.get<Velocity>("speed_print").value > objMaxSpeed)
		{
			objMaxSpeed = mesh_group_settings.get<Velocity>("speed_print").value;
		}
		if (mesh_group_settings.get<Velocity>("speed_infill").value > objMaxSpeed)
		{
			objMaxSpeed = mesh_group_settings.get<Velocity>("speed_infill").value;
		}
		if (mesh_group_settings.get<Velocity>("speed_wall").value > objMaxSpeed)
		{
			objMaxSpeed = mesh_group_settings.get<Velocity>("speed_wall").value;
		}

		if (objMaxSpeed <= 0) objMaxSpeed = 250.;
		return objMaxSpeed;
	}

	void SkirtBrim::generateAutoBrim(SliceDataStorage& storage, Polygons& first_layer_outline, std::vector<size_t>& vct_primary_line_count)
	{
		double maxSpeed = getMaxSpeed(storage);
		for (SliceMeshStorage& amesh : storage.meshes)
		{
			if (amesh.layers.size() > 0)
			{
				Polygons aExpolysHole;
				int index = 0;
				for (int alayer = 0; alayer < amesh.layers.size(); alayer++)
				{
					if (amesh.layers[alayer].parts.size() > 0)
					{
						index = alayer;
						break;
					}
				}

				Polygons allExpolys = amesh.layers[index].getOutlines(true);

				aExpolysHole = allExpolys.getEmptyHoles();
				allExpolys = allExpolys.removeEmptyHoles();
				allExpolys = allExpolys.getOutsidePolygons();
				first_layer_outline.add(allExpolys);

				for (ClipperLib::Paths::iterator it = allExpolys.begin(); it != allExpolys.end(); it++)
				{
					double polygonArea = abs(INT2MM2(ClipperLib::Area(*it)));
					Polygons aExpolys;
					AABB box(*it);
					ClipperLib::IntPoint center = box.getMiddle();

					for (ClipperLib::IntPoint& aPoint : *it)
					{
						aPoint -= center;
						aPoint.X *= 1000.0;
						aPoint.Y *= 1000.0;
					}
					aExpolys.add(*it);

					ClipperLib::Paths aPolysPaths;
					for (ClipperLib::Paths::iterator iit = aExpolys.begin(); iit != aExpolys.end(); iit++)
					{
						aPolysPaths.push_back(*iit);
					}

					auto bbox_size = amesh.bounding_box;
					double height = INT2MM(bbox_size.max.z - bbox_size.min.z);
					double Ixx = -1.e30, Iyy = -1.e30;
					if (!aExpolys.empty())
					{
						ClipperLib::Paths path;
						if (!compSecondMoment(aPolysPaths, Ixx, Iyy, path))
							Ixx = Iyy = -1.e30;

						static constexpr double SCALING_FACTOR = 0.000001;
						Ixx = Ixx * SCALING_FACTOR * SCALING_FACTOR * SCALING_FACTOR * SCALING_FACTOR;
						Iyy = Iyy * SCALING_FACTOR * SCALING_FACTOR * SCALING_FACTOR * SCALING_FACTOR;

						// bounding box of the expolygons of the first layer of the volume
						//BoundingBox bbox2;
						//for (const auto& expoly : expolys)
						//	bbox2.merge(get_extents(expoly.contour));
						const double& bboxX = MM2INT(amesh.bounding_box.max.x - amesh.bounding_box.min.x);
						const double& bboxY = MM2INT(amesh.bounding_box.max.y - amesh.bounding_box.min.y);
						double thermalLength = sqrt(bboxX * bboxX + bboxY * bboxY) * SCALING_FACTOR;
						double thermalLengthRef = 200;//Ĭ��Pla;

						double height_to_area = std::max(height / Ixx * (bboxY * SCALING_FACTOR), height / Iyy * (bboxX * SCALING_FACTOR)) * height / 1920;
						double brim_width = 1.0 * std::min(std::min(std::max(height_to_area * maxSpeed / 24, thermalLength * 8. / thermalLengthRef * std::min(height, 30.) / 30.), 18.), 1.5 * thermalLength);
						// small brims are omitted
						if (brim_width < 5 && brim_width < 1.5 * thermalLength)
							brim_width = 0;
						// large brims are omitted
						if (brim_width > 10) brim_width = 10.;
						if (brim_width == 0 && polygonArea < MINAREA)
						{
							brim_width = 3;
						}
						vct_primary_line_count.push_back(brim_width);
					}
				}
			}
		}

		Polygons support_outline;
		if (storage.support.generated && !storage.support.supportLayers.empty())
		{ // remove model-brim from support
			SupportLayer& support_layer = storage.support.supportLayers[0];
			for (const SupportInfillPart& support_infill_part : support_layer.support_infill_parts)
			{
				support_outline.add(support_infill_part.outline);
			}
			support_outline.add(support_layer.support_bottom);
			support_outline.add(support_layer.support_roof);
		}
		if (storage.primeTower.enabled && !storage.application->currentGroup()->settings.get<bool>("prime_tower_brim_enable"))
		{
			support_outline.add(storage.primeTower.outer_poly_first_layer); // don't remove parts of the prime tower, but make a brim for it
		}
		constexpr coord_t join_distance = 20;
		support_outline = support_outline.offset(join_distance).offset(-join_distance); // merge adjacent models into single polygon
		constexpr coord_t smallest_line_length = 200;
		constexpr coord_t largest_error_of_removed_point = 50;
		support_outline = Simplify(smallest_line_length, largest_error_of_removed_point, 0).polygon(support_outline);
		for (ClipperLib::Paths::iterator it = support_outline.begin(); it != support_outline.end(); it++)
		{
			first_layer_outline.add(*it);
			vct_primary_line_count.push_back(5.0);
		}
	}



	void generateLace(FloatPaths& originPaths, FloatPaths& newPaths)
	{
		double currentDis = 0.0;
		double const radius = 3.0;
		for (FloatPath& originFPath : originPaths)
		{
			//ClipperLib::Path& pathTemp = *ppathTemp;
			if (originFPath.size() == 0)
				continue;

			FloatPath newPath;
			FloatPoint pointStart = originFPath.at(originFPath.size() - 1);
			newPath.push_back(pointStart);
			for (int nn = 0; nn < originFPath.size(); nn++)
			{
				currentDis = sqrt(pow(originFPath[nn].x - pointStart.x, 2) + pow(originFPath[nn].y - pointStart.y, 2));
				if (currentDis == 0)
				{
					continue;
				}
				else if (abs(currentDis - radius) < 0.01)
				{
					newPath.push_back(originFPath[nn]);
					pointStart = originFPath[nn];
				}
				else if (currentDis > radius)
				{
					FloatPoint pointEnd;
					if (originFPath[nn].y == pointStart.y)
					{
						pointEnd.y = pointStart.y;
						if (originFPath[nn].x > pointStart.x)
						{
							pointEnd.x = pointStart.x + radius;
						}
						else
						{
							pointEnd.x = pointStart.x - radius;
						}

					}
					else if (originFPath[nn].x == pointStart.x)
					{
						pointEnd.x = pointStart.x;
						if (originFPath[nn].y > pointStart.y)
						{
							pointEnd.y = pointStart.y + radius;
						}
						else
						{
							pointEnd.y = pointStart.y - radius;
						}
					}
					else
					{
						pointEnd.x = (originFPath[nn].x - pointStart.x) / currentDis * radius + pointStart.x;
						pointEnd.y = (originFPath[nn].y - pointStart.y) / currentDis * radius + pointStart.y;
					}
					newPath.push_back(pointEnd);
					pointStart = pointEnd;
					nn--;
				}
				else
				{
					if (originFPath.size() > nn + 1)
					{
						if (originFPath[nn + 1].x == originFPath[nn].x && originFPath[nn + 1].y == originFPath[nn].y)
						{
							continue;
						}

						double A = pow(originFPath[nn + 1].x - originFPath[nn].x, 2) + pow(originFPath[nn + 1].y - originFPath[nn].y, 2);
						double B = 2 * (originFPath[nn + 1].x - originFPath[nn].x) * (originFPath[nn].x - pointStart.x) + (originFPath[nn + 1].y - originFPath[nn].y) * (originFPath[nn].y - pointStart.y);
						double C = pow(pointStart.x, 2) + pow(pointStart.y, 2) + pow(originFPath[nn].x, 2) + pow(originFPath[nn].y, 2) - 2 * (pointStart.x * originFPath[nn].x + pointStart.y * originFPath[nn].y) - pow(radius, 2);
						double u_1 = (-B + sqrt(pow(B, 2) - 4 * A * C)) / (2 * A);
						double u_2 = (-B - sqrt(pow(B, 2) - 4 * A * C)) / (2 * A);
						if (u_1 > 0 && u_1 < 1 && u_2>0 && u_2 < 1)//����߶κ�Բ���������㣬��uֵ�������ⶼ��0��1֮��
						{

							FloatPoint pointOne;
							pointOne.x = originFPath[nn].x + u_1 * (originFPath[nn + 1].x - originFPath[nn].x);
							pointOne.y = originFPath[nn].y + u_1 * (originFPath[nn + 1].y - originFPath[nn].y);
							FloatPoint pointSecond;
							pointSecond.x = originFPath[nn].x + u_2 * (originFPath[nn + 1].x - originFPath[nn].x);
							pointSecond.y = originFPath[nn].y + u_2 * (originFPath[nn + 1].y - originFPath[nn].y);

							//2�����������һ����ľ��룬ѡ������ĵ�
							double OneDis = sqrt(pow(originFPath[nn + 1].x - pointOne.x, 2) + pow(originFPath[nn + 1].y - pointOne.y, 2));
							double SecondDis = sqrt(pow(originFPath[nn + 1].x - pointSecond.x, 2) + pow(originFPath[nn + 1].y - pointSecond.y, 2));
							if (OneDis < SecondDis)
							{
								newPath.push_back(pointOne);
								pointStart = pointOne;
							}
							else
							{
								newPath.push_back(pointSecond);
								pointStart = pointSecond;
							}

						}
						else if ((u_1 > 0 && u_1 < 1) || (u_2 > 0 && u_2 < 1)) //����߶κ�Բֻ��һ�����㣬��uֵ����һ������0��1֮�䣬��һ������
						{
							double ture_u;
							if (u_1 > 0 && u_1 < 1)
							{
								ture_u = u_1;
							}
							else
							{
								ture_u = u_2;
							}
							FloatPoint pointOne;
							pointOne.x = originFPath[nn].x + u_1 * (originFPath[nn + 1].x - originFPath[nn].x);
							pointOne.y = originFPath[nn].y + u_1 * (originFPath[nn + 1].y - originFPath[nn].y);
							newPath.push_back(pointOne);
							pointStart = pointOne;
						}
					}
				}
			}
			newPaths.push_back(newPath);
		}
	}


	ClipperLib::Paths SkirtBrim::skirt2Lace(ClipperLib::Paths& outlinePaths)
	{
		ClipperLib::PolyTree polyTree;

		ClipperLib::Paths oldPaths = outlinePaths;
		size_t size = oldPaths.size();
		//if (size == 0)
		//	return polyTree;

		//INT2MM ��С1000
		std::vector<FloatPath> fpathes(size);
		for (size_t i = 0; i < size; ++i)
		{
			ClipperLib::Path path = oldPaths.at(i);
			FloatPath& fpath = fpathes.at(i);

			for (ClipperLib::IntPoint& pointTemp : path)
			{
				FloatPoint fPoint;
				fPoint.x = INT2MM(pointTemp.X);
				fPoint.y = INT2MM(pointTemp.Y);
				fpath.push_back(fPoint);
			}
		}

		FloatPaths newPathes;

		generateLace(fpathes, newPathes);
		//��Բ
		FloatPaths circleFPathes;
		for (FloatPath fPathTemp : newPathes)
		{
			FloatPath circlePath;
			for (int n = 1; n < fPathTemp.size(); n += 2)
			{
				float Rx = 3.0;
				FloatPath newPath;
				for (unsigned int i = 0; i < 100; i++)//˳ʱ��ȡ��
				{
					float Angle = 2 * M_PIf * (i / 100.0);
					size_t nSize = newPath.size();
					newPath.emplace_back();
					newPath[nSize].x = (fPathTemp.at(n).x + Rx * cosf(Angle));
					newPath[nSize].y = (fPathTemp.at(n).y + Rx * sinf(Angle));
				}
				circleFPathes.push_back(newPath);
			}
		}

		ClipperLib::Clipper clipper;
		for (ClipperLib::Path path : oldPaths)
			clipper.AddPath(path, ClipperLib::ptSubject, true);

		for (FloatPath& fpath : circleFPathes)
		{
			ClipperLib::Path circlePath;// = new ClipperLib::Path();
			for (FloatPoint& pointTemp : fpath)
			{
				ClipperLib::IntPoint p;
				p.X = (int)(1000.0f * pointTemp.x);
				p.Y = (int)(1000.0f * pointTemp.y);
				circlePath.push_back(p);
			}
			clipper.AddPath(circlePath, ClipperLib::ptClip, true);
		}
		clipper.Execute(ClipperLib::ctUnion, polyTree, ClipperLib::pftNonZero, ClipperLib::pftNonZero);
		ClipperLib::Paths pathsTemp;
		ClipperLib::PolyTreeToPaths(polyTree, pathsTemp);
		return pathsTemp;
	}

	coord_t SkirtBrim::generatePrimarySkirtBrimLines(SliceDataStorage& storage, const coord_t start_distance, size_t& primary_line_count, const coord_t primary_extruder_minimal_length, const Polygons& first_layer_outline, Polygons& skirt_brim_primary_extruder)
	{
		const Settings& adhesion_settings = storage.application->currentGroup()->settings.get<ExtruderTrain&>("skirt_brim_extruder_nr").settings;
		const coord_t primary_extruder_skirt_brim_line_width = adhesion_settings.get<coord_t>("skirt_brim_line_width") * adhesion_settings.get<Ratio>("initial_layer_line_width_factor");
		coord_t offset_distance = start_distance - primary_extruder_skirt_brim_line_width / 2;
		for (unsigned int skirt_brim_number = 0; skirt_brim_number < primary_line_count; skirt_brim_number++)
		{
			offset_distance += primary_extruder_skirt_brim_line_width;

			Polygons outer_skirt_brim_line = first_layer_outline.offset(offset_distance, ClipperLib::jtRound);

			// Remove small inner skirt and brim holes. Holes have a negative area, remove anything smaller then 100x extrusion "area"
			for (unsigned int n = 0; n < outer_skirt_brim_line.size(); n++)
			{
				double area = outer_skirt_brim_line[n].area();
				if (area < 0 && area > -primary_extruder_skirt_brim_line_width * primary_extruder_skirt_brim_line_width * 100)
				{
					outer_skirt_brim_line.remove(n--);
				}
			}

			skirt_brim_primary_extruder.add(outer_skirt_brim_line);

			const coord_t length = skirt_brim_primary_extruder.polygonLength();
			if (skirt_brim_number + 1 >= primary_line_count && length > 0 && length < primary_extruder_minimal_length) // Make brim or skirt have more lines when total length is too small.
			{
				primary_line_count++;
			}
		}
		return offset_distance;
	}


	coord_t SkirtBrim::generatePrimaryAutoBrimLines(SliceDataStorage& storage, const coord_t start_distance, std::vector<size_t>& vct_primary_line_count, const coord_t primary_extruder_minimal_length, const Polygons& first_layer_outline, Polygons& skirt_brim_primary_extruder, int max_line_count)
	{
		const Settings& adhesion_settings = storage.application->currentGroup()->settings.get<ExtruderTrain&>("skirt_brim_extruder_nr").settings;
		const coord_t primary_extruder_skirt_brim_line_width = adhesion_settings.get<coord_t>("skirt_brim_line_width") * adhesion_settings.get<Ratio>("initial_layer_line_width_factor");
		coord_t offset_distance = start_distance - primary_extruder_skirt_brim_line_width / 2;
		for (int skirt_brim_number = 0; skirt_brim_number < max_line_count; skirt_brim_number++)
		{
			offset_distance += primary_extruder_skirt_brim_line_width;
			Polygons current_polygons;
			for (int icount = 0; icount < vct_primary_line_count.size(); icount++)
			{
				if ((int)vct_primary_line_count[icount] > skirt_brim_number)
				{
					current_polygons.add(first_layer_outline[icount]);
				}
			}

			Polygons outer_skirt_brim_line = current_polygons.offset(offset_distance, ClipperLib::jtRound);

			// Remove small inner skirt and brim holes. Holes have a negative area, remove anything smaller then 100x extrusion "area"
			for (unsigned int n = 0; n < outer_skirt_brim_line.size(); n++)
			{
				double area = outer_skirt_brim_line[n].area();
				if (area < 0 && area > -primary_extruder_skirt_brim_line_width * primary_extruder_skirt_brim_line_width * 100)
				{
					outer_skirt_brim_line.remove(n--);
				}
			}

			skirt_brim_primary_extruder.add(outer_skirt_brim_line);
		}

		skirt_brim_primary_extruder = skirt_brim_primary_extruder.difference(first_layer_outline);
		return offset_distance;
	}


	void SkirtBrim::generate(SliceDataStorage& storage, Polygons first_layer_outline, const coord_t start_distance, size_t primary_line_count, const bool allow_helpers /*= true*/)
	{
		const bool is_skirt = start_distance > 0;
		const size_t skirt_brim_extruder_nr = storage.application->currentGroup()->settings.get<ExtruderTrain&>("skirt_brim_extruder_nr").extruder_nr;
		const Settings& adhesion_settings = storage.application->extruders()[skirt_brim_extruder_nr].settings;
		const coord_t primary_extruder_skirt_brim_line_width = adhesion_settings.get<coord_t>("skirt_brim_line_width") * adhesion_settings.get<Ratio>("initial_layer_line_width_factor");
		const coord_t primary_extruder_minimal_length = adhesion_settings.get<coord_t>("skirt_brim_minimal_length");

		Polygons& skirt_brim_primary_extruder = storage.skirt_brim[skirt_brim_extruder_nr];

		const bool has_ooze_shield = allow_helpers && storage.oozeShield.size() > 0 && storage.oozeShield[0].size() > 0;
		const bool has_draft_shield = allow_helpers && storage.draft_protection_shield.size() > 0;

		coord_t gap;
		if (is_skirt && (has_ooze_shield || has_draft_shield))
		{ // make sure we don't generate skirt through draft / ooze shield
			first_layer_outline = first_layer_outline.offset(start_distance - primary_extruder_skirt_brim_line_width / 2, ClipperLib::jtRound).unionPolygons(storage.draft_protection_shield);
			if (has_ooze_shield)
			{
				first_layer_outline = first_layer_outline.unionPolygons(storage.oozeShield[0]);
			}
			first_layer_outline = first_layer_outline.approxConvexHull();
			gap = primary_extruder_skirt_brim_line_width / 2;
		}
		else
		{
			gap = start_distance;
		}

		//add LACE
		coord_t offset_distance = 0;
		if (storage.application->get_adhesion_type() == EPlatformAdhesion::LACE)
		{
			skirt_brim_primary_extruder.add(first_layer_outline);
		}
		else
		{
			offset_distance = generatePrimarySkirtBrimLines(storage, gap, primary_line_count, primary_extruder_minimal_length, first_layer_outline, skirt_brim_primary_extruder);
		}
		//coord_t offset_distance = generatePrimarySkirtBrimLines(storage, gap, primary_line_count, primary_extruder_minimal_length, first_layer_outline, skirt_brim_primary_extruder);

		// Skirt needs to be 'locked' first, otherwise the optimizer can change to order, which can cause undesirable outcomes w.r.t combo w. support-brim or prime-tower brim.
		// If this method is called multiple times, the max order shouldn't reset to 0, so the maximum is taken.
		storage.skirt_brim_max_locked_part_order[skirt_brim_extruder_nr] = std::max(is_skirt ? primary_line_count : 0, storage.skirt_brim_max_locked_part_order[skirt_brim_extruder_nr]);

		// handle support-brim
		const ExtruderTrain& support_infill_extruder = storage.application->currentGroup()->settings.get<ExtruderTrain&>("support_infill_extruder_nr");
		if (allow_helpers && support_infill_extruder.settings.get<bool>("support_brim_enable"))
		{
			const bool merge_with_model_skirtbrim = !is_skirt;
			generateSupportBrim(storage, merge_with_model_skirtbrim);
		}

		// generate brim for ooze shield and draft shield
		if (!is_skirt && (has_ooze_shield || has_draft_shield))
		{
			// generate areas where to make extra brim for the shields
			// avoid gap in the middle
			//    V
			//  +---+     +----+
			//  |+-+|     |+--+|
			//  || ||     ||[]|| > expand to fit an extra brim line
			//  |+-+|     |+--+|
			//  +---+     +----+
			const coord_t primary_skirt_brim_width = (primary_line_count + primary_line_count % 2) * primary_extruder_skirt_brim_line_width; // always use an even number, because we will fil the area from both sides

			Polygons shield_brim;
			if (has_ooze_shield)
			{
				shield_brim = storage.oozeShield[0].difference(storage.oozeShield[0].offset(-primary_skirt_brim_width - primary_extruder_skirt_brim_line_width));
			}
			if (has_draft_shield)
			{
				shield_brim = shield_brim.unionPolygons(storage.draft_protection_shield.difference(storage.draft_protection_shield.offset(-primary_skirt_brim_width - primary_extruder_skirt_brim_line_width)));
			}
			const Polygons outer_primary_brim = first_layer_outline.offset(offset_distance, ClipperLib::jtRound);
			shield_brim = shield_brim.difference(outer_primary_brim.offset(primary_extruder_skirt_brim_line_width));

			// generate brim within shield_brim
			skirt_brim_primary_extruder.add(shield_brim);
			while (shield_brim.size() > 0)
			{
				shield_brim = shield_brim.offset(-primary_extruder_skirt_brim_line_width);
				skirt_brim_primary_extruder.add(shield_brim);
			}

			// update parameters to generate secondary skirt around
			first_layer_outline = outer_primary_brim;
			if (has_draft_shield)
			{
				first_layer_outline = first_layer_outline.unionPolygons(storage.draft_protection_shield);
			}
			if (has_ooze_shield)
			{
				first_layer_outline = first_layer_outline.unionPolygons(storage.oozeShield[0]);
			}

			offset_distance = 0;
		}

		if (first_layer_outline.polygonLength() > 0)
		{ // process other extruders' brim/skirt (as one brim line around the old brim)
			int last_width = primary_extruder_skirt_brim_line_width;
			std::vector<bool> extruder_is_used = storage.getExtrudersUsed();
			for (size_t extruder_nr = 0; extruder_nr < storage.application->extruderCount(); extruder_nr++)
			{
				if (extruder_nr == skirt_brim_extruder_nr || !extruder_is_used[extruder_nr])
				{
					continue;
				}
				const ExtruderTrain& train = storage.application->extruders()[extruder_nr];
				const coord_t width = train.settings.get<coord_t>("skirt_brim_line_width") * train.settings.get<Ratio>("initial_layer_line_width_factor");
				const coord_t minimal_length = train.settings.get<coord_t>("skirt_brim_minimal_length");
				//offset_distance += last_width / 2 + width / 2;
				gap += primary_line_count * width + width / 2;
				last_width = width;
				//while (storage.skirt_brim[extruder_nr].polygonLength() < minimal_length)
				for (int n = 0; n < primary_line_count; n++)
				{
					storage.skirt_brim[extruder_nr].add(first_layer_outline.offset(gap, ClipperLib::jtRound));
					gap += width;
				}
			}
		}
	}

	void SkirtBrim::generateEX(SliceDataStorage& storage, Polygons first_layer_outline, const coord_t start_distance, std::vector<size_t> vct_primary_line_count, const bool allow_helpers /*= true*/)
	{
		if (first_layer_outline.size() != vct_primary_line_count.size())
		{
			return;
		}
		size_t max_line_count = 0;
		for (size_t& acount : vct_primary_line_count)
		{
			if (max_line_count < acount)
			{
				max_line_count = acount;
			}
		}

		const size_t skirt_brim_extruder_nr = storage.application->currentGroup()->settings.get<ExtruderTrain&>("skirt_brim_extruder_nr").extruder_nr;
		const Settings& adhesion_settings = storage.application->extruders()[skirt_brim_extruder_nr].settings;
		const coord_t primary_extruder_skirt_brim_line_width = adhesion_settings.get<coord_t>("skirt_brim_line_width") * adhesion_settings.get<Ratio>("initial_layer_line_width_factor");
		const coord_t primary_extruder_minimal_length = adhesion_settings.get<coord_t>("skirt_brim_minimal_length");

		Polygons& skirt_brim_primary_extruder = storage.skirt_brim[skirt_brim_extruder_nr];

		const bool has_ooze_shield = allow_helpers && storage.oozeShield.size() > 0 && storage.oozeShield[0].size() > 0;
		const bool has_draft_shield = allow_helpers && storage.draft_protection_shield.size() > 0;

		coord_t offset_distance = generatePrimaryAutoBrimLines(storage, start_distance, vct_primary_line_count, primary_extruder_minimal_length, first_layer_outline, skirt_brim_primary_extruder, max_line_count);

		storage.skirt_brim_max_locked_part_order[skirt_brim_extruder_nr] = std::max(max_line_count, storage.skirt_brim_max_locked_part_order[skirt_brim_extruder_nr]);

		// handle support-brim
		const ExtruderTrain& support_infill_extruder = storage.application->currentGroup()->settings.get<ExtruderTrain&>("support_infill_extruder_nr");
		if (allow_helpers && support_infill_extruder.settings.get<bool>("support_brim_enable"))
		{
			generateSupportBrim(storage, true);
		}

		// generate brim for ooze shield and draft shield
		if (has_ooze_shield || has_draft_shield)
		{
			// generate areas where to make extra brim for the shields
			// avoid gap in the middle
			//    V
			//  +---+     +----+
			//  |+-+|     |+--+|
			//  || ||     ||[]|| > expand to fit an extra brim line
			//  |+-+|     |+--+|
			//  +---+     +----+ 
			const coord_t primary_skirt_brim_width = (max_line_count + max_line_count % 2) * primary_extruder_skirt_brim_line_width; // always use an even number, because we will fil the area from both sides

			Polygons shield_brim;
			if (has_ooze_shield)
			{
				shield_brim = storage.oozeShield[0].difference(storage.oozeShield[0].offset(-primary_skirt_brim_width - primary_extruder_skirt_brim_line_width));
			}
			if (has_draft_shield)
			{
				shield_brim = shield_brim.unionPolygons(storage.draft_protection_shield.difference(storage.draft_protection_shield.offset(-primary_skirt_brim_width - primary_extruder_skirt_brim_line_width)));
			}
			const Polygons outer_primary_brim = first_layer_outline.offset(offset_distance, ClipperLib::jtRound);
			shield_brim = shield_brim.difference(outer_primary_brim.offset(primary_extruder_skirt_brim_line_width));

			// generate brim within shield_brim
			skirt_brim_primary_extruder.add(shield_brim);
			while (shield_brim.size() > 0)
			{
				shield_brim = shield_brim.offset(-primary_extruder_skirt_brim_line_width);
				skirt_brim_primary_extruder.add(shield_brim);
			}

			// update parameters to generate secondary skirt around
			first_layer_outline = outer_primary_brim;
			if (has_draft_shield)
			{
				first_layer_outline = first_layer_outline.unionPolygons(storage.draft_protection_shield);
			}
			if (has_ooze_shield)
			{
				first_layer_outline = first_layer_outline.unionPolygons(storage.oozeShield[0]);
			}

			offset_distance = 0;
		}
	}


	void SkirtBrim::generateSupportBrim(SliceDataStorage& storage, const bool merge_with_model_skirtbrim)
	{
		constexpr coord_t brim_area_minimum_hole_size_multiplier = 100;

		const ExtruderTrain& support_infill_extruder = storage.application->currentGroup()->settings.get<ExtruderTrain&>("support_infill_extruder_nr");
		const coord_t brim_line_width = support_infill_extruder.settings.get<coord_t>("skirt_brim_line_width") * support_infill_extruder.settings.get<Ratio>("initial_layer_line_width_factor");
		size_t line_count = support_infill_extruder.settings.get<size_t>("support_brim_line_count");
		const coord_t minimal_length = support_infill_extruder.settings.get<coord_t>("skirt_brim_minimal_length");
		if (!storage.support.generated || line_count <= 0 || storage.support.supportLayers.empty())
		{
			return;
		}

		const coord_t brim_width = brim_line_width * line_count;
		Polygons& skirt_brim = storage.skirt_brim[support_infill_extruder.extruder_nr];

		SupportLayer& support_layer = storage.support.supportLayers[0];

		Polygons support_outline;
		for (SupportInfillPart& part : support_layer.support_infill_parts)
		{
			support_outline.add(part.outline);
		}
		const Polygons brim_area = support_outline.difference(support_outline.offset(-brim_width));
		support_layer.excludeAreasFromSupportInfillAreas(brim_area, AABB(brim_area));

		Polygons support_brim;

		coord_t offset_distance = brim_line_width / 2;
		for (size_t skirt_brim_number = 0; skirt_brim_number < line_count; skirt_brim_number++)
		{
			offset_distance -= brim_line_width;

			Polygons brim_line = support_outline.offset(offset_distance, ClipperLib::jtRound);

			// Remove small inner skirt and brim holes. Holes have a negative area, remove anything smaller then multiplier x extrusion "area"
			for (size_t n = 0; n < brim_line.size(); n++)
			{
				const double area = brim_line[n].area();
				if (area < 0 && area > -brim_line_width * brim_line_width * brim_area_minimum_hole_size_multiplier)
				{
					brim_line.remove(n--);
				}
			}

			support_brim.add(brim_line);

			const coord_t length = skirt_brim.polygonLength() + support_brim.polygonLength();
			if (skirt_brim_number + 1 >= line_count && length > 0 && length < minimal_length) // Make brim or skirt have more lines when total length is too small.
			{
				line_count++;
			}
			if (brim_line.empty())
			{ // the fist layer of support is fully filled with brim
				break;
			}
		}

		if (support_brim.size())
		{
			if (merge_with_model_skirtbrim)
			{
				// to ensure that the skirt brim is printed from outside to inside, the support brim lines must
				// come before the skirt brim lines in the Polygon object so that the outermost skirt brim line
				// is at the back of the list
				support_brim.add(skirt_brim);
				skirt_brim = support_brim;
			}
			else
			{
				// OTOH, if we use a skirt instead of a brim for the polygon, the skirt line(s) should _always_ come first.
				skirt_brim.add(support_brim);
			}
		}
	}

} // namespace cura52
