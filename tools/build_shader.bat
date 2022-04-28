.\shaderc.exe -f ..\homework\vs.sc -i ..\bgfx\src -o ..\build\shaders\glsl\vs.bin --type v --platform windows --Werror
.\shaderc.exe -f ..\homework\fs.sc -i ..\bgfx\src -o ..\build\shaders\glsl\fs.bin --type f --platform windows --Werror
.\shaderc.exe -f ..\homework\vs_sky.sc -i ..\bgfx\src -o ..\build\shaders\glsl\vs_sky.bin --type v --varyingdef ..\homework\varying.def_sky.sc --platform windows --Werror
.\shaderc.exe -f ..\homework\fs_sky.sc -i ..\bgfx\src -o ..\build\shaders\glsl\fs_sky.bin --type f --varyingdef ..\homework\varying.def_sky.sc --platform windows --Werror
.\shaderc.exe -f ..\homework\vs_shadow.sc -i ..\bgfx\src -o ..\build\shaders\glsl\vs_shadow.bin --type v --varyingdef ..\homework\varying.def_shadow.sc --platform windows --Werror
.\shaderc.exe -f ..\homework\fs_shadow.sc -i ..\bgfx\src -o ..\build\shaders\glsl\fs_shadow.bin --type f --varyingdef ..\homework\varying.def_shadow.sc --platform windows --Werror

.\shaderc.exe -f ..\homework\vs.sc -i ..\bgfx\src -o ..\build\shaders\dx9\vs.bin --type v --platform windows -p vs_3_0
.\shaderc.exe -f ..\homework\fs.sc -i ..\bgfx\src -o ..\build\shaders\dx9\fs.bin --type f --platform windows -p ps_3_0
.\shaderc.exe -f ..\homework\vs_sky.sc -i ..\bgfx\src -o ..\build\shaders\dx9\vs_sky.bin --type v --varyingdef ..\homework\varying.def_sky.sc --platform windows -p vs_3_0
.\shaderc.exe -f ..\homework\fs_sky.sc -i ..\bgfx\src -o ..\build\shaders\dx9\fs_sky.bin --type f --varyingdef ..\homework\varying.def_sky.sc --platform windows -p ps_3_0
.\shaderc.exe -f ..\homework\vs_shadow.sc -i ..\bgfx\src -o ..\build\shaders\dx9\vs_shadow.bin --type v --varyingdef ..\homework\varying.def_shadow.sc --platform windows -p vs_3_0
.\shaderc.exe -f ..\homework\fs_shadow.sc -i ..\bgfx\src -o ..\build\shaders\dx9\fs_shadow.bin --type f --varyingdef ..\homework\varying.def_shadow.sc --platform windows -p ps_3_0

.\shaderc.exe -f ..\homework\vs.sc -i ..\bgfx\src -o ..\build\shaders\dx11\vs.bin --type v --platform windows -p vs_4_0
.\shaderc.exe -f ..\homework\fs.sc -i ..\bgfx\src -o ..\build\shaders\dx11\fs.bin --type f --platform windows -p ps_4_0
.\shaderc.exe -f ..\homework\vs_sky.sc -i ..\bgfx\src -o ..\build\shaders\dx11\vs_sky.bin --type v --varyingdef ..\homework\varying.def_sky.sc --platform windows -p vs_4_0
.\shaderc.exe -f ..\homework\fs_sky.sc -i ..\bgfx\src -o ..\build\shaders\dx11\fs_sky.bin --type f --varyingdef ..\homework\varying.def_sky.sc --platform windows -p ps_4_0
.\shaderc.exe -f ..\homework\vs_shadow.sc -i ..\bgfx\src -o ..\build\shaders\dx11\vs_shadow.bin --type v --varyingdef ..\homework\varying.def_shadow.sc --platform windows -p vs_4_0
.\shaderc.exe -f ..\homework\fs_shadow.sc -i ..\bgfx\src -o ..\build\shaders\dx11\fs_shadow.bin --type f --varyingdef ..\homework\varying.def_shadow.sc --platform windows -p ps_4_0

pause