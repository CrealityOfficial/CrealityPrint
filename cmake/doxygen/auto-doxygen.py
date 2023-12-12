import sys
import os
   
if __name__ == "__main__":
    directory = sys.path[0]
    sys.path.append(sys.path[0] + '/../python/')

    import osSystem;
    import ReadWrite;
    
    origin_path = sys.path[0] + '/../../'
    
    print('AUTO DOXYGEN ....')
    print('origin_path : ' + origin_path)
    
    doxygen_path = origin_path + '/doxygen_generate/'
    osSystem.mkdirs(doxygen_path)
    
    doxygen_template = doxygen_path + 'DoxyFile.template'
    doxygen_config = doxygen_path + 'DoxyFile' 
    user_config_file = origin_path + 'DoxyFile'
    user_items = ReadWrite.parse_items(user_config_file, 'default')
    
    #add items
    user_items['INPUT '] = origin_path + user_items['project_name']
    user_items['OUTPUT_DIRECTORY'] = doxygen_path
    user_items['GENERATE_LATEX'] = 'NO'
    user_items['RECURSIVE'] = 'YES'
    
    gen_doxy_template_cmd = 'doxygen -g ' + doxygen_template
    os.system(gen_doxy_template_cmd)
    
    with open(doxygen_template, 'r') as t, open(doxygen_config, 'w') as f:
        for line in t:
            if(line.startswith('#') == False):
                for key in user_items:
                    upper_key = key.upper()
                    if upper_key in line:
                        line = upper_key + ' = ' + user_items[key] + '\n'
                
            f.write(line)
    
    doxygen_cmd = 'doxygen ' + doxygen_config
    os.system(doxygen_cmd)
    
    
    



