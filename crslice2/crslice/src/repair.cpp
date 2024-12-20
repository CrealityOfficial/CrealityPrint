#include "crslice2/repair.h"
#include "TriangleMesh.hpp"
#include "libslic3r/Model.hpp"
#include "libslic3r/CutUtils.hpp"

namespace crslice2
{
	struct MeshErrorsInfo
	{
		std::string    tooltip;
		std::string warning_icon_name;
	};
	void trimesh2stl_file(trimesh::TriMesh* mesh, stl_file& stl)
	{
		if (!mesh)
			return;
		int pointSize = (int)mesh->vertices.size();
		int facesSize = (int)mesh->faces.size();
		if (pointSize < 3 || facesSize < 1)
			return;

		indexed_triangle_set indexedTriangleSet;
		indexedTriangleSet.vertices.resize(pointSize);
		indexedTriangleSet.indices.resize(facesSize);
		for (int i = 0; i < pointSize; i++)
		{
			const trimesh::vec3& v = mesh->vertices.at(i);
			indexedTriangleSet.vertices.at(i) = stl_vertex(v.x, v.y, v.z);
		}

		for (int i = 0; i < facesSize; i++)
		{
			const trimesh::TriMesh::Face& f = mesh->faces.at(i);
			stl_triangle_vertex_indices faceIndex(f[0], f[1], f[2]);
			indexedTriangleSet.indices.at(i) = faceIndex;
		}

#if 0
		tmesh = Slic3r::TriangleMesh(indexedTriangleSet);
#else

		stl.stats.type = inmemory;
		// count facets and allocate memory
		stl.stats.number_of_facets = uint32_t(indexedTriangleSet.indices.size());
		stl.stats.original_num_facets = int(stl.stats.number_of_facets);
		stl_allocate(&stl);

#pragma omp parallel for
		for (int i = 0; i < (int)stl.stats.number_of_facets; ++i) {
			stl_facet facet;
			facet.vertex[0] = indexedTriangleSet.vertices[size_t(indexedTriangleSet.indices[i](0))];
			facet.vertex[1] = indexedTriangleSet.vertices[size_t(indexedTriangleSet.indices[i](1))];
			facet.vertex[2] = indexedTriangleSet.vertices[size_t(indexedTriangleSet.indices[i](2))];
			facet.extra[0] = 0;
			facet.extra[1] = 0;

			stl_normal normal;
			stl_calculate_normal(normal, &facet);
			stl_normalize_vector(normal);
			facet.normal = normal;

			stl.facet_start[i] = facet;
		}

		stl_get_size(&stl);
		//tmesh.from_stl(stl, true);
#endif
	}
	void clear_before_change_mesh(Slic3r::ModelObject* mo)
	{
		//ModelObject* mo = model().objects[obj_idx];

		// If there are custom supports/seams/mmu segmentation, remove them. Fixed mesh
		// may be different and they would make no sense.
		//bool paint_removed = false;
		for (Slic3r::ModelVolume* mv : mo->volumes) {
			//paint_removed |= !mv->supported_facets.empty() || !mv->seam_facets.empty() || !mv->mmu_segmentation_facets.empty();
			mv->supported_facets.reset();
			mv->seam_facets.reset();
			mv->mmu_segmentation_facets.reset();
		}
		//if (paint_removed) {
		//	// snapshot_time is captured by copy so the lambda knows where to undo/redo to.
		//	get_notification_manager()->push_notification(
		//		NotificationType::CustomSupportsAndSeamRemovedAfterRepair,
		//		NotificationManager::NotificationLevel::PrintInfoNotificationLevel,
		//		_u8L("Custom supports and color painting were removed before repairing."));
		//}
	}
	void changed_mesh(Slic3r::ModelObject* mo)
	{
		//ModelObject* mo = model().objects[obj_idx];
		//sla::reproject_points_and_holes(mo);
		//update();
		//p->object_list_changed();
		//p->schedule_background_process();
	}
	static std::string get_warning_icon_name(const Slic3r::TriangleMeshStats& stats)
	{
		return stats.manifold() ? (stats.repaired() ? "obj_warning" : "") : "obj_warning";
	}
}

#ifdef WIN32
#include "FixModelByWin10.h"

