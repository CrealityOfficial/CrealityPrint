#include "PrinterMgr.hpp"

#include "../I18N.hpp"
#include "PrinterMgr.hpp"
#include "slic3r/GUI/GUI_App.hpp"
#include "slic3r/GUI/MainFrame.hpp"
#include "libslic3r_version.h"
#include "libslic3r/Utils.hpp"
#include "AppUtils.hpp"

#include <algorithm>
#include <cerrno>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <limits>
#include <map>
#include <set>
namespace pt = boost::property_tree;
using json = nlohmann::json;

namespace DM {

    class DeviceLoaderV512
    {
    public:
        void Load(DeviceMgr* mgr, boost::filesystem::path& path)
        {
            boost::nowide::ifstream t(path.string());
            std::stringstream buffer;
            buffer << t.rdbuf();

            json data = json::parse(buffer);
            std::vector<std::string> vtGroup;
            if (data.contains("deviceGroupNames"))
            {
                for (auto& group : data["deviceGroupNames"])
                {
                    mgr->AddGroup(group);
                    vtGroup.push_back(group);
                }
            }

            if (data.contains("deviceInfomation"))
            {
                for (auto& device : data["deviceInfomation"])
                {
                    DeviceMgr::Data data;
                    if (device.contains("connectType"))data.connectType = device["connectType"];
                    if (device.contains("modelName"))data.model = device["modelName"];
                    if (device.contains("macAddress"))data.mac = device["macAddress"];
                    if (device.contains("ipAddress"))data.address = device["ipAddress"];
                    if (device.contains("deviceName"))data.name = device["deviceName"];

                    std::string sGroup;
                    if (device.contains("group")) {
                        int groupIndex = device["group"].get<int>() - 1;
                        sGroup = vtGroup[groupIndex];
                    }

                    if (!mgr->IsPrinterExist(data.mac))
                        mgr->AddDevice(sGroup, data);
                }
            }
        }
    };

    class CurrentDeviceCfigV603{
    public:
        static std::string get_current_device_mac(){
            std::string mac;
            boost::filesystem::path device_file = boost::filesystem::path(Slic3r::data_dir()) / "current_device.json";
            if (boost::filesystem::exists(device_file)){

                boost::nowide::ifstream t(device_file.string());
                std::stringstream buffer;
                buffer << t.rdbuf();

                json data = json::parse(buffer);
                if(data.contains("current_device")&&data["current_device"].contains("mac")){
                    mac = data["current_device"]["mac"];
                }
            }

            return mac;
        }
    };

    struct DeviceMgr::priv
    {
        json data;
        std::map<std::string, std::vector<DeviceMgr::Data>> store;
        std::vector<std::string> order;
    };

    template <typename T>
    static T json_value_or(const json& item, const char* key, T fallback)
    {
        const auto value = item.find(key);
        if (value == item.end())
            return fallback;
        try {
            return value->get<T>();
        } catch (...) {
            return fallback;
        }
    }

    static std::uint64_t address_revision_from_json(const json& item)
    {
        const auto value = item.find("addressRevision");
        if (value == item.end())
            return 0;
        if (value->is_number_unsigned())
            return value->get<std::uint64_t>();
        if (value->is_number_integer()) {
            const std::int64_t revision = value->get<std::int64_t>();
            return revision >= 0 ? static_cast<std::uint64_t>(revision) : 0;
        }
        return 0;
    }

    static void advance_address_revision(json& item)
    {
        const std::uint64_t revision = address_revision_from_json(item);
        if (revision < std::numeric_limits<std::uint64_t>::max())
            item["addressRevision"] = revision + 1;
        else
            item["addressRevision"] = revision;
    }

    static DeviceMgr::Data device_data_from_json(const json& item)
    {
        DeviceMgr::Data data{};
        data.connectType         = json_value_or(item, "connectType", 0);
        data.model               = json_value_or(item, "model", std::string());
        data.mac                 = json_value_or(item, "mac", std::string());
        data.address             = json_value_or(item, "address", std::string());
        data.addressRevision     = address_revision_from_json(item);
        data.name                = json_value_or(item, "name", std::string());
        data.deviceUI            = json_value_or(item, "deviceUI", std::string());
        data.oldPrinter          = json_value_or(item, "oldPrinter", false);
        data.secureConnection    = json_value_or(item, "secureConnection", false);
        data.wssPort             = json_value_or(item, "wssPort", 0);
        data.videoPort           = json_value_or(item, "videoPort", 0);
        data.moonrakerPort        = json_value_or(item, "moonrakerPort", 0);
        data.fluiddPort           = json_value_or(item, "fluiddPort", 0);
        data.mainsailPort         = json_value_or(item, "mainsailPort", 0);
        data.apiKey               = json_value_or(item, "apiKey", std::string());
        data.hostType             = json_value_or(item, "hostType", 0);
        data.caFile               = json_value_or(item, "caFile", std::string());
        data.ignoreCertRevocation = json_value_or(item, "ignoreCertRevocation", false);
        return data;
    }

    static bool is_modern_lan_device(const json& item)
    {
        return item.is_object() &&
               json_value_or(item, "connectType", 0) == 3 &&
               !json_value_or(item, "oldPrinter", false);
    }

