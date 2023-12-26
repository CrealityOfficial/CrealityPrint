import sys
from pathlib import Path

class Parameter:
    def __init__(self, name, type_name):
        self.name = name
        self.type_name = type_name
        self.flag = 'scene' # scene group mesh extruder

def write_function(out, data, prefix):
    out.write('inline {0} {2}_{1}() const{{return {1};}}\n'.format(data.type_name, data.name, prefix))

def write_member(out, data):
    out.write('{0} {1} = 0;\n'.format(data.type_name, data.name))
    
def write_initialize(out, data):
    out.write('{1} = settings->get<{0}>("{1}");\n'.format(data.type_name, data.name))

class ParameterCpper():
    '''
    convert json to C++ files
    parameter_wrapper.h
    parameter_wrapper.cpp
    '''
    def __init__(self, sdir, ddir):
        self.source_dir = sdir
        self.dest_dir = ddir
        
        cpp_path = Path(dest_dir + '/parameter_wrapper.cpp')
        h_path = Path(dest_dir + '/parameter_wrapper.h')
        self.cpp_file = cpp_path.open('w', encoding='utf8')
        self.h_file = h_path.open('w', encoding='utf8')
        
    def parse_jsons(self, jsons):
        datas = []
        d1 = Parameter('wireframe_enabled', 'bool')
        d2 = Parameter('layer_height', 'coord_t')
        datas.append(d1)
        datas.append(d2)
        return datas
        
    def generate(self, jsons):
        datas = self.parse_jsons(jsons)
        #.h
        self.generate_h(self.h_file, datas)
        #.cpp
        self.generate_cpp(self.cpp_file, datas)
        
    def generate_h(self, out, datas):
        out.write('#ifndef _PARAMETER_WRAPPER_H_\n')
        out.write('#define _PARAMETER_WRAPPER_H_\n')
        out.write('#include <string>\n')
        out.write('#include "utils/Coord_t.h"\n')
        out.write('#include "types/Temperature.h"\n')
        out.write('#include "types/EnumSettings.h"\n')
        out.write('namespace cura52{class Settings;\n') #namespace
        
        out.write('class SceneParamWrapper1{public:\n')  #scene
        out.write('void initialize(Settings* settings);\n')
        for data in datas:
            if 'scene' in data.flag:
                write_function(out, data, 'scene')
        out.write('protected:\n')
        for data in datas:
            if 'scene' in data.flag:
                write_member(out, data)
        out.write('};\n')                                  #scene
        
        out.write('}\n')                                #namespace
        out.write('#endif //_PARAMETER_WRAPPER_H_\n')
        
    def generate_cpp(self, out, datas):
        out.write('#include "parameter_wrapper.h"\n')
        out.write('#include "settings/Settings.h"\n')
        out.write('namespace cura52{')
        
        out.write('void SceneParamWrapper1::initialize(Settings* settings){if(!settings)return;\n')   #scene
        for data in datas:
            if 'scene' in data.flag:
                write_initialize(out, data)
        out.write('}')                                                            #scene
        
        out.write('}')
        
    def __del__(self):
        self.cpp_file.close()
        self.h_file.close()

if __name__ == "__main__":
    source_dir = sys.argv[1]
    dest_dir = sys.argv[2]
    jsons = sys.argv[3]
    
    print('source_dir {}'.format(source_dir))
    print('dest_dir {}'.format(dest_dir))
    print('jsons {}'.format(jsons))
    
    pcc = ParameterCpper(source_dir, dest_dir)
    pcc.generate(jsons)