import os
import sys, getopt
import tempfile
import shutil
import ParamPackUtil
import json
from PIL import Image
from datetime import datetime
def delete_json_folder(directory_path):
    print(directory_path)
    for file_name in os.listdir(directory_path):
        file_path = os.path.join(directory_path, file_name)
        if file_name.endswith('.json') and os.path.isfile(file_path) and not file_name.startswith("fdm_"):
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
                printer_name = "Creality "+printer_name
            printer["name"] = printer_name
            #print(f"---------------------process printer {printer_name}")
            param_pack_dir = os.path.join(os.path.join(working_path,f"server_{server_id}","orca","default"),"parampack",printer["printerIntName"])
            for nozzleDiameter in printer["nozzleDiameter"]:
                param_pack_dir = param_pack_dir + "-"+nozzleDiameter
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
        else:
            machine_model_data["bed_model"] = "creality_k1_buildplate_model.stl"
        if os.path.exists(os.path.join(out_path,f"{printerIntName}_buildplate_texture.png")):
            machine_model_data["bed_texture"] = f"{printerIntName}_buildplate_texture.png"
        else:
            machine_model_data["bed_texture"] = ""
        
        nozzle_diameter = []
        for printer in printerList:
            nozzle_diameter.append(printer["nozzleDiameter"][0])
            machine_model_data["default_bed_type"] = printer["bed_type"]
        machine_model_data["nozzle_diameter"] = ";".join(nozzle_diameter)
        print("--------------------",machine_model_data["default_bed_type"])
        out_machine_model_json_file = os.path.join(out_path,"machine",printerIntName+".json")
        with open(out_machine_model_json_file, 'w', encoding='utf-8') as f:
            json.dump(machine_model_data, f, ensure_ascii=False, indent=4)
        sub_paths.append({
            "name": printerIntName,
            "sub_path": os.path.join("machine",printerIntName+".json").replace("\\","/")
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
                printer_name = printer["name"].strip() +" "+printer["nozzleDiameter"][0]+" nozzle"
                if basename.find("@") == -1:
                    basename = basename+" @"+printer_name
                else:
                    basename = basename[0:basename.find("@")]+" @"+printer_name
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
def process_filament_json(printer,package_path, out_path):    
    array_keys = ["filament_type", "filament_vendor", "filament_start_gcode", "filament_end_gcode"]
    sub_paths = []
    filament_map = {}
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
                filament_data["filament_id"] = data["metadata"]["id"]
                machine_profile_name = printer["name"].strip()+" "+printer["nozzleDiameter"][0]+" nozzle"
                basename = os.path.splitext(filename)[0]
                basename = basename[0:basename.rfind("-")]
                basename = basename+" @"+machine_profile_name
                filament_data["name"] = basename
                filament_map[filament_data["filament_id"]] = basename

                #处理特殊的key
                filament_data["compatible_printers"] = [machine_profile_name]
                filament_data["inherits"] = "fdm_filament_common"
                filament_data["default_filament_colour"] = "\"\""

                if "material_flow_dependent_temperature" in filament_data:
                    del filament_data["material_flow_dependent_temperature"]
                if "material_flow_temp_graph" in filament_data:
                    del filament_data["material_flow_temp_graph"]

                #删除空的key
                for key in list(filament_data.keys()):
                    if filament_data[key] == "":
                        del filament_data[key]
                    elif key in array_keys:
                        filament_data[key] = [filament_data[key]]
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
    machine_json_file = os.path.join(package_path,printer["printerIntName"])
    for nozzleDiameter in printer["nozzleDiameter"]:
                machine_json_file = machine_json_file + "-"+nozzleDiameter
    machine_json_file = machine_json_file + ".def.json"
    machine_name = printer["name"].strip()
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
    out_data["default_print_profile"] = preferred_process + " @"+machine_name+" "+printer["nozzleDiameter"][0]+" nozzle"
    out_data["default_filament_profile"] = ["Hyper PLA @"+machine_name+" "+printer["nozzleDiameter"][0]+" nozzle"]

    if "nozzle_diameter" in out_data:
        out_data["nozzle_diameter"] = [out_data["nozzle_diameter"]]
    else:
        out_data["nozzle_diameter"] = [printer["nozzleDiameter"][0]]
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
    machine_profile_name = machine_name+" "+printer["nozzleDiameter"][0]+" nozzle"
    out_data["name"] = machine_profile_name
    out_data["inherits"] = "fdm_creality_common"
    out_data["setting_id"] = str(hash(machine_profile_name))[1:6]
    out_data["support_multi_bed_types"] = "1"
    out_data["printer_model"] = machine_name
    #删除空的key
    for key in list(out_data.keys()):
        if out_data[key] == "":
            del out_data[key]
    out_machine_json_file = os.path.join(out_path,"machine",machine_profile_name+".json")
    with open(out_machine_json_file, 'w', encoding='utf-8') as f:
        json.dump(out_data, f, ensure_ascii=False, indent=4)
    machine_sub_data = {
        "name": machine_profile_name,
        "sub_path": os.path.join("machine",machine_profile_name+".json").replace("\\","/")
    }
    return [machine_sub_data],top_material,bed_type

def working_path_from_ci(path):
    return os.path.join(path,"out")



def main():
    print("[cmake/ci] generate creality presets")
    build_type = 'Beta'
    engine_type = 'orca'
    engine_version = '2.2.3'
    use_local_package = 0
    argv = sys.argv[1:]
    try:
        opts, args = getopt.getopt(argv, '-d-c-t:-b:-n:-p:')
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
    working_path = working_path_from_ci(sys.path[0])
    print("[cmake/ci] working path :" + working_path)
    ParamPackUtil.downloadParamPack(working_path, build_type, engine_type, engine_version)
    make_parameter_package(0, engine_version)
    #make_parameter_package(1)

main()