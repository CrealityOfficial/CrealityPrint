#ifndef PAINTSUPPORT_274874
#define PAINTSUPPORT_274874
#include <vector>
#include <string>


namespace cura52
{
    class Mesh;
    class Settings;
    class SliceDataStorage;
    class SliceContext;
    class SlicerLayer;

    void getBinaryData(std::string fileName, std::vector<Mesh>& meshs, Settings& setting);
    void getPaintSupport(SliceDataStorage& storage,SliceContext* application, const int layer_thickness, const int slice_layer_count, const bool use_variable_layer_heights);
    void getPaintSupport_anti(SliceDataStorage& storage, SliceContext* application, const int layer_thickness, const int slice_layer_count, const bool use_variable_layer_heights);

    void getZseamLine(SliceDataStorage& storage, SliceContext* application, const int layer_thickness, const int slice_layer_count, const bool use_variable_layer_heights);
    void getZseamLine_anti(SliceDataStorage& storage, SliceContext* application, const int layer_thickness, const int slice_layer_count, const bool use_variable_layer_heights);

    void mergePaintSupport(SliceDataStorage& storage);

    void mergeOpenPloygons(const Mesh* mesh,std::vector<SlicerLayer>& layers);
}

#endif //PAINTSUPPORT_274874

