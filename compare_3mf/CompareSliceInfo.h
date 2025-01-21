#ifndef COMPARE_SLICE_INFO_H
#define COMPARE_SLICE_INFO_H
#include "CompareBase.h"
#include <unordered_map>

class CompareSliceInfo : public CompareBase
{
public:
    struct PlateInfo
    {
        std::unordered_map<std::string, std::string> metadata_;
        struct ObjectInfo
        {
            int         identify_id_{0};
            std::string name_;
            bool        skipped_{false};
            bool        operator==(const ObjectInfo& obj) const { return name_ == obj.name_ && skipped_ == obj.skipped_; }
        };
        struct Filament
        {
            int         id_{0};
            std::string tray_info_idx_;
            std::string type_;
            std::string color_;
            float       used_m_{0.0f};
            float       used_g_{0.0f};

            bool operator==(const Filament& fil) const
            {
                const float epsilon = 1e-6f;
                return tray_info_idx_ == fil.tray_info_idx_ && type_ == fil.type_ && color_ == fil.color_ &&
                       std::fabs(used_m_ - fil.used_m_) < epsilon && std::fabs(used_g_ - fil.used_g_) < epsilon;
            }
        };
        ObjectInfo object_;
        Filament   filament_;

        bool operator==(const PlateInfo& plate) const
        {
            return metadata_ == plate.metadata_ && object_ == plate.object_ && filament_ == plate.filament_;
        }
    };

public:
    CompareSliceInfo() = default;
    CompareSliceInfo(CompareOptions compare_option) : CompareBase(compare_option) {}
    ~CompareSliceInfo() = default;

    void compare_files(const std::string& file1, const std::string& file2) override;
    void compare_buffer(const char* buffer1, size_t size1, const char* buffer2, size_t size2) override;
};
#endif // !COMPARE_SLICE_INFO_H
