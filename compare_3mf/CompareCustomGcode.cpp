#include "CompareCustomGcode.h"
#include "tinyxml2.h"
#include <iostream>
#include <unordered_map>

using namespace tinyxml2;

void CompareCustomGcode::compare_files(const std::string& file1, const std::string& file2) {}

void CompareCustomGcode::compare_buffer(const char* buffer1, size_t size1, const char* buffer2, size_t size2)
{
    XMLDocument doc1;
    XMLError    error1 = doc1.Parse(buffer1, size1);
    XMLDocument doc2;
    XMLError    error2 = doc2.Parse(buffer2, size2);
    if (error1 != XML_SUCCESS || error2 != XML_SUCCESS) {
        set_compare_result(false);
        set_compare_result_str("Invalid XML file");
        return;
    }

    std::vector<PlateInfo> plate_list1;
    std::vector<PlateInfo> plate_list2;

    auto get_info = [](const XMLElement* root, std::vector<PlateInfo>& plate_list) {
        const XMLElement* child = root->FirstChildElement("plate");
        while (child) {
            PlateInfo         plate_info;
            const XMLElement* plate_info_child = child->FirstChildElement();
            while (plate_info_child) {
                auto name = plate_info_child->Name();
                if (name == "plate_info") {
                    const XMLAttribute* attribute = plate_info_child->FindAttribute("id");
                    plate_info.id_                = std::atoi(attribute->Value());
                } else if (name == "mode") {
                    const XMLAttribute* attribute = plate_info_child->FindAttribute("value");
                    plate_info.mode_              = attribute->Value();
                } else if (name == "layer") {
					PlateInfo::LayerInfo layer_info;
					const XMLAttribute* attribute = plate_info_child->FindAttribute("top_z");
					layer_info.top_z_             = attribute->Value();
					attribute                      = plate_info_child->FindAttribute("type");
					layer_info.type_               = std::atoi(attribute->Value());
					attribute                      = plate_info_child->FindAttribute("extruder");
					layer_info.extruder_           = std::atoi(attribute->Value());
					attribute                      = plate_info_child->FindAttribute("color");
					layer_info.color_              = attribute->Value();
					attribute                      = plate_info_child->FindAttribute("extra");
					layer_info.extra_              = attribute->Value();
					layer_info.gcode_              = plate_info_child->GetText();
					plate_info.layers_.emplace_back(layer_info);
				}
				plate_info_child = plate_info_child->NextSiblingElement();
            }
            plate_list.emplace_back(plate_info);
            child = child->NextSiblingElement("plate");
        }
    };
    get_info(doc1.RootElement(), plate_list1);
    get_info(doc2.RootElement(), plate_list2);
    if (plate_list1.size() != plate_list2.size()) {
        set_compare_result(false);
        set_compare_result_str("Metadata mismatch");
        return;
    }
    for (size_t i = 0; i < plate_list1.size(); ++i) {
        if (!(plate_list1[i] == plate_list2[i])) {
            set_compare_result(false);
            set_compare_result_str("Metadata mismatch");
            return;
        }
    }
}
