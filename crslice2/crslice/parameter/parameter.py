import sys
import json
from pathlib import Path

class Parameter:
    def __init__(self, name, type_name):
        self.name = name
        self.type_name = type_name
        self.tags = [] # scene group mesh extruder

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
        
    #def parse_jsons(self, jsons):
    #    datas = []
    #    d1 = Parameter('wireframe_enabled', 'bool')
    #    d1.tags.append('scene')
    #    d1.tags.append('extruder')
    #    d2 = Parameter('layer_height', 'coord_t')
    #    d2.tags.append('scene')
    #    d2.tags.append('group')
    #    datas.append(d1)
    #    datas.append(d2)
    #    return datas
    
    def _parse_parameter(self, key, value):
        data = Parameter(key, 'std::string')
        if value['type'] == 'int' or\
            value['type'] == 'extruder' or\
            value['type'] == 'optional_extruder':
            data.type_name = 'int'
        elif value['type'] == 'float':
            data.type_name = 'double'
        
        if not 'settable_globally' in value or value['settable_globally'] == 'true':
            data.tags.append('scene')           
        if 'settable_per_meshgroup' in value and value['settable_per_meshgroup'] == 'true':
            data.tags.append('group')
        if 'settable_per_mesh' in value and value['settable_per_mesh'] == 'true':
            data.tags.append('mesh')
        if 'settable_per_extruder' in value and value['settable_per_extruder'] == 'true':
            data.tags.append('extruder') 
        
        if len(data.tags) == 0:
            print('set default tag for {}'.format(data.name))
            data.tags.append('scene')
        return data
     
    def _parse_json_data(self, json_data):
        datas = []
        for k, v in json_data.items():
            datas.append(self._parse_parameter(k, v))
        return datas
        
    def parse_jsons(self, jsons):
        j_files = jsons.split()
        datas = []
        for j_file in j_files:
            json_path = Path('{0}/{1}'.format(source_dir, j_file))
            #print(str(json_path))
            with open(str(json_path), 'r') as fcc_file:
                fcc_data = json.load(fcc_file)
                #print(json.dumps(fcc_data,indent=2))
                datas += self._parse_json_data(fcc_data)
            fcc_file.close()
        
        #print(datas)
        return datas
        
    def _check(self, parameter, tag):
        return parameter.tags.count(tag) > 0
        
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
        
        out.write('class SceneParameterWrapper{public:\n')  #scene
        out.write('void initialize(Settings* settings);\n')
        for data in datas:
            if self._check(data, 'scene'):
                write_function(out, data, 'scene')
        out.write('protected:\n')
        for data in datas:
            if self._check(data, 'scene'):
                write_member(out, data)
        out.write('};\n')                                  #scene
        
        out.write('class GroupParameterWrapper{public:\n')  #group
        out.write('void initialize(Settings* settings);\n')
        for data in datas:
            if self._check(data, 'group'):
                write_function(out, data, 'group')
        out.write('protected:\n')
        for data in datas:
            if self._check(data, 'group'):
                write_member(out, data)
        out.write('};\n')                                  #group
        
        out.write('class MeshParameterWrapper{public:\n')  #mesh
        out.write('void initialize(Settings* settings);\n')
        for data in datas:
            if self._check(data, 'mesh'):
                write_function(out, data, 'mesh')
        out.write('protected:\n')
        for data in datas:
            if self._check(data, 'mesh'):
                write_member(out, data)
        out.write('};\n')                                  #mesh
        
        out.write('class ExtruderParameterWrapper{public:\n')  #extruder
        out.write('void initialize(Settings* settings);\n')
        for data in datas:
            if self._check(data, 'extruder'):
                write_function(out, data, 'extruder')
        out.write('protected:\n')
        for data in datas:
            if self._check(data, 'extruder'):
                write_member(out, data)
        out.write('};\n')                                  #extruder
        
        out.write('} //namespace cura52 \n')                                #namespace
        out.write('#endif //_PARAMETER_WRAPPER_H_\n')
        
    def generate_cpp(self, out, datas):
        out.write('#include "parameter_wrapper.h"\n')
        out.write('#include "settings/Settings.h"\n')
        out.write('namespace cura52{')
        
        out.write('void SceneParameterWrapper::initialize(Settings* settings){if(!settings)return;\n')   #scene
        for data in datas:
            if self._check(data, 'scene'):
                write_initialize(out, data)
        out.write('}\n')                                                            #scene
        
        out.write('void GroupParameterWrapper::initialize(Settings* settings){if(!settings)return;\n')   #group
        for data in datas:
            if self._check(data, 'group'):
                write_initialize(out, data)
        out.write('}\n')                                                            #group
        
        out.write('void MeshParameterWrapper::initialize(Settings* settings){if(!settings)return;\n')   #mesh
        for data in datas:
            if self._check(data, 'mesh'):
                write_initialize(out, data)
        out.write('}\n')                                                            #mesh
        
        out.write('void ExtruderParameterWrapper::initialize(Settings* settings){if(!settings)return;\n')   #extruder
        for data in datas:
            if self._check(data, 'extruder'):
                write_initialize(out, data)
        out.write('}\n')                                                            #extruder
        
        out.write('} //namespace cura52')
        
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