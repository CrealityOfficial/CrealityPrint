#include "SliceCommand.hpp"
#include "CLIError.hpp"
#include "libslic3r/Diagnostics/Report.hpp"
#include "libslic3r/Diagnostics/Session.hpp"

#include "libslic3r/BuildVolume.hpp"
#include "libslic3r/CustomGCode.hpp"
#include "libslic3r/FlushVolCalc.hpp"
#include "libslic3r/Format/3mf.hpp"
#include "libslic3r/Format/bbs_3mf.hpp"
#include "libslic3r/Geometry.hpp"
#include "libslic3r/libslic3r.h"
#include "libslic3r/LocalesUtils.hpp"
#include "libslic3r/Model.hpp"
#include "libslic3r/ModelArrange.hpp"
#include "libslic3r/ModelInstance.hpp"
#include "libslic3r/ModelVolume.hpp"
#include "libslic3r/Preset.hpp"
#include "libslic3r/PresetBundle.hpp"
#include "libslic3r/Print.hpp"
#include "libslic3r/PrintConfig.hpp"
#include "libslic3r/Semver.hpp"
#include "libslic3r/Thread.hpp"
#include "libslic3r/Time.hpp"
#include "libslic3r/Utils.hpp"

#include "slic3r/GUI/BitmapCache.hpp"
#include "slic3r/GUI/PartPlate.hpp"
#include "slic3r/GUI/Plater.hpp"

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/log/trivial.hpp>
#include <boost/nowide/fstream.hpp>
#include <boost/nowide/iostream.hpp>
#include <nlohmann/json.hpp>

#if defined(__linux__) || defined(__LINUX__)
    #include <condition_variable>
    #include <fcntl.h>
    #include <mutex>
    #include <unistd.h>

    #include <boost/thread.hpp>
#endif

using nlohmann::json;
using namespace Slic3r;




typedef struct  _sliced_plate_info{
    int plate_id{0};
    size_t sliced_time {0};
    size_t sliced_time_with_cache {0};
    size_t triangle_count{0};
    std::string warning_message;
}sliced_plate_info_t;

typedef struct _sliced_info {
    int                 plate_count {0};
    int                 plate_to_slice {0};

    std::vector<sliced_plate_info_t> sliced_plates;
    size_t prepare_time {0};
    size_t export_time {0};
    std::vector<std::string> upward_machines;
}sliced_info_t;
std::vector<PrintBase::SlicingStatus> g_slicing_warnings;
namespace Slic3r {
enum class DiagnosticMode
{
    None,
    Fingerprint,
    Performance
};

struct SliceExecutionContext
{
    Slic3r::GUI::PartPlateList &partplate_list;
    int plate_to_slice;
    std::map<int, bool> &skip_maps;
    int max_triangle_count_per_plate;
    int max_slicing_time_per_plate;
    bool no_check;
    int filament_count;
    bool load_slicedata;
    const std::string &load_slice_data_dir;
    bool export_slicedata;
    const std::string &export_slice_data_dir;
    const std::string &outfile_dir;
    DiagnosticMode diagnostic_mode;
    bool need_gcode_file;
    Diagnostics::Report &fingerprint_report;
    PlateDataPtrs &plate_data_src;
    sliced_info_t &sliced_info;
    const std::string &new_printer_name;
    const std::string &current_printer_system_name;
};
struct InputLoadContext
{
    PrinterTechnology &printer_technology;
    int &plate_to_slice;
    bool normative_check;
    bool allow_newer_file;
    const std::vector<int> &loaded_filament_ids;
    const std::vector<std::string> &load_filaments;
    const std::vector<std::string> &load_configs;
    ForwardCompatibilitySubstitutionRule config_substitution_rule;
    const std::string &outfile_dir;
    sliced_info_t &sliced_info;
    PlateDataPtrs &plate_data_src;
    bool &is_project_input;
    bool &is_creality_project_3mf;
    std::string &creality_project_file;
    Semver &creality_project_file_version;
    std::vector<Preset*> &project_presets;
    std::string &current_printer_system_name;
    int &filament_count;
    std::set<int> &used_filament_set;
};
struct ModelPresetContext
{
    PrinterTechnology &printer_technology;
    const std::vector<std::string> &load_configs;
    const std::vector<std::string> &load_filaments;
    bool use_first_filament_as_default;
    ForwardCompatibilitySubstitutionRule config_substitution_rule;
    const std::string &outfile_dir;
    sliced_info_t &sliced_info;
    int &filament_count;
    std::string &new_printer_name;
};
} // namespace Slic3r

#if defined(__linux__) || defined(__LINUX__)
#define PIPE_BUFFER_SIZE 512

typedef struct _cli_callback_mgr {
    int                 m_plate_count {0};
    int                 m_plate_index {0};
    int                 m_progress { 0 };
    int                 m_total_progress { 0 };
    std::string         m_message;
    int                 m_warning_step;
    bool                m_exit {false};
    bool                m_data_ready {false};
    bool                m_started {false};
    boost::thread               m_thread;
    // Mutex and condition variable to synchronize m_thread with the UI thread.
    std::mutex                  m_mutex;
    std::condition_variable     m_condition;
    int                 m_pipe_fd{-1};

    bool    is_started()
    {
        bool result;
        std::unique_lock<std::mutex> lck(m_mutex);
        result = m_started;
        lck.unlock();

        return result;
    }

    void set_plate_info(int index, int count)
    {
        BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << ": index="<<index<< ", count = "<< count;
        std::unique_lock<std::mutex> lck(m_mutex);
        m_plate_count = count;
        m_plate_index = index;
        m_progress = 0;
        lck.unlock();

        return;
    }

    void    notify()
    {
        if (m_pipe_fd < 0)
            return;

        json j;
        //record the headers
        j["plate_index"] = m_plate_index;
        j["plate_count"] = m_plate_count;
        j["plate_percent"] = m_progress;
        j["total_percent"] = m_total_progress;
        if (m_warning_step >= 0)
            j["warning"] = m_message;
        else
            j["message"] = m_message;

        std::string notify_message = j.dump();
        //notify_message = "Plate "+ std::to_string(m_plate_index) + "/" +std::to_string(m_plate_count)+  ": Percent " + std::to_string(m_progress) + ": "+m_message;

        char pipe_message[PIPE_BUFFER_SIZE] = {0};
        snprintf(pipe_message, PIPE_BUFFER_SIZE, "%s\n", notify_message.c_str());

        int ret = write(m_pipe_fd, pipe_message, strlen(pipe_message));
        BOOST_LOG_TRIVIAL(debug) << __FUNCTION__ << ": write returns "<<ret;

        return;
    }

    void    thread_proc()
    {
        std::unique_lock<std::mutex> lck(m_mutex);
        m_started = true;
        m_data_ready = false;
        lck.unlock();
        m_condition.notify_one();
        boost::this_thread::sleep(boost::posix_time::milliseconds(20));
        BOOST_LOG_TRIVIAL(info) << "cli_callback_mgr_t::thread_proc started.";
        while(1) {
            lck.lock();
            m_condition.wait(lck, [this](){ return m_data_ready || m_exit; });
            BOOST_LOG_TRIVIAL(debug) << __FUNCTION__ << ": wakup.";
            if (m_data_ready) {
                notify();
                m_data_ready = false;
            }
            if (m_exit) {
                BOOST_LOG_TRIVIAL(info) << "cli_callback_mgr_t::thread_proc will exit.";
                break;
            }
            lck.unlock();
            m_condition.notify_one();
        }
        lck.unlock();
        BOOST_LOG_TRIVIAL(info) << "cli_callback_mgr_t::thread_proc exit.";
    }

    void    update(int percent, std::string message, int warning_step)
    {
        std::unique_lock<std::mutex> lck(m_mutex);
        if (!m_started) {
            lck.unlock();
            return;
        }

        if ((m_progress >= percent)&&(warning_step == -1)) {
            //already update before
            lck.unlock();
            return;
        }
        int old_total_progress = m_total_progress;
        BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << ": percent="<<percent<< ", warning_step=" << warning_step << ", plate_index = "<< m_plate_index<<", plate_count="<< m_plate_count<<", message="<<message;
        if (warning_step == -1) {
            m_progress = percent;
            if ((m_plate_count <= 1) && (m_plate_index >= 1))
                m_total_progress = 3 + 0.9*m_progress;
            else if ((m_plate_count > 1) && (m_plate_index >= 1)) {
                m_total_progress = 3 + ((float)(m_plate_index - 1)*90)/m_plate_count + ((float)m_progress*0.9)/m_plate_count;
            }
            else
                m_total_progress = m_progress;
        }
        if (m_total_progress < old_total_progress)
            m_total_progress = old_total_progress;
        m_message = message;
        m_warning_step = warning_step;
        m_data_ready = true;
        lck.unlock();
        m_condition.notify_one();
        BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << ": m_total_progress="<<m_total_progress;
        return;
    }

    bool start(std::string pipe_name)
    {
        int retry_count = 0;
        BOOST_LOG_TRIVIAL(info) << "cli_callback_mgr_t::start enter.";
        m_pipe_fd = open(pipe_name.c_str(),O_WRONLY|O_NONBLOCK);
        while (m_pipe_fd < 0) {
            if ((retry_count%10) == 0)
                BOOST_LOG_TRIVIAL(warning) << boost::format("could not open pipe for %1%, errno %2%, reason: %3%, retry_count = %4%")%pipe_name %errno %strerror(errno) %retry_count;
            retry_count ++;
            if (retry_count >= 50) {
                BOOST_LOG_TRIVIAL(warning) << boost::format("reach max retry_count, failed to open pipe");
                return false;
            }
            boost::this_thread::sleep(boost::posix_time::milliseconds(20));
            m_pipe_fd = open(pipe_name.c_str(),O_WRONLY|O_NONBLOCK);
        }
        std::unique_lock<std::mutex> lck(m_mutex);
        m_thread = create_thread([this]{
                this->thread_proc();
        });
        m_condition.wait(lck, [this](){ return m_started; });
        lck.unlock();
        m_condition.notify_one();
        BOOST_LOG_TRIVIAL(info) << "cli_callback_mgr_t::start successfully.";
        return true;
    }

    void stop()
    {
        BOOST_LOG_TRIVIAL(info) << "cli_callback_mgr_t::stop enter.";
        std::unique_lock<std::mutex> lck(m_mutex);
        if (!m_started) {
            lck.unlock();
	    BOOST_LOG_TRIVIAL(info) << "cli_callback_mgr_t::stop not started before, return directly.";
            return;
        }
        m_exit = true;
        lck.unlock();
        m_condition.notify_one();
        // Wait until the worker thread exits.
        m_thread.join();
        if (m_pipe_fd > 0) {
            close(m_pipe_fd);
            m_pipe_fd = -1;
        }
        BOOST_LOG_TRIVIAL(info) << "cli_callback_mgr_t::stop successfully.";
    }
}cli_callback_mgr_t;

cli_callback_mgr_t g_cli_callback_mgr;
void cli_status_callback(const PrintBase::SlicingStatus& slicing_status)
{
    if (slicing_status.warning_step != -1) {
        g_slicing_warnings.push_back(slicing_status);
        BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << boost::format(": percent=%1%, warning_step=%2%, message=%3%, message_type=%4%, flag=%5%")
            %slicing_status.percent %slicing_status.warning_step %slicing_status.text %(int)(slicing_status.message_type) %slicing_status.flags;
    }
    g_cli_callback_mgr.update(slicing_status.percent, slicing_status.text, slicing_status.warning_step);
    return;
}
#endif

void default_status_callback(const PrintBase::SlicingStatus& slicing_status)
{
    if (slicing_status.warning_step != -1) {
        g_slicing_warnings.push_back(slicing_status);
    }
    BOOST_LOG_TRIVIAL(debug) << __FUNCTION__ << boost::format(": percent=%1%, warning_step=%2%, message=%3%, message_type=%4%")%slicing_status.percent %slicing_status.warning_step %slicing_status.text %(int)(slicing_status.message_type);

    return;
}


static PrinterTechnology get_printer_technology(const DynamicConfig &config)
{
    const ConfigOptionEnum<PrinterTechnology> *opt = config.option<ConfigOptionEnum<PrinterTechnology>>("printer_technology");
    return (opt == nullptr) ? ptUnknown : opt->value;
}

static bool is_sequential_print(Slic3r::GUI::PartPlate *plate, DynamicPrintConfig &print_config)
{
    const PrintSequence plate_sequence = plate->get_print_seq();
    if (plate_sequence == PrintSequence::ByObject) {
        BOOST_LOG_TRIVIAL(info) << "plate print by object, set from plate self";
        return true;
    }

    if (plate_sequence == PrintSequence::ByDefault) {
        const auto *global_sequence = print_config.option<ConfigOptionEnum<PrintSequence>>("print_sequence");
        if (global_sequence && global_sequence->value == PrintSequence::ByObject) {
            BOOST_LOG_TRIVIAL(info) << "plate print by object, set from global";
            return true;
        }
    }

    return false;
}

static bool has_cli_action(const std::vector<std::string> &actions, const char *name)
{
    return std::find(actions.begin(), actions.end(), name) != actions.end();
}

namespace Slic3r {

struct SliceCommandOptions
{
    std::string outfile_dir;
    DiagnosticMode diagnostic_mode {DiagnosticMode::None};
    bool need_gcode_file {false};
    std::vector<std::string> load_configs;
    std::vector<std::string> load_filaments;
    std::map<int, bool> skip_maps;
    std::vector<int> loaded_filament_ids;
    std::string custom_gcode_file;
    std::string pipe_name;
    std::string load_slice_data_dir;
    std::string export_slice_data_dir;
    int plate_to_slice {0};
    int max_triangle_count_per_plate {0};
    int max_slicing_time_per_plate {0};
    bool normative_check {true};
    bool use_first_filament_as_default {false};
    bool allow_newer_file {false};
    bool avoid_extrusion_cali_region {false};
    bool allow_multicolor_oneplate {false};
    bool load_slicedata {false};
    bool export_slicedata {false};
    bool no_check {false};

};

struct SliceInput
{
    PrinterTechnology printer_technology {ptUnknown};
    bool is_project_input {false};
    bool is_creality_project_3mf {false};
    bool filament_color_changed {false};

    int filament_count {0};
    int plate_to_slice {0};
    long long started_at {(long long) Slic3r::Utils::get_current_time_utc()};
    sliced_info_t sliced_info;
    PlateDataPtrs plate_data_src;
    std::vector<Preset*> project_presets;
    std::set<int> used_filament_set;
    std::map<int, bool> skip_maps;
    std::string new_printer_name;
    std::string current_printer_system_name;
    std::string creality_project_file;
    Semver creality_project_file_version;
    std::map<int, CustomGCode::Info> custom_gcodes;
    Diagnostics::Report fingerprint_report;
    std::unique_ptr<Slic3r::GUI::PartPlateList> partplate_list;

