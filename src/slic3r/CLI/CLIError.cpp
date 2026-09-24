#include "CLIError.hpp"

#include "libslic3r/Utils.hpp"

namespace Slic3r {
namespace CLI {

std::string error_message(int error_code)
{
    switch (error_code) {
    case CLI_SUCCESS:
        return "Success.";
    case CLI_ENVIRONMENT_ERROR:
        return "Failed setting up server environment.";
    case CLI_INVALID_PARAMS:
        return "Invalid parameters to the slicer.";
    case CLI_FILE_NOTFOUND:
        return "The input files to the slicer are not found.";
    case CLI_FILELIST_INVALID_ORDER:
        return "File list order to the slicer is invalid. Please make sure the 3mf in the first place.";
    case CLI_CONFIG_FILE_ERROR:
        return "The input preset file is invalid and can not be parsed.";
    case CLI_DATA_FILE_ERROR:
        return "The input model file to the slicer can not be parsed.";
    case CLI_INVALID_PRINTER_TECH:
        return "Unsupported printer technology (not FDM).";
    case CLI_UNSUPPORTED_OPERATION:
        return "Unsupported CLI instruction.";
    case CLI_OUT_OF_MEMORY:
        return "Out of memory during slicing. Please upload a model with lower geometry resolution and try again.";
    case CLI_PROCESS_NOT_COMPATIBLE:
        return "The selected printer is not compatible with the process preset in the 3mf.";
    case CLI_INVALID_VALUES_IN_3MF:
        return "Invalid parameter value(s) included in the 3mf file.";
    case CLI_POSTPROCESS_NOT_SUPPORTED:
        return "post_process is not supported under CLI.";
    case CLI_OBJECT_ARRANGE_FAILED:
        return "An error occurred when auto-arranging object(s).";
    case CLI_FILE_VERSION_NOT_SUPPORTED:
        return "Unsupported 3MF version. Please make sure the 3MF file was created with the official version of Bambu Studio, not a beta version.";
    case CLI_NO_SUITABLE_OBJECTS:
        return "One of the plate is empty or has no object fully inside it. Please check that the 3mf contains no empty plate in Creality Print before uploading.";
    case CLI_VALIDATE_ERROR:
        return "There are some incorrect slicing parameters in the 3mf. Please verify the slicing of all plates in Creality Print before uploading.";
    case CLI_OBJECTS_PARTLY_INSIDE:
        return "Some objects are located over the boundary of the heated bed.";
    case CLI_EXPORT_CACHE_DIRECTORY_CREATE_FAILED:
        return "Failed creating directory when exporting cache data.";
    case CLI_EXPORT_CACHE_WRITE_FAILED:
        return "Failed exporting cache data.";
    case CLI_IMPORT_CACHE_NOT_FOUND:
        return "Cache data not found.";
    case CLI_IMPORT_CACHE_DATA_CAN_NOT_USE:
        return "Cache data can not be parsed.";
    case CLI_IMPORT_CACHE_LOAD_FAILED:
        return "Failed importing cache data.";
    case CLI_SLICING_TIME_EXCEEDS_LIMIT:
        return "Slicing time of a certain plate exceeds the limit. Please simplify the model or use a larger slicing layer height.";
    case CLI_TRIANGLE_COUNT_EXCEEDS_LIMIT:
        return "Triangle count of single plate exceeds the limit. Please simplify the model and try to upload again.";
    case CLI_NO_SUITABLE_OBJECTS_AFTER_SKIP:
        return "No printable objects to slice after skipping.";
    case CLI_FILAMENT_NOT_MATCH_BED_TYPE:
        return "Filaments are not compatible with the plate type. Please verify the slicing of all plates in Creality Print before uploading.";
    case CLI_FILAMENTS_DIFFERENT_TEMP:
        return "The temperature difference of the filaments used is too large. Please verify the slicing of all plates in Creality Print before uploading.";
    case CLI_OBJECT_COLLISION_IN_SEQ_PRINT:
        return "Object conflicts were detected when using print-by-object mode. Please verify the slicing of all plates in Creality Print before uploading.";
    case CLI_OBJECT_COLLISION_IN_LAYER_PRINT:
        return "Object conflicts were detected. Please verify the slicing of all plates in Creality Print before uploading.";
    case CLI_SPIRAL_MODE_INVALID_PARAMS:
        return "Some slicing parameters cannot work with Spiral Vase mode. Please solve the issue in Creality Print before uploading.";
    case CLI_SLICING_ERROR:
        return "Failed slicing the model. Please verify the slicing of all plates on Creality Print before uploading.";
    case CLI_GCODE_PATH_CONFLICTS:
        return " G-code conflicts detected after slicing. Please make sure the 3mf file can be successfully sliced in the latest Creality Print.";
    default:
        return "Unknown CLI error (" + std::to_string(error_code) + ").";
    }
}

} // namespace CLI
} // namespace Slic3r
