#ifndef CRSLICE_GETLAYERDATA_1668840402293_H
#define CRSLICE_GETLAYERDATA_1668840402293_H
#include <vector>
#include <string>

namespace cura52 {

	//for cloud
	class SliceContext;
	class SliceDataStorage;
	void getLayerSupportData(SliceDataStorage& storage, SliceContext* application);

	void setPloyOrderUserSpecified(SliceDataStorage& storage,std::string str);
}
#endif // CRSLICE_GETLAYERDATA_1668840402293_H
