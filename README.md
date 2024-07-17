
## Creality Print 5.x

We are delighted to announce that the brand new version 5.0 has been released officially, with all upgrades aimed at improving user experience.  
Firstly, with a new user interface and interaction experience, the process settings system has become simpler, and the slicer has become more user-friendly and intelligent. The K series/v3 high-speed machines now support 42 kinds of official filaments and general parameters. We have not only readjusted the machine process parameters, but all parameters can be saved and exported for sharing.  
Moreover, through our ongoing efforts, version 5.0 has faster printing speed and better printing quality.Now let's see together what other surprises there.



## 1\. Key Improvements

## 1.1 Revolutionary User Experience

### Comprehensive Upgrade of UI

-   With the upgrading of the company's Corporate Identity (VI) system to version 5.0, we have rolled out a fully revamped UI system. A brand-new logo, theme color, icons, and page layout will be introduced to provide users with a fresh visual experience.

![](https://wiki.creality.com/crealityprint/500release/pic1.png)

-   We adjusted the UI layout after the upgrade according to mainstream visual habits: we repositioned the toolbar and prioritized the process setting systems. This made the operation interface clearer, not only reducing the operation steps for settings, but also greatly enhancing the smoothness of the user experience.

![](https://wiki.creality.com/crealityprint/500release/pic2.png)

### Printing Becomes a Piece of Cake

-   After extensive model testing and parameter adjustment, we have developed several configurations for different machines and nozzle types. You can conveniently select an appropriate configuration for one-click printing. For instance, we offer four configurations for the K1C-0.4 nozzle.
-   We constantly monitor the processing status of the model to minimize any factors that could negatively affect print quality. Should there be any issues, our user interface will promptly provide friendly cues in the bottom-left corner, assisting users in promptly identifying and resolving the issue to minimize any potential losses.

![](https://wiki.creality.com/crealityprint/500release/pic3.gif)

-   The variety of settings often causes confusion. For this reason, we have selected a set of commonly used parameters as the basic settings, enabling you to quickly adjust common parameters and diminish the impact of unnecessary information.

![](https://wiki.creality.com/crealityprint/500release/pic4.png)

-   5.0 Version introduces a wealth of diagrams for parameters, illustrating complex parameters in an intuitive way. Along with these diagrams and elaborate descriptions, you can quickly understand the meaning of each parameter.

![](https://wiki.creality.com/crealityprint/500release/pic5.gif)

![](https://wiki.creality.com/crealityprint/500release/pic6.png)

### More Efficient Process Tuning

-   Indeed, we also accommodate more extensive parameter configurations. Simply switch to the advanced settings mode to access comprehensive parameter information, facilitating broad and meticulous adjustment.

![](https://wiki.creality.com/crealityprint/500release/pic7.png)

Normal

![](https://wiki.creality.com/crealityprint/500release/pic8.png)

Advance

-   Evidently, the valid range of parameters is closely associated with various factors such as the types of consumables and the device model. This often makes it challenging for users to accurately judge the validity of settings. To address this issue, we have established a suitable range for each parameter and provided a color-coded input box. This allows users to easily determine the status of parameters while making changes.

![](https://wiki.creality.com/crealityprint/500release/pic9.png)

-   Previewing slices has now become WYSIWYG - what you see is what you get. We now support real-time parameter modifications during previews, enabling you to rapidly observe the effects of your adjustments.

![](https://wiki.creality.com/crealityprint/500release/pic10.gif)

### More Convenient Professional Settings

-   We not only support global settings but also offer flexible and detailed solutions. When printing multiple models in the same job, you can use object settings to apply individual parameters to each model. Each model is sliced with the parameters that suit it best, effectively enhancing the print quality and efficiency.

![](https://wiki.creality.com/crealityprint/500release/pic11.gif)

-   Supports the comparison of different types of machines, materials, and process parameters. By using this feature, you can thoroughly compare the differences，to find out the key parameters affecting quality, providing guidance on the optimal direction for your printing tasks.

![](https://wiki.creality.com/crealityprint/500release/pic12.png)

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
Creality Print is licensed under the GNU Affero General Public License, version 3. 

Creality Print slicer engine is is based on [Orca Slicer]([https://github.com/Ultimaker/CuraEngine](https://github.com/SoftFever/OrcaSlicer)) by SoftFever.

Orca Slicer is licensed under the GNU Affero General Public License, version 3. Orca Slicer is based on Bambu Studio by BambuLab.

Bambu Studio is licensed under the GNU Affero General Public License, version 3. Bambu Studio is based on PrusaSlicer by PrusaResearch.

PrusaSlicer is licensed under the GNU Affero General Public License, version 3. PrusaSlicer is owned by Prusa Research. PrusaSlicer is originally based on Slic3r by Alessandro Ranellucci.

Slic3r is licensed under the GNU Affero General Public License, version 3. Slic3r was created by Alessandro Ranellucci with the help of many other contributors.

The GNU Affero General Public License, version 3 ensures that if you use any part of this software in any way (even behind a web server), your software must be released under the same license.

Creality Print includes a pressure advance calibration pattern test adapted from Andrew Ellis' generator, which is licensed under GNU General Public License, version 3. Ellis' generator is itself adapted from a generator developed by Sineos for Marlin, which is licensed under GNU General Public License, version 3.

The GNU Affero General Public License, version 3 ensures that if you use any part of this software in any way (even behind a web server), your software must be released under the same license.