    ~SliceInput()
    {
        for (Preset *preset : project_presets)
            delete preset;
    }

    bool is_project() const { return is_project_input; }
};

} // namespace Slic3r

static int read_slice_cli_options(
    DynamicPrintAndCommandLineConfig &config,
    const std::vector<std::string> &actions,
    size_t input_file_count,
    SliceCommandOptions &options,
    std::string &error_message)
{
    options.outfile_dir = config.opt_string("outputdir", true);
    options.need_gcode_file = config.opt_bool("need_gcode_file");
    const std::string diagnostic_mode = config.opt_string("diagnostics", true);
    if (diagnostic_mode.empty())
        options.diagnostic_mode = DiagnosticMode::None;
    else if (diagnostic_mode == "fingerprint")
        options.diagnostic_mode = DiagnosticMode::Fingerprint;
    else if (diagnostic_mode == "performance")
        options.diagnostic_mode = DiagnosticMode::Performance;
    else {
        error_message = "--diagnostics must be fingerprint or performance";
        return CLI_INVALID_PARAMS;
    }

#if !defined(SLIC3R_DIAGNOSTICS_FINGERPRINT_ENABLED)
    if (options.diagnostic_mode == DiagnosticMode::Fingerprint) {
        error_message = "fingerprint diagnostics are available only in RelWithDebInfo builds";
        return CLI_INVALID_PARAMS;
    }
#endif

#if !defined(SLIC3R_DIAGNOSTICS_PERFORMANCE_ENABLED)
    if (options.diagnostic_mode == DiagnosticMode::Performance) {
        error_message = "performance diagnostics are available only in Tracy-enabled RelWithDebInfo builds";
        return CLI_INVALID_PARAMS;
    }
#endif
    options.load_configs = config.option<ConfigOptionStrings>("load_settings", true)->values;
    options.load_filaments = config.option<ConfigOptionStrings>("load_filaments", true)->values;
    options.loaded_filament_ids = config.option<ConfigOptionInts>("load_filament_ids", true)->values;
    options.allow_multicolor_oneplate =
        config.option<ConfigOptionBool>("allow_multicolor_oneplate", true)->value;

    if (const auto *option = config.option<ConfigOptionInt>("slice"))
        options.plate_to_slice = option->value;
    if (const auto *option = config.option<ConfigOptionBool>("normative_check"))
        options.normative_check = option->value;
    if (const auto *option = config.option<ConfigOptionBool>("load_defaultfila"))
        options.use_first_filament_as_default = option->value;
    if (const auto *option = config.option<ConfigOptionBool>("allow_newer_file"))
        options.allow_newer_file = option->value;
    if (const auto *option = config.option<ConfigOptionBool>("avoid_extrusion_cali_region"))
        options.avoid_extrusion_cali_region = option->value;
    if (const auto *option = config.option<ConfigOptionString>("load_custom_gcodes"))
        options.custom_gcode_file = option->value;
    if (const auto *option = config.option<ConfigOptionString>("pipe"))
        options.pipe_name = option->value;

    for (int object_id : config.option<ConfigOptionInts>("skip_objects", true)->values)
        options.skip_maps[object_id] = false;

    options.load_slicedata = has_cli_action(actions, "load_slicedata");
    options.export_slicedata = has_cli_action(actions, "export_slicedata");
    options.no_check = has_cli_action(actions, "no_check") && config.opt_bool("no_check");
    if (options.load_slicedata)
        options.load_slice_data_dir = config.opt_string("load_slicedata");
    if (options.export_slicedata)
        options.export_slice_data_dir = config.opt_string("export_slicedata");
    if (has_cli_action(actions, "mtcpp"))
        options.max_triangle_count_per_plate = config.option<ConfigOptionInt>("mtcpp")->value;
    if (has_cli_action(actions, "mstpp"))
        options.max_slicing_time_per_plate = config.option<ConfigOptionInt>("mstpp")->value;

    if (options.load_slicedata && options.export_slicedata) {
        error_message = "--load_slicedata and --export_slicedata cannot be used together";
        return CLI_INVALID_PARAMS;
    }
    if (!options.loaded_filament_ids.empty()) {
        if (options.loaded_filament_ids.size() != input_file_count) {
            error_message = "--load_filament_ids must contain one entry for every input model";
            return CLI_INVALID_PARAMS;
        }
        if (options.load_filaments.empty()) {
            error_message = "--load_filament_ids requires --load_filaments";
            return CLI_INVALID_PARAMS;
        }
    }

    if ((options.diagnostic_mode != DiagnosticMode::None || options.need_gcode_file) && options.outfile_dir.empty()) {
        error_message = "--diagnostics and --need-gcode-file require --outputdir";
        return CLI_INVALID_PARAMS;
    }
    if (options.diagnostic_mode != DiagnosticMode::None || options.need_gcode_file) {
        const boost::filesystem::path output_path(options.outfile_dir);
        boost::system::error_code directory_error;
        boost::filesystem::create_directories(output_path, directory_error);
        const bool output_is_directory =
            !directory_error && boost::filesystem::is_directory(output_path, directory_error);
        if (directory_error || !output_is_directory) {
            error_message = "cannot create or access --outputdir: " + options.outfile_dir;
            return CLI_INVALID_PARAMS;
        }
    }

    return CLI_SUCCESS;
}
void record_exit_reson(std::string outputdir, int code, int plate_id, std::string error_message, sliced_info_t& sliced_info, std::map<std::string, std::string> key_values = std::map<std::string, std::string>())
{
#if defined(__linux__) || defined(__LINUX__)
    std::string result_file;

    if (!outputdir.empty())
        result_file = outputdir + "/result.json";
    else
        result_file = "result.json";

    try {
        json j;
        //record the headers

        if (sliced_info.upward_machines.size() > 0)
            j["upward_compatible_machine"] = sliced_info.upward_machines;
        j["plate_index"] = plate_id;
        j["return_code"] = code;
        j["error_string"] = error_message;
        j["prepare_time"] = sliced_info.prepare_time;
        j["export_time"] = sliced_info.export_time;
        for (size_t index = 0; index < sliced_info.sliced_plates.size(); index++)
        {
            json plate_json;
            plate_json["id"] = sliced_info.sliced_plates[index].plate_id;
            plate_json["sliced_time"] = sliced_info.sliced_plates[index].sliced_time;
            plate_json["sliced_time_with_cache"] = sliced_info.sliced_plates[index].sliced_time_with_cache;
            plate_json["triangle_count"] = sliced_info.sliced_plates[index].triangle_count;
            plate_json["warning_message"] = sliced_info.sliced_plates[index].warning_message;
            j["sliced_plates"].push_back(plate_json);
        }
        for (auto& iter: key_values)
            j[iter.first] = iter.second;

        boost::nowide::ofstream c;
        c.open(result_file, std::ios::out | std::ios::trunc);
        c << std::setw(4) << j << std::endl;
        c.close();

        BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << ":" <<__LINE__ << boost::format(", saved config to %1%\n")%result_file;
    }
    catch (...) {}
#endif
}

static int load_key_values_from_json(const std::string &file, std::map<std::string, std::string>& key_values)
{
    json j;
    CNumericLocalesSetter locales_setter;

    BOOST_LOG_TRIVIAL(debug) << __FUNCTION__<< ": begin to parse "<<file;
    try {
        boost::nowide::ifstream ifs(file);
        ifs >> j;
        ifs.close();

        //parse the json elements
        for (auto it = j.begin(); it != j.end(); it++) {
            if (boost::iequals(it.key(),BBL_JSON_KEY_MODEL_ID)) {
                key_values.emplace(BBL_JSON_KEY_MODEL_ID, it.value());
            }
            else if (boost::iequals(it.key(), BBL_JSON_KEY_NAME)) {
                key_values.emplace(BBL_JSON_KEY_NAME, it.value());
            }
        }
    }
    catch (const std::ifstream::failure &err)  {
        BOOST_LOG_TRIVIAL(error) << __FUNCTION__<< ": parse "<<file<<" got a ifstream error, reason = " << err.what();
        return -1;
    }
    catch(nlohmann::detail::parse_error &err) {
        BOOST_LOG_TRIVIAL(error) << __FUNCTION__<< ": parse "<<file<<" got a nlohmann::detail::parse_error, reason = " << err.what();
        return -2;
    }
    catch(std::exception &err) {
        BOOST_LOG_TRIVIAL(error) << __FUNCTION__<< ": parse "<<file<<" got a generic exception, reason = " << err.what();
        return -3;
    }
    BOOST_LOG_TRIVIAL(debug) << __FUNCTION__<< ": finished parse, key_values size "<<key_values.size();
    return 0;
}

static std::set<std::string> gcodes_key_set =  {"filament_end_gcode", "filament_start_gcode", "change_filament_gcode", "layer_change_gcode", "machine_end_gcode", "machine_pause_gcode", "machine_start_gcode",
            "template_custom_gcode", "printing_by_object_gcode", "before_layer_change_gcode", "time_lapse_gcode"};

static void load_default_gcodes_to_config(DynamicPrintConfig& config, Preset::Type type)
{
    if (config.size() == 0) {
        BOOST_LOG_TRIVIAL(info) << __FUNCTION__<< ", empty config, return directly";
        return;
    }
    //add those empty gcodes by default
    if (type == Preset::TYPE_PRINTER)
    {
        std::string change_filament_gcode = config.option<ConfigOptionString>("change_filament_gcode", true)->value;
        BOOST_LOG_TRIVIAL(trace) << __FUNCTION__<< ", change_filament_gcode: "<< change_filament_gcode;

        ConfigOptionString* layer_change_gcode_opt = config.option<ConfigOptionString>("layer_change_gcode", true);
        BOOST_LOG_TRIVIAL(trace) << __FUNCTION__<< ", layer_change_gcode: "<<layer_change_gcode_opt->value;

        ConfigOptionString* machine_end_gcode_opt = config.option<ConfigOptionString>("machine_end_gcode", true);
        BOOST_LOG_TRIVIAL(trace) << __FUNCTION__<< ", machine_end_gcode: "<<machine_end_gcode_opt->value;

        ConfigOptionString* machine_pause_gcode_opt = config.option<ConfigOptionString>("machine_pause_gcode", true);
        BOOST_LOG_TRIVIAL(trace) << __FUNCTION__<< ", machine_pause_gcode: "<<machine_pause_gcode_opt->value;

        ConfigOptionString* machine_start_gcode_opt = config.option<ConfigOptionString>("machine_start_gcode", true);
        BOOST_LOG_TRIVIAL(trace) << __FUNCTION__<< ", machine_start_gcode: "<<machine_start_gcode_opt->value;

        ConfigOptionString* template_custom_gcode_opt = config.option<ConfigOptionString>("template_custom_gcode", true);
        BOOST_LOG_TRIVIAL(trace) << __FUNCTION__<< ", template_custom_gcode: "<<template_custom_gcode_opt->value;

        ConfigOptionString* printing_by_object_gcode_opt = config.option<ConfigOptionString>("printing_by_object_gcode", true);
        BOOST_LOG_TRIVIAL(trace) << __FUNCTION__<< ", printing_by_object_gcode: "<<printing_by_object_gcode_opt->value;

        ConfigOptionString* before_layer_change_gcode_opt = config.option<ConfigOptionString>("before_layer_change_gcode", true);
        BOOST_LOG_TRIVIAL(trace) << __FUNCTION__<< ", before_layer_change_gcode: "<<before_layer_change_gcode_opt->value;

        ConfigOptionString* timeplase_gcode_opt = config.option<ConfigOptionString>("time_lapse_gcode", true);
        BOOST_LOG_TRIVIAL(trace) << __FUNCTION__<< ", time_lapse_gcode: "<<timeplase_gcode_opt->value;
    }
    else if (type == Preset::TYPE_FILAMENT)
    {
        std::vector<std::string>& filament_start_gcodes = config.option<ConfigOptionStrings>("filament_start_gcode", true)->values;
        if (filament_start_gcodes.empty()) {
            filament_start_gcodes.resize(1, std::string());
            BOOST_LOG_TRIVIAL(info) << __FUNCTION__<< ", set filament_start_gcodes to empty";
        }
        else {
            BOOST_LOG_TRIVIAL(trace) << __FUNCTION__<< ", filament_start_gcodes: "<<filament_start_gcodes[0];
        }

        std::vector<std::string>& filament_end_gcodes = config.option<ConfigOptionStrings>("filament_end_gcode", true)->values;
        if (filament_end_gcodes.empty()) {
            filament_end_gcodes.resize(1, std::string());
            BOOST_LOG_TRIVIAL(info) << __FUNCTION__<< ", set filament_end_gcode to empty";
        }
        else {
            BOOST_LOG_TRIVIAL(trace) << __FUNCTION__<< ", filament_end_gcode: "<<filament_end_gcodes[0];
        }
    }
}

static int load_custom_gcode_file(const std::string &file, int plate_to_slice, std::map<int, CustomGCode::Info> &custom_gcodes, std::string &error_message)
{
    if (file.empty())
        return CLI_SUCCESS;

    if (!boost::filesystem::exists(file))
    {
        error_message = "Custom G-code file not found: " + file;
        return CLI_FILE_NOTFOUND;
    }

    try
    {
        nlohmann::json document;
        boost::nowide::ifstream stream(file);
        stream >> document;

        CustomGCode::Info info;
        info.from_json(document);
        const int plate_index = plate_to_slice == 0 ? 0 : plate_to_slice - 1;
        custom_gcodes.emplace(plate_index, info);
    }
    catch (const std::exception &error)
    {
        error_message = "Loading custom G-code file '" + file + "' failed: " + error.what();
        return CLI_CONFIG_FILE_ERROR;
    }
    return CLI_SUCCESS;
}

