import os
import sys, getopt
import tempfile
import shutil
import ParamPackUtil
import json
from PIL import Image
from datetime import datetime

DEPRECATED_FILAMENT_KEYS = {
    "dont_slow_down_outer_wall",
}

DEPRECATED_PROCESS_KEYS = {
    "wall_filament",
    "solid_infill_filament",
    "sparse_infill_filament",
}

FILAMENT_VARIANT_KEYS = {
    "filament_flow_ratio", "filament_max_volumetric_speed", "filament_ramming_volumetric_speed",
    "filament_pre_cooling_temperature", "filament_ramming_travel_time", "filament_ramming_volumetric_speed_nc",
    "filament_pre_cooling_temperature_nc", "filament_ramming_travel_time_nc", "filament_retraction_length",
    "filament_retract_length_nc", "filament_z_hop", "filament_z_hop_types", "filament_retract_restart_extra",
    "filament_retract_lift_above", "filament_retract_lift_below", "filament_retract_lift_enforce",
    "filament_retraction_speed", "filament_deretraction_speed", "filament_retraction_minimum_travel",
    "filament_retract_when_changing_layer", "filament_wipe", "filament_wipe_distance",
    "filament_retract_before_wipe", "filament_long_retractions_when_cut",
    "filament_retraction_distances_when_cut", "filament_retract_length_toolchange",
    "filament_retract_restart_extra_toolchange", "nozzle_temperature_initial_layer", "nozzle_temperature",
    "filament_flush_volumetric_speed", "filament_flush_temp", "filament_flush_temp_fast",
    "filament_enable_overhang_speed", "filament_bridge_speed", "filament_overhang_1_4_speed",
    "filament_overhang_2_4_speed", "filament_overhang_3_4_speed", "filament_overhang_4_4_speed",
    "filament_overhang_totally_speed", "override_process_overhang_speed", "volumetric_speed_coefficients",
    "filament_adaptive_volumetric_speed", "filament_preheat_temperature_delta", "slow_down_min_speed",
}

LEGACY_FILAMENT_EXTRUDER_VARIANTS = ["Direct Drive Standard"] * 4
LEGACY_FILAMENT_NOZZLE_VARIANTS = ["0", "1", "2", "3"]
MULTI_EXTRUDER_NAME_EXCEPTIONS = {"Sermoon D3 Pro"}

def machine_definition_path(printer, package_path):
    filename = printer["printerIntName"]
    for nozzle_diameter in printer["nozzleDiameter"]:
        filename += "-" + nozzle_diameter
    return os.path.join(package_path, filename + ".def.json")

def uses_multi_extruder_variant_schema(printer, machine_definition):
    nozzle_diameters = printer.get("nozzleDiameter")
    variant_list = machine_definition.get("printer", {}).get("extruder_variant_list")
    return (isinstance(nozzle_diameters, (list, tuple)) and
            len(nozzle_diameters) > 1 and
            isinstance(variant_list, list) and
            bool(variant_list))

def machine_profile_name(printer):
    machine_name = printer["name"].strip()
    if uses_bare_multi_extruder_profile_name(printer):
        return machine_name
    return nozzle_qualified_machine_profile_name(printer)

def uses_bare_multi_extruder_profile_name(printer):
    nozzle_diameters = printer.get("nozzleDiameter")
    return (isinstance(nozzle_diameters, (list, tuple)) and
            len(nozzle_diameters) > 1 and
            printer.get("printerIntName") not in MULTI_EXTRUDER_NAME_EXCEPTIONS)

def nozzle_qualified_machine_profile_name(printer):
    return (printer["name"].strip() + " " +
            printer["nozzleDiameter"][0] + " nozzle")

def parameter_package_path(printer, package_root):
    package_name = printer["printerIntName"]
    for nozzle_diameter in printer["nozzleDiameter"]:
        package_name += "-" + nozzle_diameter
    return os.path.join(package_root, package_name)

