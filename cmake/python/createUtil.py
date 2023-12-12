import os
import sys, getopt
import tempfile
import shutil
from pathlib import Path

from xml.etree import ElementTree as ET

def toDotID(name, version):
    v = version.replace('.', '_')
    n = name + "_" + v
    return n
    
def toDotName(name, version):
    n = name + "." + version
    return n     

def create_base_libs_from_xml(base_graph_file):
    tree = ET.parse(base_graph_file)
    root = tree.getroot()
    
    libs = root.findall("lib")
    subs = {}
    for lib in libs:
        sublibs = lib.findall("sublib")
        subss = []
        for sub in sublibs:
            subss.append(sub.attrib["name"])
        
        subs[lib.attrib["name"]] = subss
    return subs
    
def get_txt_libs(file_name):
    libs = []
    with open(file_name, 'r') as file:
        contents = file.readlines()
        for content in contents:
            libs.append(content.rstrip())
            
        print("{0} load recipes: {1}".format(file_name, libs))
        file.close()    
    return libs
    
def create_libs_from_txt(graph_file):
    libs = get_txt_libs(str(Path(graph_file)))
    children = Path(graph_file).parent.iterdir()
    for idx, element in enumerate(children):    
        if element.is_dir():
            child_graph_file = element.joinpath('graph.txt')
            #print("create_libs_from_txt -> load {0}".format(child_graph_file))
            
            if child_graph_file.exists() == True:
                sub_libs = get_txt_libs(str(child_graph_file))
                libs.extend(sub_libs)
        
    return libs

def collect_unique_libs(subs, libs):
    result = []
    
    first = libs;
    second = []
    while len(first) > 0:
        for value in first:
            if value not in result:
                result.append(value)
                if value in subs:
                    nex = subs[value]
                    for nvalue in  nex:
                        if nvalue not in second:
                            second.append(nvalue)
            
        first = second
        second = []
    
    return result

def collect_sequece_libs(libDict, libs):
    result = []
    
    first = libs
    second = []
    while len(first) > 0:
        for value in first:
            nex = libDict[value]
            if len(nex) == 0:
                result.append(value)
            else:
                need = False
                for nvalue in nex:
                    if nvalue not in result:
                        second.append(nvalue)
                        need = True
                
                if need == True:
                    second.append(value)
                else:
                    result.append(value)
                
        first = second
        second = []
      
    new_result = []
    for r in result:
        if r not in new_result:
            new_result.append(r)
        
    return new_result
    
def write_conan_file(conanfile, libs, channel):
    file = open(conanfile, "w")
    file.write('[requires]\n')
    for lib in libs:
        file.write(lib + '@' + channel)
        file.write('\n')
    file.close()     
    
def create_conan_file_from_graph(base_graph_file, graph_file, conanfile, channel):
    subs = create_base_libs_from_xml(base_graph_file)
    libs = create_libs_from_txt(graph_file)
    uniques = collect_unique_libs(subs, libs)
    
    print('create_conan_file_from_graph -> ' + str(uniques))
    write_conan_file(conanfile, uniques, channel)
    
def create_sub_libs():
    name = ''
    version = ''
    directory = sys.path[0]
    argv = sys.argv[1:]
    xml_file = directory + "/cmake/conan/graph/libs.xml"
    
    try:
        opts, args = getopt.getopt(argv,"n:v:p:")
    except getopt.GetoptError:
        print("create.py -n <name>")
        sys.exit(2)
    for opt, arg in opts:
        if opt in ("-n"):
            name = arg
        elif opt in ("-v"):
            version = arg
    
    return create_sub_libs_from_xml(xml_file, name, version)
    
       
def create_sub_libs_from_xml(xml_file, name, version):
    tree = ET.parse(xml_file)
    root = tree.getroot()
    ref = name + '/' + version
    
    libs = root.findall("lib")
    subs = {}
    for lib in libs:
        sublibs = lib.findall("sublib")
        subss = []
        for sub in sublibs:
            subss.append(sub.attrib["name"])
        
        subs[lib.attrib["name"]] = subss
        
    result = []
    
    first = subs[ref];
    second = []
    while len(first) > 0:
        for value in first:
            if value not in result:
                result.append(value)
                if value in subs:
                    nex = subs[value]
                    for nvalue in  nex:
                        if nvalue not in second:
                            second.append(nvalue)
            
        first = second
        second = []
    
    return result      
       
def create_param_from_argv():
    directory = sys.path[0]
    argv = sys.argv[1:]
    print("[conan DEBUG] file path " + directory)
    print("[conan DEBUG] input argv " + str(argv))
    
    name = '' 
    version = ''
    profile = 'default'
    user_channel = '_/_'
    location = ""
    try:
        opts, args = getopt.getopt(argv,"n:v:p:l:")
    except getopt.GetoptError:
        print("create.py -n <name> -v <version> -p <profile> -l <location>")
        sys.exit(2)
    for opt, arg in opts:
        if opt in ("-n"):
            name = arg
        elif opt in ("-v"):
            version = arg
        elif opt in ("-p"):
            profile = arg
        elif opt in ("-l"):
            location = arg
            
    params = {'name':name, 'version':version, 'profile':profile, 'channel':user_channel, 'location':location}
    return params
    