namespace crslice2
{
	MeshErrorsInfo get_mesh_errors_info(Slic3r::ModelObject* mo, std::string* sidebar_info /*= nullptr*/, int* non_manifold_edges)
	{

		const Slic3r::TriangleMeshStats& stats = mo->get_object_stl_stats();

		//(*m_objects)[obj_idx]->volumes[vol_idx]->mesh().stats();

		if (!stats.repaired() && stats.manifold()) {
			//if (sidebar_info)
			//    *sidebar_info = _L("No errors");
			return { {}, {} }; // hide tooltip
		}

		std::string tooltip, auto_repaired_info, remaining_info;

		// Create tooltip string, if there are errors
		if (stats.repaired()) {
			const int errors = mo->get_repaired_errors_count(-1);
			char buffer[100];
			std::sprintf(buffer, " %d errors repaired", errors);
			std::string str(buffer);
			tooltip += str + "\n";

			//auto_repaired_info = format_wxstr(_L_PLURAL("%1$d error repaired", "%1$d errors repaired", errors), errors);
			//tooltip += auto_repaired_info + "\n";
		}
		if (!stats.manifold()) {

			char buffer[100];
			std::sprintf(buffer, "Error: %d non-manifold edges.", stats.open_edges);
			remaining_info = (buffer);

			//remaining_info = format_wxstr(_L_PLURAL("Error: %1$d non-manifold edge.", "Error: %1$d non-manifold edges.", stats.open_edges), stats.open_edges);

			tooltip += std::string("Remaining errors") + ":\n";
			std::sprintf(buffer, "%d non-manifold edges \n", stats.open_edges);
			std::string str(buffer);
			tooltip += "\t" + str;
		}

		if (sidebar_info) {
			*sidebar_info = stats.manifold() ? auto_repaired_info : (remaining_info + (stats.repaired() ? ("\n" + auto_repaired_info) : ""));
		}

		if (non_manifold_edges)
			*non_manifold_edges = stats.open_edges;

		if (Slic3r::is_windows10() && !sidebar_info)
			tooltip += ("\n Right click the icon to fix model object");

		return { tooltip, get_warning_icon_name(stats) };
	}

	//here is the two interfaces
	CheckInfo checkMesh(trimesh::TriMesh* in_mesh)
	{
		//check
		stl_file stl;
		trimesh2stl_file(in_mesh, stl);

		Slic3r::TriangleMesh mesh;
		mesh.from_stl(stl, true);


		Slic3r::Model tmpmodel;
		const char* name = "";
		const char* path = "";
		Slic3r::ModelObject* mo = tmpmodel.add_object(name, path, mesh);

		std::string sidebar_info;
		int non_manifold_edges;

		MeshErrorsInfo re_info = get_mesh_errors_info(mo, &sidebar_info, &non_manifold_edges);

		CheckInfo res;
		res.tooltip = re_info.tooltip;
		res.warning_icon_name = re_info.warning_icon_name;
		return res;
	}

