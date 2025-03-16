mkdir package
cd package
del /q *
rem copy ..\README.md .\
rem copy ..\LICENSE .\
rem copy ..\qmm2.json .\
copy ..\bin\Release\qmm2.dll .\
copy qmm2.dll jampgamex86.dll
powershell "Compress-Archive -Path jampgamex86.dll -Destination zzz_qmm_ja.pk3"
del jampgamex86.dll
cd ..