def select_printers_to_generate(printer_list, package_root):
    """Select one source package for each nozzle-less multi-extruder profile.

    The backend may publish one package per installed nozzle diameter.  Those
    packages intentionally share one slicer profile name, so generating every
    package would overwrite the same machine, process and filament files.  A
    package carrying the complete variant schema is canonical; older data
    falls back to the standard 0.4 mm package.
    """
    grouped_candidates = {}
    for printer in printer_list:
        if uses_bare_multi_extruder_profile_name(printer):
            grouped_candidates.setdefault(machine_profile_name(printer), []).append(printer)

    selected_ids = set()
    for profile_name, candidates in grouped_candidates.items():
        available = [
            printer for printer in candidates
            if os.path.isdir(parameter_package_path(printer, package_root))
        ]
        ranked_candidates = available or candidates

        def candidate_rank(printer):
            has_variant_schema = False
            package_path = parameter_package_path(printer, package_root)
            definition_path = machine_definition_path(printer, package_path)
            if os.path.isfile(definition_path):
                try:
                    with open(definition_path, 'r', encoding='utf-8') as file:
                        has_variant_schema = uses_multi_extruder_variant_schema(
                            printer, json.load(file)
                        )
                except (OSError, ValueError, TypeError, KeyError) as error:
                    print(f"warning: failed to inspect {definition_path}: {error}")

            nozzle_diameters = printer.get("nozzleDiameter") or []
            is_standard_nozzle = bool(nozzle_diameters) and nozzle_diameters[0] == "0.4"
            return has_variant_schema, is_standard_nozzle

        selected = max(ranked_candidates, key=candidate_rank)
        selected_ids.add(id(selected))
        if len(candidates) > 1:
            print(
                f"multi-extruder profile {profile_name}: selected "
                f"{os.path.basename(parameter_package_path(selected, package_root))} "
                f"from {len(candidates)} packages"
            )

    return [
        printer for printer in printer_list
        if (not uses_bare_multi_extruder_profile_name(printer) or
            id(printer) in selected_ids)
    ]

def normalize_multi_extruder_filament_variants(filament_data):
    """Normalize material values to the selector rows supplied by the ZIP.

    The cloud backend stores material defaults in physical-extruder-major
    order, even though material presets are selected only by extruder/nozzle
    variant.  Fold those duplicated physical-extruder rows to the first row
    before writing the slicer preset.
    """
    extruder_variants = filament_data.get("filament_extruder_variant")
    nozzle_variants = filament_data.get("filament_nozzle_variant")

    if extruder_variants is None and nozzle_variants is None:
        # Keep older multi-extruder packages working until their material
        # selectors are provided by the backend.
        extruder_variants = LEGACY_FILAMENT_EXTRUDER_VARIANTS.copy()
        nozzle_variants = LEGACY_FILAMENT_NOZZLE_VARIANTS.copy()
    elif (not isinstance(extruder_variants, list) or
          not isinstance(nozzle_variants, list) or
          not extruder_variants or
          len(extruder_variants) != len(nozzle_variants)):
        raise ValueError(
            "Multi-extruder material selectors filament_extruder_variant and "
            "filament_nozzle_variant must be non-empty arrays of equal length"
        )

    variant_count = len(nozzle_variants)
    for key in FILAMENT_VARIANT_KEYS:
        if key not in filament_data:
            continue
        value = filament_data[key]
        if not isinstance(value, list):
            filament_data[key] = [value] * variant_count
        elif len(value) == 1:
            filament_data[key] = value * variant_count
        elif len(value) != variant_count:
            if len(value) > variant_count and len(value) % variant_count == 0:
                canonical_row = value[:variant_count]
                physical_rows = [
                    value[offset:offset + variant_count]
                    for offset in range(0, len(value), variant_count)
                ]
                if any(row != canonical_row for row in physical_rows[1:]):
                    print(
                        f"warning: multi-extruder material option {key} has conflicting "
                        "physical-extruder rows; using the first row"
                    )
                filament_data[key] = canonical_row
            elif value and all(item == value[0] for item in value):
                filament_data[key] = [value[0]] * variant_count
            else:
                raise ValueError(
                    f"Multi-extruder material option {key} has {len(value)} values, "
                    f"expected 1 or {variant_count}"
                )

    filament_data["filament_extruder_variant"] = extruder_variants
    filament_data["filament_nozzle_variant"] = nozzle_variants
    return filament_data

