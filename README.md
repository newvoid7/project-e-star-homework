# BGFX based E-star project homework @Netease

# 作业压缩包说明

二进制文件：`./bin/homework.exe`  
录制的演示视频：`./screen record.mp4`  
源码文件目录：`./homework/`  
程序架构说明文档：`./README.md`（本文档）

# 操作手册与主要特点

## 编译与生成

在build文件夹下：`$ cmake ..`即可。打开VS解决方案`EStarHomework.sln`，编译运行，会自动编译出OpenGL和DirectX11的shader添加到`shaders`文件夹。

## 操作

* 鼠标左键旋转环绕相机（顶端和底端限位）。键盘WASD或按下鼠标中键并拖拽都可以控制相机位置。鼠标滚轮缩放（远近都限位）。
* 左侧菜单显示CPU和GPU时间、帧率、显存占用。
* 左侧菜单可选择光照模型*Blinn-Phong*或*PBR*。
* 默认情况下一个平行光源自动地模拟日夜切换。可以使用复选框和滑杆手动控制。
* 左侧菜单底部提供了一个测试场景复选框。排列了25个不同粗糙度和金属度的球体。

## 主要特点

* 作业要求部分不再赘述。
* 草地使用了4个instance。但这个草地模型本身就很大（*OBJ文件385MB*），所以资源占用还是很高。
* 采用了透明混合。~~Ava的裙子蕾丝边是透明的😍🤤~~
* 支持多灯光（平行光、点光源和聚光灯皆可，最大数量可以改宏），但只有主灯有阴影，否则耗时太久。
* 阴影用了PCF滤波进行平滑。
* *其他填平的坑在程序架构说明里细说。*

# 程序架构说明

## 主程序源文件 `homework.cpp`

主程序中架构很简单：

* 使用`map`管理渲染项、光源、相机等。其他成员的定义不赘述。
* `init()`函数负责初始化BGFX和GUI、绑定着色器程序、构造几何体、构造灯光、构造一些uniform变量缓冲区等。
* `update()`函数刷新GUI（`buildGUI()`）、处理消息（`msgProc()`）、传输一些uniform变量，然后先渲染shadow pass，再渲染scene pass。
    - `buildGUI()`：从上至下布局菜单界面。
    - `msgProc()`：接受参数程序运行时间，对灯光进行变换；处理鼠标和键盘消息，对相机进行变换。
* `shutdown()`函数销毁一些程序、缓冲区。

## 渲染项目相关的头文件 `RenderItem.h`

* 类`RenderItem`，成员：
    - 指向mesh的指针，使用`meshLoad()`读入。
    - 使用到的贴图组（不区分颜色贴图、法线贴图和AORM贴图）。
    - 每个group的颜色贴图序号。由于`meshLoad()`读入的多group的mesh会全部乱掉，因此贴图全部需要重新指定。😭
    - 如果无颜色贴图，使用内置的颜色组。同样需要指定每个group的序号。
    - 每个group的法线贴图序号（如有）。
    - 每个group的AORM材质贴图序号（如有）。
    - 如果无AORM贴图，使用内置的AORM组。同样需要指定每个group的序号。
    - 关于颜色、法线、AORM是否采用贴图，需要一个另外的变量来标记，用来在着色器中决定是采样还是直接使用uniform变量。
    - 模型矩阵。
    - 多instance的矩阵（如有），在传入instance buffer前先将instance矩阵和模型矩阵相乘。
    - 指定各个group是否透明的变量。
* `RenderItem::render()`，scene pass的渲染。透明与不透明的group设置不同的state。
* `RenderItem::renderShadow()`，shadow pass的渲染，没什么好说的。
* `Build*RItem()`，构造`RenderItem`、读入一些mesh文件、进行一些设置的函数。
* `BuildTestSpheres()`，由于测试球的材质等信息不同，instancing的缓冲区变量又不够，只好定义了25个几何体。
* 类`EnvRenderItem`，负责天空盒和环境IBL相关，成员：
    - 顶点和索引缓冲区。
    - 环境光高光的IBL贴图。
    - 环境光漫反射的IBL贴图。
    - Unreal风格的环境光BRDF贴图。
* `EnvRenderItem::renderEnv()`：渲染天空盒。
* `EnvRenderItem::setAmbient()`：设置IBL相关的贴图。
* `BuildSkyEnv()`：构造`EnvRenderItem`，读入一些贴图文件。

## 灯光相关类的头文件 `Light.h`

