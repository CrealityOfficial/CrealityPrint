'''    cmake_args  (-DCXX_VLD=ON/OFF vld debug   
                    -DRENDER_DOC=ON/OFF renderdoc debug
                    -MIX_PYTHON=ON/OFF python 
                )
'''
                
import sys
import getopt

sys.path.append(sys.path[0] + '/../python/')
sys.path.append(sys.path[0] + '/../pmodules/')
source_path = sys.path[0] + '/../../'

import ci_cmake
cmake = ci_cmake.CMake(source_path)
cmake.print()

cmake_args = ""
install_conan = False
build_conan = False
upload_conan = False
conan_channel = 'desktop'
#parse args
try:
    opts, args = getopt.getopt(sys.argv[1:], '-c-b-u',['cmake_args=', 'channel_name='])
    print("getopt.getopt -> :" + str(opts))
except getopt.GetoptError:
    print("_parse_args error.")
    sys.exit(2)
for opt, arg in opts:
    if opt == '--cmake_args':
        cmake_args = arg 
    if opt == '-c':
        install_conan = True
    if opt == '-b':
        build_conan = True
    if opt == '-u':
        upload_conan = True
    if opt == '--channel_name':
        conan_channel = arg 

if install_conan == True:
    import ci_conan
    print('cmake install conan channel: {}'.format(conan_channel))
    conan = ci_conan.Conan(cmake.cmake_path)
    if build_conan == True:
        conan.create_project_conan(conan_channel, upload_conan, True)
    
    conan.install(cmake.project_path, cmake.source_path, conan_channel)
    
print("cmake args : {}".format(cmake_args))       
cmake.build(cmake_args)

import ShaderBinarization
ShaderBinarization.exec(str(cmake.project_path))