@echo off

mkdir emcc-build
cd emcc-build


if "%1"=="-D" (
    echo "Build Debug"
    mkdir Debug
    cd Debug
    conan install -g cmake_multi -s build_type=Debug ../../  -pr=emscripten
    conan build ../../ 
) else (
    mkdir Release
    cd Release
    conan install -g cmake_multi -s build_type=Release ../../ -pr=emscripten
    conan build ../../ 
)			 
cd ../../