    static std::vector<DeviceMgr::ModernLanDeviceSnapshot> modern_lan_snapshot_from_json(
        const json& data)
    {
        std::vector<DeviceMgr::ModernLanDeviceSnapshot> snapshot;
        if (!data.contains("groups") || !data["groups"].is_array())
            return snapshot;

        std::size_t raw_index = 0;
        for (const auto& group : data["groups"]) {
            if (!group.is_object() || !group.contains("list") || !group["list"].is_array())
                continue;

            const std::string group_name = json_value_or(group, "group", std::string());
            for (const auto& item : group["list"]) {
                if (is_modern_lan_device(item)) {
                    snapshot.push_back(DeviceMgr::ModernLanDeviceSnapshot{
                        raw_index,
                        group_name,
                        device_data_from_json(item)
                    });
                }
                ++raw_index;
            }
        }
        return snapshot;
    }
    static std::string normalize_mac_for_compare(const std::string& mac)
    {
        std::string normalized;
        normalized.reserve(mac.size());
        for (unsigned char ch : mac) {
            if (ch == ':' || ch == '-' || ch == '.' || std::isspace(ch))
                continue;
            if (!std::isxdigit(ch))
                return {};
            normalized.push_back(static_cast<char>(std::toupper(ch)));
        }
        return normalized.size() == 12 ? normalized : std::string();
    }

    static bool save_device_data_checked(const json& data, std::string& error)
    {
        const boost::filesystem::path device_file =
            boost::filesystem::path(Slic3r::data_dir()) / "deviceInfo.json";
        boost::filesystem::path temporary_file = device_file;
        temporary_file += ".reconcile.tmp";

        auto remove_temporary_file = [&] {
            boost::system::error_code remove_error;
            boost::filesystem::remove(temporary_file, remove_error);
        };

        try {
            boost::nowide::ofstream stream;
            stream.open(temporary_file.string(), std::ios::out | std::ios::trunc);
            if (!stream.is_open()) {
                error = "failed to open temporary device file";
                remove_temporary_file();
                return false;
            }

            stream << std::setw(4) << data << std::endl;
            stream.flush();
            if (!stream.good()) {
                error = "failed to write temporary device file";
                stream.close();
                remove_temporary_file();
                return false;
            }
            stream.close();
            if (stream.fail()) {
                error = "failed to close temporary device file";
                remove_temporary_file();
                return false;
            }
        } catch (const std::exception& e) {
            error = e.what();
            remove_temporary_file();
            return false;
        }

        try {
#ifdef _WIN32
            const std::error_code rename_error =
                Slic3r::rename_file(temporary_file.string(), device_file.string());
            if (rename_error) {
                error = "failed to replace device file: " + rename_error.message();
                remove_temporary_file();
                return false;
            }
#else
            if (std::rename(temporary_file.string().c_str(), device_file.string().c_str()) != 0) {
                const int rename_errno = errno;
                error = "failed to replace device file: " + std::string(std::strerror(rename_errno));
                remove_temporary_file();
                return false;
            }
#endif
        } catch (const std::exception& e) {
            error = e.what();
            remove_temporary_file();
            return false;
        }
        return true;
    }

    DeviceMgr::DeviceMgr() :p(new priv)
    {

    }

    DeviceMgr::~DeviceMgr()
    {

    }

    void DeviceMgr::Load()
    {
        boost::filesystem::path device_file = boost::filesystem::path(Slic3r::data_dir()) / "deviceInfo.json";
        if (!boost::filesystem::exists(device_file))
        {
            boost::filesystem::path device_old_file = boost::filesystem::path(Slic3r::data_dir()).parent_path().parent_path() / "Creative3D/deviceInfo.json";
            if (boost::filesystem::exists(device_old_file))
            {
                DeviceLoaderV512 loader;
                loader.Load(this, device_old_file);
                this->Save();
            }
        }
        else//load from current custom folder
        {
            try{
            boost::nowide::ifstream t(device_file.string());
            std::stringstream buffer;
            buffer << t.rdbuf();
            
            p->data = json::parse(buffer);
            }
            catch (const std::exception& e)
            {
                boost::filesystem::remove(device_file);
                p->data = json::object();
            }
            if (p->data.contains("groups"))
            {
                for (auto& group : p->data["groups"])
                {
                    if (group.contains("list") && !group["list"].is_null())
                    {
                        for (auto jt = group["list"].begin(); jt != group["list"].end(); jt++) {
                            if (!jt.value().is_object())
                                continue;
                            jt.value().erase("identityReconcile");
                            jt.value().erase("addressUpdate");
                            jt.value()["addressRevision"] = address_revision_from_json(jt.value());
                        }
                    }
                }
            }

#ifdef __WXGTK__
            if (is_uos_system() && p->data.contains("groups")) {
                int total = 0;
                for (auto& group : p->data["groups"]) {
                    if (!group.contains("list") || group["list"].is_null())
                        continue;
                    auto& list = group["list"];
                    if (!list.is_array())
                        continue;
                    if (total >= 20) {
                        list = nlohmann::json::array();
                        continue;
                    }
                    int remaining = 20 - total;
                    if (remaining <= 0) {
                        list = nlohmann::json::array();
                        continue;
                    }
                    if (static_cast<int>(list.size()) > remaining) {
                        nlohmann::json new_list = nlohmann::json::array();
                        for (int i = 0; i < remaining; ++i)
                            new_list.push_back(list[i]);
                        list = new_list;
                        total = 20;
                    } else {
                        total += static_cast<int>(list.size());
                    }
                }
            }
#endif
        }

        if (p->data.empty())
        {
            this->AddGroup("New Group1");
        }

        // can set current device is empty now
        /*if(this->GetCurrentDevice().empty()){
            std::string mac = CurrentDeviceCfigV603::get_current_device_mac();
            if(!mac.empty())
            {
                this->SetCurrentDevice(mac);
            }
        }*/

        //clear not use default group
        std::vector<int> remove_group_ids;
        int index = 0;
        for (auto& group : p->data["groups"]){
            std::string name = group["group"];
            if(name == "Default" && group.contains("list") && group["list"].is_null()){
                remove_group_ids.push_back(index);
            }

            index++;
        }

        for(int i =remove_group_ids.size()-1; i>=0 ;i-- ){
             p->data["groups"].erase(remove_group_ids[i]);
        }

        remove_group_ids.clear();
        index = 0;
        int cnt = 0;
        int null_cnt = 0;
        for (auto& group : p->data["groups"]) {
            std::string name = group["group"];
            if (name == "Default") {
                if(!group.contains("list")){
                    null_cnt++;
                    remove_group_ids.push_back(index);
                }

                cnt++;
            }

            index++;
        }

        int n = cnt==null_cnt?1:0;
        for (int i = remove_group_ids.size()-1;  i >= n; i--) {//Just keep one
            p->data["groups"].erase(remove_group_ids[i]);
        }

        this->Save();
        //  CLEAR END
    }

