@echo off
setlocal
rem Run after building Debug|x64, from an x64 Visual Studio Developer Command Prompt.
where cl >nul 2>nul
if errorlevel 1 (
    echo Run this script from an x64 Visual Studio Developer Command Prompt.
    exit /b 1
)
set "GR_TEST_ROOT=%~dp0.."
set "GR_TEST_INCLUDE=%GR_TEST_ROOT%\GeometryRendererLib"
set "GR_TEST_OUTPUT=%TEMP%\GeometryRendererPropertyTests"
if not exist "%GR_TEST_OUTPUT%" mkdir "%GR_TEST_OUTPUT%"
pushd "%GR_TEST_OUTPUT%"
cl /nologo /W4 /TC /c /I "%GR_TEST_INCLUDE%" "%GR_TEST_INCLUDE%\line_item.c" "%GR_TEST_INCLUDE%\flatten_item.c" "%GR_TEST_INCLUDE%\dash_pattern_item.c" "%GR_TEST_INCLUDE%\thick_line_item.c" "%GR_TEST_INCLUDE%\path_stages.c" "%GR_TEST_INCLUDE%\rasterize_pipeline.c" "%GR_TEST_INCLUDE%\base_object.c" "%GR_TEST_INCLUDE%\raster_slab_item.c"
if errorlevel 1 goto failed
lib /nologo /out:property_test_core.lib line_item.obj flatten_item.obj dash_pattern_item.obj thick_line_item.obj path_stages.obj rasterize_pipeline.obj base_object.obj raster_slab_item.obj
if errorlevel 1 goto failed
cl /nologo /W4 /TC /I "%GR_TEST_INCLUDE%" "%GR_TEST_ROOT%\tests\stage_property_tests.c" /Fe:stage_property_tests.exe /link property_test_core.lib
if errorlevel 1 goto failed
stage_property_tests.exe
if errorlevel 1 goto failed
cl /nologo /W4 /TC /I "%GR_TEST_INCLUDE%" "%GR_TEST_ROOT%\tests\reliability_tests.c" /Fe:reliability_tests.exe /link property_test_core.lib
if errorlevel 1 goto failed
reliability_tests.exe
if errorlevel 1 goto failed
cl /nologo /W4 /TC /I "%GR_TEST_INCLUDE%" "%GR_TEST_ROOT%\tests\thick_geometry_tests.c" /Fe:thick_geometry_tests.exe /link property_test_core.lib
if errorlevel 1 goto failed
thick_geometry_tests.exe
if errorlevel 1 goto failed
cl /nologo /W4 /TC /I "%GR_TEST_INCLUDE%" "%GR_TEST_ROOT%\tests\raster_slab_tests.c" /Fe:raster_slab_tests.exe /link property_test_core.lib
if errorlevel 1 goto failed
raster_slab_tests.exe
if errorlevel 1 goto failed
cl /nologo /W4 /TC /I "%GR_TEST_INCLUDE%" "%GR_TEST_ROOT%\tests\dash_pattern_tests.c" /Fe:dash_pattern_tests.exe /link property_test_core.lib
if errorlevel 1 goto failed
dash_pattern_tests.exe
if errorlevel 1 goto failed
copy /y "%GR_TEST_ROOT%\x64\Debug\GeometryRenderer.dll" . >nul
if errorlevel 1 goto failed
copy /y "%GR_TEST_ROOT%\GeometryRenderer.NET.Bindings\bin\Debug\GeometryRenderer.NET.Bindings.dll" . >nul
if errorlevel 1 goto failed
"%VSINSTALLDIR%MSBuild\Current\Bin\Roslyn\csc.exe" /nologo /platform:x64 /r:GeometryRenderer.NET.Bindings.dll /r:System.Drawing.dll /out:stage_property_bindings_tests.exe "%GR_TEST_ROOT%\tests\stage_property_bindings_tests.cs"
if errorlevel 1 goto failed
stage_property_bindings_tests.exe
if errorlevel 1 goto failed
popd
exit /b 0
:failed
popd
exit /b 1
