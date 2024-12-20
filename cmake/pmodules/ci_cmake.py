import sys
import os
import getopt
import platform
import executor
from pathlib import Path

class CMake():
    '''
    system
    source_path  source position
    project_path
    '''
    def __init__(self, origin, logger):
        self.system = platform.system()
        self.source_path = Path(origin).resolve()
        self.logger = logger
        
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
        # example: VS_VERSION=Visual Studio 17 2022 in system PATH
        vs_version = os.environ.get('VS_VERSION', 'Visual Studio 16 2019')
        cmake_str = ' -G "{}" -T host=x64 -A x64 {}'.format(vs_version, cmake_args)

        if self.system == 'Linux':
            cmake_str = ' {}'.format(cmake_args)
        if self.system == 'Darwin':
            cmake_str = ' {}'.format(cmake_args)
        return cmake_str
        
    def print(self):
        self.logger.info("system : {}".format(self.system))
        self.logger.info("source root : {}".format(self.source_path))
        self.logger.info("cmake root : {}".format(self.cmake_path))
        self.logger.info("project path : {}".format(self.project_path))
        
    def build(self, cmake_args):
        cmake_str = "cmake -S {} -B {} -DCMAKE_USE_CONAN=ON ".format(self.source_path, self.project_path)
        system_str = self._system_cmake_str(cmake_args)
        cmd = cmake_str + system_str
        
        executor.run(cmd, True, self.logger)
        
    def compile(self):
        cmd = "make"
        if self.system == 'Linux':
            cmd = 'make -j8'
        if self.system == 'Darwin':
            cmd = 'make -j8'

        executor.run(cmd, True, self.logger)
        
    def test(self):
        executor.run('make test', True, self.logger)
        
    #build, compile, test
    def run_test(self, cmake_args):
        if self.system == "Windows":
            self.logger.info("system windows not support")
            return
        
        import osSystem
        osSystem.cd(str(self.project_path))
        
        self.build(cmake_args)
        self.compile()
        self.test()
        
