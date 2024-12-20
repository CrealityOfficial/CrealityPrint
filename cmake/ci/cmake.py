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

import log
logger = log.create_log('cmake')

import ci_cmake
cmake = ci_cmake.CMake(source_path, logger)
cmake.print()

cmake_args = ""
install_conan = False
build_conan = False
upload_conan = False
use_external_rep = False
update_from_remote = False
use_conandata = True
conan_channel = 'desktop'
#parse args
try:
    opts, args = getopt.getopt(sys.argv[1:], '-c-b-u-r-e-p',['cmake_args=', 'channel_name='])
    logger.info("getopt.getopt -> : {}".format(str(opts)))
except getopt.GetoptError:
    logger.warning("_parse_args error.")
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
    if opt == '-e':
        use_external_rep = True
    if opt == '-r':
        update_from_remote = True
    if opt == '-p':
        use_conandata = False
    if opt == '--channel_name':
        conan_channel = arg 

if install_conan == True:
    import ci_conan
    logger.info('cmake install conan channel: {}'.format(conan_channel))
    conan = ci_conan.Conan(cmake.cmake_path, logger, use_external_rep)
        
    if build_conan == True:
        conan.create_circle_conan(conan_channel, upload_conan)
    
    if use_conandata == True:
        conan.install_from_conandata_file(cmake.project_path, cmake.source_path, update_from_remote, conan_channel)
    else:
        conan.install_from_txt(cmake.project_path, cmake.source_path, update_from_remote, conan_channel)
    
logger.info("cmake args : {}".format(cmake_args))       
cmake.build(cmake_args)

import ShaderBinarization
ShaderBinarization.exec(str(cmake.project_path))