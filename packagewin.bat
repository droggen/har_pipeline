@echo off

echo Packaging...

IF "%1"=="" GOTO HAVE_0

rd /q /s windows
del /q har_pipeline-win.zip
md windows


set vcinstalldir=C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\

%1\windeployqt.exe build-har_pipeline-Desktop_Qt_5_15_0_MSVC2019_64bit-Release\release\har_pipeline.exe --dir windows --force --compiler-runtime 

copy build-har_pipeline-Desktop_Qt_5_15_0_MSVC2019_64bit-Release\release\har_pipeline.exe windows
rem copy "%1\libstdc++-6.dll" windows
rem copy %1\libwinpthread-1.dll windows
rem copy %1\libgcc_s_dw2-1.dll windows


cd windows
zip -r ..\har_pipeline-win.zip * -x *.svn*
cd..

echo Packaging done

exit /b

:HAVE_0
echo Specify the path to qt. Example: %0 C:\Qt\5.15.0\msvc2019_64\bin

