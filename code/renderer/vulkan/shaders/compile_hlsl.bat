@echo off
set "VSCMD_START_DIR=%CD%"
call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\Common7\Tools\VsDevCmd.bat"

set tools_dir=..\..\..\..\tools
set bin2hex=%tools_dir%\bin2hex.exe
set bin2hex_cpp=%tools_dir%\bin2hex.cpp

if not exist %bin2hex% (
    cl.exe /EHsc /nologo /Fe%tools_dir%\ /Fo%tools_dir%\ %bin2hex_cpp%
)

set PATH=%tools_dir%;%PATH%

@rem single texture VS
fxc.exe /nologo /T vs_4_0 /E single_texture_vs /Fo shader.bin shaders.hlsl
%bin2hex% shader.bin single_texture_vs > hlsl_compiled/single_texture_vs.cpp
del shader.bin

fxc.exe /nologo /T vs_4_0 /E single_texture_clipping_plane_vs /Fo shader.bin shaders.hlsl
%bin2hex% shader.bin single_texture_clipping_plane_vs > hlsl_compiled/single_texture_clipping_plane_vs.cpp
del shader.bin

@rem multi texture VS
fxc.exe /nologo /T vs_4_0 /E multi_texture_vs /Fo shader.bin shaders.hlsl
%bin2hex% shader.bin multi_texture_vs > hlsl_compiled/multi_texture_vs.cpp
del shader.bin

fxc.exe /nologo /T vs_4_0 /E multi_texture_clipping_plane_vs /Fo shader.bin shaders.hlsl
%bin2hex% shader.bin multi_texture_clipping_plane_vs > hlsl_compiled/multi_texture_clipping_plane_vs.cpp
del shader.bin

@rem signle texture PS
fxc.exe /nologo /T ps_4_0 /E single_texture_ps /Fo shader.bin shaders.hlsl
%bin2hex% shader.bin single_texture_ps > hlsl_compiled/single_texture_ps.cpp
del shader.bin

fxc.exe /nologo /T ps_4_0 /E single_texture_ps /Fo shader.bin shaders.hlsl /DALPHA_TEST_GT0
%bin2hex% shader.bin single_texture_gt0_ps > hlsl_compiled/single_texture_gt0_ps.cpp
del shader.bin

fxc.exe /nologo /T ps_4_0 /E single_texture_ps /Fo shader.bin shaders.hlsl /DALPHA_TEST_LT80
%bin2hex% shader.bin single_texture_lt80_ps > hlsl_compiled/single_texture_lt80_ps.cpp
del shader.bin

fxc.exe /nologo /T ps_4_0 /E single_texture_ps /Fo shader.bin shaders.hlsl /DALPHA_TEST_GE80
%bin2hex% shader.bin single_texture_ge80_ps > hlsl_compiled/single_texture_ge80_ps.cpp
del shader.bin

@rem multi texture mul PS
fxc.exe /nologo /T ps_4_0 /E multi_texture_mul_ps /Fo shader.bin shaders.hlsl
%bin2hex% shader.bin multi_texture_mul_ps > hlsl_compiled/multi_texture_mul_ps.cpp
del shader.bin

fxc.exe /nologo /T ps_4_0 /E multi_texture_mul_ps /Fo shader.bin shaders.hlsl /DALPHA_TEST_GT0
%bin2hex% shader.bin multi_texture_mul_gt0_ps > hlsl_compiled/multi_texture_mul_gt0_ps.cpp
del shader.bin

fxc.exe /nologo /T ps_4_0 /E multi_texture_mul_ps /Fo shader.bin shaders.hlsl /DALPHA_TEST_LT80
%bin2hex% shader.bin multi_texture_mul_lt80_ps > hlsl_compiled/multi_texture_mul_lt80_ps.cpp
del shader.bin

fxc.exe /nologo /T ps_4_0 /E multi_texture_mul_ps /Fo shader.bin shaders.hlsl /DALPHA_TEST_GE80
%bin2hex% shader.bin multi_texture_mul_ge80_ps > hlsl_compiled/multi_texture_mul_ge80_ps.cpp
del shader.bin

@rem multi texture add PS
fxc.exe /nologo /T ps_4_0 /E multi_texture_add_ps /Fo shader.bin shaders.hlsl
%bin2hex% shader.bin multi_texture_add_ps > hlsl_compiled/multi_texture_add_ps.cpp
del shader.bin

fxc.exe /nologo /T ps_4_0 /E multi_texture_add_ps /Fo shader.bin shaders.hlsl /DALPHA_TEST_GT0
%bin2hex% shader.bin multi_texture_add_gt0_ps > hlsl_compiled/multi_texture_add_gt0_ps.cpp
del shader.bin

fxc.exe /nologo /T ps_4_0 /E multi_texture_add_ps /Fo shader.bin shaders.hlsl /DALPHA_TEST_LT80
%bin2hex% shader.bin multi_texture_add_lt80_ps > hlsl_compiled/multi_texture_add_lt80_ps.cpp
del shader.bin

fxc.exe /nologo /T ps_4_0 /E multi_texture_add_ps /Fo shader.bin shaders.hlsl /DALPHA_TEST_GE80
%bin2hex% shader.bin multi_texture_add_ge80_ps > hlsl_compiled/multi_texture_add_ge80_ps.cpp
del shader.bin
