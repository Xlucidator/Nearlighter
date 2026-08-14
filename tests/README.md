# Nearlighter 测试说明

本文档记录 `tests/` 中各测试的目的、覆盖内容和实现逻辑。新增或实质修改测试时，应在同一轮修改中同步更新本文档。

## 测试设置原则

- 只为正常运行时不容易发现、但会破坏核心语义或结果可信度的错误添加测试。
- 对于正常执行本身就会明确报错、容易直接观察，或者实现非常直白的行为，不额外添加单元测试。
- 测试应保持规模小、结果确定、职责聚焦，并与实际回归风险相匹配。
- 修改 BVH、采样、求交等高风险或性能敏感代码前，应先建立相应的确定性测试安全网。

## 使用方式

```bash
# 配置并构建 Debug
cmake --preset debug
cmake --build --preset debug

# 运行全部 Debug 测试
ctest --preset debug
```

CTest 负责启动测试可执行文件并汇总退出状态。每个测试程序使用 [`test_support.h`](test_support.h) 中的轻量 expectation 工具收集失败信息，不依赖 GoogleTest。

## `nearlighter.geometry`

对应文件：[`geometry_tests.cpp`](geometry_tests.cpp)

### 测试目的

验证基础图元和变换的求交语义，防止几何算法修改后产生仍可运行、但命中位置、法线或朝向错误的静默回归。

### 测试内容

- Sphere 的外部命中、内部命中、局部 outward normal，以及偏心球内原点的
  均匀方向 PDF。
- Quad 的中心命中、UV 坐标和边界外未命中。
- Triangle 的双面命中、重心坐标、边界外未命中、面积 PDF 和表面均匀采样。
- Box 的外部与内部命中、UV、outward normal、面积采样、方向长度无关的 PDF，以及同一方向上前后表面贡献的 PDF 累加。
- Mesh 私有 Triangle BVH 的命中、插值 UV 与面积加权法线生成。
- AABB 与 Box 各自 slab 求交对穿过、平行未命中和平行边界命中的处理。
- `Vec3<T>` 的 double 精度别名，`Vec4<T>` 的构造、分量、标量运算和点积，以及反射和全反射方向计算。
- `Mat4<T>` 的 double 精度、列向量乘法、加法、Hadamard 乘积、转置、行列式、一般逆矩阵与奇异矩阵拒绝。
- Transform 的右手旋转约定，以及对仿射 `Mat4f` 的接受和对射影矩阵的拒绝。
- Primitive 平移、Transform 组合、恒等法线精确保真、非均匀缩放法线、方向 PDF Jacobian 与奇异变换拒绝。

### 实现逻辑

测试使用解析结果已知的固定向量、几何和射线，通过绝对误差比较向量运算、`t`、命中点、两类法线、UV 与 PDF。纯 Shape 测试只检查局部几何；Primitive 测试额外检查世界空间变换、`front_face` 和 Material 绑定。Triangle 与 Box 的固定 seed 样本必须落在各自表面；由两个 indexed Triangle 构成的 Mesh 覆盖共享数据、私有 BVH 和生成法线的组合语义。

## `nearlighter.mesh_io`

对应文件：[`mesh_io_tests.cpp`](mesh_io_tests.cpp)

### 测试目的

验证 OBJ/PLY 导入后仍不易从程序是否正常退出中发现的拓扑和属性语义。索引解析、顶点属性对齐、字节序或多边形拆分错误通常仍会生成可渲染 Mesh，但会静默改变几何或着色。

### 测试内容

- OBJ 负索引解析、位置/UV/法线属性对齐和四边形三角化。
- ASCII PLY 四边形读取与三角化。
- binary little-endian PLY 标量、索引和三角形读取。

### 实现逻辑

测试在系统临时目录生成三个最小 fixture，读取后直接检查顶点、属性和三角形数量。fixture 只覆盖当前公开支持范围，不承担第三方格式兼容性全集测试；测试结束后删除临时目录。

## `nearlighter.bvh`

对应文件：[`bvh_tests.cpp`](bvh_tests.cpp)

### 测试目的

验证 BVH 只改变求交效率，不改变 Intersectable 集合的最近命中语义，并验证 Instance 能够共享和整体放置同一 BVH。

### 测试内容

- 多组固定射线的命中与未命中状态。
- 最近命中的 `t`、命中点和法线。
- 命中的 front-face 与 Material 状态。
- 同一 BVH 的两个 Instance 具有独立命中结果和世界包围盒。

### 实现逻辑

同一组 Sphere Primitive 同时构造为 `LinearAggregate` 和 `BVH`。BVH 在私有对象副本上按 centroid 中位数构建二叉层次，单对象叶只求交一次。对每条射线分别使用相同 seed 的独立 Sampler 求交，并将线性聚合结果作为参考。实例测试让两个 Instance 共享同一底层 BVH，只改变各自 Transform。

## `nearlighter.medium`

对应文件：[`medium_tests.cpp`](medium_tests.cpp)

### 测试目的与逻辑

验证 ConstantMedium 从 Shape 层迁移到 Intersectable 后仍保持随机自由飞行语义。测试以高密度球形边界和固定 Sampler seed 解析计算预期散射参数，检查命中距离、交点、phase Material、neutral facing 状态和复用的边界包围盒。

## `nearlighter.sampler`

对应文件：[`sampler_tests.cpp`](sampler_tests.cpp)

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

对应文件：[`renderer_smoke_tests.cpp`](renderer_smoke_tests.cpp)

### 测试目的

验证 `Scene -> Camera -> Sampler -> Renderer -> Image` 的最小核心渲染链路。它不评价图像质量，而是捕获各模块单独正确、组合后却产生空图、非有限值或不可复现结果的集成错误。

### 测试内容

- 输出 Image 的宽高与 RenderSettings 一致。
- 图像不是全黑，所有通道均为有限值。
- 相同 Scene、设置和 seed 两次渲染逐像素一致。
- 修改 seed 后采样结果发生变化。
- `sample_count` 等于宽、高和实际 SPP 的乘积。
- 行级进度回调按顺序覆盖每一行，并报告正确的图像尺寸和单调积分时间。

### 实现逻辑

测试程序化构造一个 `13 x 11`、`3 SPP` 的微型发光球场景。小尺寸保证测试快速完成，非平方数 SPP 同时覆盖“SPP 表示实际样本数”的契约。第一次渲染记录每次行级回调，验证完成行数、图像尺寸和积分时间顺序；两次相同 seed 的结果按像素精确比较，另用不同 seed 渲染一次验证随机序列确实参与成像。