* 类似《3D Game Programming with DirectX 12》中的做法，统一点光源、平行光源和聚光灯光源的参数，使用3个`float3`和3个`float`。而BGFX的语法中`uniform`只能用`mat4`了，多出一个`float4`用来表示灯光种类（超级奢侈，除非更紧凑地排列，每3个`mat4`表达4个光源）。详见注释。
* `getViewMtx()`、`getProjMtx()`、`getViewProjCropMtx()`：阴影贴图相关的矩阵计算。**目前只实现了平行光源的部分。**
* 友元函数`lightsToUniform`：将几个不同类型的light打包成适合shader的uniform变量并传输。由于DX和GL的矩阵索引方式不同需要视情况转置。

## 相机相关类的头文件 `Camera.h`

* 提供了相机基础的视图矩阵和投影矩阵。
* 为了方便计算设置了一些其他成员。
* 与BGFX自己的`camera.h`不同，将消息处理与相机的控制分开。*（为什么要把键盘和鼠标处理写进camera类？？？不打算换控制方式？？？完全不能理解。而且为什么要用那么复杂的键盘消息处理？？？）*
* `Camera::AEDR()`和`Camera::Move()`：显而易见。需要注意的是环绕相机（使用方位角、俯仰角控制）与追踪球不同。环绕相机的up是不需要变化的，也因此需要做俯仰角的限位处理；而跟踪球的up会变化，不需要限位。

## *为什么不把头文件和源文件分开？*

理论上讲在头文件中只定义符号而源文件中定义实现是标准的做法。但这个项目中我只会使用一次这些头文件，而且不会互相包含，简单一点就好。

## 着色器文件 `*.sc`

共有三组着色器：

* 负责主要场景绘制（Scene Pass）的`vs.sc`、`fs.sc`和配套的定义文件`varying.def.sc`。
    - 顶点着色器直接使用instance buffer的变量组成模型矩阵。
    - 假设了模型变换局限于平移、旋转和缩放（不包括斜切）。因此使用了简单方法计算法线变换矩阵。
    - 在世界空间计算光照。
    - 片元着色器中菲涅尔系数的计算采用了Unreal的方案而非石里克近似。
    - `ComputeLight()`计算光照，遍历所有光源直到遇到未指明类别的光源。
    - ToneMapping使用了ACES方案。
* 负责天空盒绘制的`vs_sky.sc`、`fs_sky.sc`和定义文件`varying.def_sky.sc`。
    - 天空盒使用去掉了平移的视图矩阵
    - 将坐标设置为裁剪空间中深度始终为最大值
* 负责shadow pass的`vs_shadow.sc`、`fs_shadow.sc`和定义文件`varying.def_shadow.sc`。

# 不吐不快😤

BGFX的跨平台基本等于没有。如果同时写GL和DX，分别调试两个平台可能更快。GL和DX现有的文档非常丰富，与之对比，BGFX跨平台之后，逐一排查莫名其妙的问题简直是折磨。

目前发现的问题有：

1. 文档提及的，但`shaderc`编译时***不报错***，开Werror也不报错：
    - 必须使用`mtxFromCols()`或`mtxFromRows()`。实例中大量索引赋值。
    - `varying.def.sc`中不允许注释（实际上可以在最后注释）。
    - 使用`vec[2|3|4]_splat()`代替单形参的构造函数。虽然这一点好像没有影响。
    - *这些应该在编译期解决的问题被迫拖到运行时解决。*

2. BGFX文档里允许const，定义const之后GL也没有问题，但DX平台出现问题。

3. DX的矩阵索引方式与GL不同，BGFX未作转换，文档里也没有指出。

4. 直接将shadow2D得到的`float`作为返回值会出现问题。原因未知。本例中可以尝试将`fs.sc`中的`hardShadow`返回值改为`float`观察这一奇妙的现象，会报错`error C7623: implicit narrowing of type from "vec4" to "float"`，但指向的着色器代码是将`shadow2D`的结果赋给一个`float`，与正确的`shadow2D`代码区别仅在于`shadow2D`字符后多了三个空格。相关issue: https://github.com/bkaradzic/bgfx/issues/1380 ，可惜提出者认为是自己的问题，关闭了issue。

5. DX的texture系列函数似乎也有问题，就此项目而言，**DX的PBR环境光会完全丢失，因为涉及的几个采样结果为0**，而GL不会。疑似与上述问题4相关。

6. 着色器的输入变量必须使用固定的关键字，如`a_texcoord0`，否则也不报错，完全正常运行，但就是值为0。神奇的是instance变量只允许定义5个，`i_data0`到`i_data4`。？？？困惑。不解。

