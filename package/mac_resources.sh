# CMAKE_INSTALL_APP="/Users/creality/Orca_work/c3d_6.0/C3DSlicer/build_x86_64/_CPack_Packages/macx-x86_64/DragNDrop/CrealityPrint_6.0.0.495_Alpha/"
# xxx=$0
# yyy=$1
# echo "Fix macOS app package333...$xxx , $yyy"
# (
#     cd "$CMAKE_INSTALL_APP"
#     echo "CMAKE_INSTALL_APP=$CMAKE_INSTALL_APP"
#     # fix resources
#     resources_path=$(readlink ./CrealityPrint.app/Contents/resources)
#     rm ./CrealityPrint.app/Contents/Resources
#     cp -R "$resources_path" ./CrealityPrint.app/Contents/Resources
#     # delete .DS_Store file
#     find ./CrealityPrint.app/ -name '.DS_Store' -delete
# )

#!/bin/bash

pushd "CrealityPrint.app/Contents/"

dirs=`find . -type d -depth 1`

for dir in $dirs; do
    filename="$(basename "${dir}")"
    mv "${dir}" "../Resources/"
    ln -s "../Resources/${filename}" .
done

popd