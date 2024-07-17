
# Creality Print
Creality Print is feature-rich FDM slicing software from [Creality](https://www.creality.com/). It contains model libraries, machine control and other functions, as well as cutting-edge slicing algorithms, providing users with the best printing experience.
Creality Print slicer engine is based on [CuraEngine](https://github.com/Ultimaker/CuraEngine) by Ultimaker，Some functions refer to [PrusaSlicer](https://github.com/prusa3d/PrusaSlicer) (Auto-arrange,Auto-orient,Support G2/G3,Overhang down speed).  

Calibration function ideas and models come from the open source community. Thanks [OrcaSlicer](https://github.com/SoftFever/OrcaSlicer)
  1. The Flowrate test and retraction test is inspired by [SuperSlicer](https://github.com/supermerill/SuperSlicer).
  2. The PA Tower method is inspired by [Klipper](https://marlinfw.org/tools/lin_advance/k-factor.html).
  3. The temp tower model is remixed from [Smart compact temperature calibration tower](https://www.thingiverse.com/thing:2729076).
  4. The max flowrate test was inspired by [Stefan(CNC Kitchen)](), and the model used in the test is a remix of his [Extrusion Test Structure](https://www.printables.com/model/342075-extrusion-test-structure).
If you notice something missing from this list, please contact crealityprint@creality.com or add an issue to this github project.
See the [wiki](http://wiki.creality.com) and the documentation directory for more informations.

## Main features
### Key features are:
- Basic slicing features (TThis feature comes from CuraEngine, Thanks Ultimaker)
- Normal/Tree support (This feature comes from CuraEngine, Thanks Ultimaker)
- Auto-arrange objects (This feature comes from PrusaSlicer, Thanks PrusaSlicer)
- Auto-orient objects (This feature comes from PrusaSlicer, Thanks PrusaSlicer)
- G-Code viewer
- Creality Cloud model libraries
- Local network control & monitoring
- Split & Hollow &Drill
- Measure
- Customized support 
- multi-platform (Win/Mac/Linux) support
  
### Other major features are:
- Variable line width (This feature comes from CuraEngine, Thanks Ultimaker)
- Auto temperature (This feature comes from CuraEngine, Thanks Ultimaker)
- Support G2/G3(This feature comes from PrusaSlicer, Thanks PrusaSlicer)
- Overhang down speed (This feature comes from PrusaSlicer, Thanks PrusaSlicer)
- Supports many kinds of model file formats (stl, obj, ply, 3mf, 3ds, dae, off, STEP)
- Repair
- Convert image to model
- Standard models
- Calibration

## How to compile
```
cmake, git, python3.7+, conan 1.50, compiler toolchains should be installed
remember call "git submodule update --init" to update the submodules after clone
 
build project as follow for the first time:
windows .\cmake\ci\cmake.py -c -b -e --channel_name=opensource
linux or mac python3 ./cmake/ci/cmake.py -c -b -e --channel_name=opensource

if the conan libs haved been installed, you can build project directly:
windows .\cmake\ci\cmake.py 
linux or mac python3 ./cmake/ci/cmake.py
```




## License
Creality Print is licensed under the GNU Affero General Public License, version 3. Creality Print slicer engine is is based on [CuraEngine](https://github.com/Ultimaker/CuraEngine) by Ultimaker.
CuraEngine is licensed under the GNU Affero General Public License, version 3. CuraEngine is owned by Ultimaker. 
The GNU Affero General Public License, version 3 ensures that if you use any part of this software in any way (even behind a web server), your software must be released under the same license.
