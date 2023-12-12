#ifndef _IMG2GCODE_H
#define _IMG2GCODE_H
#include <vector>
#include <string>
namespace gcode
{
	class  Img2Gcode
	{
	public:
		Img2Gcode();
		~Img2Gcode();

		static bool imgEncode(const std::vector<unsigned char>& prevData, std::vector<std::string>& encodeData,
			const std::string& imgFormat, const std::string& imageSize, const std::string& imageMeg, const char* saveFile);
		static bool imgDecode(std::vector<std::string>& prevData, std::vector<unsigned char>& decodeData);

		static bool image2base(const std::vector<unsigned char>& prevData, const std::string& imgSizes, const std::string& imgFormat,
			const std::string& imgPixelSE, std::vector<std::string>& encodeData);
		static bool base2image(const std::vector<std::string>& prevData, std::vector<unsigned char>& decodeData);
	};
}
#endif