@echo off
echo Building Kick Chat Overlay...

REM Check if CMake is available
where cmake >nul 2>nul
if %ERRORLEVEL% neq 0 (
    echo ERROR: CMake not found in PATH. Please install CMake and add it to your PATH.
    echo Visit https://cmake.org/download/ to download CMake.
    pause
    exit /b 1
)

REM Check if build directory exists and create it if it doesn't
if not exist build mkdir build

REM Navigate to build directory
cd build

REM Configure with CMake
echo Configuring project with CMake...
cmake ..
if %ERRORLEVEL% neq 0 (
    echo ERROR: CMake configuration failed. Please check if Qt is properly installed.
    echo You may need to specify Qt path with -DCMAKE_PREFIX_PATH=path/to/Qt/6.x/msvc2019_64
    pause
    exit /b 1
)

REM Build the project
echo Building project...
cmake --build . --config Release
if %ERRORLEVEL% neq 0 (
    echo ERROR: Build failed. Please check the errors above.
    pause
    exit /b 1
)

echo.
echo Build completed successfully!
echo The executable should be in the build\Release directory.
echo.
echo Run the application, right-click on the overlay, and connect to your Kick channel.
pause 