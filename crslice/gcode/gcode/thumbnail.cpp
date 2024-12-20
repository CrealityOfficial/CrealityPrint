#include "crslice/gcode/thumbnail.h"
#include "img2gcode.h"

namespace gcode
{
    bool thumbnail_to_gcode(const std::vector<unsigned char>& inPrevData, const std::string& inImgSizes, const std::string& inImgFormat,
        const std::string& imgPixelSE, const int& inlayerCount, std::vector<std::string>& outGcodeStr, const float& layerHeight)
    {
        std::string imageMeg;
        if (layerHeight != 0.)
        {
            imageMeg = imgPixelSE + " " + std::to_string(inlayerCount);
        }
        return  Img2Gcode::imgEncode(inPrevData, outGcodeStr, inImgFormat, inImgSizes, imageMeg, nullptr);
    }

    bool thumbnail_image2base64(const std::vector<unsigned char>& inPrevData, const std::string& inImgSizes, const std::string& inImgFormat, std::vector<std::string>& outGcodeStr)
    {
        return  Img2Gcode::image2base(inPrevData, inImgSizes, inImgFormat, std::string(), outGcodeStr);
    }

    bool thumbnail_base2image(const std::vector<std::string>& inPrevData, std::vector<unsigned char>& outGcodeStr)
    {
        return  Img2Gcode::base2image(inPrevData, outGcodeStr);
    }
}