def delete_json_folder(directory_path):
    print(directory_path)
    for file_name in os.listdir(directory_path):
        file_path = os.path.join(directory_path, file_name)
        if file_name.endswith('.json') and os.path.isfile(file_path) and not file_name.startswith("fdm_") and file_name != "filaments_color.json":
            os.remove(file_path)
def delete_png_folder(directory_path):
    for file_name in os.listdir(directory_path):
        file_path = os.path.join(directory_path, file_name)
        if file_name.endswith('_cover.png') and os.path.isfile(file_path):
            os.remove(file_path)
def make_parameter_package(server_id, engine_version):
    working_path = working_path_from_ci(sys.path[0])
    print(f"make_parameter_package {working_path}")
    out_path = os.path.join(working_path,"..","..","resources","profiles", f"Creality") 
    if not os.path.exists(out_path):
        os.makedirs(out_path)
    
    if os.path.exists(os.path.join(out_path,"machine")):
        delete_json_folder(os.path.join(out_path,"machine"))

    if os.path.exists(os.path.join(out_path,"filament")):
        delete_json_folder(os.path.join(out_path,"filament"))

    if os.path.exists(os.path.join(out_path,"process")):
        delete_json_folder(os.path.join(out_path,"process"))

    delete_png_folder(out_path)

    machine_json_file = os.path.join(os.path.join(working_path,f"server_{server_id}","orca","default"),"machineList.json")
    creality_json_file = os.path.join(os.path.join(working_path,"..","..","resources","profiles"),f"Creality.json")
    shutil.copy(machine_json_file, os.path.join(out_path,"machineList.json"))
    
    # Parse machineList.json
    with open(machine_json_file, 'r', encoding='utf-8') as file:
        machine_data = json.load(file)

    creality_list = []
    for printer in machine_data.get('printerList', []):
        printer_name = printer["name"].strip()
        if printer_name.find("Creality") == -1:
            if printer_name.find("SPARKX") >= 0:
                printer_name = printer_name
            else:
                printer_name = "Creality "+printer_name
        creality_list.append({
            "name": printer_name,
            "showVersion": printer.get("showVersion"),
            "nozzleDiameter": printer.get("nozzleDiameter")
        })

    # Save to profile_version.json
    profile_version_file = os.path.join(out_path, "profile_version.json")
    with open(profile_version_file, 'w', encoding='utf-8') as file:
        json.dump({"Creality": creality_list}, file, indent=4)
    
    creality_data = {
        "name": "Creality",
        "version": "02.01.03.00",
        "force_update": "1",
        "description": "Creality configurations",
        "machine_list": [],
        "machine_model_list": [],
        "filament_list": [],
        "process_list": []

    }
    creality_data["version"] = datetime.now().strftime('%y.%m.%d.%H')
    creality_data["machine_list"] = [
        {
            "name": "fdm_machine_common",
            "sub_path": "machine/fdm_machine_common.json"
        },
        {
            "name": "fdm_creality_common",
            "sub_path": "machine/fdm_creality_common.json"
        }
        
    ]
    creality_data["process_list"] = [
        {
            "name": "fdm_process_common",
            "sub_path": "process/fdm_process_common.json"
        },
        {
            "name": "fdm_process_creality_common",
            "sub_path": "process/fdm_process_creality_common.json"
        },
        {
            "name": "fdm_process_common_klipper",
            "sub_path": "process/fdm_process_common_klipper.json"
        },
        {
            "name": "fdm_process_creality_common_0_2",
            "sub_path": "process/fdm_process_creality_common_0_2.json"
        },
        {
            "name": "fdm_process_creality_common_0_25",
            "sub_path": "process/fdm_process_creality_common_0_25.json"
        },
        {
            "name": "fdm_process_creality_common_0_3",
            "sub_path": "process/fdm_process_creality_common_0_3.json"
        },
        {
            "name": "fdm_process_creality_common_0_5",
            "sub_path": "process/fdm_process_creality_common_0_5.json"
        },
        {
            "name": "fdm_process_creality_common_0_6",
            "sub_path": "process/fdm_process_creality_common_0_6.json"
        },
        {
            "name": "fdm_process_creality_common_0_8",
            "sub_path": "process/fdm_process_creality_common_0_8.json"
        },
        {
            "name": "fdm_process_creality_common_1_0",
            "sub_path": "process/fdm_process_creality_common_1_0.json"
        }
    ]
    creality_data["filament_list"] = [
        {
            "name": "fdm_filament_common",
            "sub_path": "filament/fdm_filament_common.json"
        },
        {
            "name": "fdm_filament_abs",
            "sub_path": "filament/fdm_filament_abs.json"
        },
        {
            "name": "fdm_filament_asa",
            "sub_path": "filament/fdm_filament_asa.json"
        },
        {
            "name": "fdm_filament_pa",
            "sub_path": "filament/fdm_filament_pa.json"
        },
        {
            "name": "fdm_filament_pc",
            "sub_path": "filament/fdm_filament_pc.json"
        },
        {
            "name": "fdm_filament_pet",
            "sub_path": "filament/fdm_filament_pet.json"
        },
        {
            "name": "fdm_filament_pla",
            "sub_path": "filament/fdm_filament_pla.json"
        },
        {
            "name": "fdm_filament_pp",
            "sub_path": "filament/fdm_filament_pp.json"
        },
        {
            "name": "fdm_filament_pps",
            "sub_path": "filament/fdm_filament_pps.json"
        },
        {
            "name": "fdm_filament_pva",
            "sub_path": "filament/fdm_filament_pva.json"
        },
        {
            "name": "fdm_filament_petg",
            "sub_path": "filament/fdm_filament_petg.json"
        },
        {
            "name": "fdm_filament_hips",
            "sub_path": "filament/fdm_filament_hips.json"
        },
        {
            "name": "fdm_filament_tpu",
            "sub_path": "filament/fdm_filament_tpu.json"
        }
    ]
    with open(machine_json_file, 'r',encoding='utf-8') as file:
        data = json.load(file)
        printerList = data["printerList"]
        default_materials_map = {}
        for printer in printerList:
            printer_name = printer["name"].strip()
            if printer_name.find("Creality") == -1:
                if printer_name.find("SPARKX") >= 0:
                    printer_name = printer_name
                else:
                    printer_name = "Creality "+printer_name
            printer["name"] = printer_name

        package_root = os.path.join(working_path, f"server_{server_id}", "orca", "default", "parampack")
        printers_to_generate = select_printers_to_generate(printerList, package_root)
        for printer in printers_to_generate:
            printer_name = printer["name"]
            #print(f"---------------------process printer {printer_name}")
            param_pack_dir = parameter_package_path(printer, package_root)
            if printer["printerIntName"] == "Bambu Lab A1":
                continue
            # if printer["printerIntName"] != "Sermoon D3 Pro":
            #    continue
            if os.path.exists(param_pack_dir):
                machine_path,process_path,filament_path,default_materials,bed_type = process_param_pack(printer,param_pack_dir, out_path)
                
                printer["bed_type"] = bed_type
                if len(default_materials) > 0:
                    if printer_name in default_materials_map:
                        if len(default_materials)>len(default_materials_map[printer_name]) :
                            default_materials_map[printer_name] = default_materials
                    else:
                        default_materials_map[printer_name] = default_materials
                else:
                    if printer_name not in default_materials_map:
                        default_materials_map[printer_name] = ["Hyper PLA"]
                creality_data["machine_list"] = creality_data["machine_list"] + machine_path
                creality_data["filament_list"] = creality_data["filament_list"]+filament_path
                creality_data["process_list"] = creality_data["process_list"] + process_path
                 #copy thumbnail
                img = Image.open(os.path.join(working_path,f"server_{server_id}","orca","default","machineImages",printer["printerIntName"]+".png"))
                img.save(os.path.join(out_path,printer_name+"_cover.png"))
                #shutil.copy2(os.path.join(working_path,f"server_{server_id}","orca","default","machineImages",printer["printerIntName"]+".png"), os.path.join(out_path,printer["name"]+"_cover.png"))
                #break
        file.close()
        machine_model_path = process_machine_model_json(printerList, default_materials_map, out_path)
        creality_data["machine_model_list"] = creality_data["machine_model_list"] + machine_model_path
    with open(creality_json_file, 'w', encoding='utf-8') as f:
        json.dump(creality_data, f, ensure_ascii=False, indent=4)
    #for root, dirs, files in os.walk(os.path.join(working_path,"server_{server_id}","orca","default")):
    #    for dir in dirs:
    #        process_param_pack(dir, out_path)
    pass

