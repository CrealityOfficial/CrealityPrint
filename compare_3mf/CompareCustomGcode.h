#ifndef COMPARE_CUSTOM_GCODE_H
#define COMPARE_CUSTOM_GCODE_H
#include "CompareBase.h"
#include <list>
#include <vector>

class CompareCustomGcode : public CompareBase
{
public:
    struct PlateInfo
	{
        int         id_{0};
        std::string mode_;
		struct LayerInfo
		{
            std::string top_z_;
            int         type_{0};
            int         extruder_{0};
            std::string color_;
            std::string extra_;
            std::string gcode_;
            bool        operator==(const LayerInfo& layer) const { return top_z_ == layer.top_z_ && type_ == layer.type_ && extruder_ == layer.extruder_ && color_ == layer.color_ && extra_ == layer.extra_ && gcode_ == layer.gcode_; }
		};
        std::vector<LayerInfo> layers_;

		bool operator==(const PlateInfo& plate) const
		{
			if (mode_ != plate.mode_ || layers_.size() != plate.layers_.size())
				return false;
			for (size_t i = 0; i < layers_.size(); ++i)
			{
				if (!(layers_[i] == plate.layers_[i]))
					return false;
			}
			return true;
		}
	};
public:
    CompareCustomGcode() = default;
    CompareCustomGcode(CompareOptions compare_option) : CompareBase(compare_option) {}
    ~CompareCustomGcode() = default;

    void compare_files(const std::string& file1, const std::string& file2) override;
    void compare_buffer(const char* buffer1, size_t size1, const char* buffer2, size_t size2) override;
};
#endif // !COMPARE_CUSTOM_GCODE_H
