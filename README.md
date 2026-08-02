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

当前阶段：在基础工程结构重构完成后，继续进行正确性修复、测试建设和性能优化。

---

待实现特性

- [ ] 接入第三方窗口管理和UI
- [ ] 支持导入Mesh模型
- [ ] 曲面细分
- [ ] 更优加速结构: SAH
- [ ] 多线程加速渲染
- [ ] GPU并行加速渲染(CUDA)
- [ ] 实时光线追踪支持，降噪算法

### 环境与依赖

基础构建环境：

- CMake 3.20 或更高版本
- 支持 C++17 的编译器
- Git 与项目 submodule
- GNU Make；仓库内置 preset 当前使用 `Unix Makefiles`

集成评估额外需要 Python 3，基础构建和运行不依赖 Python。第三方库统一以 submodule 存放在 `thirdparty/`，克隆项目后初始化一次：

```bash
# 获取或更新项目依赖
git submodule update --init --recursive

# 确认 CMake 版本
cmake --version
```

如需使用 Ninja、Visual Studio 等其他 generator，可在不提交的 `CMakeUserPresets.json` 中定义本地 preset。VS Code 的 preset 配置见 [docs/vscode-cmake.md](docs/vscode-cmake.md)。

### 构建

Debug 与 Release 使用独立构建目录，分别生成到 `build/debug/` 和 `build/release/`：

```bash
# Release：用于正常运行和性能评估
cmake --preset release
cmake --build --preset release

# Debug：用于开发和错误检查
cmake --preset debug
cmake --build --preset debug
```

配置完成后，也可以直接调用 preset 生成的 Makefile 进行增量构建：

```bash
# 等价的 Debug 增量构建
make -C build/debug -j

# 等价的 Release 增量构建
make -C build/release -j
```

每个构建目录的根部包含 `Nearlighter` 可执行文件，静态库位于 `lib/`，测试程序位于 `ctest/`。配置阶段会优先为 `assets/` 创建指向源码资源的符号链接；平台或权限不支持时，改为增量复制资源。

### 使用方式

CLI 默认加载项目提供的 Cornell Box，在终端显示渲染进度，并将已完成的图像行持续写入当前目录的 `out.ppm`：

```bash
# 使用默认场景和设置
./build/release/Nearlighter

# 逻辑名称：从可执行文件同级的 assets/scenes/ 加载，可省略 .json
./build/release/Nearlighter --scene earth
./build/release/Nearlighter --scene cornell_box_rtow.json

# 包含目录的相对路径：相对于当前工作目录加载
./build/release/Nearlighter --scene experiments/test.json

# 绝对路径：直接加载指定文件
./build/release/Nearlighter --scene /path/to/test.json
```

内置逻辑名称包括：

```text
bouncing_spheres   checker_spheres  earth          perlin_sphere
quads              simple_light     cornell_box_rtow
cornell_smoke      final_scene      cornell_ball
```

常用输出选项：

```bash
# 关闭终端进度显示
./build/release/Nearlighter --no-progress

# 每 0.5 秒将新增图像行刷新到输出文件
./build/release/Nearlighter --flush-interval 0.5

# 查看完整参数说明
./build/release/Nearlighter --help
```

JSON 只是 CLI 的场景输入方式。作为 C++ 模块使用时，也可以通过公开的 `Scene` 构造函数组织相机、渲染默认值和 `ShapeList`，再直接交给 `Renderer` 渲染，不需要依赖 `nearlighter_io` 或场景 JSON。

### 单元测试

构建完成后，通过对应 preset 运行 CTest：

```bash
# Debug 测试
ctest --preset debug

# Release 测试
ctest --preset release

# 已进入 Make 工作流时的等价 Debug 测试
make -C build/debug test
```

各测试的目的、覆盖内容和实现逻辑见 [tests/README.md](tests/README.md)。

### 集成评估

集成评估用于一次增量开发后的正确性与性能检查。脚本默认配置并构建 Release、运行 CTest、执行 suite 中的固定 cases，并将渲染结果与固定线性 PFM reference 比较：

```bash
# 日常快速检查
python3 scripts/evaluate.py --suite quick

# 阶段性完整检查
python3 scripts/evaluate.py --suite full

# 复用已有 Release 构建
python3 scripts/evaluate.py --suite quick --skip-build
```

评估输出包括 MSE、RMSE、relative MSE、PSNR、渲染时间和采样吞吐量。每次运行的图像、日志与汇总报告独立写入 `build/evaluation/runs/`。

固定 reference 位于 `benchmark/references/`，由 `scripts/generate_reference.py` 显式生成；evaluation 只读取，不会自动创建或覆盖。reference 生成方式、配置结构和完整产物说明见 [scripts/README.md](scripts/README.md)。

### 渲染示例

Cornell Box

- WSL2 - Ubuntu 24.04 - 9955HX - Single Core : SPP = 64, depth = 25, 400x400, Rendering Time: 1m43s

![Cornell Box](./docs/figs/optim/cb_spp64_md25_400-3_mix_sampling.png)

MultiBalls

![MultiBalls](./docs/figs/blur-bouncingballs.png)

### 项目结构

```
.
├── include/nearlighter/  # 项目公开头文件，按功能模块组织
├── src/
│   ├── main.cpp          # 命令行程序入口
│   └── nearlighter/      # 与公开头文件对应的实现文件
├── thirdparty/           # 第三方库 submodule
│   ├── argparse/
│   ├── glm/
│   ├── json/
│   └── stb/
├── assets/               # 运行时场景、纹理与模型资源
├── benchmark/            # 测评数据与固定 reference
├── cmake/                # 项目 CMake 辅助模块
├── scripts/              # 构建、reference 生成与集成评估脚本
├── tests/                # 确定性 C++ 测试
└── docs/                 # 实现笔记与渲染结果
```

### 项目文档

- [脚本与 evaluation 说明](scripts/README.md)
- [单元测试说明](tests/README.md)
- [VS Code CMake Preset 配置](docs/vscode-cmake.md)
- [渲染实现笔记](docs/note.md)
- [几何求交笔记](docs/hit-calculation.md)

### 参考资料

- [Ray Tracing in Oneweekend系列](https://github.com/RayTracing/raytracing.github.io)
- [GAMES101课程](https://games-cn.org/intro-graphics/)
- [STB图像库文档](https://github.com/nothings/stb)
- [GLM数学库](https://glm.g-truc.net/)
- [argparse](https://github.com/p-ranav/argparse)
- [JSON for Modern C++](https://github.com/nlohmann/json)
