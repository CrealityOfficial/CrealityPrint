#ifndef slic3r_CLI_SliceCommand_hpp_
#define slic3r_CLI_SliceCommand_hpp_

#include "libslic3r/Config.hpp"
#include "libslic3r/Model.hpp"

#include <string>
#include <vector>

namespace Slic3r {

struct SliceCommandOptions;
struct SliceInput;
struct SliceExecutionContext;
struct InputLoadContext;
struct ModelPresetContext;

class SliceCommand
{
public:
    SliceCommand(
        DynamicPrintAndCommandLineConfig &config,
        DynamicPrintConfig &print_config,
        DynamicPrintConfig &extra_config,
        const std::vector<std::string> &input_files,
        const std::vector<std::string> &actions,
        std::vector<Model> &models);

    int run();

private:
    int finish_with_error(int error_code);
    int parse_options(SliceCommandOptions &options);

    int load_input(SliceInput &input, const SliceCommandOptions &options);
    int load_input_models(InputLoadContext &context);

    int prepare_configuration(SliceInput &input, const SliceCommandOptions &options);
    void refresh_creality_project_configuration(SliceInput &input);
    int load_model_presets(ModelPresetContext &context);
    int update_filament_colors_and_flush(SliceInput &input, const SliceCommandOptions &options);
    int finalize_configuration(SliceInput &input, const SliceCommandOptions &options);

    int prepare_plates(SliceInput &input, const SliceCommandOptions &options);
    void arrange_model_input(SliceInput &input, const SliceCommandOptions &options);

    int execute(SliceInput &input, const SliceCommandOptions &options);
    int execute_plate_slicing(SliceExecutionContext &context);

    DynamicPrintAndCommandLineConfig &m_config;
    DynamicPrintConfig &m_print_config;
    DynamicPrintConfig &m_extra_config;
    const std::vector<std::string> &m_input_files;
    const std::vector<std::string> &m_actions;
    std::vector<Model> &m_models;
};

} // namespace Slic3r

#endif // slic3r_CLI_SliceCommand_hpp_