    void DeviceMgr::Save()
    {
        boost::filesystem::path device_file = boost::filesystem::path(Slic3r::data_dir()) / "deviceInfo.json";

        boost::nowide::ofstream c;
        c.open(device_file.string(), std::ios::out | std::ios::trunc);
        c << std::setw(4) << p->data << std::endl;

    }
    void DeviceMgr::UpdateDevice(std::string mac, Data& data)
    {
        for (auto& group : p->data["groups"])
        {
            int index = 0;
            for (auto& item : group["list"])
            {
                std::string omac = item["mac"];
                if (mac == omac)
                {
                    if (item.value("address", std::string()) != data.address)
                        advance_address_revision(item);
                    item["mac"] = data.mac;
                    item["address"] = data.address;
                    item["model"] = data.model;
                    item["connectType"] = data.connectType;
                    item["secureConnection"] = data.secureConnection;
                    item["wssPort"] = data.wssPort;
                    item["videoPort"] = data.videoPort;
                    this->Save();
                    return;
                }
            }
        }
        std::string ip = data.address;
        for (auto& group : p->data["groups"])
        {
            int index = 0;
            for (auto& item : group["list"])
            {
                std::string address = item["address"];
                if (ip == address)
                {
                    if (address != data.address)
                        advance_address_revision(item);
                    item["mac"] = data.mac;
                    item["address"] = data.address;
                    item["model"] = data.model;
                    item["connectType"] = data.connectType;
                    item["secureConnection"] = data.secureConnection;
                    item["wssPort"] = data.wssPort;
                    item["videoPort"] = data.videoPort;
                    this->Save();
                    return;
                }
            }
        }
    }

    bool DeviceMgr::UpdateDevicePatch(const std::string& lookupMac, const UpdatePatch& patch, bool allowSecureDowngrade)
    {
        if (lookupMac.empty() && !patch.lookupAddress)
            return false;
        if (patch.mac && patch.mac->empty())
            return false;

        const bool hasSecureConnection = patch.connection.secureConnection.has_value();
        const bool hasWssPort = patch.connection.wssPort.has_value();
        const bool hasVideoPort = patch.connection.videoPort.has_value();
        const bool hasAnyConnectionField = hasSecureConnection || hasWssPort || hasVideoPort;
        const bool hasCompleteConnection = hasSecureConnection && hasWssPort && hasVideoPort;

        if (hasAnyConnectionField && !hasCompleteConnection)
            return false;

        bool secureConnection = false;
        int wssPort = 0;
        int videoPort = 0;
        if (hasCompleteConnection) {
            secureConnection = *patch.connection.secureConnection;
            wssPort = *patch.connection.wssPort;
            videoPort = *patch.connection.videoPort;

            if (secureConnection) {
                if (wssPort <= 0 || wssPort > 65535 || videoPort <= 0 || videoPort > 65535)
                    return false;
            } else {
                wssPort = 0;
                videoPort = 0;
            }
        }

        if (!p->data.contains("groups") || !p->data["groups"].is_array())
            return false;

        json* target = nullptr;
        for (auto& group : p->data["groups"]) {
            if (!group.is_object() || !group.contains("list") || !group["list"].is_array())
                continue;
            for (auto& item : group["list"]) {
                if (!lookupMac.empty() && item.is_object() &&
                    item.value("mac", std::string()) == lookupMac) {
                    target = &item;
                    break;
                }
            }
            if (target != nullptr)
                break;
        }

        if (target == nullptr && patch.lookupAddress) {
            for (auto& group : p->data["groups"]) {
                if (!group.is_object() || !group.contains("list") || !group["list"].is_array())
                    continue;
                for (auto& item : group["list"]) {
                    if (item.is_object() && item.value("mac", std::string()).empty() &&
                        item.value("address", std::string()) == *patch.lookupAddress) {
                        target = &item;
                        break;
                    }
                }
                if (target != nullptr)
                    break;
            }
        }

        if (target == nullptr)
            return false;

        if (patch.mac && target->value("mac", std::string()) != *patch.mac) {
            for (auto& group : p->data["groups"]) {
                if (!group.is_object() || !group.contains("list") || !group["list"].is_array())
                    continue;
                for (auto& item : group["list"]) {
                    if (&item != target && item.is_object() &&
                        item.value("mac", std::string()) == *patch.mac) {
                        return false;
                    }
                }
            }
            (*target)["mac"] = *patch.mac;
        }
        if (patch.address && target->value("address", std::string()) != *patch.address) {
            advance_address_revision(*target);
            (*target)["address"] = *patch.address;
        }
        if (patch.model)
            (*target)["model"] = *patch.model;
        if (patch.connectType)
            (*target)["connectType"] = *patch.connectType;

        if (hasCompleteConnection) {
            const bool wasSecure = target->value("secureConnection", false);
            if (!wasSecure || secureConnection || allowSecureDowngrade) {
                (*target)["secureConnection"] = secureConnection;
                (*target)["wssPort"] = wssPort;
                (*target)["videoPort"] = videoPort;
            }
        }

        Save();
        return true;
    }

