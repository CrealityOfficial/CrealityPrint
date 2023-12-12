from conans import ConanFile, CMake, tools
import os
import shutil

class MainEntry(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False], "fPIC": [True, False]}
    default_options = {"shared": False, "fPIC": True}
    
    generators = "cmake"

    def init(self):
        print("[conan DEBUG] conanfile.py init.")
        print("[conan DEBUG] self.recipe_folder: " + self.recipe_folder)
        
        self.origin_recipe_folder = self.recipe_folder
        self.conanfile_file = self.recipe_folder + "/requires.txt"
        print("[conan DEBUG] self.conanfile_file: " + self.conanfile_file)
        
        self.version = self.conan_data["version"]
        self.name = self.conan_data["name"]
        self.license = self.conan_data["license"]
        self.author = self.conan_data["author"]
        self.url = self.conan_data["url"]
        self.description = self.conan_data["description"]
        self.userchannel = self.conan_data["channel"]
        self.defs = []
            
        if "defs" in self.conan_data:
            self.defs = self.conan_data["defs"].split(" ")
            
        self.conan_data
        print("[conan DEBUG] name: " + self.name)
        print("[conan DEBUG] version: " + self.version)
        print("[conan DEBUG] license: " + self.license)
        print("[conan DEBUG] author: " + self.author)
        print("[conan DEBUG] url: " + self.url)
        print("[conan DEBUG] description: " + self.description)
        print("[conan DEBUG] defs: " + str(self.defs))
        
    def config_options(self):
        print("[conan DEBUG] config_options.")
        if self.settings.os == "Windows":
            del self.options.fPIC

    def source(self):
        pass
        
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
            
        if self.settings.os != "Windows":
            cmake.parallel = False
            
        for d in self.defs :
            cmake.definitions[d] = "ON"
            
        cmake.configure(source_folder=".", build_folder=".")
        cmake.build()

    def package(self):
        print("[conan DEBUG] package.")
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.libs = [self.name]

