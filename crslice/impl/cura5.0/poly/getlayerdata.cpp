#include "getlayerdata.h"
#include "communication/slicecontext.h"
#include "communication/sliceDataStorage.h"
#include "utils/Coord_t.h"
#include "conv.h"

namespace cura52 {
    void getLayerSupportData(SliceDataStorage& storage, SliceContext* application)
    {
        std::vector<float> _heights;
        std::vector<float> _thicknesss;

        for (unsigned int meshIdx = 0; meshIdx < storage.meshes.size(); meshIdx++)
        {
            SliceMeshStorage& meshStorage = storage.meshes.at(meshIdx);
            size_t layerSize = meshStorage.layers.size();
            for (int layerIdx = 0; layerIdx < layerSize; ++layerIdx)
            {
                if (layerIdx >= _heights.size())
                {
                    SliceLayer& layer = meshStorage.layers.at(layerIdx);
                        float z = INT2MM(layer.printZ) ;
                        float thickness = INT2MM(layer.thickness);
                        _heights.push_back(z);
                        _thicknesss.push_back(thickness);
                }
            }
        }

        if (application->debugger())
        {
            SupportStorage& supportStorage = storage.support;
            size_t layerSize = supportStorage.supportLayers.size();
            float thickness = 0.0f;
            float z = 0.0f;
            ClipperLib::Paths paths;
            for (size_t layerIdx = 0; layerIdx < layerSize; ++layerIdx)
            {
                paths.clear();
                SupportLayer& layer = supportStorage.supportLayers.at(layerIdx);
                size_t partSize = layer.support_infill_parts.size();
                
                float z = _heights.size() > layerIdx ? _heights[layerIdx] : 0.0f;
                float thickness = _thicknesss.size() > layerIdx ? _thicknesss[layerIdx] : 0.0f;

                for (size_t partIdx = 0; partIdx < partSize; ++partIdx)
                {
                    SupportInfillPart& part = layer.support_infill_parts.at(partIdx);

                    cura52::PolygonRef plogon = part.outline.outerPolygon();
                    paths.push_back(*plogon);
                }

                std::vector<std::vector<trimesh::vec3>> tPaths;
                crslice::convertRaw(paths, tPaths);
                application->debugger()->onSupports((int)layerIdx,z, thickness, tPaths);
            }
        }
    }


    void setPloyOrderUserSpecified(SliceDataStorage& storage, std::string str)
    {
        storage.polyOrderUserDef.clear();
        //[0.0,0.0] //[x1,y1,x2,y2]
        if (str.length() > 3)
        {
            str = str.substr(0, str.length() - 1);
            str = str.substr(1, str.length() - 1);
        }
        std::vector<float> order;
        int findIndex = str.find(',');
        std::string temp = "";
        while (findIndex >= 0)
        {
            temp = str.substr(0, findIndex);
            str = str.substr(findIndex + 1, str.length());
            order.push_back(std::atof(temp.c_str()));
            findIndex = str.find(',');
        }
        order.push_back(std::atof(str.c_str()));

        for (size_t i = 0; i < order.size() - 1; i++, i++)
        {
            ClipperLib::IntPoint p;
            p.X = MM2INT(order[i]);
            p.Y = MM2INT(order[i + 1]);
            storage.polyOrderUserDef.push_back(p);
        }
    }
}