    DeviceMgr::UpdateDeviceResult DeviceMgr::UpdateDevicePatchCas(
        const std::string& lookupMac,
        const UpdatePatch& patch,
        const AddressUpdateExpectation& expectation,
        bool allowSecureDowngrade)
    {
        UpdateDeviceResult result;
        const std::string lookup_mac = normalize_mac_for_compare(lookupMac);
        if (lookup_mac.empty() || !patch.address || patch.address->empty() ||
            expectation.expectedAddressRevision == std::numeric_limits<std::uint64_t>::max()) {
            result.error = "lookup MAC, target address, or expected revision is invalid";
            return result;
        }
        if (patch.mac && normalize_mac_for_compare(*patch.mac) != lookup_mac) {
            result.error = "the patched MAC must match the lookup MAC";
            return result;
        }

        const bool has_secure_connection = patch.connection.secureConnection.has_value();
        const bool has_wss_port = patch.connection.wssPort.has_value();
        const bool has_video_port = patch.connection.videoPort.has_value();
        const bool has_any_connection_field =
            has_secure_connection || has_wss_port || has_video_port;
        const bool has_complete_connection =
            has_secure_connection && has_wss_port && has_video_port;
        if (has_any_connection_field && !has_complete_connection) {
            result.error = "connection fields must be provided together";
            return result;
        }

        bool secure_connection = false;
        int wss_port = 0;
        int video_port = 0;
        if (has_complete_connection) {
            secure_connection = *patch.connection.secureConnection;
            wss_port = *patch.connection.wssPort;
            video_port = *patch.connection.videoPort;
            if (wss_port < 0 || wss_port > 65535 || video_port < 0 || video_port > 65535 ||
                (secure_connection && (wss_port == 0 || video_port == 0))) {
                result.error = "connection ports are invalid";
                return result;
            }
            if (!secure_connection) {
                wss_port = 0;
                video_port = 0;
            }
        }

        json next_data = p->data;
        if (!next_data.contains("groups") || !next_data["groups"].is_array()) {
            result.error = "device groups are unavailable";
            return result;
        }

        json* target = nullptr;
        std::size_t target_count = 0;
        for (auto& group : next_data["groups"]) {
            if (!group.is_object() || !group.contains("list") || !group["list"].is_array())
                continue;
            for (auto& item : group["list"]) {
                if (!item.is_object() ||
                    normalize_mac_for_compare(item.value("mac", std::string())) != lookup_mac)
                    continue;
                ++target_count;
                if (target == nullptr)
                    target = &item;
            }
        }

        if (target_count == 0) {
            result.outcome = UpdateDeviceOutcome::SourceNotFound;
            result.error = "device record was not found";
            return result;
        }
        if (target_count > 1) {
            result.outcome = UpdateDeviceOutcome::DuplicateMacConflict;
            result.error = "multiple records found for the lookup MAC";
            return result;
        }

        result.device = device_data_from_json(*target);
        const std::string target_address = *patch.address;
        const std::string current_address = target->value("address", std::string());
        const std::uint64_t current_revision = address_revision_from_json(*target);
        const bool already_applied =
            current_address == target_address &&
            current_revision == expectation.expectedAddressRevision + 1;
        if (!already_applied &&
            (current_address != expectation.expectedAddress ||
             current_revision != expectation.expectedAddressRevision)) {
            result.outcome = UpdateDeviceOutcome::StaleSource;
            result.error = "device address or revision has changed";
            return result;
        }

        for (auto& group : next_data["groups"]) {
            if (!group.is_object() || !group.contains("list") || !group["list"].is_array())
                continue;
            for (auto& item : group["list"]) {
                if (&item != target && item.is_object() &&
                    item.value("address", std::string()) == target_address) {
                    result.outcome = UpdateDeviceOutcome::AddressConflict;
                    result.error = "target address is occupied by another device record";
                    return result;
                }
            }
        }
        if (already_applied) {
            result.outcome = UpdateDeviceOutcome::AlreadyApplied;
            return result;
        }

        (*target)["address"] = target_address;
        (*target)["addressRevision"] = expectation.expectedAddressRevision + 1;
        if (patch.model)
            (*target)["model"] = *patch.model;
        if (patch.connectType)
            (*target)["connectType"] = *patch.connectType;

        if (has_complete_connection) {
            const bool was_secure = target->value("secureConnection", false);
            if (!was_secure || secure_connection || allowSecureDowngrade) {
                (*target)["secureConnection"] = secure_connection;
                (*target)["wssPort"] = wss_port;
                (*target)["videoPort"] = video_port;
            }
        }

        std::string save_error;
        if (!save_device_data_checked(next_data, save_error)) {
            result.outcome = UpdateDeviceOutcome::PersistenceFailed;
            result.error = save_error;
            return result;
        }

        result.device = device_data_from_json(*target);
        p->data.swap(next_data);
        result.outcome = UpdateDeviceOutcome::Updated;
        result.changed = true;
        return result;
    }