def process_machine_model_json(printerList, default_materials_map, out_path):
    sub_paths = []    
    working_path = working_path_from_ci(sys.path[0])
    out_path = os.path.join(working_path,"..","..","resources","profiles", f"Creality") 
    machine_model_data = {
        "type": "machine_model",
        "name": "Creality CR-6 Max",
        "nozzle_diameter": "0.2;0.4;0.6;0.8",
        "bed_model": "",
        "default_bed_type": "Textured PEI Plate",
        "bed_texture": "",
        "family": "Creality",
        "hotend_model": "",
        "machine_tech": "FFF",
        "model_id": "Creality_CR_6_Max"
    }
    printerDict = {}
    for printer in printerList:
        if printer["name"] in printerDict:
            printerDict[printer["name"]].append(printer)
        else:
            printerDict[printer["name"]] = [printer]
        
        
    for printerIntName in printerDict:
        if printerIntName not in default_materials_map:
            continue
        printerList = printerDict[printerIntName]
        machine_model_data["name"] = printerIntName
        machine_model_data["model_id"] = printerIntName.replace(" ","_").replace("-","_")
        machine_model_data["default_materials"] = ';'.join(map(str, default_materials_map[printerIntName]))
        if os.path.exists(os.path.join(out_path,f"{printerIntName}_buildplate_model.stl")):
            machine_model_data["bed_model"] = f"{printerIntName}_buildplate_model.stl"
        elif printerIntName.find("SPARKX") != -1:
            machine_model_data["bed_model"] = "Creality F022_buildplate_model.stl"
        else:
            machine_model_data["bed_model"] = "creality_k1_buildplate_model.stl"
        if os.path.exists(os.path.join(out_path,f"{printerIntName}_buildplate_texture.png")):
            machine_model_data["bed_texture"] = f"{printerIntName}_buildplate_texture.png"
        elif printerIntName.find("SPARKX") != -1:
            machine_model_data["bed_texture"] = ""
        else:
            machine_model_data["bed_texture"] = ""
        
        nozzle_diameter = []
        for printer in printerList:
            nozzle_diameter.append(printer["nozzleDiameter"][0])
            if "bed_type" in printer:
                machine_model_data["default_bed_type"] = printer["bed_type"]
        machine_model_data["nozzle_diameter"] = ";".join(nozzle_diameter)
        print("--------------------",machine_model_data["default_bed_type"])
        machine_model_file_name = printerIntName + "_model"
        out_machine_model_json_file = os.path.join(out_path,"machine",machine_model_file_name+".json")
        with open(out_machine_model_json_file, 'w', encoding='utf-8') as f:
            json.dump(machine_model_data, f, ensure_ascii=False, indent=4)
        sub_paths.append({
            "name": printerIntName,
            "sub_path": os.path.join("machine",machine_model_file_name+".json").replace("\\","/")
        })
       
    return sub_paths
