@echo off
SET ROOT=%CD%
SET ROOT_C3D=%ROOT%
IF EXIST "%ROOT_C3D%\tools\vswhere.exe" (
    for /f "usebackq tokens=*" %%i in (`%ROOT_C3D%\tools\vswhere.exe -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property catalog_productLineVersion`) do (
    set LastVerion=%%i
    )
)
if "%LastVerion%"=="2019" (
    set VS_Version=Visual Studio 16 2019
) else (
    set VS_Version=Visual Studio 17 2022
)
echo LastVerion= %LastVerion%

SET ACTION_DepBuild=DepBuild
SET ACTION_AllGenerate=AllGenerate
SET ACTION_Extract=ExtractDeps
SET ACTION_Generate=Generate
SET ACTION_Build=Build
SET ACTION_Project=Project

@REM SET TASK=orca
@REM SET TASK_All=all

SET TYPE_Debug=Debug
SET TYPE_Release=Release

SET LIST_TYPE[0]=%TYPE_Debug%
SET LIST_TYPE[1]=%TYPE_Release%

setlocal enabledelayedexpansion

SET ACTION=%ACTION_Project%


@REM orca Debug
SET LIST_C3d[0]=OFF
@REM orca Release
SET LIST_C3d[1]=OFF


@REM deps path
SET LIST_DEPS_LIB[0]=%ROOT_C3D%\thirdDeps\dep_%TYPE_Debug%
SET LIST_DEPS_LIB[1]=%ROOT_C3D%\thirdDeps\dep_%TYPE_Release%


@REM Count the Argument
SET Argc=0
for %%x in (%*) do Set /A Argc+=1
IF %Argc% LSS 2 (
    goto Help
)

SET Arg1=%1
SET Arg2=%2
SET index=0

IF /i %Arg2%==%TYPE_Debug% (
    SET LIST_C3d[0]=ON
    SET index=0
)
IF /i %Arg2%==%TYPE_Release% (
    SET LIST_C3d[1]=ON
    SET index=1
)
SET SLICER_HEADER=1
if [-c] == [%3] (
    SET SLICER_HEADER=0
) else (
    SET SLICER_HEADER=1
)

SET USER_BUILD_DEPLIB[0]=%ROOT_C3D%\dep_%TYPE_Debug%
SET USER_BUILD_DEPLIB[1]=%ROOT_C3D%\dep_%TYPE_Release%

IF /i %Arg1%==%ACTION_Generate% (
    SET ACTION=%ACTION_Generate%
) ELSE (
    IF /i %Arg1%==%ACTION_Build% (
        SET ACTION=%ACTION_Build%
    ) ELSE (
        IF /i %Arg1%==%ACTION_Project% (
        SET ACTION=%ACTION_Project%
        @REM goto DepBuild
        ) ELSE (
            IF /i %Arg1%==%ACTION_Extract% (
            SET ACTION=%ACTION_Extract%
            ) ELSE (
                IF /i %Arg1%==%ACTION_DepBuild% (
                SET ACTION=%ACTION_DepBuild%
                goto DepBuild
                ) ELSE (
                    IF /i %Arg1%==%ACTION_AllGenerate% (
                    SET ACTION=%ACTION_AllGenerate%
                    goto DepBuild
                    ) ELSE (
                    SET ACTION=%ACTION_Project%
                    )
                )
            )
        )
    )
)


:GenerateC3d_Before
echo "Current ACTION = GenerateC3d_Before"
IF %ACTION%==%ACTION_Extract% (
    goto ExtractSource
) 
IF %ACTION%==%ACTION_Generate% (
    goto GenerateC3d
) ELSE (
    IF %ACTION%==%ACTION_Project% (
        goto GenerateC3d
    ) ELSE (
        IF %ACTION%==%ACTION_AllGenerate% (
            goto GenerateC3d
        ) ELSE (
            goto GenerateC3d_After
        )
    )
)

:DepBuild
echo "Current ACTION = DepBuild"
IF /i %Arg2%==%TYPE_Debug% (
    echo "build dep debug"
    call build_deps.bat Debug
) else (
    echo "build dep release"
    call build_deps.bat Release
)

:DepBuild_After
echo DepBuild_After
if %ACTION% == %ACTION_AllGenerate% (   
    goto GenerateC3d_Before
) ELSE (
    if %ACTION% == %ACTION_Project% (   
        goto GenerateC3d_Before
    ) else (
        exit /b 0
    )
)

:ExtractSource
@REM Extract thirdDeps.zip
cd %ROOT%
echo "Goto ExtractSource"
IF /i %Arg2%==%TYPE_Debug% (
    echo "unzip deps debug"
    call extract_deps.bat %ROOT% Debug || exit /b 1
) else (
    echo "unzip deps release"
    call extract_deps.bat %ROOT%  || exit /b 1
)