static int load_cli_preset_file(const std::string &file, ForwardCompatibilitySubstitutionRule substitution_rule, DynamicPrintConfig &config,
    std::string &config_type,
    std::string &config_name,
    std::string &filament_id,
    std::string &config_from)
{
    if (!boost::filesystem::exists(file))
    {
        boost::nowide::cerr << "Setting file not found: " << file << std::endl;
        return CLI_FILE_NOTFOUND;
    }

    try
    {
        std::map<std::string, std::string> key_values;
        std::string reason;

        const ConfigSubstitutions substitutions = config.load_from_json(file, substitution_rule, key_values, reason);

        if (!reason.empty())
        {
            BOOST_LOG_TRIVIAL(error) << "Can not load config from " << file << ": " << reason;
            return CLI_CONFIG_FILE_ERROR;
        }

        config_name = key_values[BBL_JSON_KEY_NAME];
        if (const auto iter = key_values.find(BBL_JSON_KEY_FROM); iter != key_values.end())
            config_from = iter->second;

        if (config_from != "system" && config_from != "User" && config_from != "user")
        {
            BOOST_LOG_TRIVIAL(error) << "Unsupported preset source '" << config_from << "' in " << file;
            return CLI_CONFIG_FILE_ERROR;
        }

        if (const auto iter = key_values.find(BBL_JSON_KEY_TYPE); iter != key_values.end())
            config_type = iter->second;

        if (config_type == "filament")
        {
            if (const auto iter = key_values.find(BBL_JSON_KEY_FILAMENT_ID); iter != key_values.end())
                filament_id = iter->second;
        }
        else if (config_type != "machine" && config_type != "process")
        {
            BOOST_LOG_TRIVIAL(error) << "Unknown preset type '" << config_type << "' in " << file;
            return CLI_CONFIG_FILE_ERROR;
        }

        config.normalize_fdm();
        for (const ConfigSubstitution &substitution : substitutions)
        {
            BOOST_LOG_TRIVIAL(info) << "Substituted legacy value " << substitution.opt_def->opt_key
                << " while loading " << file;
        }
    }
    catch (const std::exception &error)
    {
        boost::nowide::cerr << "Loading setting file '" << file << "' failed: " << error.what() << std::endl;
        return CLI_CONFIG_FILE_ERROR;
    }
    return 0;
}

static int apply_external_preset(DynamicPrintConfig &destination, const DynamicPrintConfig &source)
{
    for (const t_config_option_key &key : source.keys())
    {
        const ConfigOption *source_option = source.option(key);
        if (!source_option)
            return CLI_CONFIG_FILE_ERROR;

        if (key == "compatible_prints" || key == "compatible_printers" || key == "model_id" ||
            key == "inherits" || key == "dev_model_name" || key == "name" || key == "from" ||
            key == "type" || key == "version" || key == "setting_id" || key == "instantiation")
            continue;

        ConfigOption *destination_option = destination.option(key, true);
        if (!destination_option)
            return CLI_CONFIG_FILE_ERROR;
        destination_option->set(source_option);
    }
    return 0;
}

static void apply_cli_safe_machine_limits(DynamicPrintConfig &config, const std::string &printer_model)
{
    const std::string cli_config_file = resources_dir() + "/profiles/BBL/cli_config.json";
    if (!boost::filesystem::exists(cli_config_file))
    {
        BOOST_LOG_TRIVIAL(warning) << "CLI safety configuration not found: " << cli_config_file;
        return;
    }

    try
    {
        json root;
        boost::nowide::ifstream stream(cli_config_file);
        stream >> root;
        if (printer_model.empty() || !root.contains("printer") || !root["printer"].contains(printer_model))
            return;

        const json &printer = root["printer"][printer_model];
        if (!printer.contains("machine_limits"))
            return;

        const auto limits = printer["machine_limits"].get<std::map<std::string, std::string>>();
        for (const auto &entry : limits)
        {
            std::string machine_key = entry.first;
            machine_key.replace(0, 8, "machine_max");
            ConfigOptionFloats *values = config.option<ConfigOptionFloats>(machine_key);
            if (!values)
                continue;

            ConfigOptionFloats safe_values;
            safe_values.deserialize(entry.second);
            for (size_t index = 0; index < values->size() && index < safe_values.size(); ++index)
            {
                const double safe_value = safe_values.values[index];
                if (safe_value != 0. && safe_value < values->values[index])
                    values->values[index] = safe_value;
            }
        }
    }
    catch (const std::exception &error)
    {
        BOOST_LOG_TRIVIAL(error) << "Failed to parse " << cli_config_file << ": " << error.what();
    }
}

SliceCommand::SliceCommand(
    DynamicPrintAndCommandLineConfig &config,
    DynamicPrintConfig &print_config,
    DynamicPrintConfig &extra_config,
    const std::vector<std::string> &input_files,
    const std::vector<std::string> &actions,
    std::vector<Model> &models)
    : m_config(config)
    , m_print_config(print_config)
    , m_extra_config(extra_config)
    , m_input_files(input_files)
    , m_actions(actions)
    , m_models(models)
{
}

