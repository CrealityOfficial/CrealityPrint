import sys
from pathlib import Path

class Textlizer():
    '''
    convert file 2 ascii 
    '''
    def __init__(self):
        pass
        
    def convert(self, source, dest):
        source_path = Path(source)
        dest_path = Path(dest)
        with source_path.open('r', encoding='utf8') as source_file:
            content = source_file.read()
            with dest_path.open('w', encoding='utf8') as dest_file:
                dest_file.write('const unsigned char {}[] = {{'.format(source_path.stem))
                for c in content:
                    dest_file.write('{},'.format(ord(c)))
                dest_file.write('\'\\0\'};')
                
                dest_file.close()
                
            source_file.close()

if __name__ == "__main__":
    source = sys.argv[1]
    dest = sys.argv[2]
    
    t = Textlizer()
    t.convert(source, dest)
    