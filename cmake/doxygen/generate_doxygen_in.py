import sys
import os
   
if __name__ == "__main__":
    directory = sys.path[0]
    sys.path.append(sys.path[0] + '/../python/')

    import osSystem;
    import ReadWrite;
    
    origin_path = sys.path[0]
    
    doxygen_template = origin_path + '/DoxyFile.template'
    doxygen_config = origin_path + '/DoxyFile.in.templates' 
    
    gen_doxy_template_cmd = 'doxygen -g ' + doxygen_template
    os.system(gen_doxy_template_cmd)
    
    with open(doxygen_template, 'r') as t, open(doxygen_config, 'w') as f:
        for line in t:
            if(line.startswith('#') == False and line):                
                f.write(line)
    
    
    



