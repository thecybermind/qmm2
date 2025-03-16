rmdir /s /q package

mkdir package
copy README.md .\package\
copy LICENSE .\package\
copy msvc\Release\qmm2.dll .\package\
cd package
if "%GITHUB_TOKEN%"=="" (
  powershell "Compress-Archive -Path * -Destination ..\qmm2-latest.zip"
) else (
  powershell "Compress-Archive -Path * -Destination ..\qmm2-${{ github.ref_name }}.zip"
)
cd ..
rmdir /s /q package

mkdir package
copy README.md .\package\
copy LICENSE .\package\
copy msvc\Debug\qmm2.dll .\package\
cd package
if "%GITHUB_TOKEN%"=="" (
  powershell "Compress-Archive -Path * -Destination ..\qmm2-latest-debug.zip"
) else (
  powershell "Compress-Archive -Path * -Destination ..\qmm2-${{ github.ref_name }}-debug.zip"
)  
cd ..
rmdir /s /q package
