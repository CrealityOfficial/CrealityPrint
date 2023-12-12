echo off

set remain=%1
:submodule_loop
for /f "tokens=1,* delims=." %%a in ("%remain%") do (
	git submodule add git@gitee.com:zeng_gui/%%a.git
	set remain=%%b
)
if defined remain goto :submodule_loop


