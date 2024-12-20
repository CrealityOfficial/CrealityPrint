import sys;

if __name__ == "__main__":
    directory = sys.path[0]
    sys.path.append(sys.path[0] + '/../python/')

    import osSystem
    project_path = osSystem.conan_cmake()
    
    sys.path.append(sys.path[0] + '/../pmodules/')
    import ShaderBinarization
    ShaderBinarization.exec(project_path)