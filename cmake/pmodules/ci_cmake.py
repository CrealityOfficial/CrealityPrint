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
        self.source_path = Path(origin).resolve()
        
        self._calculate_project_path()     
    
    def _suffix(self):
        if self.system == 'Windows':
            return 'win32-build/build/'
        if self.system == 'Linux':
            return 'linux-build/build/'
        if self.system == 'Darwin':
            return 'mac-build/build/'
        return 'win32-build/build/'        
        
    def _calculate_project_path(self):
        self.project_path = self.source_path.joinpath(self._suffix())
        self.cmake_path = self.source_path.joinpath('cmake')
        
        if not self.project_path.exists():
            self.project_path.mkdir(parents=True, exist_ok=True)
    
    def _system_cmake_str(self, cmake_args):
        cmake_str = ' -G "Visual Studio 16 2019" -T host=x64 -A x64 {}'.format(cmake_args)
        if self.system == 'Linux':
            cmake_str = '-G "Ninja" {}'.format(cmake_args)
        if self.system == 'Darwin':
            cmake_str = ' {}'.format(cmake_args)
        return cmake_str
        
    def print(self):
        print("system : {}".format(self.system))
        print("source root : {}".format(self.source_path))
        print("cmake root : {}".format(self.cmake_path))
        print("project path : {}".format(self.project_path))
        
    def build(self, cmake_args):
        cmake_str = "cmake -S {} -B {} -DCMAKE_USE_CONAN=ON".format(self.source_path, self.project_path)
        system_str = self._system_cmake_str(cmake_args)
        cmd = cmake_str + system_str
        
        print("start build: {}".format(cmd))
        os.system(cmd)
    
        
