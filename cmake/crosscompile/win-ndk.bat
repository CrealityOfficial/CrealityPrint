@echo OFF

set PATH=C:/Users/cx1959/AppData/Local/Android/Sdk/cmake/3.10.2.4988404/bin/;%PATH%
set ANDROID_NDK=C:/Users/cx1959/AppData/Local/Android/Sdk/ndk/22.1.7171670/
:: NDK
if not defined ANDROID_NDK (
	echo "ERROR:environment variable ANDROID_NDK not defined" 
	exit /b 255
	)

cd %~dp0

cd ../../

mkdir ndk-build
cd ndk-build

mkdir debug
cd debug

for %%a in (armeabi-v7a,arm64-v8a,x86,x86_64) do (
	echo "abi %%a"
	mkdir %%a
	cd %%a

	if "%%a"=="armeabi-v7a" ( 
		set ANDROID_TOOLCHAIN_NAME=arm-linux-androideabi
		set ARCH=ARMEABI-V7A
	) else if "%%a"=="arm64-v8a" (
		set ANDROID_TOOLCHAIN_NAME="aarch64-linux-android"
		set ARCH=ARM64-V8A
	) else if "%%%a"=="x86" (
		set ANDROID_TOOLCHAIN_NAME=i686-linux-android
		set ARCH=X86
	) else if "%%a"=="x86_64" (
		set ANDROID_TOOLCHAIN_NAME=x86_64-linux-android
		set ARCH=x64
	) else (
		echo "Invalid Android ABI: %%a." 
		exit /b 255
	)

	cmake -S ../../../ ^
		-G "Ninja" ^
		-DCMAKE_BUILD_TYPE=Debug ^
		-DANDROID_CROSSING=1 ^
		-DCMAKE_CXX_COMPILER_ARCHITECTURE_ID=%ARCH% ^
		-DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=BOTH ^
		-DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=BOTH ^
		-DCMAKE_TOOLCHAIN_FILE=%ANDROID_NDK%\build\cmake\android.toolchain.cmake ^
		|| exit /b
		
	ninja || exit /b 2
	
	cd ..
)

cd ..

mkdir release
cd release

for %%a in (armeabi-v7a,arm64-v8a,x86,x86_64) do (
	echo "abi %%a"
	mkdir %%a
	cd %%a

	if "%%a"=="armeabi-v7a" ( 
		set ANDROID_TOOLCHAIN_NAME=arm-linux-androideabi
		set ARCH=ARMEABI-V7A
	) else if "%%a"=="arm64-v8a" (
		set ANDROID_TOOLCHAIN_NAME="aarch64-linux-android"
		set ARCH=ARM64-V8A
	) else if "%%%a"=="x86" (
		set ANDROID_TOOLCHAIN_NAME=i686-linux-android
		set ARCH=X86
	) else if "%%a"=="x86_64" (
		set ANDROID_TOOLCHAIN_NAME=x86_64-linux-android
		set ARCH=x64
	) else (
		echo "Invalid Android ABI: %%a." 
		exit /b 255
	)

	cmake -S ../../../ ^
		-G "Ninja" ^
		-DCMAKE_BUILD_TYPE=Release ^
		-DANDROID_CROSSING=1 ^
		-DCMAKE_CXX_COMPILER_ARCHITECTURE_ID=%ARCH% ^
		-DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=BOTH ^
		-DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=BOTH ^
		-DCMAKE_TOOLCHAIN_FILE=%ANDROID_NDK%\build\cmake\android.toolchain.cmake ^
		|| exit /b
		
	ninja || exit /b 2
	
	cd ..
)

cd ../../