int SliceCommand::execute_plate_slicing(SliceExecutionContext &context)
{
    auto &partplate_list = context.partplate_list;
    const int plate_to_slice = context.plate_to_slice;
    auto &skip_maps = context.skip_maps;
    const int max_triangle_count_per_plate = context.max_triangle_count_per_plate;
    const int max_slicing_time_per_plate = context.max_slicing_time_per_plate;
    const bool no_check = context.no_check;
    const int filament_count = context.filament_count;
    const bool load_slicedata = context.load_slicedata;
    const std::string &load_slice_data_dir = context.load_slice_data_dir;
    const bool export_slicedata = context.export_slicedata;
    const std::string &export_slice_data_dir = context.export_slice_data_dir;
    const std::string &outfile_dir = context.outfile_dir;
    const DiagnosticMode diagnostic_mode = context.diagnostic_mode;
    const bool fingerprint_enabled = diagnostic_mode == DiagnosticMode::Fingerprint;
    const bool need_gcode_file = context.need_gcode_file;
    Diagnostics::Report &fingerprint_report = context.fingerprint_report;
#if defined(SLIC3R_DIAGNOSTICS_FINGERPRINT_ENABLED) || defined(SLIC3R_DIAGNOSTICS_PERFORMANCE_ENABLED)
    Diagnostics::Session::Config session_config;
    session_config.fingerprint_report = fingerprint_enabled ? &fingerprint_report : nullptr;
    session_config.performance_enabled = diagnostic_mode == DiagnosticMode::Performance;
    Diagnostics::Session diagnostic_session(session_config);
#endif
    PlateDataPtrs &plate_data_src = context.plate_data_src;
    sliced_info_t &sliced_info = context.sliced_info;
    const std::string &new_printer_name = context.new_printer_name;
    const std::string &current_printer_system_name = context.current_printer_system_name;
    bool pre_check = plate_to_slice == 0;
    bool finished = false;

    BOOST_LOG_TRIVIAL(info) << "Need to slice for plate "<<plate_to_slice <<", total plate count "<<partplate_list.get_plate_count()<<" partplates!" << std::endl;
#if defined(__linux__) || defined(__LINUX__)
    if (g_cli_callback_mgr.is_started()) {
        PrintBase::SlicingStatus slicing_status{3, "Prepare slicing"};
        cli_status_callback(slicing_status);
    }
#endif
    for (Model &model : m_models) 
    {
        std::string outfile;
        std::vector<size_t> plate_triangle_counts(partplate_list.get_plate_count(), 0);

        while(!finished)
        {
            //BBS: slice every partplate one by one
            PrintBase  *print=NULL;
            Print *print_fff = NULL;

            Slic3r::GUI::GCodeResult *gcode_result = NULL;
            int print_index;
            for (int index = 0; index < partplate_list.get_plate_count(); index ++)
            {
                if ((plate_to_slice != 0) && (plate_to_slice != (index + 1)))
                {
                    BOOST_LOG_TRIVIAL(info) << "Skip plate " << index+1 << std::endl;
                    continue;
                }
                sliced_plate_info_t sliced_plate_info;
                sliced_plate_info.plate_id = index+1;

                model.curr_plate_index = index;
                BOOST_LOG_TRIVIAL(info) << boost::format("Plate %1%: pre_check %2%, start")%(index+1)%pre_check;
                long long start_time = 0, end_time = 0, temp_time = 0, time_using_cache = 0;
                start_time = (long long)Slic3r::Utils::get_current_time_utc();
                //get the current partplate
                Slic3r::GUI::PartPlate* part_plate = partplate_list.get_plate(index);
                part_plate->get_print(&print, &gcode_result, &print_index);

                print_fff = dynamic_cast<Print *>(print);

                // Refresh instance states for the current plate before applying the model to Print.
                // This is required for correct multi-plate slicing, object skipping and metrics.
                const double print_height = m_print_config.opt_float("printable_height");
                BuildVolume build_volume(part_plate->get_shape(), print_height);
                const unsigned int printable_count = model.update_print_volume_state(build_volume);
                if (printable_count == 0) {
                    BOOST_LOG_TRIVIAL(error)
                        << "plate " << index + 1
                        << ": no object is fully inside the print volume before apply";
                    record_exit_reson(
                        outfile_dir,
                        CLI_NO_SUITABLE_OBJECTS,
                        index + 1,
                        CLI::error_message(CLI_NO_SUITABLE_OBJECTS),
                        sliced_info);
                    return CLI_NO_SUITABLE_OBJECTS;
                }

                if (plate_to_slice != 0 || pre_check) 
                {
                    size_t triangle_count = 0;
                    int printable_instances = 0;
                    int skipped_count = 0;

                    for (ModelObject *model_object : model.objects) 
                    {
                        for (ModelInstance *instance : model_object->instances) 
                        {
                            instance->use_loaded_id_for_label = true;
                            if (skip_maps.find(instance->loaded_id) != skip_maps.end()) 
                            {
                                skip_maps[instance->loaded_id] = true;
                                instance->printable = false;
                                if (instance->print_volume_state == ModelInstancePVS_Inside) 
                                {
                                    ++skipped_count;
                                    BOOST_LOG_TRIVIAL(info)
                                        << "Plate " << index + 1
                                        << ": skip object " << instance->loaded_id;
                                    if (plate_data_src.size() > size_t(index)) {
                                        PlateData *plate_data = plate_data_src[index];
                                        plate_data->thumbnail_file.clear();
                                        plate_data->no_light_thumbnail_file.clear();
                                        plate_data->top_file.clear();
                                        plate_data->pick_file.clear();
                                    }
                                }
                                continue;
                            }

                            if (instance->print_volume_state == ModelInstancePVS_Partly_Outside) 
                            {
                                BOOST_LOG_TRIVIAL(error)
                                    << "plate " << index + 1 << ": object "
                                    << model_object->name << " is partly inside the print volume";
                                record_exit_reson(
                                    outfile_dir,
                                    CLI_OBJECTS_PARTLY_INSIDE,
                                    index + 1,
                                    CLI::error_message(CLI_OBJECTS_PARTLY_INSIDE),
                                    sliced_info);
                                return CLI_OBJECTS_PARTLY_INSIDE;
                            }

                            if (instance->print_volume_state != ModelInstancePVS_Inside)
                                continue;

                            ++printable_instances;
                            for (const ModelVolume *volume : model_object->volumes) 
                            {
                                if (!volume->is_model_part())
                                    continue;
                                triangle_count += volume->mesh().facets_count();
                                if (max_triangle_count_per_plate != 0 &&
                                    triangle_count > size_t(max_triangle_count_per_plate)) {
                                    BOOST_LOG_TRIVIAL(error)
                                        << "plate " << index + 1 << ": triangle count "
                                        << triangle_count << " exceeds the limit "
                                        << max_triangle_count_per_plate;
                                    record_exit_reson(
                                        outfile_dir,
                                        CLI_TRIANGLE_COUNT_EXCEEDS_LIMIT,
                                        index + 1,
                                        CLI::error_message(CLI_TRIANGLE_COUNT_EXCEEDS_LIMIT),
                                        sliced_info);
                                    return CLI_TRIANGLE_COUNT_EXCEEDS_LIMIT;
                                }
                            }
                        }
                    }

                    if (printable_instances == 0)
                    {
                        BOOST_LOG_TRIVIAL(error)
                            << "plate " << index + 1
                            << ": no suitable objects remain after skipping "
                            << skipped_count << " objects";
                        record_exit_reson(
                            outfile_dir,
                            CLI_NO_SUITABLE_OBJECTS_AFTER_SKIP,
                            index + 1,
                            CLI::error_message(CLI_NO_SUITABLE_OBJECTS_AFTER_SKIP),
                            sliced_info);
                        return CLI_NO_SUITABLE_OBJECTS_AFTER_SKIP;
                    }

                    plate_triangle_counts[index] = triangle_count;
                }

                //update plate's bounding box to model
                DynamicPrintConfig new_print_config = m_print_config;
                new_print_config.apply(*part_plate->config());
                new_print_config.apply(m_extra_config, true);
                const std::string &printer_model = new_print_config.opt_string("printer_model", true);
                const std::string &printer_identity = !printer_model.empty()
                    ? printer_model
                    : (!new_printer_name.empty() ? new_printer_name : current_printer_system_name);
                const bool is_bbl_vendor = boost::starts_with(printer_identity, "Bambu Lab") ||
                    boost::starts_with(printer_identity, "Bambu");
                const bool is_cx_vendor = boost::starts_with(printer_identity, "Creality") ||
                    boost::starts_with(printer_identity, "SPARKX") ||
                    boost::starts_with(printer_identity, "sparkx");

                print_fff->set_is_BBL_printer(is_bbl_vendor);
                print_fff->set_is_CX_printer(is_cx_vendor);
                BOOST_LOG_TRIVIAL(info)
                    << "printer_identity: " << printer_identity
                    << ", is_bbl_vendor: " << is_bbl_vendor
                    << ", is_cx_vendor: " << is_cx_vendor;

                print->apply(model, new_print_config);
                BOOST_LOG_TRIVIAL(info) << boost::format("set no_check to %1%:")%no_check;
                print->set_no_check_flag(no_check);//BBS
                StringObjectException warning;
                auto err = print->validate(&warning);
                if (!err.string.empty()) 
                {
                    if ((STRING_EXCEPT_LAYER_HEIGHT_EXCEEDS_LIMIT == err.type) && no_check) 
                    {
                        BOOST_LOG_TRIVIAL(warning) << "got warnings: "<< err.string << std::endl;
                    }
                    else
                    {
                        BOOST_LOG_TRIVIAL(error) << "got error when validate: "<< err.string << std::endl;
                        boost::nowide::cerr << err.string << std::endl;
                        int validate_error;
                        switch (err.type)
                        {
                            case STRING_EXCEPT_FILAMENT_NOT_MATCH_BED_TYPE:
                                validate_error = CLI_FILAMENT_NOT_MATCH_BED_TYPE;
                                break;
                            case STRING_EXCEPT_FILAMENTS_DIFFERENT_TEMP:
                                validate_error = CLI_FILAMENTS_DIFFERENT_TEMP;
                                break;
                            case STRING_EXCEPT_OBJECT_COLLISION_IN_SEQ_PRINT:
                                validate_error = CLI_OBJECT_COLLISION_IN_SEQ_PRINT;
                                break;
                            case STRING_EXCEPT_OBJECT_COLLISION_IN_LAYER_PRINT:
                                validate_error = CLI_OBJECT_COLLISION_IN_LAYER_PRINT;
                                break;
                            default:
                                validate_error = CLI_VALIDATE_ERROR;
                                break;
                        }
                        if (no_check)
                            record_exit_reson(outfile_dir, validate_error, index+1, err.string, sliced_info);
                        else
                            record_exit_reson(outfile_dir, validate_error, index+1, CLI::error_message(validate_error), sliced_info);
                        return validate_error;
                    }
                }
                else if (!warning.string.empty())
                {
                    BOOST_LOG_TRIVIAL(warning) << "got warnings: "<< warning.string << std::endl;
                }

                if (print->empty())
                {
                    BOOST_LOG_TRIVIAL(error) << "plate "<< index+1<< ": Nothing to be sliced, Either the print is empty or no object is fully inside the print volume after apply." << std::endl;
                    record_exit_reson(outfile_dir, CLI_NO_SUITABLE_OBJECTS, index+1, CLI::error_message(CLI_NO_SUITABLE_OBJECTS), sliced_info);
                    return CLI_NO_SUITABLE_OBJECTS;
                }
                else
                {
                    if (pre_check && (partplate_list.get_plate_count() > 1)) //continue to next plate directly
                        continue;

                    try
                    {
#if defined(SLIC3R_DIAGNOSTICS_FINGERPRINT_ENABLED) || defined(SLIC3R_DIAGNOSTICS_PERFORMANCE_ENABLED)
                        Diagnostics::Session::Binding diagnostic_binding;
                        if (diagnostic_mode != DiagnosticMode::None)
                            diagnostic_binding = diagnostic_session.attach(*print_fff, index + 1);
#endif
                        BOOST_LOG_TRIVIAL(info) << "start Print::process for partplate "<<index+1 << std::endl;
#if defined(__linux__) || defined(__LINUX__)
                        BOOST_LOG_TRIVIAL(info) << "cli callback mgr started:  "<<g_cli_callback_mgr.m_started << std::endl;
                        if (g_cli_callback_mgr.is_started()) {
                            BOOST_LOG_TRIVIAL(info) << "set print's callback to cli_status_callback.";
                            print->set_status_callback(cli_status_callback);
                            g_cli_callback_mgr.set_plate_info(index+1, (plate_to_slice== 0)?partplate_list.get_plate_count():1);
                            if (!warning.string.empty()) {
                                PrintBase::SlicingStatus slicing_status{4, warning.string, 0, 0};
                                cli_status_callback(slicing_status);
                            }
                            else 
                            {
                                PrintBase::SlicingStatus slicing_status{4, "Slicing begins"};
                                cli_status_callback(slicing_status);
                            }
                        }
                        else 
                        {
                            BOOST_LOG_TRIVIAL(info) << "set print's callback to default_status_callback.";
                            print->set_status_callback(default_status_callback);
                        }
#else
                        BOOST_LOG_TRIVIAL(info) << "set print's callback to default_status_callback.";
                        print->set_status_callback(default_status_callback);
#endif
                        //update information for brim
                        const PrintConfig& print_config = print_fff->config();
                        Model::setExtruderParams(m_print_config, filament_count);
                        Model::setPrintSpeedTable(m_print_config, print_config);
                        if (load_slicedata) 
                        {
                            std::string plate_dir = load_slice_data_dir+"/"+std::to_string(index+1);
                            int ret = print->load_cached_data(plate_dir);
                            if (ret)
                            {
                                BOOST_LOG_TRIVIAL(warning) << "plate "<< index+1<< ": load Slicing data error, ret=" << ret;
                                BOOST_LOG_TRIVIAL(warning) << "plate "<< index+1<< ": switch normal slicing";
                                print->process();
                            }
                            else
                            {
                                BOOST_LOG_TRIVIAL(info) << "plate "<< index+1<< ": load cached data success, go on.";
#if defined(__linux__) || defined(__LINUX__)
                                if (g_cli_callback_mgr.is_started())
                                {
                                    PrintBase::SlicingStatus slicing_status{69, "Cache data loaded"};
                                    cli_status_callback(slicing_status);
                                }
#endif
                                print->process(nullptr, true);
                                BOOST_LOG_TRIVIAL(info) << "plate "<< index+1<< ": finished print::process.";
                            }
                        }
                        else
                        {
                            print->process(&time_using_cache);
                            BOOST_LOG_TRIVIAL(info) << "print::process: first time_using_cache is " << time_using_cache << " secs.";
                        }

                        std::string conflict_result = print_fff->get_conflict_string();

                        if (!conflict_result.empty())
                        {
                           BOOST_LOG_TRIVIAL(error) << "plate "<< index+1<< ": found slicing result conflict!"<< std::endl;
                           record_exit_reson(outfile_dir, CLI_GCODE_PATH_CONFLICTS, index+1, CLI::error_message(CLI_GCODE_PATH_CONFLICTS), sliced_info);
                           return CLI_GCODE_PATH_CONFLICTS;
                        }

                        //check the warnings
                        if (!g_slicing_warnings.empty())
                        {
                            for (unsigned int i = 0; i < g_slicing_warnings.size(); i++)
                            {
                                PrintBase::SlicingStatus& status = g_slicing_warnings[i];
                                if ((status.warning_step != -1) && (status.message_type != PrintStateBase::SlicingDefaultNotification))
                                {
                                    sliced_plate_info.warning_message = status.text;

                                    if (status.warning_level == PrintStateBase::WarningLevel::NON_CRITICAL) 
                                    {
                                        BOOST_LOG_TRIVIAL(warning) << "plate "<< index+1<< ": found NON_CRITICAL slicing warnings: "<<status.text <<std::endl;
                                    }
                                    else 
                                    {
                                        BOOST_LOG_TRIVIAL(warning) << boost::format("plate %1%: found slicing warnings: %2%, no_check=%3%")%(index+1) %status.text %no_check;
                                        if (!no_check) {
                                            //only following message will be reported under import mode
                                            if (status.message_type == PrintStateBase::SlicingEmptyGcodeLayers
                                                || status.message_type == PrintStateBase::SlicingGcodeOverlap)
                                            {
                                                sliced_info.sliced_plates.push_back(sliced_plate_info);
                                                record_exit_reson(outfile_dir, CLI_SLICING_ERROR, index+1, CLI::error_message(CLI_SLICING_ERROR), sliced_info);
                                                return CLI_SLICING_ERROR;
                                            }
                                        }
                                    }
                                }
                            }
                            g_slicing_warnings.clear();
                        }
                        sliced_plate_info.triangle_count = plate_triangle_counts[index];

                        const std::string retained_gcode_path =
                            (boost::filesystem::path(outfile_dir) /
                                ("plate_" + std::to_string(index + 1) + ".gcode")).string();
                        outfile = need_gcode_file ? retained_gcode_path : part_plate->get_tmp_gcode_path();
                        if (need_gcode_file)
                            part_plate->set_tmp_gcode_path(outfile);

                        BOOST_LOG_TRIVIAL(info)
                            << (need_gcode_file ? "will export G-code to " : "will generate G-code temporarily at ")
                            << outfile;
                        temp_time = (long long)Slic3r::Utils::get_current_time_utc();
                        outfile = print_fff->export_gcode(outfile, gcode_result, nullptr);
                        time_using_cache +=
                            (long long)Slic3r::Utils::get_current_time_utc() - temp_time;
                        BOOST_LOG_TRIVIAL(info)
                            << "export_gcode finished: time_using_cache update to "
                            << time_using_cache << " secs.";

                        if (need_gcode_file)
                        {
                            BOOST_LOG_TRIVIAL(info) << "G-code exported to " << outfile;
                        }
                        else
                        {
                            boost::system::error_code remove_error;
                            boost::filesystem::remove(outfile, remove_error);
                            part_plate->set_tmp_gcode_path(std::string());
                            if (remove_error)
                            {
                                BOOST_LOG_TRIVIAL(warning)
                                    << "Failed to remove temporary G-code " << outfile
                                    << ": " << remove_error.message();
                            }
                            else
                            {
                                BOOST_LOG_TRIVIAL(info) << "Temporary G-code removed: " << outfile;
                            }
                        }
                        part_plate->update_slice_result_valid_state(true);
#if defined(__linux__) || defined(__LINUX__)
                        if (g_cli_callback_mgr.is_started()) {
                            PrintBase::SlicingStatus slicing_status{100, "Slicing finished"};
                            cli_status_callback(slicing_status);
                        }
#endif
                        if (export_slicedata) {
                            BOOST_LOG_TRIVIAL(info) << "plate "<< index+1<< ":will export Slicing data to " << export_slice_data_dir;
                            std::string plate_dir = export_slice_data_dir+"/"+std::to_string(index+1);
                            bool with_space = (get_logging_level() >= 4)?true:false;
                            int ret = print->export_cached_data(plate_dir, with_space);
                            if (ret) {
                                BOOST_LOG_TRIVIAL(error) << "plate "<< index+1<< ": export Slicing data error, ret=" << ret;
                                if (boost::filesystem::exists(plate_dir))
                                    boost::filesystem::remove_all(plate_dir);
                                record_exit_reson(outfile_dir, ret, index+1, CLI::error_message(ret), sliced_info);
                                return ret;
                            }
                        }
                        end_time = (long long)Slic3r::Utils::get_current_time_utc();
                        sliced_plate_info.sliced_time = end_time - start_time;
                        sliced_plate_info.sliced_time_with_cache = time_using_cache;

                        if (max_slicing_time_per_plate != 0) {
                            long long time_cost = end_time - start_time;
                            if (time_cost > max_slicing_time_per_plate) {
                                sliced_plate_info.warning_message = (boost::format("plate %1%'s slice time %2% exceeds the limit %3%, return error.")%(index+1) %time_cost %max_slicing_time_per_plate).str();
                                BOOST_LOG_TRIVIAL(error) << sliced_plate_info.warning_message;
                                sliced_info.sliced_plates.push_back(sliced_plate_info);
                                record_exit_reson(outfile_dir, CLI_SLICING_TIME_EXCEEDS_LIMIT, index+1, CLI::error_message(CLI_SLICING_TIME_EXCEEDS_LIMIT), sliced_info);
                                return CLI_SLICING_TIME_EXCEEDS_LIMIT;
                            }
                        }
                        sliced_info.sliced_plates.push_back(sliced_plate_info);
                    }
                    catch (const std::exception &ex) 
                    {
                        BOOST_LOG_TRIVIAL(error) << "found slicing or export error for partplate "<<index+1 << std::endl;
                        boost::nowide::cerr << ex.what() << std::endl;
                        record_exit_reson(outfile_dir, CLI_SLICING_ERROR, index+1, CLI::error_message(CLI_SLICING_ERROR), sliced_info);
                        return CLI_SLICING_ERROR;
                    }
                }
            }
            if (pre_check&& (partplate_list.get_plate_count() > 1))
                pre_check = false;
            else
                finished = true;
        }//end for partplate

#if defined(__linux__) || defined(__LINUX__)
        if (g_cli_callback_mgr.is_started()) {
            int plate_count = (plate_to_slice== 0)?partplate_list.get_plate_count():1;
            g_cli_callback_mgr.set_plate_info(0, plate_count);
        }
#endif
    }

    if (fingerprint_enabled)
    {
        const std::string report_path =
            (boost::filesystem::path(outfile_dir) / "fingerprint-report.json").string();
        std::string report_error;

        if (!fingerprint_report.save(report_path, report_error))
        {
            BOOST_LOG_TRIVIAL(error) << report_error;
            record_exit_reson(
                outfile_dir,
                CLI_EXPORT_CACHE_WRITE_FAILED,
                plate_to_slice,
                report_error,
                sliced_info);
            return CLI_EXPORT_CACHE_WRITE_FAILED;
        }
        BOOST_LOG_TRIVIAL(info) << "Stage report written to " << report_path;
    }

    return 0;
}

