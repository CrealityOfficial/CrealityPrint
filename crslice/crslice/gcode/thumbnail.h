#ifndef GCODE_THUMBNAIL_1635927924764_H
#define GCODE_THUMBNAIL_1635927924764_H
#include "crslice/interface.h"
#include <vector>
#include <string>
#include <memory>

namespace gcode
{
	CRSLICE_API bool thumbnail_to_gcode(const std::vector<unsigned char>& inPrevData, const std::string& inImgSizes, const std::string& inImgFormat,
		const std::string& imgPixelSE, const int& inlayerCount, std::vector<std::string>& outGcodeStr, const float& layerHeight = 0.);
	//
	CRSLICE_API bool thumbnail_image2base64(const std::vector<unsigned char>& inPrevData, const std::string& inImgSizes, const std::string& inImgFormat, std::vector<std::string>& outGcodeStr);
	CRSLICE_API bool thumbnail_base2image(const std::vector<std::string>& inPrevData, std::vector<unsigned char>& outGcodeStr);
}
#endif // _THUMBNAIL_1635927924764_H