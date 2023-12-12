import sys;

directory = sys.path[0]
sys.path.append(sys.path[0] + '/../')

import osSystem
import os

script_file = sys.argv[0]
exe_file = sys.argv[1]

print('testing : ' + script_file, flush=True)
print('testing : ' + exe_file, flush=True)

cmd = exe_file
osSystem.system(cmd)

local_path, local_file_name = os.path.split(exe_file)

import os
import pandas
import matplotlib.pyplot as plt

for file in os.listdir(local_path):
    complete_name = os.path.join(local_path, file)
    if os.path.isfile(complete_name) and file.endswith('csv'):
        result = pandas.read_csv(complete_name)

        for row in range(result.shape[0]): 
            plt.plot(list(result.iloc[row][1:result.shape[1]]))
        
        plt.savefig(local_path + '/' + file + '.png')
        plt.show()