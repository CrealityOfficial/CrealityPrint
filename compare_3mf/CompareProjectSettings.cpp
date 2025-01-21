#include "CompareProjectSettings.h"
#include <iostream>
#include <unordered_map>

void CompareProjectSettings::compare_files(const std::string& file1, const std::string& file2) {}

void CompareProjectSettings::compare_buffer(const char* buffer1, size_t size1, const char* buffer2, size_t size2)
{
    nlohmann::json json1 = nlohmann::json::parse(buffer1, buffer1 + size1);
    nlohmann::json json2 = nlohmann::json::parse(buffer2, buffer2 + size2);
    nlohmann::json json_diff;
    diff_objects(json2, json_diff, json1);

    if (json_diff.empty()) {
        std::cout << "Project settings are identical" << std::endl;
    } else {
        std::cout << "Project settings are different" << std::endl;
        std::cout << json_diff.dump(4) << std::endl;
    }
}

bool CompareProjectSettings::diff_objects(json const& in, json& out, json const& base)
{
    for (auto& el : in.items()) {
        if (el.value().empty()) {
            continue;
        }

        if (!base.contains(el.key())) {
            out[el.key()] = el.value();
            continue;
        }

        if (el.value().type() != base[el.key()].type()) {
            out[el.key()] = el.value();
            continue;
        }

        if (el.value().is_object()) {
            json recur_out;
            bool  recur_ret = diff_objects(el.value(), recur_out, base[el.key()]);
            if (!recur_ret) {
                out[el.key()] = recur_out;
            }
            continue;
        }

        if (el.value() != base[el.key()]) {
            out[el.key()] = el.value();
            continue;
        }
    }
    if (out.empty()) {
        return true;
    }
    return false;
}
