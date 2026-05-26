@echo off
set BUILD_DIR=build\ui\Debug
set VCPKG_DIR=C:\vcpkg\installed\x64-windows\debug

echo Copying Qt DLLs...
copy /Y "%VCPKG_DIR%\bin\Qt6Cored.dll" "%BUILD_DIR%"
copy /Y "%VCPKG_DIR%\bin\Qt6Guid.dll" "%BUILD_DIR%"
copy /Y "%VCPKG_DIR%\bin\Qt6Widgetsd.dll" "%BUILD_DIR%"
copy /Y "%VCPKG_DIR%\bin\Qt6Networkd.dll" "%BUILD_DIR%"
copy /Y "%VCPKG_DIR%\bin\Qt6WebSocketsd.dll" "%BUILD_DIR%"

echo Copying plugins...
xcopy /E /I /Y "%VCPKG_DIR%\Qt6\plugins\platforms" "%BUILD_DIR%\platforms"
xcopy /E /I /Y "%VCPKG_DIR%\Qt6\plugins\imageformats" "%BUILD_DIR%\imageformats"
xcopy /E /I /Y "%VCPKG_DIR%\Qt6\plugins\tls" "%BUILD_DIR%\tls"

echo Done!