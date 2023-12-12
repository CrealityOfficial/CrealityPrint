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

cmake_args = ""
#parse args
try:
    opts, args = getopt.getopt(sys.argv[1:], '',['cmake_args='])
    print("getopt.getopt -> :" + str(opts))
except getopt.GetoptError:
    print("_parse_args error.")
    sys.exit(2)
for opt, arg in opts:
    if opt in ('--cmake_args'):
        cmake_args = arg 
       
print("cmake args : {}".format(cmake_args))       
cmake.print()
cmake.build(cmake_args)