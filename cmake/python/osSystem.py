import os
import sys, getopt
import tempfile
import shutil
import createUtil
import ParamPackUtil

Global_Debug = False
Global_conan = True

def mkdirs(path_dir):
    if not os.path.exists(path_dir):
        os.makedirs(path_dir)
    
def cd(ab_path):
    os.chdir(ab_path)
    
def system(cmd):
    print('cmake system : ' + cmd)
    os.system(cmd)
    
def working_path_from_ci(path):
    return path[0:-8]
    
def conan_install(working_path, project_path, channel):
    if Global_conan == False:
        return
        
    conan_file = project_path + '/conanfile.txt'
    graph_file = working_path + '/graph.txt'
    base_graph_file = working_path + '/cmake/conan/graph/libs.xml'
    
    createUtil.create_conan_file_from_graph(base_graph_file, graph_file, conan_file, channel)
    
    if os.path.exists(conan_file):
        cmd = 'conan install  -g cmake_multi -s build_type=Debug --build=missing -if ' + project_path + ' ' + project_path + ' --update'
        os.system(cmd)
        cmd = 'conan install -g cmake_multi -s build_type=Release --build=missing -if ' + project_path + ' ' + project_path + ' --update'
        os.system(cmd)    
    
def win_conan_cmake(working_path, channel):
    project_path = working_path + '/win32-build/build/'
    mkdirs(project_path)
      
    print("[cmake/ci] project path :" + project_path)
    debug_str = ""
    if(Global_Debug == True):
        debug_str = " -DCXX_VLD=ON"
    conan_install(working_path, project_path, channel)
    cmd = 'cmake -G "Visual Studio 16 2019" -DCMAKE_USE_CONAN=ON -S ' + working_path + ' -B ' + project_path + debug_str + ' -T host=x64 -A x64'
        
    os.system(cmd)
    return project_path
    
def emcc_conan_cmake(working_path):
    project_path = working_path + '/emcc-build/build/'
    mkdirs(project_path)
      
    print("[cmake/ci] project path :" + project_path)
    conan_install(working_path, project_path, 'desktop/emscripten')
    cmd = 'emcmake cmake -G "MinGW Makefiles" -DCMAKE_USE_CONAN=ON -S ' + working_path + ' -B ' + project_path
    os.system(cmd)
    return project_path
    
def linux_conan_cmake(working_path, channel):
    project_path = working_path + '/linux-build/build/'
    mkdirs(project_path)
      
    print("[cmake/ci] project path :" + project_path)
    conan_install(working_path, project_path, channel)
    cmd = 'cmake -G "Ninja" -DCMAKE_USE_CONAN=ON -S ' + working_path + ' -B ' + project_path
        
    os.system(cmd)
    return project_path
    
def mac_conan_cmake(working_path, channel):
    project_path = working_path + '/mac-build/build/'
    mkdirs(project_path)
      
    print("[cmake/ci] mac project path :" + project_path)
    conan_install(working_path, project_path, channel)
    cmd = 'cmake -DCMAKE_USE_CONAN=ON -S ' + working_path + ' -B ' + project_path
        
    os.system(cmd)
    return project_path
    
def jwin_conan_cmake(working_path):
    project_path = working_path + '/build/'
    mkdirs(project_path)
      
    print("[cmake/ci] project path :" + project_path)
    conan_install(working_path, project_path, 'desktop/win')
    cmd = 'cmake -G "Ninja" -DCMAKE_USE_CONAN=ON -S ' + working_path + ' -B ' + project_path   
    os.system(cmd)  
    return project_path
    
def conan_cmake():
    argv = sys.argv[1:]
    working_path = working_path_from_ci(sys.path[0])
    print("[cmake/ci] working path :" + working_path)
    
    work_type = 'win'
    build_type = 'Alpha'
    app_name = ''
    try:
        opts, args = getopt.getopt(argv, '-d-c-t:-b:-n:')
        print("getopt.getopt -> :" + str(opts))
    except getopt.GetoptError:
        print("create.py -t <type>")
        sys.exit(2)
    for opt, arg in opts:
        if opt in ('-t'):
            work_type = arg
        if opt in ('-d'):
            global Global_Debug
            Global_Debug = True
        if opt in ('-c'):
            global Global_conan
            Global_conan = False
        if opt in ('-b'):
            build_type = arg
        if opt in ('-n'):
            app_name = arg
            
    project_path = ''
    if work_type == 'win':
        project_path = win_conan_cmake(working_path, 'desktop/win')
    if work_type == 'opensource-win':
        project_path = win_conan_cmake(working_path, 'opensource/win')
    if work_type == 'jwin':
        project_path = jwin_conan_cmake(working_path)
    elif work_type == 'linux':
        project_path = linux_conan_cmake(working_path, 'desktop/linux')
    elif work_type == 'opensource-linux':
        project_path = linux_conan_cmake(working_path, 'opensource/linux')
    elif work_type == 'emcc':
        project_path = emcc_conan_cmake(working_path)
    elif work_type == 'mac':
        project_path = mac_conan_cmake(working_path, 'desktop/mac')
    elif work_type == 'opensource-mac':
        project_path = mac_conan_cmake(working_path, 'opensource/mac')
    else:
        pass
    if app_name == 'Creality_Print':
        ParamPackUtil.downloadParamPack(working_path, build_type)
    return project_path