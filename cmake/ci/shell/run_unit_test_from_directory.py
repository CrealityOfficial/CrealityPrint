import os
import sys, getopt
import tempfile
import shutil
    
def main(directory, argv):
    print("file path " + sys.path[0])
    print("input argv " + str(argv))

if __name__ == "__main__":
    main(sys.path[0], sys.argv[1:])
    
