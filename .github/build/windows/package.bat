mkdir package
pushd package
del /q *
rem copy ..\README.md .\
rem copy ..\LICENSE .\
rem copy ..\qmm2.json .\
copy ..\bin\Release\x86\qmm2.dll .\
copy ..\bin\Release\x86_64\qmm2_x86_64.dll .\
copy qmm2.dll jampgamex86.dll
powershell "Compress-Archive -Path jampgamex86.dll -Destination zzz_qmm_jamp.zip"
ren zzz_qmm_jamp.zip zzz_qmm_jamp.pk3
del jampgamex86.dll
popd
