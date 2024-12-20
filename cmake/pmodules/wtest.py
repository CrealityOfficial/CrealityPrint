import ccglobal
t = ccglobal.Tracer()

import mix_io
fileName = "C:\\Users\\anoob\\code\\data\\model\\cylinder.STL"
print(mix_io.load(fileName, t))
    
# ## test derived
#
#import pw_trimesh
#
#base = pw_trimesh.Base()
#
#class Derived(pw_trimesh.Base):
#    def f(self):
#        pass
#
#derived = Derived()
#
#print(pw_trimesh.useBase())
#print(pw_trimesh.useBase(base))
#print(pw_trimesh.useBase(derived))    