int SliceCommand::load_model_presets(ModelPresetContext &context)
{
    PrinterTechnology &printer_technology = context.printer_technology;
    const std::vector<std::string> &load_configs = context.load_configs;
    const std::vector<std::string> &load_filaments = context.load_filaments;
    const bool use_first_fila_as_default = context.use_first_filament_as_default;
    const ForwardCompatibilitySubstitutionRule config_substitution_rule = context.config_substitution_rule;
    const std::string &outfile_dir = context.outfile_dir;
    sliced_info_t &sliced_info = context.sliced_info;
    int &filament_count = context.filament_count;
    std::string &new_printer_name = context.new_printer_name;

    std::string new_process_name, new_process_system_name, new_printer_system_name, printer_model;
    std::vector<std::string> new_print_compatible_printers;
    DynamicPrintConfig load_process_config, load_machine_config;
    bool new_process_config_is_system = true, new_printer_config_is_system = true;
    std::string different_process_setting;
    BOOST_LOG_TRIVIAL(info) << __FUNCTION__<< ":before load settings, file count="<< load_configs.size() << std::endl;
    // load config files supplied via --load
    for (auto const &file : load_configs) {
        DynamicPrintConfig  config;
        std::string config_type, config_name, filament_id, config_from;
        int ret = load_cli_preset_file(file, config_substitution_rule, config, config_type, config_name, filament_id, config_from);
        if (ret) {
            record_exit_reson(outfile_dir, ret, 0, CLI::error_message(ret), sliced_info);
            return ret;
        }

        PrinterTechnology other_printer_technology = get_printer_technology(config);
        if (printer_technology == ptUnknown) {
            printer_technology = other_printer_technology;
        }

        if ((printer_technology != other_printer_technology)&&(other_printer_technology != ptUnknown)){
            boost::nowide::cerr << "invalid printer_technology " <<printer_technology<<", from config "<< file <<std::endl;
            record_exit_reson(outfile_dir, CLI_INVALID_PRINTER_TECH, 0, CLI::error_message(CLI_INVALID_PRINTER_TECH), sliced_info);
            return CLI_INVALID_PRINTER_TECH;
        }
        if (config_type == "machine") {
            if (!new_printer_name.empty()) {
                boost::nowide::cerr << "duplicate machine config file: " << file << std::endl;
                record_exit_reson(outfile_dir, CLI_CONFIG_FILE_ERROR, 0, CLI::error_message(CLI_CONFIG_FILE_ERROR), sliced_info);
                return CLI_CONFIG_FILE_ERROR;
            }
            new_printer_name = config_name;
            if (config_from == "system") {
                new_printer_system_name = new_printer_name;
                new_printer_config_is_system = true;
            }
            else {
                new_printer_system_name = config.option<ConfigOptionString>("inherits", true)->value;
                new_printer_config_is_system = false;
            }
            config.set("printer_settings_id", new_printer_name, true);

            printer_model = config.option<ConfigOptionString>("printer_model", true)->value;

            //printer_inherits = config.option<ConfigOptionString>("inherits", true)->value;
            load_machine_config = std::move(config);
            BOOST_LOG_TRIVIAL(info) << boost::format("loaded machine config %1%, name %2%, source %3%, inherits %4%")
                % file % config_name % config_from % new_printer_system_name;
        }
        else if (config_type == "process") {
            if (!new_process_name.empty()) {
                boost::nowide::cerr << "duplicate process config file: " << file << std::endl;
                record_exit_reson(outfile_dir, CLI_CONFIG_FILE_ERROR, 0, CLI::error_message(CLI_CONFIG_FILE_ERROR), sliced_info);
                return CLI_CONFIG_FILE_ERROR;
            }
            new_process_name = config_name;
            if (config_from == "system") {
                new_process_system_name = new_process_name;
                new_process_config_is_system = true;
            }
            else {
                new_process_system_name = config.option<ConfigOptionString>("inherits", true)->value;
                new_process_config_is_system = false;
            }
            config.set("print_settings_id", new_process_name, true);
            //print_inherits = config.option<ConfigOptionString>("inherits", true)->value;
            new_print_compatible_printers = config.option<ConfigOptionStrings>("compatible_printers", true)->values;

            if (const auto *different = config.option<ConfigOptionStrings>("different_settings_to_system");
                different && !different->values.empty()) {
                different_process_setting = different->values.front();
            }
            load_process_config = std::move(config);
            BOOST_LOG_TRIVIAL(info) << boost::format("loaded process config %1%, type %2%, name %3%, inherits %4%")%file %config_name %config_from % new_process_system_name;
        }
        else {
            BOOST_LOG_TRIVIAL(error) << "--load_settings accepts only machine and process presets";
            record_exit_reson(outfile_dir, CLI_CONFIG_FILE_ERROR, 0, CLI::error_message(CLI_CONFIG_FILE_ERROR), sliced_info);
            return CLI_CONFIG_FILE_ERROR;
        }


    }

    //load filaments files
    int load_filament_count = load_filaments.size();
    std::vector<int> load_filaments_index;


    std::vector<DynamicPrintConfig> load_filaments_config;
    std::vector<std::string> load_filaments_id;
    std::vector<std::string> load_filaments_name, load_filaments_inherit;
    int current_index = 0;
    std::string default_load_fila_name, default_load_fila_id, default_filament_file, default_filament_inherit;
    DynamicPrintConfig  default_load_fila_config;
    if (use_first_fila_as_default) {
        //construct default filament
        for (int index = 0; index < load_filament_count; index++) {
            const std::string& file = load_filaments[index];
            if (default_filament_file.empty() && !file.empty()) {
                DynamicPrintConfig  config;
                std::string config_type, config_name, filament_id, config_from;
                int ret = load_cli_preset_file(file, config_substitution_rule, config, config_type, config_name, filament_id, config_from);
                if (ret) {
                    record_exit_reson(outfile_dir, ret, 0, CLI::error_message(ret), sliced_info);
                    return ret;
                }

                if (config_type != "filament") {
                    BOOST_LOG_TRIVIAL(error) <<__FUNCTION__ << boost::format(": unknown config type %1% of file %2% in load-filaments") % config_type % file;
                    record_exit_reson(outfile_dir, CLI_CONFIG_FILE_ERROR, 0, CLI::error_message(CLI_CONFIG_FILE_ERROR), sliced_info);
                    return CLI_CONFIG_FILE_ERROR;
                }

                if ((config_from == "User")||(config_from == "user")) {
                    default_filament_inherit = config.option<ConfigOptionString>("inherits", true)->value;
                }

                default_filament_file = file;
                default_load_fila_name = config_name;
                default_load_fila_id = filament_id;
                default_load_fila_config = std::move(config);

                BOOST_LOG_TRIVIAL(info) << boost::format("loaded default filament config %1%, type %2%, name %3%, inherits %4%")%file  %config_from %config_name % default_filament_inherit;
                break;
            }
        }
        if ((load_filament_count > 0) && default_filament_file.empty())
        {
            BOOST_LOG_TRIVIAL(error) <<__FUNCTION__ << boost::format(": load_filament_count is %1%, but can not load a default filament") % load_filament_count;
            record_exit_reson(outfile_dir, CLI_CONFIG_FILE_ERROR, 0, CLI::error_message(CLI_CONFIG_FILE_ERROR), sliced_info);
            return CLI_CONFIG_FILE_ERROR;
        }
    }
    for (int index = 0; index < load_filament_count; index++) {
        const std::string& file = load_filaments[index];
        current_index++;
        if (!file.empty()) {
            DynamicPrintConfig  config;
            std::string config_type, config_name, filament_id, config_from;
            int ret = load_cli_preset_file(file, config_substitution_rule, config, config_type, config_name, filament_id, config_from);
            if (ret) {
                record_exit_reson(outfile_dir, ret, 0, CLI::error_message(ret), sliced_info);
                return ret;
            }

            if (config_type != "filament") {
                BOOST_LOG_TRIVIAL(error) <<__FUNCTION__ << boost::format(": unknown config type %1% of file %2% in load-filaments") % config_type % file;
                record_exit_reson(outfile_dir, CLI_CONFIG_FILE_ERROR, 0, CLI::error_message(CLI_CONFIG_FILE_ERROR), sliced_info);
                return CLI_CONFIG_FILE_ERROR;
            }

            PrinterTechnology other_printer_technology = get_printer_technology(config);
            if (printer_technology == ptUnknown) {
                printer_technology = other_printer_technology;
            }
            if ((printer_technology != other_printer_technology) && (other_printer_technology != ptUnknown)) {
                BOOST_LOG_TRIVIAL(error) << "invalid printer_technology " <<printer_technology<<", from filament file "<< file;
                record_exit_reson(outfile_dir, CLI_INVALID_PRINTER_TECH, 0, CLI::error_message(CLI_INVALID_PRINTER_TECH), sliced_info);
                return CLI_INVALID_PRINTER_TECH;
            }
            std::string inherits;
            if ((config_from == "User")||(config_from == "user")) {
                inherits = config.option<ConfigOptionString>("inherits", true)->value;
            }
            load_filaments_inherit.push_back(inherits);
            load_filaments_id.push_back(filament_id);
            load_filaments_name.push_back(config_name);
            load_filaments_config.push_back(std::move(config));
            load_filaments_index.push_back(current_index);



            BOOST_LOG_TRIVIAL(info) << boost::format("loaded filament %1% from file %2%, type %3%, name %4%, inherits %5%")%(index+1) %file %config_from %config_name % inherits;
        }
        else {
            if (use_first_fila_as_default) {
                BOOST_LOG_TRIVIAL(info)<<__FUNCTION__ << boost::format(": load filament %1% from default, config name %2%, filament_id %3%, current_index %4%") % (index+1) % default_load_fila_name %default_load_fila_id %current_index;
                load_filaments_id.push_back(default_load_fila_id);
                load_filaments_name.push_back(default_load_fila_name);
                load_filaments_config.push_back(default_load_fila_config);
                load_filaments_index.push_back(current_index);
                load_filaments_inherit.push_back(default_filament_inherit);


            }
            continue;
        }
    }

    if (filament_count == 0)
        filament_count = load_filament_count;

    // STL/OBJ slicing requires machine and process presets as a compatible pair.
    const bool has_machine = !new_printer_name.empty();
    const bool has_process = !new_process_name.empty();
    if (has_machine != has_process) {
        BOOST_LOG_TRIVIAL(error) << "STL/OBJ slicing requires both machine and process settings";
        record_exit_reson(outfile_dir, CLI_CONFIG_FILE_ERROR, 0, CLI::error_message(CLI_CONFIG_FILE_ERROR), sliced_info);
        return CLI_CONFIG_FILE_ERROR;
    }

    if (has_machine) {
        const bool process_compatible = std::find(
            new_print_compatible_printers.begin(),
            new_print_compatible_printers.end(),
            new_printer_system_name) != new_print_compatible_printers.end();
        if (!process_compatible) {
            BOOST_LOG_TRIVIAL(error) << "The process preset is not compatible with the machine preset";
            record_exit_reson(outfile_dir, CLI_PROCESS_NOT_COMPATIBLE, 0, CLI::error_message(CLI_PROCESS_NOT_COMPATIBLE), sliced_info);
            return CLI_PROCESS_NOT_COMPATIBLE;
        }
    }
    std::vector<std::string>& different_settings = m_print_config.option<ConfigOptionStrings>("different_settings_to_system", true)->values;
    std::vector<std::string>& inherits_group = m_print_config.option<ConfigOptionStrings>("inherits_group", true)->values;
    inherits_group.resize(filament_count + 2, std::string());
    different_settings.resize(filament_count + 2, std::string());
    if (!different_process_setting.empty()) {
        different_settings[0] = different_process_setting;
    }
    // Apply the external machine and process presets used for STL/OBJ slicing.
    if (!new_printer_name.empty()) {
        different_settings[filament_count + 1].clear();
        inherits_group[filament_count + 1] = new_printer_config_is_system ? "" : new_printer_system_name;

        load_default_gcodes_to_config(load_machine_config, Preset::TYPE_PRINTER);
        const int ret = apply_external_preset(m_print_config, load_machine_config);
        if (ret) {
            record_exit_reson(outfile_dir, ret, 0, CLI::error_message(ret), sliced_info);
            return ret;
        }

        apply_cli_safe_machine_limits(m_print_config, printer_model);
    }

    if (!new_process_name.empty()) {
        different_settings[0].clear();
        inherits_group[0] = new_process_config_is_system ? "" : new_process_system_name;
        m_print_config.option<ConfigOptionStrings>("print_compatible_printers", true)->values =
            new_print_compatible_printers;

        load_default_gcodes_to_config(load_process_config, Preset::TYPE_PRINT);
        const int ret = apply_external_preset(m_print_config, load_process_config);
        if (ret) {
            record_exit_reson(outfile_dir, ret, 0, CLI::error_message(ret), sliced_info);
            return ret;
        }
    }
    // Apply external filament presets used for STL/OBJ slicing.
    for (size_t config_index = 0; config_index < load_filaments_config.size(); ++config_index) {
        DynamicPrintConfig &config = load_filaments_config[config_index];
        const int filament_index = load_filaments_index[config_index];
        load_default_gcodes_to_config(config, Preset::TYPE_FILAMENT);

        auto *filament_settings = m_print_config.option<ConfigOptionStrings>("filament_settings_id", true);
        const std::string &filament_name = load_filaments_name[config_index];
        ConfigOptionString filament_name_option(filament_name);
        if (filament_settings->size() < filament_count)
            filament_settings->resize(filament_count, &filament_name_option);
        filament_settings->set_at(&filament_name_option, filament_index - 1, 0);
        config.erase("filament_settings_id");

        auto *filament_ids = m_print_config.option<ConfigOptionStrings>("filament_ids", true);
        ConfigOptionString filament_id_option(load_filaments_id[config_index]);
        if (filament_ids->size() < filament_count)
            filament_ids->resize(filament_count, &filament_id_option);
        filament_ids->set_at(&filament_id_option, filament_index - 1, 0);

        different_settings[filament_index].clear();
        inherits_group[filament_index] = load_filaments_inherit[config_index];

        for (const t_config_option_key &opt_key : config.keys()) {
            const ConfigOption *source_opt = config.option(opt_key);
            if (!source_opt) {
                BOOST_LOG_TRIVIAL(error) << boost::format("can not find %1% from filament %2%: %3%")
                    % opt_key % filament_index % filament_name;
                record_exit_reson(outfile_dir, CLI_CONFIG_FILE_ERROR, 0, CLI::error_message(CLI_CONFIG_FILE_ERROR), sliced_info);
                return CLI_CONFIG_FILE_ERROR;
            }

            if (source_opt->is_scalar()) {
                if (opt_key == "compatible_printers_condition") {
                    auto *destination = m_print_config.option<ConfigOptionStrings>("compatible_machine_expression_group", true);
                    if (destination->size() == 0)
                        destination->resize(filament_count + 2, new ConfigOptionString());
                    destination->set_at(source_opt, filament_index, 0);
                } else if (opt_key == "compatible_prints_condition") {
                    auto *destination = m_print_config.option<ConfigOptionStrings>("compatible_process_expression_group", true);
                    if (destination->size() == 0)
                        destination->resize(filament_count, new ConfigOptionString());
                    destination->set_at(source_opt, filament_index - 1, 0);
                }
                continue;
            }

            if (opt_key == "compatible_prints" || opt_key == "compatible_printers" ||
                opt_key == "model_id" || opt_key == "dev_model_name" ||
                opt_key == "filament_settings_id")
                continue;

            ConfigOption *destination = m_print_config.option(opt_key, true);
            if (!destination) {
                BOOST_LOG_TRIVIAL(error) << boost::format("can not create option %1% in config, from filament %2%: %3%")
                    % opt_key % filament_index % filament_name;
                record_exit_reson(outfile_dir, CLI_CONFIG_FILE_ERROR, 0, CLI::error_message(CLI_CONFIG_FILE_ERROR), sliced_info);
                return CLI_CONFIG_FILE_ERROR;
            }

            auto *destination_values = static_cast<ConfigOptionVectorBase *>(destination);
            const auto *source_values = static_cast<const ConfigOptionVectorBase *>(source_opt);
            destination_values->set_at(source_values, filament_index - 1, 0);
        }
    }
    return CLI_SUCCESS;
}