    DeviceMgr::DeviceOwnershipBatchResult DeviceMgr::ReconcileDeviceOwnershipBatch(
        const DeviceOwnershipBatchRequest& request)
    {
        DeviceOwnershipBatchResult result;
        result.snapshot = modern_lan_snapshot_from_json(p->data);

        if (!p->data.contains("groups") || !p->data["groups"].is_array()) {
            result.error = "device groups are unavailable";
            return result;
        }
        if (request.expectedRecords.size() > 4096 || request.probes.size() > 1024) {
            result.error = "ownership batch is too large";
            return result;
        }

        struct RecordRef
        {
            json* item = nullptr;
            std::size_t rawIndex = 0;
            std::size_t groupIndex = 0;
            std::size_t recordIndex = 0;
            std::string group;
            std::string mac;
            std::string normalizedMac;
        };

        json next_data = p->data;
        std::vector<RecordRef> records;
        std::size_t raw_index = 0;
        for (std::size_t group_index = 0; group_index < next_data["groups"].size(); ++group_index) {
            auto& group = next_data["groups"][group_index];
            if (!group.is_object() || !group.contains("list") || !group["list"].is_array())
                continue;
            const std::string group_name = json_value_or(group, "group", std::string());
            for (std::size_t record_index = 0; record_index < group["list"].size(); ++record_index) {
                auto& item = group["list"][record_index];
                if (is_modern_lan_device(item)) {
                    const std::string mac = json_value_or(item, "mac", std::string());
                    const std::string normalized_mac = normalize_mac_for_compare(mac);
                    if (!normalized_mac.empty()) {
                        records.push_back(RecordRef{
                            &item,
                            raw_index,
                            group_index,
                            record_index,
                            group_name,
                            mac,
                            normalized_mac
                        });
                    }
                }
                ++raw_index;
            }
        }

        if (records.size() != request.expectedRecords.size()) {
            result.outcome = DeviceOwnershipBatchOutcome::StaleSource;
            result.error = "modern device snapshot has changed";
            return result;
        }

        std::set<std::size_t> expected_indices;
        for (std::size_t index = 0; index < records.size(); ++index) {
            const auto& current = records[index];
            const auto& expected = request.expectedRecords[index];
            if (!expected_indices.insert(expected.rawIndex).second ||
                expected.rawIndex != current.rawIndex ||
                expected.group != current.group ||
                expected.mac != current.mac ||
                expected.address != current.item->value("address", std::string()) ||
                expected.addressRevision != address_revision_from_json(*current.item)) {
                result.outcome = DeviceOwnershipBatchOutcome::StaleSource;
                result.error = "modern device snapshot has changed";
                return result;
            }
            if (expected.addressRevision == std::numeric_limits<std::uint64_t>::max()) {
                result.error = "address revision cannot be advanced";
                return result;
            }
        }

        std::map<std::string, std::size_t> canonical_by_mac;
        std::set<std::string> duplicate_macs;
        std::vector<std::size_t> duplicate_indices;
        for (std::size_t index = 0; index < records.size(); ++index) {
            const auto& normalized_mac = records[index].normalizedMac;
            const auto inserted = canonical_by_mac.emplace(normalized_mac, index);
            if (!inserted.second) {
                duplicate_macs.insert(normalized_mac);
                duplicate_indices.push_back(index);
            }
        }

        std::set<std::string> expected_addresses;
        std::map<std::string, std::size_t> address_counts;
        for (const auto& record : records) {
            const std::string address = record.item->value("address", std::string());
            if (!address.empty()) {
                expected_addresses.insert(address);
                ++address_counts[address];
            }
        }

        std::map<std::string, const DeviceOwnershipProbe*> probes_by_address;
        for (const auto& probe : request.probes) {
            if (probe.address.empty() ||
                probes_by_address.find(probe.address) != probes_by_address.end()) {
                result.error = "probe addresses must be non-empty and unique";
                return result;
            }
            if (probe.ok) {
                const bool ports_out_of_range = probe.wssPort < 0 || probe.wssPort > 65535 ||
                                                probe.videoPort < 0 || probe.videoPort > 65535;
                if (ports_out_of_range ||
                    (probe.secureConnection && (probe.wssPort == 0 || probe.videoPort == 0))) {
                    result.error = "probe connection ports are invalid";
                    return result;
                }
            }
            if (!expected_addresses.count(probe.address)) {
                const std::string observed_mac = normalize_mac_for_compare(probe.observedMac);
                if (!probe.ok || observed_mac.empty() ||
                    canonical_by_mac.find(observed_mac) == canonical_by_mac.end()) {
                    result.error = "new probe addresses must belong to a known observed MAC";
                    return result;
                }
            }
            probes_by_address.emplace(probe.address, &probe);
        }

        std::map<std::string, const DeviceOwnershipProbe*> authoritative_probe_by_mac;
        std::map<std::string, std::string> owner_address_by_mac;
        for (const auto& record : records) {
            const std::string address = record.item->value("address", std::string());
            const auto probe_it = probes_by_address.find(address);
            if (address.empty() || probe_it == probes_by_address.end() || !probe_it->second->ok)
                continue;
            const std::string observed_mac = normalize_mac_for_compare(probe_it->second->observedMac);
            if (observed_mac.empty() || canonical_by_mac.find(observed_mac) == canonical_by_mac.end())
                continue;
            if (authoritative_probe_by_mac.find(observed_mac) == authoritative_probe_by_mac.end()) {
                authoritative_probe_by_mac.emplace(observed_mac, probe_it->second);
                owner_address_by_mac.emplace(observed_mac, address);
            }
        }
        for (const auto& entry : probes_by_address) {
            const DeviceOwnershipProbe& probe = *entry.second;
            if (!probe.ok)
                continue;
            const std::string observed_mac = normalize_mac_for_compare(probe.observedMac);
            if (observed_mac.empty() || canonical_by_mac.find(observed_mac) == canonical_by_mac.end() ||
                authoritative_probe_by_mac.find(observed_mac) != authoritative_probe_by_mac.end())
                continue;
            authoritative_probe_by_mac.emplace(observed_mac, &probe);
            owner_address_by_mac.emplace(observed_mac, entry.first);
        }

        std::vector<std::string> desired_addresses(records.size());
        for (std::size_t index = 0; index < records.size(); ++index) {
            auto& record = records[index];
            const std::string current_address = record.item->value("address", std::string());
            desired_addresses[index] = current_address;

            const auto owner_it = owner_address_by_mac.find(record.normalizedMac);
            if (owner_it != owner_address_by_mac.end()) {
                desired_addresses[index] = owner_it->second;
                continue;
            }

            const auto probe_it = probes_by_address.find(current_address);
            const bool duplicate_mac = duplicate_macs.count(record.normalizedMac) > 0;
            const bool duplicate_address = !current_address.empty() &&
                address_counts[current_address] > 1;
            if (duplicate_mac) {
                desired_addresses[index].clear();
                continue;
            }
            if (probe_it != probes_by_address.end()) {
                if (probe_it->second->ok) {
                    const std::string observed_mac =
                        normalize_mac_for_compare(probe_it->second->observedMac);
                    if (!observed_mac.empty() && observed_mac != record.normalizedMac)
                        desired_addresses[index].clear();
                } else if (duplicate_address) {
                    desired_addresses[index].clear();
                }
            } else if (duplicate_address) {
                desired_addresses[index].clear();
            }
        }

        bool changed = false;
        for (const auto& canonical : canonical_by_mac) {
            const std::size_t index = canonical.second;
            auto& record = records[index];
            const std::string current_address = record.item->value("address", std::string());
            const std::string& desired_address = desired_addresses[index];
            if (current_address != desired_address) {
                const std::uint64_t revision = address_revision_from_json(*record.item);
                if (revision == std::numeric_limits<std::uint64_t>::max()) {
                    result.error = "address revision cannot be advanced";
                    return result;
                }
                (*record.item)["address"] = desired_address;
                (*record.item)["addressRevision"] = revision + 1;
                changed = true;
            }

            const auto probe_it = authoritative_probe_by_mac.find(canonical.first);
            if (probe_it == authoritative_probe_by_mac.end())
                continue;
            const DeviceOwnershipProbe& probe = *probe_it->second;
            if (!probe.model.empty() && record.item->value("model", std::string()) != probe.model) {
                (*record.item)["model"] = probe.model;
                changed = true;
            }

            const bool was_secure = record.item->value("secureConnection", false);
            if (!was_secure || probe.secureConnection || probe.allowSecureDowngrade) {
                const int wss_port = probe.secureConnection ? probe.wssPort : 0;
                const int video_port = probe.secureConnection ? probe.videoPort : 0;
                if (!record.item->contains("secureConnection") ||
                    was_secure != probe.secureConnection ||
                    !record.item->contains("wssPort") ||
                    record.item->value("wssPort", 0) != wss_port ||
                    !record.item->contains("videoPort") ||
                    record.item->value("videoPort", 0) != video_port) {
                    (*record.item)["secureConnection"] = probe.secureConnection;
                    (*record.item)["wssPort"] = wss_port;
                    (*record.item)["videoPort"] = video_port;
                    changed = true;
                }
            }
        }

        std::sort(duplicate_indices.begin(), duplicate_indices.end(), [&](std::size_t lhs, std::size_t rhs) {
            const auto& left = records[lhs];
            const auto& right = records[rhs];
            if (left.groupIndex != right.groupIndex)
                return left.groupIndex > right.groupIndex;
            return left.recordIndex > right.recordIndex;
        });
        for (const std::size_t index : duplicate_indices) {
            auto& record = records[index];
            auto& list = next_data["groups"][record.groupIndex]["list"];
            if (record.recordIndex < list.size()) {
                list.erase(list.begin() + static_cast<std::ptrdiff_t>(record.recordIndex));
                changed = true;
            }
        }

        if (!changed) {
            result.outcome = DeviceOwnershipBatchOutcome::AlreadyApplied;
            result.snapshot = modern_lan_snapshot_from_json(p->data);
            return result;
        }

        std::string save_error;
        if (!save_device_data_checked(next_data, save_error)) {
            result.outcome = DeviceOwnershipBatchOutcome::PersistenceFailed;
            result.error = save_error;
            result.snapshot = modern_lan_snapshot_from_json(p->data);
            return result;
        }

        p->data.swap(next_data);
        result.changed = true;
        result.outcome = DeviceOwnershipBatchOutcome::Reconciled;
        result.snapshot = modern_lan_snapshot_from_json(p->data);
        return result;
    }
    void DeviceMgr::AddDevice(std::string group, Data& data)
    {
        if (!this->IsGroupExist(group)) {
            this->AddGroup(group, false);
        }

        nlohmann::json item;
        item["address"] = data.address;
        item["addressRevision"] = data.addressRevision;
        item["mac"] = data.mac;
        item["model"] = data.model;
        item["name"] = data.name;
        item["connectType"] = data.connectType;
        item["oldPrinter"] = data.oldPrinter;
        item["secureConnection"] = data.secureConnection;
        item["wssPort"] = data.wssPort;
        item["videoPort"] = data.videoPort;
        item["deviceUI"] = data.deviceUI;

        item["moonrakerPort"] = data.moonrakerPort;
        item["fluiddPort"] = data.fluiddPort;
        item["mainsailPort"] = data.mainsailPort;

        if (data.connectType == 1001) 
        {
            item["apiKey"] = data.apiKey;
            item["hostType"] = data.hostType;
            item["caFile"] = data.caFile;
            item["ignoreCertRevocation"] = data.ignoreCertRevocation;
        }

        for (auto& g : p->data["groups"])
        {
            if (g["group"] == group)
            {
                if (!g.contains("list"))
                {
                    g["list"].push_back(item);
                }
                else
                {
                    if(g["list"].is_null()){
                        g["list"] = nlohmann::json::array();
                        g["list"].push_back(item);
                    }
                    else
                    {
                        g["list"].insert(g["list"].begin(), item);
                    }
                }

                this->Save();
                break;
            }
        }
    }

