import sys;
from pathlib import Path

path = Path(sys.argv[1])
    
out = Path(path, "files")
with out.open(mode='w') as f:
    for child in path.iterdir():
        if child.is_file():
            f.write('"' + child.name + '"\n')
    
    
    
    



