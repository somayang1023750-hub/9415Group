@echo off
setlocal
pushd "%~dp0..\.."
cmake -S . -B build-report -DCRAFT_BUILD_MEMBER_A_SHOWCASE=ON
if errorlevel 1 goto failed
cmake --build build-report --config Debug --target MemberAShowcase --parallel 4
if errorlevel 1 goto failed
echo Build complete. Run build-report\Debug\MemberAShowcase.exe
if exist build-report\MemberAShowcase.exe echo Or run build-report\MemberAShowcase.exe for a single-config generator.
popd
pause
exit /b 0
:failed
echo Build failed. Use a Visual Studio Developer Command Prompt with the same dependencies as your normal game build.
popd
pause
exit /b 1
