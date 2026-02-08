msbuild .\msvc\qmm2.vcxproj /p:Configuration=Debug /p:Platform=x86
if errorlevel 1 exit /b errorlevel
msbuild .\msvc\qmm2.vcxproj /p:Configuration=Debug /p:Platform=x64
if errorlevel 1 exit /b errorlevel