7. 其他影响不大的问题：
    - 能定义`mul`和`instMul`这种换一下参数位置的函数，却不定义一个矩阵取逆的函数。限定了instance变量数量，又不允许在着色器中取逆，请问做instancing的话normal matrix要怎么输进去？
    - 为什么不能在着色器中定义结构体？数据宽度的对齐效率问题让用户解决呗。
    - 没有一个统一的消息处理接口。

8. 总的来说，这次E星计划80%的时间花在调教BGFX上。我不知道这是否值得，往好的方面想我也算学会了不少。但只讨论图形学的话，我可能更愿意直接与OpenGL或DirectX打交道。我不知道工业界是怎么实现跨平台的，但我觉得应该不是用BGFX。这个框架实在打消了我很多想法。


# 原始文档
## 前置需求

1. 安装Visual Studio 2022（或2019）
   
   https://visualstudio.microsoft.com/zh-hans/downloads/

2. 安装最新版本CMake

	https://cmake.org/download/

## 如何编译、运行作业框架

!!! warning "特别注意"
	路径中不能含有中文、空格和特殊符号，否则无法正确运行作业框架。

1. 创建**build**目录：

	```powershell
	mkdir build
	```

2. 进入**build**目录，运行**cmake**：

	```powershell
	cd build
	cmake ../
	```

3. 双击**build**目录下的**EStarHomework.sln** ，打开工程：
   
   ![](doc/1.png)

4. 将项目**homework**设置为启动项目：
   
   ![](doc/2.png)

5. 点击**调试运行**：
   
   ![](doc/3.png)

6. 编译结束后，将看见如下窗口，说明成功了！
   
   ![](doc/4.png)

## bgfx介绍

作业基于bgfx编写。

bgfx是一个开源的跨平台渲染库，封装了统一的接口，屏蔽了底层不同图形API的差异（DirectX、OpenGL等）。

bgfx文档网站为：<https://bkaradzic.github.io/bgfx/>。在这里可也以看见教程和范例。

### shader编写

bgfx的shader采用了GLSL语法。编写后，需要使用bgfx自制shader编译工具**shaderc**编译，随后才能使用。**shaderc.exe**位于**tools**目录。

关于shader编写和shader编译，详见 bgfx文档：<https://bkaradzic.github.io/bgfx/tools.html#shader-compiler-shaderc>

### 模型与贴图导入

bgfx采用了自定义格式模型与贴图，原始模型和贴图需要经过转换才能使用。

我们提供了一份已转换好的模型与贴图，位于**resource**目录，同学们可以直接使用。其中模型为.bin格式，可以使用**tools/geometryv.exe**打开预览；贴图为.dds格式，可以使用**tools/texturev.exe**打开预览。另外，也可以使用Visual Studio打开.dds贴图查看。

如果希望使用其它模型和贴图，则需要自己进行格式转换。

转换模型的程序为**geometryc.exe**。关于模型转换的文档详见：<https://bkaradzic.github.io/bgfx/tools.html#shader-compiler-shaderc>

转换贴图的程序为**texturec.exe**。关于贴图转换的文档详见：<https://bkaradzic.github.io/bgfx/tools.html#texture-compiler-texturec>

## 作业需求

作业分为5个level。每完成一个level的所有需求，则可继续挑战下一level。若未完成低等级level需求，则无法获取高等级level的分数。

### Level1

1. 加载模型，绘制在屏幕上。
2. 添加环绕相机（Orbit Camera），并可以使用鼠标操控：
	- 鼠标左键拖拽以旋转镜头（类似geometryv里的操作方式）
	- 鼠标滚轮缩放镜头（类似geometryv里的操作方式）

### Level2

1. 为模型添加基础纹理。
2. 为模型添加基础光照（Blinn-Phong）
3. 在保留环绕相机功能的情况下，为相机添加键盘控制：
	- ADWS控制镜头左右上下平移

### Level3

1. 把模型的直接光漫反射光照改为PBR模型实现
    - 模型的金属度能正确影响漫反射光照
    - 模型的albedo（反照率）使用纹理控制
2. 把模型的直接光高光改为PBR模型实现
    - 模型的金属度，粗糙度通过纹理控制

### Level4

1. 使用IBL(Image-Based Lighting)，为模型添加环境光照的漫反射部分
2. 使用IBL(Image-Based Lighting)，为模型添加环境光照的高光反射部分
3. 添加一个包住场景的天空盒
    天空盒使用的cubemap纹理，对应IBL图的mipmap level 0

### Level5

1. 使用ShadowMap的方式，为模型添加阴影