int SliceCommand::update_filament_colors_and_flush(SliceInput &input, const SliceCommandOptions &options)
{
    const std::string &outfile_dir = options.outfile_dir;
    sliced_info_t &sliced_info = input.sliced_info;
    const int filament_count = input.filament_count;
    bool &filament_color_changed = input.filament_color_changed;

    // Compute flush volumes after project or external-preset configuration is loaded.
    ConfigOptionStrings *selected_filament_colors_option = m_extra_config.option<ConfigOptionStrings>("filament_colour");
    ConfigOptionStrings *project_filament_colors_option = m_print_config.option<ConfigOptionStrings>("filament_colour");
    if ((!project_filament_colors_option || (project_filament_colors_option->values.size() == 0)) && selected_filament_colors_option)
    {
        BOOST_LOG_TRIVIAL(info) << boost::format("initial project_filament_colors is null, create it due to filament_colour set in cli");
        project_filament_colors_option = m_print_config.option<ConfigOptionStrings>("filament_colour", true);
        std::vector<std::string>& project_filament_colors = project_filament_colors_option->values;
        project_filament_colors.resize(filament_count, "#FFFFFF");
    }
    if (project_filament_colors_option && (selected_filament_colors_option || !m_print_config.option<ConfigOptionFloats>("flush_volumes_matrix")))
    {
        std::vector<std::string>  selected_filament_colors;
        if (selected_filament_colors_option) {
            selected_filament_colors = selected_filament_colors_option->values;
            //erase here
            m_extra_config.erase("filament_colour");

        }

        std::vector<std::string>  &project_filament_colors = project_filament_colors_option->values;
        size_t project_filament_count = project_filament_colors.size();
        BOOST_LOG_TRIVIAL(info) << boost::format("select filament color from cli, size %1%")%selected_filament_colors.size();
        BOOST_LOG_TRIVIAL(info) << boost::format("project filament colors size %1%")%project_filament_colors.size();
        if (project_filament_count > 0)
        {
            for ( size_t index = 0; index < project_filament_count; index++ )
            {
                BOOST_LOG_TRIVIAL(info) << boost::format("project filament %1% original color %2%")%index %project_filament_colors[index];
                if (selected_filament_colors.size() > index)
                {
                    if (!selected_filament_colors[index].empty())
                    {
                        BOOST_LOG_TRIVIAL(info) << boost::format("changed to new color %1%")%selected_filament_colors[index];
                        unsigned char ori_rgb_color[4] = {}, new_rgb_color[4] = {};
                        Slic3r::GUI::BitmapCache::parse_color4(project_filament_colors[index], ori_rgb_color);
                        Slic3r::GUI::BitmapCache::parse_color4(selected_filament_colors[index], new_rgb_color);
                        if ((ori_rgb_color[0] != new_rgb_color[0]) || (ori_rgb_color[1] != new_rgb_color[1]) || (ori_rgb_color[2] != new_rgb_color[2]) || (ori_rgb_color[3] != new_rgb_color[3]))
                        {
                            BOOST_LOG_TRIVIAL(info) << boost::format("found color changes, need to regenerate thumbnail");
                            filament_color_changed = true;
                        }
                        project_filament_colors[index] = selected_filament_colors[index];
                    }
                }
            }

            //computing
            ConfigOptionBools* filament_is_support = m_print_config.option<ConfigOptionBools>("filament_is_support", true);
            std::vector<double>& flush_vol_matrix = m_print_config.option<ConfigOptionFloats>("flush_volumes_matrix", true)->values;
            //std::vector<float>& flush_vol_vector = m_print_config.option<ConfigOptionFloats>("flush_volumes_vector", true)->values;
            flush_vol_matrix.resize(project_filament_count*project_filament_count, 0.f);
            //flush_vol_vector.resize(project_filament_count);
            //set multiplier to 1?
            m_print_config.option<ConfigOptionFloat>("flush_multiplier", true)->set(new ConfigOptionFloat(1.f));

            const std::vector<int>& min_flush_volumes = Slic3r::GUI::get_min_flush_volumes(m_print_config);

            if (filament_is_support->size() != project_filament_count)
            {
                BOOST_LOG_TRIVIAL(error) << boost::format("filament_is_support's count %1% not equal to filament_colour's size %2%")%filament_is_support->size() %project_filament_count;
                record_exit_reson(outfile_dir, CLI_CONFIG_FILE_ERROR, 0, CLI::error_message(CLI_CONFIG_FILE_ERROR), sliced_info);
                return CLI_CONFIG_FILE_ERROR;
            }

            {
                std::ostringstream volumes_str;
                std::copy(min_flush_volumes.begin(), min_flush_volumes.end(), std::ostream_iterator<int>(volumes_str, ","));
                BOOST_LOG_TRIVIAL(info) << boost::format("extra_flush_volume: %1%") % volumes_str.str();
                BOOST_LOG_TRIVIAL(info) << boost::format("filament_is_support: %1%") % filament_is_support->serialize();
                BOOST_LOG_TRIVIAL(info) << boost::format("flush_volumes_matrix before computing: %1%") % m_print_config.option<ConfigOptionFloats>("flush_volumes_matrix")->serialize();
            }
            for (int from_idx = 0; from_idx < project_filament_count; from_idx++) {
                const std::string& from_color = project_filament_colors[from_idx];
                unsigned char from_rgb[4] = {};
                Slic3r::GUI::BitmapCache::parse_color4(from_color, from_rgb);
                bool is_from_support = filament_is_support->get_at(from_idx);
                for (int to_idx = 0; to_idx < project_filament_count; to_idx++) {
                    bool is_to_support = filament_is_support->get_at(to_idx);
                    if (from_idx == to_idx) {
                        flush_vol_matrix[project_filament_count*from_idx + to_idx] = 0.f;
                    }
                    else {
                        int flushing_volume = 0;
                        if (is_to_support) {
                            flushing_volume = Slic3r::g_flush_volume_to_support;
                        }
                        else {
                            const std::string& to_color = project_filament_colors[to_idx];
                            unsigned char to_rgb[4] = {};
                            Slic3r::GUI::BitmapCache::parse_color4(to_color, to_rgb);
                            //BOOST_LOG_TRIVIAL(info) << boost::format("src_idx %1%, src color %2%, dst idex %3%, dst color %4%")%from_idx %from_color %to_idx %to_color;
                            //BOOST_LOG_TRIVIAL(info) << boost::format("src_rgba {%1%,%2%,%3%,%4%} dst_rgba {%5%,%6%,%7%,%8%}")%(unsigned int)(from_rgb[0]) %(unsigned int)(from_rgb[1]) %(unsigned int)(from_rgb[2]) %(unsigned int)(from_rgb[3])
                            //       %(unsigned int)(to_rgb[0]) %(unsigned int)(to_rgb[1]) %(unsigned int)(to_rgb[2]) %(unsigned int)(to_rgb[3]);

                            Slic3r::FlushVolCalculator calculator(min_flush_volumes[from_idx], Slic3r::g_max_flush_volume);

                            flushing_volume = calculator.calc_flush_vol(from_rgb[3], from_rgb[0], from_rgb[1], from_rgb[2], to_rgb[3], to_rgb[0], to_rgb[1], to_rgb[2]);
                            if (is_from_support) {
                                flushing_volume = std::max(Slic3r::g_min_flush_volume_from_support, flushing_volume);
                            }
                        }

                        flush_vol_matrix[project_filament_count * from_idx + to_idx] = flushing_volume;
                        //flushing_volume = int(flushing_volume * get_flush_multiplier());
                    }
                }
            }
            BOOST_LOG_TRIVIAL(info) << boost::format("flush_volumes_matrix after computed: %1%")%m_print_config.option<ConfigOptionFloats>("flush_volumes_matrix")->serialize();
        }
        else
        {
            BOOST_LOG_TRIVIAL(warning) << boost::format("filament colors count is 0 in projects");
        }
    }
    else
    {
        BOOST_LOG_TRIVIAL(warning) << boost::format("no filament colors found in projects");
    }

    return CLI_SUCCESS;
}

int SliceCommand::load_input_models(InputLoadContext &context)
{
    PrinterTechnology &printer_technology = context.printer_technology;
    int &plate_to_slice = context.plate_to_slice;
    const bool normative_check = context.normative_check;
    const bool allow_newer_file = context.allow_newer_file;
    const std::vector<int> &loaded_filament_ids = context.loaded_filament_ids;
    const std::vector<std::string> &load_filaments = context.load_filaments;
    const std::vector<std::string> &load_configs = context.load_configs;
    const ForwardCompatibilitySubstitutionRule config_substitution_rule = context.config_substitution_rule;
    const std::string &outfile_dir = context.outfile_dir;
    sliced_info_t &sliced_info = context.sliced_info;
    PlateDataPtrs &plate_data_src = context.plate_data_src;
    bool &is_project_input = context.is_project_input;
    bool &is_creality_project_3mf = context.is_creality_project_3mf;
    std::string &creality_project_file = context.creality_project_file;
    Semver &creality_project_file_version = context.creality_project_file_version;
    std::vector<Preset*> &project_presets = context.project_presets;
    std::string &current_printer_system_name = context.current_printer_system_name;
    int &filament_count = context.filament_count;
    std::set<int> &used_filament_set = context.used_filament_set;
    bool first_file = true;
    Semver file_version;
    BOOST_LOG_TRIVIAL(info) << boost::format("plate_to_slice=%1%, normative_check=%2%")
        % plate_to_slice % normative_check;
    unsigned int input_index = 0;
        for (const std::string& file : m_input_files) {
            if (!boost::filesystem::exists(file)) {
                boost::nowide::cerr << "No such file: " << file << std::endl;
                record_exit_reson(outfile_dir, CLI_FILE_NOTFOUND, 0, CLI::error_message(CLI_FILE_NOTFOUND), sliced_info);
                return CLI_FILE_NOTFOUND;
            }
            Model model;
            const bool current_is_creality_project_3mf =
                first_file &&
                boost::algorithm::iends_with(file, ".3mf") &&
                bbs_is_creality_3mf(file.c_str());
            BOOST_LOG_TRIVIAL(info) << "read model file:" << file << "\n";
            try {
                // When loading an AMF or 3MF, config is imported as well, including the printer technology.
                DynamicPrintConfig config;
                ConfigSubstitutionContext config_substitutions(config_substitution_rule);

                bool file_is_project = false;
                LoadStrategy strategy;
                if (boost::algorithm::iends_with(file, ".3mf") && first_file) {
                    if (!loaded_filament_ids.empty())
                    {
                        BOOST_LOG_TRIVIAL(error) << boost::format("cannot load a 3MF project together with load_filament_ids");
                        record_exit_reson(outfile_dir, CLI_INVALID_PARAMS, 0, CLI::error_message(CLI_INVALID_PARAMS), sliced_info);
                        return CLI_INVALID_PARAMS;
                    }
                    strategy = LoadStrategy::LoadModel | LoadStrategy::LoadConfig|LoadStrategy::AddDefaultInstances | LoadStrategy::LoadAuxiliary;
                }
                else {
                    strategy = LoadStrategy::LoadModel | LoadStrategy::AddDefaultInstances;
                }
                model = Model::read_from_file(file, &config, &config_substitutions, strategy, &plate_data_src, &project_presets, &file_is_project, &file_version, nullptr, nullptr, nullptr, nullptr, nullptr, plate_to_slice);
                if (current_is_creality_project_3mf) {
                    is_creality_project_3mf       = true;
                    creality_project_file         = file;
                    creality_project_file_version = file_version;
                }

                if (file_is_project || current_is_creality_project_3mf)
                {
                    if (!first_file)
                    {
                        BOOST_LOG_TRIVIAL(info) << "A 3MF project should be placed at the first position, filename=" << file << "\n";
                        record_exit_reson(outfile_dir, CLI_FILELIST_INVALID_ORDER, 0, CLI::error_message(CLI_FILELIST_INVALID_ORDER), sliced_info);
                        return CLI_FILELIST_INVALID_ORDER;
                    }
                    is_project_input = true;
                    if (m_input_files.size() != 1) {
                        BOOST_LOG_TRIVIAL(error) << "A 3MF project cannot be combined with STL/OBJ model inputs in one slicing command";
                        record_exit_reson(outfile_dir, CLI_INVALID_PARAMS, 0, CLI::error_message(CLI_INVALID_PARAMS), sliced_info);
                        return CLI_INVALID_PARAMS;
                    }
                    BOOST_LOG_TRIVIAL(info) << boost::format("the first file is a 3mf, version %1%, got plate count %2%") %file_version.to_string() %plate_data_src.size();

                    Semver cli_ver = *Semver::parse(SLIC3R_VERSION);
                    if (!allow_newer_file && ((cli_ver.maj() != file_version.maj()) || (cli_ver.min() < file_version.min()))){
                        BOOST_LOG_TRIVIAL(error) << boost::format("Version Check: File Version %1% not supported by current cli version %2%")%file_version.to_string() %SLIC3R_VERSION;
                        record_exit_reson(outfile_dir, CLI_FILE_VERSION_NOT_SUPPORTED, 0, CLI::error_message(CLI_FILE_VERSION_NOT_SUPPORTED), sliced_info);
                        return CLI_FILE_VERSION_NOT_SUPPORTED;
                    }
                    if (normative_check) {
                        const auto *postprocess_scripts = config.option<ConfigOptionStrings>("post_process");
                        if (postprocess_scripts && !postprocess_scripts->values.empty()) {
                            BOOST_LOG_TRIVIAL(error) << boost::format("normative_check: postprocess not supported, array size %1%")
                                % postprocess_scripts->values.size();
                            record_exit_reson(outfile_dir, CLI_POSTPROCESS_NOT_SUPPORTED, 0, CLI::error_message(CLI_POSTPROCESS_NOT_SUPPORTED), sliced_info);
                            return CLI_POSTPROCESS_NOT_SUPPORTED;
                        }
                    }

                    const std::string current_printer_name =
                        config.option<ConfigOptionString>("printer_settings_id")->value;
                    const auto &current_filaments_name =
                        config.option<ConfigOptionStrings>("filament_settings_id")->values;
                    current_printer_system_name = current_printer_name;
                    if (const auto *inherits = config.option<ConfigOptionStrings>("inherits_group");
                        inherits && !inherits->values.empty() && !inherits->values.back().empty()) {
                        current_printer_system_name = inherits->values.back();
                    }
                    filament_count = static_cast<int>(current_filaments_name.size());
                }
                else
                {
                    int object_extruder_id = 0;
                    if (loaded_filament_ids.size() > input_index) {
                        if (loaded_filament_ids[input_index] > 0) {
                            if (loaded_filament_ids[input_index] > load_filaments.size()) {
                                BOOST_LOG_TRIVIAL(error) << boost::format("invalid filament_id %1% at index %2%, max %3%")%loaded_filament_ids[input_index] % (input_index + 1) %load_filaments.size();
                                record_exit_reson(outfile_dir, CLI_INVALID_PARAMS, 0, CLI::error_message(CLI_INVALID_PARAMS), sliced_info);
                                return CLI_INVALID_PARAMS;
                            }
                            object_extruder_id = loaded_filament_ids[input_index];
                            used_filament_set.emplace(object_extruder_id);
                        }
                    }

                    for (ModelObject* o : model.objects)
                    {
                        if (object_extruder_id != 0) {
                            o->config.set_key_value("extruder", new ConfigOptionInt(object_extruder_id));
                        }

                        BOOST_LOG_TRIVIAL(info) << "object "<<o->name <<", id :"  << o->id().id << ", from stl or other 3mf\n";
                        if (!current_is_creality_project_3mf)
                            o->ensure_on_bed();
                    }
                }
                first_file = false;

                PrinterTechnology other_printer_technology = get_printer_technology(config);
                if (printer_technology == ptUnknown) {
                    printer_technology = other_printer_technology;
                }
                if ((printer_technology != other_printer_technology) && (other_printer_technology != ptUnknown)) {
                    boost::nowide::cerr << "invalid printer_technology " <<printer_technology<<", from source file "<< file <<std::endl;
                    record_exit_reson(outfile_dir, CLI_INVALID_PRINTER_TECH, 0, CLI::error_message(CLI_INVALID_PRINTER_TECH), sliced_info);
                    return CLI_INVALID_PRINTER_TECH;
                }
                if (!config_substitutions.substitutions.empty()) {
                    BOOST_LOG_TRIVIAL(info) << "Found legacy configuration values, substituted when loading " << file << ":\n";
                    for (const ConfigSubstitution &subst : config_substitutions.substitutions)
                        BOOST_LOG_TRIVIAL(info) << "\tkey = \"" << subst.opt_def->opt_key << "\"\t old_value = \"" << subst.old_value << "\tnew_value = \"" << subst.new_value->serialize() << "\"\n";
                }

                // config is applied to m_print_config before the current m_config values.
                config += std::move(m_print_config);
                m_print_config = std::move(config);
                input_index++;
            }
            catch (std::exception& e) {
                boost::nowide::cerr << file << ": " << e.what() << std::endl;
                record_exit_reson(outfile_dir, CLI_DATA_FILE_ERROR, 0, CLI::error_message(CLI_DATA_FILE_ERROR), sliced_info);
                return CLI_DATA_FILE_ERROR;
            }
            if (model.objects.empty()) {
                boost::nowide::cerr << "Error: file is empty: " << file << std::endl;
                continue;
            }
            m_models.push_back(std::move(model));
        }

    if (!is_project_input && plate_to_slice > 0)
    {
        BOOST_LOG_TRIVIAL(warning) << boost::format("%1%: not support to slice plate %2%, reset to 0")%__LINE__ %plate_to_slice;
        plate_to_slice = 0;
    }

    // Project files already contain their effective printer, process and filament settings.
    // External preset files belong exclusively to the STL/OBJ slicing path.
    if (is_project_input && (!load_configs.empty() || !load_filaments.empty())) {
        BOOST_LOG_TRIVIAL(error) << "--load_settings and --load_filaments cannot be used when slicing a 3MF project";
        record_exit_reson(outfile_dir, CLI_INVALID_PARAMS, 0, CLI::error_message(CLI_INVALID_PARAMS), sliced_info);
        return CLI_INVALID_PARAMS;
    }
    return 0;
}