	trimesh::TriMesh* repairMesh(trimesh::TriMesh* in_mesh, ccglobal::Tracer* tracer)
	{
		//check
		stl_file stl;
		trimesh2stl_file(in_mesh, stl);

		Slic3r::TriangleMesh mesh;
		mesh.from_stl(stl, true);



		//repair
		trimesh::TriMesh* res = 0;

		//const std::string& model_name = model_names[model_idx];
		//wxString msg = _L("Repairing model object");
		//if (model_names.size() == 1)
		//	msg += ": " + from_u8(model_name) + "\n";
		//else {
		//	msg += ":\n";
		//	for (int i = 0; i < int(model_names.size()); ++i)
		//		msg += (i == model_idx ? " > " : "   ") + from_u8(model_names[i]) + "\n";
		//	msg += "\n";
		//}
		//TriangleMesh(std::vector<Vec3f> &&vertices, const std::vector<Vec3i> &&faces);
		//Slic3r::ModelObject tmpmo;
		Slic3r::Model tmpmodel;
		//const char* name="tt.stl";
		//const char* path="c://";
		const char* name = "";
		const char* path = "";
		Slic3r::ModelObject* mo = tmpmodel.add_object(name, path, mesh);

		std::string sidebar_info;
		int non_manifold_edges;

		MeshErrorsInfo re_info = get_mesh_errors_info(mo, &sidebar_info, &non_manifold_edges);

		if (tracer)
		{
			tracer->progress(0.1f);
		}
		//Slic3r::ModelObject* mo = NULL;//=stl
		clear_before_change_mesh(mo);
		std::string res_str;
		if (!fix_model_by_win10_sdk_gui(*mo, -1, res_str, tracer))
			return res;
		//wxGetApp().plater()->changed_mesh(obj_idx);
		mo->add_instance();
		mo->ensure_on_bed();
		changed_mesh(mo);

		res = new trimesh::TriMesh();
		for (int i = 0; i < mo->volumes[0]->mesh_ptr()->its.vertices.size(); i++)
		{
			auto& curv = mo->volumes[0]->mesh_ptr()->its.vertices.at(i);
			res->vertices.emplace_back(curv[0], curv[1], curv[2]);
		}
		for (int i = 0; i < mo->volumes[0]->mesh_ptr()->its.indices.size(); i++)
		{
			auto& f1 = mo->volumes[0]->mesh_ptr()->its.indices.at(i);
			res->faces.emplace_back(f1[0], f1[1], f1[2]);
		}

		//plater->get_partplate_list().notify_instance_update(obj_idx, 0);
		//plater->sidebar().obj_list()->update_plate_values_for_items();

		//if (res.empty())
		//	succes_models.push_back(model_name);
		//else
		//	failed_models.push_back({ model_name, res });

		//update_item_error_icon(obj_idx, vol_idx);
		//update_info_items(obj_idx);



		return res;
	}
}
#else
namespace crslice2
{
	CheckInfo checkMesh(trimesh::TriMesh* in_mesh)
	{
		return CheckInfo();
	}

	trimesh::TriMesh* repairMesh(trimesh::TriMesh* in_mesh, ccglobal::Tracer* tracer)
	{
		return nullptr;
	}
}
#endif

namespace crslice2
{
	bool cut_horizontal(trimesh::TriMesh* in_mesh, float z, std::vector<trimesh::TriMesh*>& outMeshes, int attributes)
	{
		// use example
		//crslice2::cut_horizontal(meshptr.get(), height, meshes);
		//if (meshes.size() > 1)
		//{
		//	meshptr.reset(meshes[1]);
		//}

		//check
		stl_file stl;
		trimesh2stl_file(in_mesh, stl);

		Slic3r::TriangleMesh mesh;
		mesh.from_stl(stl, true);


		Slic3r::Model tmpmodel;
		const char* name = "";
		const char* path = "";
		auto* mo = tmpmodel.add_object(name, path, mesh);
		mo->add_instance();
		mo->ensure_on_bed(true);
		changed_mesh(mo);

		//Slic3r::ModelObjectCutAttributes attributes = Slic3r::ModelObjectCutAttribute::KeepLower | Slic3r::ModelObjectCutAttribute::KeepUpper;
		Slic3r::Cut  cut(mo, 0, Slic3r::Geometry::translation_transform(z * Slic3r::Vec3d::UnitZ()), attributes == 1 ? Slic3r::ModelObjectCutAttribute::KeepLower : Slic3r::ModelObjectCutAttribute::KeepUpper);

		const Slic3r::ModelObjectPtrs& new_objects = cut.perform_with_plane();

		for (Slic3r::ModelObject* outmo : new_objects)
		{
			trimesh::TriMesh* res = new trimesh::TriMesh();
			for (int i = 0; i < outmo->volumes[0]->mesh_ptr()->its.vertices.size(); i++)
			{
				auto& curv = outmo->volumes[0]->mesh_ptr()->its.vertices.at(i);
				res->vertices.emplace_back(curv[0], curv[1], curv[2]);
			}
			for (int i = 0; i < outmo->volumes[0]->mesh_ptr()->its.indices.size(); i++)
			{
				auto& f1 = outmo->volumes[0]->mesh_ptr()->its.indices.at(i);
				res->faces.emplace_back(f1[0], f1[1], f1[2]);
			}
			outMeshes.push_back(res);
		}
		return true;
	}
}