    void DeviceMgr::RemoveDevice(std::string address)
    {
        for (auto& group : p->data["groups"])
        {
            int index = 0;
            for (auto& item : group["list"])
            {
                std::string ip = item["address"];
                if (ip == address)
                {
//                     std::string mac = item["mac"];
//                     if(mac == this->GetCurrentDevice())
//                     {
//                         this->SetCurrentDevice("");
//                     }
                    group["list"].erase(index);
                    this->Save();
                    return;
                }
                index++;
            }
        }
    }

    void DeviceMgr::EditDeiveName(std::string address, std::string name)
    {
        for (auto& group : p->data["groups"])
        {
            int index = 0;
            for (auto& item : group["list"])
            {
                std::string ip = item["address"];
                if (ip == address)
                {
                    item["name"] = name;
                    this->Save();
                    return;
                }
                index++;
            }
        }
    }

    void DeviceMgr::AddGroup(std::string name,  bool is_save)
    {
        auto& groups = p->data["groups"];

        json item;
        item["group"] = name;
        groups.push_back(item);

        if (is_save){
            this->Save();
        }
    }

    void DeviceMgr::RemoveGroup(std::string name)
    {
        int index = 0;
        for (auto& group : p->data["groups"])
        {
            if (group["group"] == name)
            {
                if(group.contains("list"))
                {
                    for (auto& device : group["list"]) {
                        std::string mac = device["mac"];
                        if(mac == this->GetCurrentDevice()){
                            this->SetCurrentDevice("");
                        }
                    }
                }

                p->data["groups"].erase(index);
                this->Save();
                return;
            }

            index++;
        }
    }