def invoke_conan_build(params, subLibs = []):
    print("[conan DEBUG] params : " + str(params))
    print("[conan DEBUG] subs : " + str(subLibs))
    
    directory = sys.path[0]
    name = params['name']
    profile = params['profile']
    version = params['version']
    user_channel = params['channel']
    
    with tempfile.TemporaryDirectory() as temp_directory:
        print("[conan DEBUG] created temporary directory ", temp_directory)
        print("[conan DEBUG] temp directory exist ", os.path.exists(temp_directory))
        
        create_script_src = directory + "/scripts/conanfile.py"
        cmake_script_src = directory + "/scripts/CMakeLists.txt"
        meta_data_src = directory + "/recipes/" + name + "/conandata.yml";
        shutil.copy2(create_script_src, temp_directory)
        shutil.copy2(meta_data_src, temp_directory)
        shutil.copy2(cmake_script_src, temp_directory)
        
        meta_data_dest = temp_directory + "/conandata.yml"
        meta_file = open(meta_data_dest, "a")
        if params['channel'] == 'jwin':
            user_channel = 'desktop/win'
            meta_file.write("generator: Ninja\n")
            params['channel'] = 'desktop/win'
            
        meta_file.write("version: " + "\"" + version + "\"\n")
        meta_file.write("name: " + "\"" + name + "\"\n")
        meta_file.write("channel: " + "\"" + user_channel + "\"")
        meta_file.close()
        cmake_script_dest = temp_directory + "/CMakeLists.txt"
        cmake_file = open(cmake_script_dest, "a")
        cmake_file.write("add_subdirectory(" + name + ")")
        cmake_file.close()       
        sublibs_dest = temp_directory + "/requires.txt"
        subLibs_file = open(sublibs_dest, "w")
        for sub in subLibs:
            subLibs_file.write(sub)
            subLibs_file.write('\n')
        subLibs_file.close() 
        
        #os.system("pause")
        debug_cmd = 'conan create --profile ' + profile + ' -s build_type=Debug ' + temp_directory + ' ' + user_channel
        os.system(debug_cmd)
        release_cmd = 'conan create --profile ' + profile + ' -s build_type=Release ' + temp_directory + ' ' + user_channel        
        os.system(release_cmd)

def invoke_conan_build_data(params):
    print("[conan DEBUG] params : " + str(params))
    
    directory = sys.path[0]
    name = params['name']
    profile = params['profile']
    version = params['version']
    user_channel = params['channel']
    
    with tempfile.TemporaryDirectory() as temp_directory:
        print("[conan DEBUG] created temporary directory ", temp_directory)
        print("[conan DEBUG] temp directory exist ", os.path.exists(temp_directory))
        
        create_script_src = directory + "/scripts/conanfile-data.py"
        cmake_script_src = directory + "/scripts/CMakeLists-data.txt"
        meta_data_src = directory + "/recipes/data/conandata.yml";
        shutil.copy2(create_script_src, temp_directory + '/conanfile.py')
        shutil.copy2(meta_data_src, temp_directory)
        shutil.copy2(cmake_script_src, temp_directory + '/CMakeLists.txt')
        
        meta_data_dest = temp_directory + "/conandata.yml"
        meta_file = open(meta_data_dest, "a")
        meta_file.write("version: " + "\"" + version + "\"\n")
        meta_file.write("name: " + "\"" + name + "\"\n")
        meta_file.write("channel: " + "\"" + user_channel + "\"")
        meta_file.close()
        cmake_script_dest = temp_directory + "/CMakeLists.txt"
        cmake_file = open(cmake_script_dest, "a")
        cmake_file.write("install(DIRECTORY \"" + params['location'] + "\" DESTINATION . PATTERN \"*.*\")")
        cmake_file.close()       
        
        #os.system("pause")
        debug_cmd = 'conan create --profile ' + profile + ' -s build_type=Debug ' + temp_directory + ' ' + user_channel
        os.system(debug_cmd)
        release_cmd = 'conan create --profile ' + profile + ' -s build_type=Release ' + temp_directory + ' ' + user_channel        
        os.system(release_cmd)
        
def invoke_conan_upload(recipe, channel):
    ref = recipe + '@' + channel
    cmd = 'conan upload ' + ref + ' -r artifactory --all -c'
    os.system(cmd)
            
def build_recipes(recipes, channel, profile, xml_file, upload = True):
    params = {'name':'xxx', 'version':'0.0.0', 'profile':'linux', 'channel':'desktop/linux'}
    for recipe in recipes:
        seg = recipe.split('/')
        if len(seg) == 2:
            name = seg[0]
            version = seg[1]
            params['name'] = name
            params['version'] = version
            params['profile'] = profile
            params['channel'] = channel
            
            subs = create_sub_libs_from_xml(xml_file, name, version)
            invoke_conan_build(params, subs)
            if upload == True:
                invoke_conan_upload(recipe, params['channel'])
            
def get_channel_from_type(name):
    channel = 'desktop/win'
    if name == 'linux':
        channel = 'desktop/linux'
    if name == 'mac':
        channel = 'desktop/mac'
    if name == 'mac-arm64':
        channel = 'desktop/mac-arm64'
    if name == 'opensource-linux':
        channel = 'opensource/linux'
    if name == 'opensource-mac':
        channel = 'opensource/mac'  
    if name == 'opensource-win':
        channel = 'opensource/win' 
    if name == 'jwin':
        channel = 'jwin'
        
    return channel
    
def get_profile_from_type(name):
    profile = 'win'
    if name == 'linux' or name == 'opensource-linux':
        profile = 'linux'
    if name == 'mac' or name == 'opensource-mac':
        profile = 'mac'
    if name == 'mac-arm64':
        profile = 'mac-arm64'
    return profile    
