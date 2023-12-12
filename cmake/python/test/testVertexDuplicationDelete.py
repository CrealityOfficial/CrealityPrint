# coding:utf-8
# IF NEED
# python -m pip install matplotlib
import os
import matplotlib.pyplot as plt
import numpy as np
 

os.system ("demo_test_vertex_deleteDuplication F:/testDup/")


f = open(r'vertex_duplication.txt','r')
data = list(f)
print(data)
f.close()
v0 = []
v1 = []
for d in data:
	v0.append(d.split()[0])
	v1.append(d.split()[1])

print(v0)
print(v1)

plt.plot(v0)
plt.plot(v1)


plt.savefig("vertex_duplication.png")
plt.show()