void SliceCommand::refresh_creality_project_configuration(SliceInput &input)
{
    if (!input.is_creality_project_3mf || m_print_config.empty())
        return;

    try {
        boost::filesystem::path profile_root = boost::filesystem::path(data_dir()) / PRESET_SYSTEM_DIR;
        if (!boost::filesystem::exists(profile_root / "Creality.json"))
            profile_root = boost::filesystem::path(resources_dir()) / "profiles";

        PresetBundle project_bundle;
        const auto [preset_substitutions, presets_loaded] = project_bundle.load_vendor_configs_from_json(
            profile_root.string(),
            "Creality",
            PresetBundle::LoadSystem,
            ForwardCompatibilitySubstitutionRule::EnableSystemSilent);
        if (presets_loaded == 0)
            throw Slic3r::RuntimeError("No Creality system presets were loaded");

        if (!input.project_presets.empty())
            project_bundle.load_project_embedded_presets(
                input.project_presets,
                ForwardCompatibilitySubstitutionRule::Enable);

        DynamicPrintConfig project_config;
        project_config.apply(static_cast<const ConfigBase&>(FullPrintConfig::defaults()));
        // DynamicConfig's const-lvalue operator+= does not copy values for
        // keys already present in the destination. Use ConfigBase::apply
        // so project values replace the full-config defaults as intended.
        project_config.apply(m_print_config);
        project_bundle.load_config_model(
            input.creality_project_file,
            std::move(project_config),
            input.creality_project_file_version,
            true);
        m_print_config = project_bundle.full_config();

        input.filament_count = static_cast<int>(
            m_print_config.option<ConfigOptionStrings>("filament_settings_id", true)->values.size());
        const auto serialize_option = [this](const char *key) {
            const ConfigOption *option = m_print_config.option(key);
            return option == nullptr ? std::string("<missing>") : option->serialize();
        };
        BOOST_LOG_TRIVIAL(info) << boost::format(
            "refreshed Creality project with GUI preset merge: root=%1%, print=%2%, wall_generator=%3%, bridge_flow=%4%")
            % profile_root.string()
            % m_print_config.opt_string("print_settings_id", true)
            % serialize_option("wall_generator")
            % serialize_option("bridge_flow");
    } catch (const std::exception &error) {
        // Preserve the historical ability to slice a self-contained project
        // when no matching system package is available.
        BOOST_LOG_TRIVIAL(warning)
            << "Unable to refresh Creality project presets; using embedded settings: "
            << error.what();
    }

    for (Preset *preset : input.project_presets)
        delete preset;
    input.project_presets.clear();
}

void SliceCommand::arrange_model_input(SliceInput &input, const SliceCommandOptions &options)
{
    if (m_models.empty())
        return;

    Slic3r::GUI::PartPlateList &partplate_list = *input.partplate_list;
    const std::set<int> &used_filament_set = input.used_filament_set;
    const double height_to_rod = m_print_config.opt_float("extruder_clearance_height_to_rod");
    const double height_to_lid = m_print_config.opt_float("extruder_clearance_height_to_lid");
    const double clearance_radius = m_print_config.opt_float("extruder_clearance_radius");
    const double print_height = m_print_config.opt_float("printable_height");
    const bool allow_multicolor_oneplate = options.allow_multicolor_oneplate;
    const bool avoid_extrusion_cali_region = options.avoid_extrusion_cali_region;
    Model &model = m_models.front();

    ArrangePolygons selected, unselected, unprintable, locked_aps;
    ArrangeParams arrange_cfg;
    Points beds = get_bed_shape(m_print_config);
    //global arrange
    for (size_t oidx = 0; oidx < model.objects.size(); ++oidx)
    {
        ModelObject* mo = model.objects[oidx];
        for (size_t inst_idx = 0; inst_idx < mo->instances.size(); ++inst_idx)
        {
            ModelInstance* minst = mo->instances[inst_idx];
            ArrangePolygon ap = get_instance_arrange_poly(minst, m_print_config);

            //preprocess by partplate list
            //remove the locked plate's instances, neither in selected, nor in un-selected
            bool locked = partplate_list.preprocess_arrange_polygon(oidx, inst_idx, ap, true);
            if (!locked)
            {
                ap.itemid = selected.size();
                if (minst->printable)
                    selected.emplace_back(ap);
                else
                    unprintable.emplace_back(ap);
            }
            else
            {
                //skip this object due to be locked in plate
                ap.itemid = locked_aps.size();
                locked_aps.emplace_back(ap);
                boost::nowide::cout <<__FUNCTION__ << boost::format(": skip locked instance, obj_id %1%, instance_id %2%") % oidx % inst_idx;
            }
        }
    }

    if (m_print_config.has("print_sequence")) {
        PrintSequence seq = m_print_config.option<ConfigOptionEnum<PrintSequence>>("print_sequence")->value;
        arrange_cfg.is_seq_print = (seq == PrintSequence::ByObject);
    }

    //add the virtual object into unselect list if has
    partplate_list.preprocess_exclude_areas(unselected);

    if (used_filament_set.size() > 0)
    {
        //prepare the wipe tower
        int plate_count = partplate_list.get_plate_count();
        int extruder_size = used_filament_set.size();

        auto printer_structure_opt = m_print_config.option<ConfigOptionEnum<PrinterStructure>>("printer_structure");
        const float tower_brim_width      = m_print_config.option<ConfigOptionFloat>("prime_tower_width", true)->value;
        const float tower_margin          = WIPE_TOWER_MARGIN + tower_brim_width;
        // set the default position, the same with print config(left top)
        float x = WIPE_TOWER_DEFAULT_X_POS;
        float y = WIPE_TOWER_DEFAULT_Y_POS;
        if (printer_structure_opt && printer_structure_opt->value == PrinterStructure::psI3) {
            x = I3_WIPE_TOWER_DEFAULT_X_POS;
            y = I3_WIPE_TOWER_DEFAULT_Y_POS;
        }

        if (x < tower_margin) {
            x = tower_margin;
        }
        if (y < tower_margin) {
            y = tower_margin;
        }
        ConfigOptionFloat wt_x_opt(x);
        ConfigOptionFloat wt_y_opt(y);

        //create the options using default if neccessary
        ConfigOptionFloats* wipe_x_option = m_print_config.option<ConfigOptionFloats>("wipe_tower_x", true);
        ConfigOptionFloats* wipe_y_option = m_print_config.option<ConfigOptionFloats>("wipe_tower_y", true);
        ConfigOptionFloat* width_option = m_print_config.option<ConfigOptionFloat>("prime_tower_width", true);
        ConfigOptionFloat* rotation_angle_option = m_print_config.option<ConfigOptionFloat>("wipe_tower_rotation_angle", true);
        ConfigOptionFloat* volume_option = m_print_config.option<ConfigOptionFloat>("prime_volume", true);

        BOOST_LOG_TRIVIAL(info) << boost::format("prime_tower_width %1% wipe_tower_rotation_angle %2% prime_volume %3%")%width_option->value %rotation_angle_option->value %volume_option->value ;


        for (int bedid = 0; bedid < MAX_PLATE_COUNT; bedid++) {
            int plate_index_valid = std::min(bedid, plate_count - 1);
            if (bedid < plate_count) {
                wipe_x_option->set_at(&wt_x_opt, plate_index_valid, 0);
                wipe_y_option->set_at(&wt_y_opt, plate_index_valid, 0);
            }


            ArrangePolygon wipe_tower_ap = partplate_list.get_plate(plate_index_valid)->estimate_wipe_tower_polygon(m_print_config, plate_index_valid, extruder_size, true);

            wipe_tower_ap.bed_idx = bedid;
            unselected.emplace_back(wipe_tower_ap);
        }
    }
    //Step-2:prepare the arrange params
    arrange_cfg.allow_rotations  = true;
    arrange_cfg.allow_multi_materials_on_same_plate = allow_multicolor_oneplate;
    arrange_cfg.avoid_extrusion_cali_region         = avoid_extrusion_cali_region;
    arrange_cfg.clearance_height_to_rod             = height_to_rod;
    arrange_cfg.clearance_height_to_lid             = height_to_lid;
    arrange_cfg.cleareance_radius                   = clearance_radius;
    arrange_cfg.printable_height                    = print_height;
    arrange_cfg.min_obj_distance = 0;
    if (arrange_cfg.is_seq_print) {
        arrange_cfg.bed_shrink_x = BED_SHRINK_SEQ_PRINT;
        arrange_cfg.bed_shrink_y = BED_SHRINK_SEQ_PRINT;
    }
    if (auto printer_structure_opt = m_print_config.option<ConfigOptionEnum<PrinterStructure>>("printer_structure")) {
        arrange_cfg.align_to_y_axis = (printer_structure_opt->value == PrinterStructure::psI3);
    }

    arrangement::update_arrange_params(arrange_cfg, &m_print_config, selected);
    arrangement::update_selected_items_inflation(selected, &m_print_config, arrange_cfg);
    arrangement::update_unselected_items_inflation(unselected, &m_print_config, arrange_cfg);
    arrangement::update_selected_items_axis_align(selected, &m_print_config, arrange_cfg);

    beds=get_shrink_bedpts(&m_print_config, arrange_cfg);

    partplate_list.preprocess_exclude_areas(arrange_cfg.excluded_regions, 1, scale_(1));

    {
        BOOST_LOG_TRIVIAL(debug) << "arrange bedpts:" << beds[0].transpose() << ", " << beds[1].transpose() << ", " << beds[2].transpose() << ", " << beds[3].transpose();
        BOOST_LOG_TRIVIAL(info)<< "Arrange full params: "<< arrange_cfg.to_json();
        BOOST_LOG_TRIVIAL(info) << boost::format("arrange: items selected before arranging: %1%")%selected.size();
        for (auto item : selected)
            BOOST_LOG_TRIVIAL(trace) << item.name << ", extruder: " << item.extrude_ids.back() << ", bed: " << item.bed_idx
                                    << ", trans: " << item.translation.transpose();
        BOOST_LOG_TRIVIAL(info) << boost::format("arrange: items unselected before arranging: %1%") % unselected.size();
        for (auto item : unselected)
            BOOST_LOG_TRIVIAL(trace) << item.name << ", bed: " << item.bed_idx << ", trans: " << item.translation.transpose();
    }
    arrange_cfg.progressind= [](unsigned st, std::string str = "") {
        //boost::nowide::cout << "st=" << st << ", " << str << std::endl;
    };

    //Step-3:do the arrange
    BOOST_LOG_TRIVIAL(info) << "start arranging...";
    arrangement::arrange(selected, unselected, beds, arrange_cfg);
    arrangement::arrange(unprintable, {}, beds, arrange_cfg);
    BOOST_LOG_TRIVIAL(info) << "finished arranging";

    //Step-4:postprocess by partplate list&&apply the result
    int bed_idx_max = 0;
    //clear all the relations before apply the arrangement results
    partplate_list.clear();

    // Apply the arrange result to all selected objects
    for (ArrangePolygon &ap : selected) {
        //BBS: partplate postprocess
        partplate_list.postprocess_bed_index_for_selected(ap);

        bed_idx_max = std::max(ap.bed_idx, bed_idx_max);
        BOOST_LOG_TRIVIAL(trace)<< "after arrange: name=" << ap.name << boost::format(",bed_id %1%, trans {%2%,%3%}") % ap.bed_idx % unscale<double>(ap.translation(X)) % unscale<double>(ap.translation(Y)) << "\n";
    }
    for (ArrangePolygon &ap : locked_aps) {
        bed_idx_max = std::max(ap.bed_idx, bed_idx_max);

        partplate_list.postprocess_arrange_polygon(ap, false);

        ap.apply();
    }

    // Apply the arrange result to all selected objects
    for (ArrangePolygon &ap : selected) {
        //BBS: partplate postprocess
        partplate_list.postprocess_arrange_polygon(ap, true);

        ap.apply();
    }

    // Apply the arrange result to unselected objects(due to the sukodu-style column changes, the position of unselected may also be modified)
    for (ArrangePolygon& ap : unselected)
    {
        if (ap.is_virt_object)
            continue;

        //BBS: partplate postprocess
        partplate_list.postprocess_arrange_polygon(ap, false);

        ap.apply();
    }

    // Move the unprintable items to the last virtual bed.
    // Note ap.apply() moves relatively according to bed_idx, so we need to subtract the orignal bed_idx
    for (ArrangePolygon& ap : unprintable)
    {
        ap.bed_idx = bed_idx_max + 1;
        partplate_list.postprocess_arrange_polygon(ap, true);

        ap.apply();
        BOOST_LOG_TRIVIAL(debug) << __FUNCTION__ << boost::format(":arrange m_unprintable: name: %4%, bed_id %1%, trans {%2%,%3%}") % ap.bed_idx % unscale<double>(ap.translation(X)) % unscale<double>(ap.translation(Y)) % ap.name;
    }
    // Reload every plate because automatic arrangement changes model positions and plate ownership.
    partplate_list.rebuild_plates_after_arrangement();
}