def process_process_json(printer,package_path, out_path):
    sub_paths = []
    for root, dirs, files in os.walk(os.path.join(package_path,"Processes")):
        for filename in files:
            process_data = {
                "type": "process",
                "setting_id": "GP004",
                "name": "0.08mm SuperDetail @Creality CR-6 0.2",
                "from": "system",
                "instantiation": "true",
                "inherits": "fdm_process_common_klipper"
                
            }
            process_json_file = os.path.join(root,filename)
            with open(process_json_file, 'r',encoding='utf-8') as file:
                data = json.load(file)
                process_data.update(data["engine_data"])
                basename = os.path.splitext(filename)[0]
                printer_name = machine_profile_name(printer)
                process_name = basename[0:basename.find("@")].strip() if "@" in basename else basename.strip()
                basename = process_name+" @"+printer_name
                process_data["name"] = basename
                process_data["compatible_printers"] = [printer_name]
                process_data["inherits"] = "fdm_process_creality_common"
                if "min_length_factor" in process_data and process_data["min_length_factor"] == "":
                    del process_data["min_length_factor"]
                #filament_colour, flush_multiplier, flush_volumes_matrix, flush_volumes_vector, has_scarf_joint_seam, wipe_tower_x, wipe_tower_y
                if "filament_colour" in process_data:
                    del process_data["filament_colour"]
                if "flush_multiplier" in process_data:
                    del process_data["flush_multiplier"]
                if "flush_volumes_matrix" in process_data:
                    del process_data["flush_volumes_matrix"]
                if "flush_volumes_vector" in process_data:
                    del process_data["flush_volumes_vector"]
                if "wipe_tower_x" in process_data:
                    del process_data["wipe_tower_x"]
                if "wipe_tower_y" in process_data:
                    del process_data["wipe_tower_y"]
                if "has_scarf_joint_seam" in process_data:
                    del process_data["has_scarf_joint_seam"]
                if "arc_tolerance" in process_data:
                    del process_data["arc_tolerance"]
                for key in DEPRECATED_PROCESS_KEYS:
                    process_data.pop(key, None)
                if "ensure_vertical_shell_thickness" in process_data:
                    if process_data["ensure_vertical_shell_thickness"] == "1" or process_data["ensure_vertical_shell_thickness"] == "true":
                        process_data["ensure_vertical_shell_thickness"] = "ensure_all"
                    if process_data["ensure_vertical_shell_thickness"] == "0" or process_data["ensure_vertical_shell_thickness"] == "false":
                        process_data["ensure_vertical_shell_thickness"] = "none"    
                #删除空的key
                for key in list(process_data.keys()):
                    if process_data[key] == "":
                        del process_data[key]
                out_process_json_file = os.path.join(out_path,"process",basename+".json")
                with open(out_process_json_file, 'w', encoding='utf-8') as f:
                    json.dump(process_data, f, ensure_ascii=False, indent=4)
                sub_paths.append({
                    "name": basename,
                    "sub_path": os.path.join("process",basename+".json").replace("\\","/")
                })
    return sub_paths

