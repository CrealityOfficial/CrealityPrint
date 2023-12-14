'''
'''
                
import sys, getopt
import os

sys.path.append(sys.path[0] + '/../python/')
sys.path.append(sys.path[0] + '/../pmodules/')
cmake_path = sys.path[0] + '/../'

import ci_conan
conan = ci_conan.Conan(cmake_path)

channel_name = 'desktop'

recipe_type = 'recipe|xxx/x.x.x'
'''
recipe|xxx/x.x.x)
patch|./patches/cxbin.patch
subs|./subs/xxx.subs
whole
project
'''

upload = False
use_external_rep = False
#parse args
argv = sys.argv[1:]
try:
    opts, args = getopt.getopt(argv,"n:t:u:-e")
except getopt.GetoptError:
    print("create.py -n <name>")
    sys.exit(2)
print(opts)

for opt, arg in opts:
    if opt in ("-n"):
        channel_name = arg
    if opt in ("-t"):
        recipe_type = arg
    if opt in ("-e"):
        use_external_rep = True
    if opt in ("-u"):
        if arg == "True":
            upload = True

if use_external_rep == True:
    conan.set_use_external_rep(use_external_rep)
    
if recipe_type.startswith('recipe'):
    conan.create_one(recipe_type.split('|')[1], channel_name, upload, True)
    
if recipe_type.startswith('patch'):
    conan.create_from_patch_file(recipe_type.split('|')[1], channel_name, upload, True)
    
if recipe_type.startswith('subs'):
    conan.create_from_subs_file(recipe_type.split('|')[1], channel_name, upload, True)
    
if recipe_type.startswith('whole'):
    conan.create_whole(channel_name, upload, True)
    
if recipe_type.startswith('project'):
    conan.create_project_conan(channel_name, upload, True)
    