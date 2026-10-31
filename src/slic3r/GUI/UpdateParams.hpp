#ifndef slic3r_UpdateParams_hpp_
#define slic3r_UpdateParams_hpp_
#include <vector>
#include <string>
#include <map>
#include <set>
#include <json_diff.hpp>

namespace Slic3r {
namespace GUI {

namespace nsUpdateParams {
// Read installation receipts afresh; catalog downloads are not installations.
std::map<std::string, std::string> loadInstalledPrinterVersions();
// Share the same definition of added printers between tips and automatic updates.
std::set<std::string> getAddedPrinterNames();

class CXCloud
{
public:
    CXCloud()  = default;
    ~CXCloud() = default;

    int getNeedUpdatePrintes(std::vector<std::string>& vtNeedUpdatePrinter);
};
} // namespace nsUpdateParams

class UpdateParams
{
public:
    static UpdateParams& getInstance();

    void checkParamsNeedUpdate();
    void closeParamsUpdateTip();
    void hasUpdateParams();

private:
    UpdateParams();

private:
    nsUpdateParams::CXCloud m_CXCloud;
};

}
}

#endif
