# Nearlighter

Nearlighter 是一个基于物理的CPU端路径追踪渲染器，支持简单光线追踪渲染功能，基础框架参考[Ray Tracing in Oneweekend系列](https://github.com/RayTracing/raytracing.github.io)。持续开发完善中，作为个人实践试验项目。

### 功能特性

- 基本光线传播模拟计算，BVH加速结构
- 图像输出：多采样抗锯齿(MSAA)，相机景深+散焦+动态模糊，
- 基础图元：球体(Sphere)、四边形(Quad)、实例化对象组合，变换矩阵支持的几何体
- 材质系统：漫反射表面(Lambertian)、镜面反射表面(Metal)、折射电介质(Dielectric)、自发光表面(DiffuseLight)、体积各向同性散射(Isotropic)
- 贴图系统：简单空间纹理(SolidTexture)、图片纹理(ImageTexture)、噪声纹理(NoiseTexture)与生成(Perlin Noise)
- 体渲染：恒定介质(ConstantMedium)

### 待实现

紧急： common.h 头文件引用混乱

---

待实现特性

- [ ] 接入第三方窗口管理和UI
- [ ] 支持导入Mesh模型
- [ ] 曲面细分
- [ ] 更优加速结构: SAH
- [ ] 多线程加速渲染
- [ ] GPU并行加速渲染(CUDA)
- [ ] 实时光线追踪支持，降噪算法

### 使用方式

构建： CMake

```
git submodule update --init --recursive
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/Nearlighter -s <scene_num>
```

支持跨平台

### 渲染示例

Cornell Box

- WSL2 - Ubuntu 24.04 - 9955HX - Single Core : SPP = 64, depth = 25, 400x400, Rendering Time: 1m43s

![Cornell Box](./doc/figs/optim/cb_spp64_md25_400-3_mix_sampling.png)

MultiBalls

![MultiBalls](./doc/figs/blur-bouncingballs.png)

### 项目结构

```
.
├── src/              # 项目源码与当前内部头文件
│   ├── main.cpp      # 主程序入口
│   ├── camera/       # 相机模型与路径追踪主循环
│   ├── material/     # 材质与散射模型
│   ├── shape/        # 几何图元、图元列表与实例变换
│   ├── bvh/          # AABB 与 BVH 加速结构
│   ├── texture/      # 纹理接口与具体纹理
│   ├── medium/       # 体积介质
│   ├── general/      # 基础数学、颜色、射线、区间等核心类型
│   └── utils/        # 随机数、PDF、图片加载、噪声、计时器等工具
├── thirdparty/       # 第三方库 submodule
│   ├── glm/
│   └── stb/
├── assets/           # 运行时资源
├── cmake/            # 项目 CMake 辅助脚本
└── doc/              # 实现笔记与渲染结果
```

### 参考资料

- [Ray Tracing in Oneweekend系列](https://github.com/RayTracing/raytracing.github.io)
- [GAMES101课程](https://games-cn.org/intro-graphics/)
- [STB图像库文档](https://github.com/nothings/stb)
- [GLM数学库](https://glm.g-truc.net/)
