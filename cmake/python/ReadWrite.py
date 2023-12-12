import configparser

def config_object(fileName):
    conf = configparser.ConfigParser()
    #print(type(conf))
    
    conf.read(fileName, encoding='utf-8')  
    return conf
    
def parse_sections(fileName):
    conf = config_object(fileName)
    sections = conf.sections()
    
    #print(sections)
    return sections
    
def parse_items(fileName, sectionName):
    conf = config_object(fileName)
    return conf[sectionName]
    
    
    