    void DeviceMgr::EditGroupName(std::string name, std::string nameNew)
    {
        int index = 0;
        for (auto& group : p->data["groups"])
        {
            if (group["group"] == name)
            {
                group["group"] = nameNew;
                this->Save();
                return;
            }

            index++;
        }
    }
    void DeviceMgr::remove2FirstGroup(std::string name) {
        auto& groupList = p->data["groups"];
        if (name == "" || groupList.size() < 2) {
            return;
        }
        auto& f = groupList.front();
        auto& g = (f["group"] == name) ? groupList[1] : f;
        if (g["list"].is_null()) {
            g["list"] = nlohmann::json::array();
        }
        for (auto& group : groupList) {
            if (group["group"] == name) {
                if (group.contains("list")) {
                    for (auto& device : group["list"]) {
                        g["list"].insert(g["list"].begin(), device);
                    }
                }
                break;
            }
        }
        RemoveGroup(name);
    }

    void DeviceMgr::move2Group(std::string originGroup, std::string targetGroup, std::string address)
    {
        auto& groupList = p->data["groups"];
        if (address == "" || originGroup == "" || targetGroup == "") {
            return;
        }

        json* p = nullptr;
        for (auto& group : groupList) {
            if (group["group"] == targetGroup) {
                p = &group;
                break;
            }
        }
        if (p == nullptr) {
            return;
        }
        auto& g = *p;
        for (auto& group : groupList) {
            if (group["group"] == originGroup) {
                if (group.contains("list")) {
                    int index = -1;
                    for (auto& device : group["list"]) {
                        index++;
                        if (device["address"] == address) {
                            if (g["list"].is_null()) {
                                g["list"] = nlohmann::json::array();
                            }
                            g["list"].insert(g["list"].begin(), device);
                            group["list"].erase(index);
                            this->Save();
                            return;
                        }
                    }
                }
                break;
            }
        }
    }

