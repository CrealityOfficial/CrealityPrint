import sys
import os
import getopt
import platform
import subprocess
from pathlib import Path

class CMake():
    '''
    system
    source_path  source position
    project_path
    '''
    def __init__(self, origin):
        self.system = platform.system()
        self.source_path = Path(origin)
        
        self._calculate_project_path()     
        
    def _calculate_project_path(self):
        suffix = 'win32-build/build/'
        self.project_path = self.source_path.joinpath(suffix)
        if not self.project_path.exists():
            self.project_path.mkdir(parents=True, exist_ok=True)
            
    def print(self):
        print("system : {}".format(self.system))
        print("source root : {}".format(self.source_path))
        print("project path : {}".format(self.project_path))
        
    def build(self, cmake_args):
        cmake_str = "cmake -S {} -B {} -DCMAKE_USE_CONAN=ON".format(self.source_path, self.project_path)
        system_str = ' -G "Visual Studio 16 2019" -T host=x64 -A x64 {}'.format(cmake_args)
        cmd = cmake_str + system_str
        
        print("start build: {}".format(cmd))
        os.system(cmd)
    
        
