#include "rawdata.h"

#include "msbase/mesh/checker.h"

namespace creative_kernel
{
	void makeSureDataValid(TriMeshPtr mesh,
		std::vector<std::string>& colors,
		std::vector<std::string>& seams,
		std::vector<std::string>& supports)
	{
        if (!mesh)
            return;

        std::vector<bool> valids;
        bool have = msbase::checkDegenerateFace(mesh.get(), valids, true);
        if (have)
        {
            msbase::mantainValids(colors, valids);
            msbase::mantainValids(seams, valids);
            msbase::mantainValids(supports, valids);
            qDebug() << QString("msbase::checkDegenerateFace true : [have degenerate face]");
        }
	}

    ModelDataPtr createModelData(TriMeshPtr mesh,
        const std::vector<std::string>& colors,
        const std::vector<std::string>& seams,
        const std::vector<std::string>& supports,
        int color_index,
        bool toCenter)
    {
        if (!mesh)
            return nullptr;

        ModelDataPtr model_data(new ModelData());
        model_data->colors = colors;
        model_data->seams = seams;
        model_data->supports = supports;
        model_data->defaultColor = color_index;

        cxkernel::MeshDataPtr mesh_data(new cxkernel::MeshData(mesh, toCenter));
        model_data->mesh = mesh_data;
        return model_data;
    }

    cxkernel::MeshDataPtr createMeshDataPtr(TriMeshPtr mesh, bool toCenter)
    {
        if (!mesh)
            return nullptr;

        cxkernel::MeshDataPtr mesh_data(new cxkernel::MeshData(mesh, toCenter));
        return mesh_data;
    }
}