int SliceCommand::finish_with_error(int error_code)
{
    BOOST_LOG_TRIVIAL(error) << "CLI command failed with error code " << error_code;
#if defined(__linux__) || defined(__LINUX__)
    g_cli_callback_mgr.stop();
#endif
    boost::nowide::cout.flush();
    boost::nowide::cerr.flush();
    for (Model &model : m_models)
        model.remove_backup_path_if_exist();
    return error_code;
}

int SliceCommand::parse_options(SliceCommandOptions &options)
{
    const ConfigOptionInt *log_level = m_config.opt<ConfigOptionInt>("debug");
    set_logging_level(log_level ? log_level->value : 2);

    if (const ConfigOptionString *log_file = m_config.opt<ConfigOptionString>("logfile");
        log_file && !log_file->value.empty())
    {
        set_logging_file(log_file->value);
    }

    std::string error_message;
    const int result = read_slice_cli_options(m_config, m_actions, m_input_files.size(), options, error_message);

    if (result != CLI_SUCCESS)
    {
        BOOST_LOG_TRIVIAL(error) << error_message;
        sliced_info_t empty_sliced_info;
        record_exit_reson(options.outfile_dir, result, 0, CLI::error_message(result), empty_sliced_info);
        return result;
    }

    BOOST_LOG_TRIVIAL(warning) << boost::format("cli mode, Current CrealityPrint Version %1%") % SLIC3R_VERSION;
    BOOST_LOG_TRIVIAL(info) << "Input file count: " << m_input_files.size();
    if (!options.pipe_name.empty()) {
        BOOST_LOG_TRIVIAL(info) << "Will use pipe " << options.pipe_name;
#if defined(__linux__) || defined(__LINUX__)
        g_cli_callback_mgr.start(options.pipe_name);
        cli_status_callback(PrintBase::SlicingStatus {1, "Start to load files"});
#endif
    }
    return CLI_SUCCESS;
}

int SliceCommand::load_input(SliceInput &input, const SliceCommandOptions &options)
{
    input.printer_technology = get_printer_technology(m_config);
    input.plate_to_slice = options.plate_to_slice;
    input.skip_maps = options.skip_maps;

    InputLoadContext load_context
    {
        input.printer_technology,
        input.plate_to_slice,
        options.normative_check,
        options.allow_newer_file,
        options.loaded_filament_ids,
        options.load_filaments,
        options.load_configs,
        ForwardCompatibilitySubstitutionRule::Enable,
        options.outfile_dir,
        input.sliced_info,
        input.plate_data_src,
        input.is_project_input,
        input.is_creality_project_3mf,
        input.creality_project_file,
        input.creality_project_file_version,
        input.project_presets,
        input.current_printer_system_name,
        input.filament_count,
        input.used_filament_set
    };
    const int load_result = load_input_models(load_context);
    if (load_result != CLI_SUCCESS)
        return load_result;

    std::string custom_gcode_error;
    const int custom_gcode_result = load_custom_gcode_file(
        options.custom_gcode_file,
        input.plate_to_slice,
        input.custom_gcodes,
        custom_gcode_error);

    if (custom_gcode_result != CLI_SUCCESS)
    {
        BOOST_LOG_TRIVIAL(error) << custom_gcode_error;
        record_exit_reson(
            options.outfile_dir,
            custom_gcode_result,
            0,
            CLI::error_message(custom_gcode_result),
            input.sliced_info);
    }
    return custom_gcode_result;
}

int SliceCommand::prepare_configuration(SliceInput &input, const SliceCommandOptions &options)
{
    refresh_creality_project_configuration(input);

    if (!input.is_project())
    {
        ModelPresetContext preset_context
        {
            input.printer_technology,
            options.load_configs,
            options.load_filaments,
            options.use_first_filament_as_default,
            ForwardCompatibilitySubstitutionRule::Enable,
            options.outfile_dir,
            input.sliced_info,
            input.filament_count,
            input.new_printer_name
        };

        const int preset_result = load_model_presets(preset_context);
        if (preset_result != CLI_SUCCESS)
            return preset_result;
    }

    // These vectors describe process, filament and printer inheritance. Keep their
    // layout consistent for both project input and standalone model input.
    auto &different_settings =
        m_print_config.option<ConfigOptionStrings>("different_settings_to_system", true)->values;
    auto &inherits_group =
        m_print_config.option<ConfigOptionStrings>("inherits_group", true)->values;
    different_settings.resize(input.filament_count + 2, std::string());
    inherits_group.resize(input.filament_count + 2, std::string());

    const int color_result = update_filament_colors_and_flush(input, options);
    if (color_result != CLI_SUCCESS)
        return color_result;

    return finalize_configuration(input, options);
}

int SliceCommand::finalize_configuration(SliceInput &input, const SliceCommandOptions &options)
{
    PrinterTechnology &printer_technology = input.printer_technology;
    const std::string &outfile_dir = options.outfile_dir;
    sliced_info_t &sliced_info = input.sliced_info;
    std::map<int, CustomGCode::Info> &custom_gcodes_map = input.custom_gcodes;
    //BBS: set default to ptFFF
    if (printer_technology == ptUnknown)
        printer_technology = ptFFF;

    if (m_models.empty())
    {
        record_exit_reson(outfile_dir, CLI_NO_SUITABLE_OBJECTS, 0, CLI::error_message(CLI_NO_SUITABLE_OBJECTS), sliced_info);
        return CLI_NO_SUITABLE_OBJECTS;
    }

    BOOST_LOG_TRIVIAL(info) << "total " << m_models.size() << " models, "
        << m_models.front().objects.size() << " objects" << std::endl;
    if (m_models.size() > 1)
    {
        BOOST_LOG_TRIVIAL(info) << "merge all the models into one\n";
        Model m;
        m.set_backup_path(m_models[0].get_backup_path());
        for (auto& model : m_models)
            for (ModelObject* o : model.objects)
            {
                m.add_object(*o);
            }
        m.add_default_instances();
        m_models.clear();
        m_models.emplace_back(std::move(m));
    }

    //load custom gcodes into model if needed
    if (!custom_gcodes_map.empty())
    {
        m_models[0].plates_custom_gcodes = custom_gcodes_map;
    }

    // Apply command line options to a more specific DynamicPrintConfig which provides normalize()
    // (command line options override --load files)
    m_print_config.apply(m_extra_config, true);
    // Normalizing after importing the 3MFs / AMFs
    m_print_config.normalize_fdm();

    m_print_config.option<ConfigOptionEnum<PrinterTechnology>>("printer_technology", true)->value = printer_technology;

    if (printer_technology != ptFFF)
    {
        boost::nowide::cerr << "invalid printer_technology " << std::endl;
        record_exit_reson(outfile_dir, CLI_INVALID_PRINTER_TECH, 0, CLI::error_message(CLI_INVALID_PRINTER_TECH), sliced_info);
        return CLI_INVALID_PRINTER_TECH;
    }

    FullPrintConfig fff_print_config;
    fff_print_config.apply(m_print_config, true);
    m_print_config.apply(fff_print_config, true);

    std::map<std::string, std::string> validity = m_print_config.validate(true);
    if (!validity.empty())
    {
        boost::nowide::cerr << "Param values in 3mf/config error: "<< std::endl;
        for (std::map<std::string, std::string>::iterator it=validity.begin(); it!=validity.end(); ++it)
            boost::nowide::cerr << it->first <<": "<< it->second << std::endl;
        record_exit_reson(outfile_dir, CLI_INVALID_VALUES_IN_3MF, 0, CLI::error_message(CLI_INVALID_VALUES_IN_3MF), sliced_info);
        return CLI_INVALID_VALUES_IN_3MF;
    }

    return CLI_SUCCESS;
}

int SliceCommand::prepare_plates(SliceInput &input, const SliceCommandOptions &options)
{
    input.partplate_list = std::make_unique<Slic3r::GUI::PartPlateList>(
        nullptr, m_models.data(), input.printer_technology);
    Slic3r::GUI::PartPlateList &partplate_list = *input.partplate_list;

    const Pointfs printable_area = m_print_config.opt<ConfigOptionPoints>("printable_area")->values;
    const Pointfs exclude_area = m_print_config.opt<ConfigOptionPoints>("bed_exclude_area")->values;
    const double printable_height = m_print_config.opt_float("printable_height");
    const double height_to_lid = m_print_config.opt_float("extruder_clearance_height_to_lid");
    const double height_to_rod = m_print_config.opt_float("extruder_clearance_height_to_rod");
    const int printable_width = int(printable_area[2].x() - printable_area[0].x());
    const int printable_depth = int(printable_area[2].y() - printable_area[0].y());

    if (!m_models.empty()) {
        partplate_list.reset_size(
            printable_width,
            printable_depth,
            int(printable_height),
            false);
        partplate_list.set_shapes(
            printable_area, exclude_area, std::string(), height_to_lid, height_to_rod);
    }

    // A project restores its saved plate ownership and positions. Standalone models
    // have no plate structure and will be arranged after the common validation below.
    if (input.is_project() && !input.plate_data_src.empty())
        partplate_list.load_from_3mf_structure(input.plate_data_src);

#if defined(__linux__) || defined(__LINUX__)
    if (g_cli_callback_mgr.is_started()) {
        PrintBase::SlicingStatus slicing_status{2, "Loading files finished"};
        cli_status_callback(slicing_status);
    }
#endif

    const int plate_to_slice = input.plate_to_slice;
    if (plate_to_slice < 0 || plate_to_slice > partplate_list.get_plate_count()) {
        BOOST_LOG_TRIVIAL(error) << boost::format("invalid plate id %1%, total %2%")
            % plate_to_slice % partplate_list.get_plate_count();
        record_exit_reson(
            options.outfile_dir,
            CLI_INVALID_PARAMS,
            0,
            CLI::error_message(CLI_INVALID_PARAMS),
            input.sliced_info);
        return CLI_INVALID_PARAMS;
    }

    // Filament color overrides make the thumbnails embedded in a project stale.
    if (input.filament_color_changed)
    {
        for (int index = 0; index < partplate_list.get_plate_count(); ++index)
        {
            if (plate_to_slice != 0 && plate_to_slice != index + 1)
                continue;

            if (input.plate_data_src.size() <= size_t(index))
                continue;

            PlateData &plate_data = *input.plate_data_src[index];
            if (!plate_data.thumbnail_file.empty())
            {
                BOOST_LOG_TRIVIAL(info) << boost::format("Plate %1%: clear loaded thumbnail %2%.")
                    % (index + 1) % plate_data.thumbnail_file;
                plate_data.thumbnail_file.clear();
            }

            if (!plate_data.no_light_thumbnail_file.empty())
            {
                BOOST_LOG_TRIVIAL(info) << boost::format("Plate %1%: clear loaded no_light_thumbnail %2%.")
                    % (index + 1) % plate_data.no_light_thumbnail_file;
                plate_data.no_light_thumbnail_file.clear();
            }

            if (!plate_data.top_file.empty())
            {
                BOOST_LOG_TRIVIAL(info) << boost::format("Plate %1%: clear loaded top_thumbnail %2%.")
                    % (index + 1) % plate_data.top_file;
                plate_data.top_file.clear();
            }

            if (!plate_data.pick_file.empty())
            {
                BOOST_LOG_TRIVIAL(info) << boost::format("Plate %1%: clear loaded pick_thumbnail %2%.")
                    % (index + 1) % plate_data.pick_file;
                plate_data.pick_file.clear();
            }
        }
    }

    if (input.is_project())
        return CLI_SUCCESS;

    arrange_model_input(input, options);
    return CLI_SUCCESS;
}

int SliceCommand::execute(SliceInput &input, const SliceCommandOptions &options)
{
    if (!input.partplate_list)
        return CLI_INVALID_PARAMS;

    const long long slicing_begin = (long long) Slic3r::Utils::get_current_time_utc();
    input.sliced_info.prepare_time = size_t(slicing_begin - input.started_at);

    SliceExecutionContext slice_context
    {
        *input.partplate_list,
        input.plate_to_slice,
        input.skip_maps,
        options.max_triangle_count_per_plate,
        options.max_slicing_time_per_plate,
        options.no_check,
        input.filament_count,
        options.load_slicedata,
        options.load_slice_data_dir,
        options.export_slicedata,
        options.export_slice_data_dir,
        options.outfile_dir,
        options.diagnostic_mode,
        options.need_gcode_file,
        input.fingerprint_report,
        input.plate_data_src,
        input.sliced_info,
        input.new_printer_name,
        input.current_printer_system_name
    };

    BOOST_LOG_TRIVIAL(info)
        << (input.is_project()
            ? "execute 3MF project slicing command"
            : "execute STL/OBJ model slicing command");
    const int result = execute_plate_slicing(slice_context);
    if (result != CLI_SUCCESS)
        return result;

    const long long export_begin = (long long) Slic3r::Utils::get_current_time_utc();
    if (!input.plate_data_src.empty())
        release_PlateData_list(input.plate_data_src);

#if defined(__linux__) || defined(__LINUX__)
    if (g_cli_callback_mgr.is_started())
        cli_status_callback(PrintBase::SlicingStatus {100, "All done, Success"});
    g_cli_callback_mgr.stop();
#endif

    for (Model &model : m_models)
        model.remove_backup_path_if_exist();

    input.sliced_info.export_time = size_t((long long) Slic3r::Utils::get_current_time_utc() - export_begin);

    record_exit_reson(
        options.outfile_dir,
        CLI_SUCCESS,
        input.plate_to_slice,
        CLI::error_message(CLI_SUCCESS),
        input.sliced_info);

    boost::nowide::cout.flush();
    boost::nowide::cerr.flush();
    return CLI_SUCCESS;
}

int SliceCommand::run()
{
    SliceCommandOptions options;
    int result = parse_options(options);
    if (result != CLI_SUCCESS)
        return finish_with_error(result);

    SliceInput input;
    result = load_input(input, options);
    if (result != CLI_SUCCESS)
        return finish_with_error(result);

    result = prepare_configuration(input, options);
    if (result != CLI_SUCCESS)
        return finish_with_error(result);

    result = prepare_plates(input, options);
    if (result != CLI_SUCCESS)
        return finish_with_error(result);

    result = execute(input, options);
    if (result != CLI_SUCCESS)
        return finish_with_error(result);
    return CLI_SUCCESS;
}
