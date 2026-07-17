# Nearlighter 测试说明

本文档记录 `tests/` 中各测试的目的、覆盖内容和实现逻辑。新增或实质修改测试时，应在同一轮修改中同步更新本文档。

## 测试设置原则

- 只为正常运行时不容易发现、但会破坏核心语义或结果可信度的错误添加测试。
- 对于正常执行本身就会明确报错、容易直接观察，或者实现非常直白的行为，不额外添加单元测试。
- 测试应保持规模小、结果确定、职责聚焦，并与实际回归风险相匹配。
- 修改 BVH、采样、求交等高风险或性能敏感代码前，应先建立相应的确定性测试安全网。

## 使用方式

构建并运行全部测试：

```bash
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

CTest 负责启动测试可执行文件并汇总退出状态。每个测试程序使用 `tests/test_support.h` 中的轻量 expectation 工具收集失败信息，不依赖 GoogleTest。

## `nearlighter.geometry`

对应文件：`tests/geometry_tests.cpp`

### 测试目的

验证基础图元和变换的求交语义，防止几何算法修改后产生仍可运行、但命中位置、法线或朝向错误的静默回归。

### 测试内容

- Sphere 的外部命中、内部命中和未命中。
- Quad 的中心命中、UV 坐标和边界外未命中。
- AABB 的穿过命中和平行射线未命中。
- Translate 和 Rotate 后的命中距离、世界空间命中点与法线。

### 实现逻辑

测试使用解析结果已知的固定几何和固定射线，通过绝对误差比较 `t`、命中点、法线与 UV，并检查 `front_face`。所有随机依赖均使用固定 Sampler，确保测试结果可复现。

## `nearlighter.bvh`

对应文件：`tests/bvh_tests.cpp`

### 测试目的

验证 BVH 只改变求交效率，不改变 Shape 集合的最近命中语义。BVH 排序、区间收缩或遍历顺序错误通常不会导致程序崩溃，但会产生错误图像，因此需要独立测试。

### 测试内容

- 多组固定射线的命中与未命中状态。
- 最近命中的 `t`、命中点和法线。
- 命中的 front-face 状态。

### 实现逻辑

同一组 Sphere 同时构造为线性 `ShapeList` 和 `BVHNode`。对每条射线分别使用相同 seed 的独立 Sampler 求交，并将线性列表结果作为参考，与 BVH 结果逐项比较。

## `nearlighter.sampler`

对应文件：`tests/sampler_tests.cpp`

### 测试目的

验证确定性采样的核心契约。随机序列错误可能只表现为无法复现、像素相互影响或未来并行后结果漂移，正常渲染时不容易定位。

### 测试内容

- 相同 seed 产生相同 PCG32 序列。
- 不同 seed 能改变随机序列。
- render seed、像素坐标和 sample index 都参与 path seed 派生。
- 浮点样本和闭区间整数样本始终处于约定范围。

### 实现逻辑

测试构造多个固定 seed 的 Sampler，直接比较连续原始随机值；随后分别改变 path seed 的一个输入分量，确认派生结果变化；最后通过重复采样检查浮点和整数接口的边界契约。该测试只验证确定性和范围，不承担随机分布的统计质量检验。

## `nearlighter.renderer_smoke`

对应文件：`tests/renderer_smoke_tests.cpp`

### 测试目的

验证 `Scene -> Camera -> Sampler -> Renderer -> Image` 的最小核心渲染链路。它不评价图像质量，而是捕获各模块单独正确、组合后却产生空图、非有限值或不可复现结果的集成错误。

### 测试内容

- 输出 Image 的宽高与 RenderSettings 一致。
- 图像不是全黑，所有通道均为有限值。
- 相同 Scene、设置和 seed 两次渲染逐像素一致。
- 修改 seed 后采样结果发生变化。
- `sample_count` 等于宽、高和实际 SPP 的乘积。
- Renderer 拒绝非正图像尺寸，Camera 拒绝退化视线方向。

### 实现逻辑

测试程序化构造一个 `13 x 11`、`3 SPP` 的微型发光球场景。小尺寸保证测试快速完成，非平方数 SPP 同时覆盖“SPP 表示实际样本数”的契约。两次相同 seed 的结果按像素精确比较，另用不同 seed 渲染一次验证随机序列确实参与成像。
