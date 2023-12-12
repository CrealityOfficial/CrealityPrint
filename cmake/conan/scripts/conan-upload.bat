@echo off

if [%1] == [] (
	echo "usage: conan upload [recipe]"
	exit /b 0
)

conan upload %1 -r artifactory --all
