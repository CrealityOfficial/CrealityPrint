@echo off

conan create . -s build_type=Debug
conan create . -s build_type=Release
