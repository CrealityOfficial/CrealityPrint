import sys
import os
import sys, getopt
import shutil

if __name__ == "__main__":
    argv = sys.argv[1:]
    sys.path.append(sys.path[0] + '/../python/')
    origin_path = sys.path[0] + '/../../'
    
    print('AUTO DOXYGEN ....')
    print('origin_path : ' + origin_path)
    
    name = 'dummy'
    branch = 'master'
    repository = 'https://github.com/CrealityOfficial/'
    try:
        opts, args = getopt.getopt(argv, '-b:-n:')
        print("getopt.getopt -> :" + str(opts))
    except getopt.GetoptError:
        print("jeckins_doxygen.py -n <name>")
        sys.exit(2)
    for opt, arg in opts:
        if opt in ('-n'):
            name = arg
        if opt in ('-b'):
            branch = arg
            
    import osSystem;
    import ReadWrite;
                    
    build_path = origin_path + '/build/'
    osSystem.mkdirs(build_path)
        
    cmake_cmd = 'cmake -S ./cmake/doxygen/ -B build -DCMAKE_DOXYGEN_NAME=' + name
    osSystem.system(cmake_cmd)     
    
    



