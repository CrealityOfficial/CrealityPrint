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
use_external_rep = False
update_from_remote = False
conan_channel = 'desktop'
#parse args

import ci_conan
logger.info('cmake install conan channel: {}'.format(conan_channel))
conan = ci_conan.Conan(cmake.cmake_path, logger, use_external_rep)
conan.install_from_conandata_file(cmake.project_path, cmake.source_path, update_from_remote, conan_channel)

logger.info("cmake args : {}".format(cmake_args))       
cmake.run_test(cmake_args)
