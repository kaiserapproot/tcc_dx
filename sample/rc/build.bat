tcc rc2obj.c
rc2obj.exe Rsrc.rc Rsrc.obj
tcc main_gl.c Rsrc.obj -lopengl32 -lglut32 -lgdi32 -luser32 -lkernel32 -lgdi32 -lmsvcrt -ld3d9 -limm32 -lcomdlg32 -g -ld3dcompiler_47