def get_filament_type(name):
    return ""
def process_filament_json(printer,package_path, out_path):
    array_keys = ["filament_type", "filament_vendor", "filament_start_gcode", "filament_end_gcode"]
    sub_paths = []
    filament_map = {}
    with open(machine_definition_path(printer, package_path), 'r', encoding='utf-8') as file:
        machine_definition = json.load(file)
    normalize_variant_values = uses_multi_extruder_variant_schema(printer, machine_definition)
    for root, dirs, files in os.walk(os.path.join(package_path,"Materials")):
        for filename in files:
            filament_data = {
                "type": "filament",
                "filament_id": "GFB98",
                "setting_id": "GFSA04",
                "name": "Creality Generic ASA",
                "from": "system",
                "instantiation": "true",
                "inherits": "fdm_filament_common"
            }
            filament_json_file = os.path.join(root,filename)
            with open(filament_json_file, 'r',encoding='utf-8') as file:
                data = json.load(file)
                filament_data.update(data["engine_data"])
                for key in DEPRECATED_FILAMENT_KEYS:
                    filament_data.pop(key, None)
                filament_data["filament_id"] = data["metadata"]["id"]
                printer_profile_name = machine_profile_name(printer)
                basename = os.path.splitext(filename)[0]
                basename = basename[0:basename.rfind("-")]
                basename = basename.strip()+" @"+printer_profile_name
                filament_data["name"] = basename
                filament_map[filament_data["filament_id"]] = basename

                #处理特殊的key
                filament_data["compatible_printers"] = [printer_profile_name]
                filament_type = filament_data["filament_type"]
                print("filament_type:"+filament_type)
                if filament_type == "PLA" or filament_type == "PLA-CF":
                    filament_data["inherits"] = "fdm_filament_pla"
                elif filament_type == "PETG" or filament_type == "PETG-CF":
                    filament_data["inherits"] = "fdm_filament_petg"
                elif filament_type == "TPU":
                    filament_data["inherits"] = "fdm_filament_tpu"
                elif filament_type == "ABS":
                    filament_data["inherits"] = "fdm_filament_abs"
                elif filament_type == "ASA":
                    filament_data["inherits"] = "fdm_filament_asa"
                elif filament_type == "PP":
                    filament_data["inherits"] = "fdm_filament_pp"
                elif filament_type == "PPS" or filament_type == "PPS-CF":
                    filament_data["inherits"] = "fdm_filament_pps"
                elif filament_type == "PPS":
                    filament_data["inherits"] = "fdm_filament_pet"
                elif filament_type == "PET" or filament_type == "PET-CF":
                    filament_data["inherits"] = "fdm_filament_pet"
                elif filament_type == "PC":
                    filament_data["inherits"] = "fdm_filament_pc"
                elif filament_type == "PA" or filament_type == "PA6" or filament_type == "PA6-CF" or filament_type == "PA-CF" or filament_type == "PAHT" or filament_type == "PAHT-CF":
                    filament_data["inherits"] = "fdm_filament_pa"
                elif filament_type == "HIPS":
                    filament_data["inherits"] = "fdm_filament_hips"
                elif filament_type == "ABS-CF":
                    filament_data["inherits"] = "fdm_filament_abs"
                elif filament_type == "PETG-GF":
                    filament_data["inherits"] = "fdm_filament_petg"
                elif filament_type == "PC-CF":
                    filament_data["inherits"] = "fdm_filament_pc"
                elif filament_type == "ABS-FR":
                    filament_data["inherits"] = "fdm_filament_abs"
                elif filament_type == "PLA-GF":
                    filament_data["inherits"] = "fdm_filament_pla"
                elif filament_type == "BVOH":
                    filament_data["inherits"] = "fdm_filament_common"
                    filament_data["filament_adhesiveness_category"] = "797"
                else:
                    filament_data["inherits"] = "fdm_filament_common"

                filament_data["default_filament_colour"] = "\"\""

                #if "material_flow_dependent_temperature" in filament_data:
                #    del filament_data["material_flow_dependent_temperature"]
                #if "material_flow_temp_graph" in filament_data:
                #    del filament_data["material_flow_temp_graph"]

                #删除空的key
                for key in list(filament_data.keys()):
                    if filament_data[key] == "":
                        del filament_data[key]
                    elif key in array_keys:
                        filament_data[key] = [filament_data[key]]
                if normalize_variant_values:
                    normalize_multi_extruder_filament_variants(filament_data)
                out_filament_json_file = os.path.join(out_path,"filament",basename+".json")
                with open(out_filament_json_file, 'w', encoding='utf-8') as f:
                    json.dump(filament_data, f, ensure_ascii=False, indent=4)
                sub_paths.append( {
                    "name": basename,
                    "sub_path": os.path.join("filament",basename+".json").replace("\\","/")
                })
    return sub_paths,filament_map