    bool groupSort(const json& a, const json& b, std::vector<std::string> order)
    {
        auto itA = std::find(order.begin(), order.end(), a["group"].get<std::string>());
        auto itB = std::find(order.begin(), order.end(), b["group"].get<std::string>());

        return itA < itB;
    }

    void DeviceMgr::sortGroup(std::vector<std::string> order)
    {
        auto& groupList = p->data["groups"];
        std::sort(groupList.begin(), groupList.end(), [&order](const json& a, const json& b) { return groupSort(a, b, order); });
        this->Save();
    }

    void DeviceMgr::SetMergeState(bool state)
    {
        p->data["mergeState"] = state;
        this->Save();
    }

    bool DeviceMgr::IsMergeState()
    {
        bool mergeState = false;
        if (p->data.contains("mergeState")) {
            mergeState = p->data["mergeState"].get<bool>();
        }

        return mergeState;
    }

    nlohmann::json DeviceMgr::GetData()
    {
        return p->data;
    }

    std::optional<DeviceMgr::Data> DeviceMgr::FindByAddress(const std::string& address) const
    {
        if (address.empty() || !p->data.contains("groups") || !p->data["groups"].is_array()) {
            return std::nullopt;
        }

        for (const auto& group : p->data["groups"])
        {
            if (!group.is_object() || !group.contains("list") || !group["list"].is_array()) {
                continue;
            }

            for (const auto& item : group["list"])
            {
                if (!item.is_object()) {
                    continue;
                }

                const auto address_it = item.find("address");
                if (address_it == item.end() || !address_it->is_string() || address_it->get<std::string>() != address) {
                    continue;
                }

                return device_data_from_json(item);
            }
        }

        return std::nullopt;
    }

    std::optional<DeviceMgr::Data> DeviceMgr::FindByMac(const std::string& mac) const
    {
        if (mac.empty() || !p->data.contains("groups") || !p->data["groups"].is_array())
            return std::nullopt;

        for (const auto& group : p->data["groups"]) {
            if (!group.is_object() || !group.contains("list") || !group["list"].is_array())
                continue;
            for (const auto& item : group["list"]) {
                if (!item.is_object() || item.value("mac", std::string()) != mac)
                    continue;
                return device_data_from_json(item);
            }
        }

        return std::nullopt;
    }

    void DeviceMgr::Get(std::map<std::string, std::vector<DeviceMgr::Data>>& store, std::vector<std::string>& order)
    {
        for (auto it = p->data["groups"].begin(); it != p->data["groups"].end(); it++)
        {
            auto& group = it.value();
            std::string name = group["group"];
            store[name];
            if (group.contains("list"))
            {
                for (auto jt = group["list"].begin(); jt != group["list"].end(); jt++)
                {
                    Data data;
                    data.address = jt.value()["address"].get<std::string>();
                    data.addressRevision = address_revision_from_json(jt.value());
                    data.connectType = jt.value()["connectType"].get<int>();
                    data.mac = jt.value()["mac"].get<std::string>();
                    data.model = jt.value()["model"].get<std::string>();
                    data.name = jt.value()["name"].get<std::string>();

                    store[name].push_back(data);
                }
            }

            if (std::find(order.begin(), order.end(), name) == order.end())
                order.push_back(name);
        }
    }

    bool DeviceMgr::IsGroupExist(std::string name)
    {
        for (auto& group : p->data["groups"])
        {
            if (group["group"] == name)
                return true;
        }

        return false;
    }

    bool DeviceMgr::IsPrinterExist(std::string mac)
    {
        if (p->data.contains("groups"))
        {
            for (auto& group : p->data["groups"])
            {
                if (group.contains("list"))
                {
                    for (auto jt = group["list"].begin(); jt != group["list"].end(); jt++) {
                        if (mac == jt.value()["mac"].get<std::string>())
                            return true;
                    }
                }
            }
        }

        return false;
    }

    void DeviceMgr::SetCurrentDevice(std::string mac)
    {
        /*if (mac!=""&&!IsPrinterExist(mac)) { // now can set empty mac is current deivce 
            return;
        }*/
        auto& node = p->data["current_device"];

        json item;
        item["mac"] = mac;

        node = item;

        this->Save();
    }

    std::string DeviceMgr::GetCurrentDevice()
    {
        if(p->data.contains("current_device") && p->data["current_device"].contains("mac"))
            return p->data["current_device"]["mac"];

        return std::string();
    }

    DeviceMgr& DeviceMgr::Ins()
    {
        static DeviceMgr mgr;
        return mgr;
    }

}
