from conans import ConanFile, CMake, tools
import os
import shutil

class MainEntry(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False], "fPIC": [True, False]}
    default_options = {"shared": False, "fPIC": True}
    
    generators = "cmake"

    def init(self):
        self.origin_recipe_folder = self.recipe_folder
        self.conanfile_file = self.recipe_folder + "/requires.txt"
        
        self.version = self.conan_data["version"]
        self.name = self.conan_data["name"]
        self.license = self.conan_data["license"]
        self.author = self.conan_data["author"]
        self.url = self.conan_data["url"]
        self.description = self.conan_data["description"]
        self.repository = self.conan_data["repository"]
        if 'use_external' in self.conan_data and self.conan_data['use_external'] == True and 'external_repository' in self.conan_data:
            self.repository = self.conan_data['external_repository']
            
        self.userchannel = self.conan_data["channel"]
        self.cmake_rep = 'http://172.20.180.12:8050/yanfa4/shared/cmake'
        self.defs = []
            
        if "defs" in self.conan_data:
            self.defs = self.conan_data["defs"].split(" ")
        if "cmake_rep" in self.conan_data:
            self.cmake_rep = self.conan_data["cmake_rep"]
                    
    def config_options(self):
        print("[conan DEBUG] config_options.")
        print("[conan DEBUG] conanfile.py init.")
        print("[conan DEBUG] self.recipe_folder: " + self.recipe_folder)
        print("[conan DEBUG] name: " + self.name)
        print("[conan DEBUG] version: " + self.version)
        print("[conan DEBUG] license: " + self.license)
        print("[conan DEBUG] author: " + self.author)
        print("[conan DEBUG] url: " + self.url)
        print("[conan DEBUG] description: " + self.description)
        print("[conan DEBUG] defs: " + str(self.defs))                

        if self.settings.os == "Windows":
            del self.options.fPIC

    def source(self):
        print("[conan DEBUG] source. {0} {1}".format(self.repository, self.version))

        self.run("git clone {0} cmake".format(self.cmake_rep))
        self.run("git clone " + self.repository + " -b version-" + self.version + " " + self.name)
            
    def export_sources(self):
        self.copy("CMakeLists.txt")
        self.copy("requires.txt")
        
    def build(self):
        print("[conan DEBUG] build.")        
        print("[conan DEBUG] self.build_folder: " + self.build_folder)
        conanfile_file = self.build_folder + "/requires.txt"
        conanfile_file_w = self.build_folder + "/conanfile.txt"
        if os.path.exists(conanfile_file):
            file = open(conanfile_file, 'r')
            file_w = open(conanfile_file_w, 'w')
            
            file_w.write('[requires]\n')
            contents = file.readlines()
            for content in contents:
                reference = content.rstrip() + "@" + self.userchannel
                file_w.write(reference)
                file_w.write('\n')
                
            file.close()
            file_w.close()
            
            self.run('conan install . -if . -g cmake_multi -s build_type=Debug --build=missing')
            self.run('conan install . -if . -g cmake_multi -s build_type=Release --build=missing')
        
        install_def = "CC_INSTALL_" + self.name.upper()
        cmake = CMake(self)
        cmake.definitions["CREATE_CONAN_PACKAGE"] = "ON"
        cmake.definitions[install_def] = "ON"
        cmake.definitions["CMAKE_BUILD_TYPE"] = "Release"
        if self.settings.build_type == "Debug":
            cmake.definitions["CMAKE_BUILD_TYPE"] = "Debug"
            
        #if self.settings.os != "Windows":
        #    cmake.parallel = False
            
        for d in self.defs :
            cmake.definitions[d] = "ON"
        
        if "generator" in self.conan_data:
            cmake._generator = self.conan_data['generator']
            
        cmake.configure(source_folder=".", build_folder=".")
        cmake.build()

    def package(self):
        print("[conan DEBUG] package.")
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.libs = [self.name]