def process_param_pack(printer,package_path, out_path):
    print(f"process_param_pack {package_path}")
    machine_path,top_material,bed_type = process_machine_json(printer,package_path, out_path)
    process_path =  process_process_json(printer,package_path, out_path)
    filament_path,filament_map = process_filament_json(printer,package_path, out_path)
    default_materials = []
    for material in top_material:
        if material in filament_map:
            filament_name = filament_map[material]
            default_materials.append(filament_name[0:filament_name.rfind("@")].strip())
    return machine_path,process_path,filament_path,default_materials,bed_type
    
def process_machine_json(printer,package_path, out_path):
    machine_json_file = machine_definition_path(printer, package_path)
    machine_name = printer["name"].strip()
    printer_profile_name = machine_profile_name(printer)
    bed_type = "High Temp Plate"
    top_material = []
    out_data = {
        "type": "machine",
        "from": "system",
        "instantiation": "true",
        "inherits": "fdm_creality_common",
        "printer_model": machine_name,
        "printer_structure": "i3"
    }
    with open(machine_json_file, 'r',encoding='utf-8') as file:
        data = json.load(file)
        printer_data = data["printer"]
        if(printer_data is not None):
            out_data.update(printer_data)
        extruder_data = data["extruders"][0]["engine_data"]
        if(extruder_data is not None):
            out_data.update(extruder_data)
        if data["metadata"]["top_material"] is not None:
            top_material = data["metadata"]["top_material"]
    #处理特殊的key
    preferred_process = data["metadata"]["preferred_process"]
    process_index = preferred_process.rfind("@")
    if process_index != -1:
        preferred_process =data["metadata"]["preferred_process"][0:process_index].strip() 
    out_data["default_print_profile"] = preferred_process + " @" + printer_profile_name
    if "01001" in top_material:
        out_data["default_filament_profile"] = ["Hyper PLA @" + printer_profile_name]
    elif "00001" in top_material:
        out_data["default_filament_profile"] = ["Generic PLA @" + printer_profile_name]
    elif "04001" in top_material:
        out_data["default_filament_profile"] = ["CR-PLA @" + printer_profile_name]
    elif "08001" in top_material:
        out_data["default_filament_profile"] = ["Ender-PLA @" + printer_profile_name]

    #if "nozzle_diameter" in out_data:
    #    out_data["nozzle_diameter"] = [out_data["nozzle_diameter"]]
    #else:
    out_data["nozzle_diameter"] = printer["nozzleDiameter"]
    if "printer_variant" in out_data:
        out_data["printer_variant"] = printer["nozzleDiameter"][0]
    if "material_flow_temp_graph" in out_data:
        del out_data["material_flow_temp_graph"]
    if "material_flow_dependent_temperature" in out_data:
        del out_data["material_flow_dependent_temperature"]
    if "curr_bed_type" in out_data:
        bed_type = out_data["curr_bed_type"]
        del out_data["curr_bed_type"]
    #写入机型文件
    machine_file_name = printer_profile_name
    machine_setting_name = nozzle_qualified_machine_profile_name(printer)
    out_data["name"] = printer_profile_name
    out_data["inherits"] = "fdm_creality_common"
    out_data["setting_id"] = str(hash(machine_setting_name))[1:6]
    out_data["support_multi_bed_types"] = "1"
    out_data["printer_model"] = machine_name
    #删除空的key
    for key in list(out_data.keys()):
        if out_data[key] == "":
            del out_data[key]
    out_machine_json_file = os.path.join(out_path,"machine",machine_file_name+".json")
    with open(out_machine_json_file, 'w', encoding='utf-8') as f:
        json.dump(out_data, f, ensure_ascii=False, indent=4)
    machine_sub_data = {
        "name": printer_profile_name,
        "sub_path": os.path.join("machine",machine_file_name+".json").replace("\\","/")
    }
    return [machine_sub_data],top_material,bed_type

def working_path_from_ci(path):
    return os.path.join(path,"out")



def main():
    print("[cmake/ci] generate creality presets")
    build_type = 'Beta'
    engine_type = 'orca'
    engine_version = '2.2.3'
    app_version = '7.2.0.5226'
    use_local_package = 0
    argv = sys.argv[1:]
    try:
        opts, args = getopt.getopt(argv, '-d-c-t:-b:-n:-p:-v:')
        print("getopt.getopt -> :" + str(opts))
    except getopt.GetoptError:
        print("create.py -t <type>")
        sys.exit(2)
    for opt, arg in opts:
        if opt in ('-b'):
            build_type = arg
        if opt in ('-n'):
            engine_version = arg
        if opt in ('-p'):
            use_local_package = arg
        if opt in ('-v'):
            app_version = arg
    working_path = working_path_from_ci(sys.path[0])
    print("[cmake/ci] working path :" + working_path)
    ParamPackUtil.downloadParamPack(working_path, build_type, engine_type, engine_version, app_version)
    make_parameter_package(0, engine_version)
    #make_parameter_package(1)

if __name__ == "__main__":
    main()
