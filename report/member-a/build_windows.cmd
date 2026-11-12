@echo off
setlocal
pushd "%~dp0..\.."
if errorlevel 1 goto failed_no_pop
set "TRAINING_BUILD=%CD%\build-training-range"
echo SOURCE_ROOT: "%CD%"
echo SOURCE_COMMIT:
git log -1 --oneline
findstr /l /c:"MemberATrainingRange v2" "report\member-a\main.cpp" >nul
if errorlevel 1 goto wrong_source
echo Building the NEW MemberATrainingRange v2. This script launches it automatically.
where cmake >nul 2>nul
if errorlevel 1 goto missing_cmake
cmake -S "%CD%" -B "%TRAINING_BUILD%" -DCMAKE_BUILD_TYPE=Debug -DCRAFT_BUILD_MEMBER_A_SHOWCASE=ON
if errorlevel 1 goto failed
cmake --build "%TRAINING_BUILD%" --config Debug --target MemberAShowcase --parallel 4
if errorlevel 1 goto failed
set "TRAINING_EXE="
if not exist "%TRAINING_BUILD%\member-a-training-Debug.txt" goto failed
set /p "TRAINING_EXE="<"%TRAINING_BUILD%\member-a-training-Debug.txt"
if not defined TRAINING_EXE goto failed
if not exist "%TRAINING_EXE%" goto failed
echo LAUNCHING_EXACT_FILE: "%TRAINING_EXE%"
for %%I in ("%TRAINING_EXE%") do echo FILE_TIMESTAMP: %%~tI
"%TRAINING_EXE%"
if errorlevel 1 goto failed
echo Training range closed.
popd
pause
exit /b 0
:wrong_source
echo The source file is not training-range v2. Check git status and update this checkout.
goto failed
:missing_cmake
echo CMake was not found. Run this script in the Visual Studio Developer Command Prompt.
:failed
echo Build or launch FAILED. No older executable will be launched as a fallback.
echo Keep this window open and copy the FIRST error above.
popd
:failed_no_pop
pause
exit /b 1
