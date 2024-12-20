import sys;
sys.path.append('./cmake/python/')

import createUtil
import sys, getopt
import os

if __name__ == "__main__":
    directory = sys.path[0]
    argv = sys.argv[1:]
    xml_file = directory + "/cmake/conan/graph/libs.xml"
    
    name = ''
    file = ''
    upload = True
    try:
        opts, args = getopt.getopt(argv,"n:f:u:")
    except getopt.GetoptError:
        print("create.py -n <name>")
        sys.exit(2)
    for opt, arg in opts:
        if opt in ("-n"):
            name = arg
        if opt in ("-f"):
            file = arg
        if opt in ("-u"):
            if arg != "True":
                upload = False
            
    libs = createUtil.create_libs_from_txt(file)
    print('create patch :' + str(libs))
    
    libDict = createUtil.create_base_libs_from_xml(xml_file)
    #print('create patch all : ' + str(libDict))
    
    sequence_libs = createUtil.collect_sequece_libs(libDict, libs)
    print('create patch qequence : ' + str(sequence_libs))
    createUtil.build_recipes(sequence_libs, createUtil.get_channel_from_type(name), createUtil.get_profile_from_type(name), xml_file, upload)