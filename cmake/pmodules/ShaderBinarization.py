# -*- coding: utf-8 -*-
import os
import sys;

##### class #####
class file_info_t:
    def __init__(self, dir_path, file_name):
        self.basename = os.path.basename(file_name)
        self.name = os.path.splitext(file_name)[0]
        self.ext = os.path.splitext(file_name)[1][1:]
        self.path = dir_path + "/" + file_name
        
class shader_binarization_t:
    def __init__(self, name, alias):
        self.name = name
        self.alias = alias
        self.code = ""
        self.array = []
        self.shader_names = []
        self.shader_types = []
    
    def declare_shader(self, shader_file_info):
        shader_id = ""
        if self.alias == "":      
            shader_id = f"{shader_file_info.name}_{shader_file_info.ext}"
        else:
            shader_id = f"{self.alias}_{shader_file_info.name}_{shader_file_info.ext}"
        ## code
        self.code += (f"const char* {shader_id} =\n")
        shader_file = open(shader_file_info.path, 'r', encoding='utf-8')
        for line in shader_file:
            line = line.replace("\n", "\\n")
            self.code += ("                             ")
            self.code += (f"\"{line}\"\n")
        shader_file.close()
        self.code += (";\n")
        ## array
        self.array.append(shader_id)
        ## other
        self.shader_names.append(shader_file_info.name)
        self.shader_types.append(shader_file_info.ext)
        
    def out(self, out_path, format):
        binarization_str = f"//------------ {self.name} code\n"
        binarization_str += (self.code)
        binarization_str += f"//------------ {self.name} code array.\n"
        
        array_str = ""
        array_len = len(self.array)
        if self.alias == "":      
            array_str = "const char* shader_code_array[%d] = {\n" % (array_len)
        else:
            array_str = "const char* %s_shader_code_array[%d] = {\n" % (self.alias, array_len)
        for shader_name in self.array:
            array_str += (f"	{shader_name},\n")
        array_str += ("};\n")
        
        binarization_str += (array_str)
        
        out_file = open(out_path, format, encoding='utf-8')
        out_file.write(binarization_str)
        out_file.close()
        
    def out_program_def(self, out_path, format):
        program_def_map = {}
        for i in range(0, len(self.shader_names)):
            shader_name = self.shader_names[i]
            shader_type = self.shader_types[i]
            if (shader_name in program_def_map) == False:
                program_def_map[shader_name] = [-1, -1, -1, -1, -1]
                
            if shader_type == "vert":
                program_def_map[shader_name][0] = i
            elif shader_type == "tcs":
                program_def_map[shader_name][1] = i
            elif shader_type == "tes":
                program_def_map[shader_name][2] = i
            elif shader_type == "geom":
                program_def_map[shader_name][3] = i
            elif shader_type == "frag" or shader_type == "fragment":
                program_def_map[shader_name][4] = i
            
        program_def_str = "struct ProgramDef\n"
        program_def_str += ("{\n")
        program_def_str += ("	const char* name;\n")
        program_def_str += ("	int vIndex;\n")
        program_def_str += ("    int tcsIndex;\n")
        program_def_str += ("    int tesIndex;\n")
        program_def_str += ("	int gIndex;\n")
        program_def_str += ("	int fIndex;\n")
        program_def_str += ("};\n")
        program_def_str += ("int programs_meta_size = %d;\n" % (len(program_def_map)))
        program_def_str += ("ProgramDef programs_meta[%d] = {\n" % (len(program_def_map)))

        is_first_line = True
        for shader_name in program_def_map:
            if is_first_line == True:
                is_first_line = False
            else:
                program_def_str += (",\n")
            indexs = program_def_map[shader_name] 
            program_def_str += ("   { \"%s\" , %d  ,  %d   ,  %d   ,  %d   , %d  }" % (shader_name, indexs[0], indexs[1], indexs[2], indexs[3], indexs[4]))
        program_def_str += ("\n};")
        
        out_file = open(out_path, format, encoding='utf-8')
        out_file.write(program_def_str)
        out_file.close()
        
##### function #####  
valid_exts = ["vert", "frag", "fragment", "geom", "tes", "tcs"]
def add_shader(shader_binarization, dir_path, sharder_name):
    file_info = file_info_t(dir_path, sharder_name)
    if file_info.ext in valid_exts:
        shader_binarization.declare_shader(file_info)
    else:
        print(f"The file format of {file_info.basename} is not valid.")
    
def traverse_directory(dir_path, operate_func, parameter): 
    for root, dirs, files in os.walk(dir_path):
        for file in files:
            operate_func(parameter, dir_path, file)
      
##### main #####        
def exec(project_path = ''):
    
    script_path = sys.path[0].replace("\\", "/")
    if project_path == '':
        project_path = script_path + "/../../win32-build/build"
        
    if os.path.exists(project_path + '/shader_entity') == False:
        return
        
    print("shader binarization begin.")
    print("project path: " + project_path)
    base_directory = script_path + "/../../shader_entity/shaders"
    if os.path.exists(base_directory) == False:
        return
    
    GL3_input_path = base_directory + "/gl/3.3"
    GL3_output_path = project_path + "/shader_entity/GL.code"
    GLES2_input_path = base_directory + "/gles/3"
    GLES2_output_path = project_path + "/shader_entity/GL.code"
    print("gl3 shader directory: " + GL3_input_path)
    print("gles shader directory: " + GLES2_input_path)
    print("output: " + GL3_output_path)
    
    gl3_shader_binarization = shader_binarization_t("gl3.0", "")
    traverse_directory(GL3_input_path, add_shader, gl3_shader_binarization)
    gl3_shader_binarization.out(GL3_output_path, "w")
    
    gles_shader_binarization = shader_binarization_t("gles", "gles")
    traverse_directory(GLES2_input_path, add_shader, gles_shader_binarization)
    gles_shader_binarization.out(GLES2_output_path, "a")
    
    gl3_shader_binarization.out_program_def(GL3_output_path, "a")
    
    print("shader binarization completed.")

if __name__ == "__main__":
    if len(sys.argv) > 1:
        exec(sys.argv[1])
    else:
        exec()