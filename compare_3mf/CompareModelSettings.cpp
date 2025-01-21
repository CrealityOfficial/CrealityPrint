#include "CompareModelSettings.h"
#include "tinyxml2.h"
#include <iostream>
#include <unordered_map>

using namespace tinyxml2;

void CompareModelSettings::compare_files(const std::string& file1, const std::string& file2) {}

void CompareModelSettings::compare_buffer(const char* buffer1, size_t size1, const char* buffer2, size_t size2)
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

    std::unordered_map<std::string, std::string> map1;
    std::unordered_map<std::string, std::string> map2;

    auto get_info = [](const XMLElement* root, std::unordered_map<std::string, std::string>& map) {
        const XMLAttribute* attribute = root->FindAttribute("unit");
        if (attribute) {
            map["unit"] = attribute->Value();
        }

        const XMLElement* child = root->FirstChildElement("metadata");
        while (child) {
            const XMLAttribute* attribute = child->FindAttribute("name");
            auto                text      = child->GetText();
            auto                name      = attribute->Value();
            map.emplace(name, text ? text : "");
            child = child->NextSiblingElement("metadata");
        }
    };
    get_info(doc1.RootElement(), map1);
    get_info(doc2.RootElement(), map2);

    for (auto& [key, value] : map1) {
        if (map2.find(key) == map2.end() || map2[key] != value) {
            set_compare_result(false);
            set_compare_result_str("Metadata mismatch");
            return;
        }
    }
}