:ExtractSource_After
if %ACTION% == %ACTION_Extract% (   
    exit /b 0
) ELSE (
    goto GenerateC3d_Before
)

:GenerateC3d
echo GenerateC3d
SET CALL_RUN_GETTEXT=FALSE
echo %index%
if exist "!USER_BUILD_DEPLIB[%index%]!" (
    set LIST_DEPS_LIB[%index%]=!USER_BUILD_DEPLIB[%index%]!
)
if !LIST_C3d[%index%]!==ON (
    echo LIST_DEPS_LIB[%index%] is !LIST_DEPS_LIB[%index%]!
    if not exist "!LIST_DEPS_LIB[%index%]!" (
        goto ExtractSource
    )
    @REM Remove orca build dir if exist
    SET C3D_BUILD_DIR=%ROOT_C3D%\build_!LIST_TYPE[%index%]!
    @REM IF EXIST !C3D_BUILD_DIR!\ (
    @REM     echo RD /S /Q !C3D_BUILD_DIR!\
    @REM     RD /S /Q !C3D_BUILD_DIR!\
    @REM ) ELSE (
    @REM     echo HINT: !C3D_BUILD_DIR!\ NOT EXIST, skip remove
    @REM )
    
    @REM build orca
    echo "%ROOT_C3D%"
    cd %ROOT_C3D%
    mkdir build_!LIST_TYPE[%index%]!
    cd build_!LIST_TYPE[%index%]!
    SET DEP_INSTALL_DIR=!LIST_DEPS_LIB[%index%]!
    echo cmake .. -G "%VS_Version%" -A x64 -DGENERATE_ORCA_HEADER=!SLICER_HEADER! -DBBL_RELEASE_TO_PUBLIC=1 -DCMAKE_PREFIX_PATH="!DEP_INSTALL_DIR!\usr\local" -DCMAKE_INSTALL_PREFIX="./CrealityPrint" -DCMAKE_BUILD_TYPE=!LIST_TYPE[%index%]! -DWIN10SDK_PATH="%WindowsSdkDir%Include\%WindowsSDKLibVersion%"
    cmake .. -G "%VS_Version%" -A x64 -DGENERATE_ORCA_HEADER=!SLICER_HEADER! -DBBL_RELEASE_TO_PUBLIC=1 -DCMAKE_PREFIX_PATH="!DEP_INSTALL_DIR!\usr\local" -DCMAKE_INSTALL_PREFIX="./CrealityPrint" -DCMAKE_BUILD_TYPE=!LIST_TYPE[%index%]! -DWIN10SDK_PATH="%WindowsSdkDir%Include\%WindowsSDKLibVersion%" || exit /b 1

    @REM call text
    IF NOT !CALL_RUN_GETTEXT!==TRUE (
        cd ..
        echo call run_gettext.bat
        call run_gettext.bat || exit /b 1
        SET CALL_RUN_GETTEXT=TRUE
    ) ELSE (
        echo HINT: CALL_RUN_GETTEXT==TRUE, Skip call
    )

) ELSE (
    echo HINT: LIST_C3d[%index%]=OFF, Skip Build
)

:GenerateC3d_After
echo GenerateC3d After

:BuildC3d_Before
echo BuildC3d Before
echo action is %ACTION%
@REM goto BuildC3d_After
IF %ACTION%==%ACTION_Build% (
    goto BuildC3d
) ELSE (
    IF %ACTION%==%ACTION_Project% (
        goto BuildC3d
    ) ELSE (
        goto BuildC3d_After
    )
)

:BuildC3d
echo BuildC3d
if !LIST_C3d[%index%]!==ON (
    SET C3D_BUILD_DIR=%ROOT_C3D%\build_!LIST_TYPE[%index%]!
    cd !C3D_BUILD_DIR!
    echo cmake --build . --target install --config !LIST_TYPE[%index%]!
    cmake --build . --target install --config !LIST_TYPE[%index%]!  || exit /b 1
) ELSE (
    echo HINT: LIST_C3d[%index%]=OFF, Skip Build
)

:BuildC3d_After
echo BuildC3d After
goto End

:Help
echo -------------------------Help-------------------------
echo C3d:
echo project_build.bat DepBuild Debug
echo project_build.bat DepBuild Release
echo project_build.bat ExtractDeps  Debug
echo project_build.bat ExtractDeps  Release
echo project_build.bat Generate  Debug
echo project_build.bat Generate  Release
echo project_build.bat AllGenerate  Debug
echo project_build.bat AllGenerate  Release
echo project_build.bat Build  Debug
echo project_build.bat Build  Release
echo project_build.bat Project  Debug
echo project_build.bat Project  Release
echo -------------------------Help-------------------------
goto End

:End