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
- ONB 对任意方向构造右手正交基，并保持负 z 极点附近的数值稳定性和方向长度无关的局部到父空间映射。
- `Vec3<T>` 的 double 精度别名，`Vec4<T>` 的构造、分量、标量运算和点积，以及反射和全反射方向计算。
- `Mat4<T>` 的 double 精度、列向量乘法、加法、Hadamard 乘积、转置、行列式、一般逆矩阵与奇异矩阵拒绝。
- Transform 的右手旋转约定，以及对仿射 `Mat4f` 的接受和对射影矩阵的拒绝。
- Primitive 平移、Transform 组合、恒等法线精确保真、非均匀缩放法线、方向 PDF Jacobian 与奇异变换拒绝。

### 实现逻辑

测试使用解析结果已知的固定向量、几何和射线，通过绝对误差比较向量运算、`t`、命中点、两类法线、UV 与 PDF。ONB 使用对角方向、极短同向向量和接近负 z 极点的方向，检查正交性、右手性、数值稳定性和方向缩放不变性。纯 Shape 测试只检查局部几何；Primitive 测试额外检查世界空间变换、`front_face` 和 Material 绑定。Triangle 与 Box 的固定 seed 样本必须落在各自表面；由两个 indexed Triangle 构成的 Mesh 覆盖共享数据、私有 BVH 和生成法线的组合语义。

## `nearlighter.interaction`

对应文件：[`interaction_tests.cpp`](interaction_tests.cpp)

### 测试目的

验证表面着色坐标与新射线起点的契约。相关错误通常表现为方向相关的
BSDF 偏差、自相交或漏光，无法由求交测试单独发现。

### 测试内容与逻辑

- ShadingFrame 的正交性、右手性、法线映射及世界/局部方向往返。
- SurfaceInteraction 对命中点、出射方向和源 Primitive 的保留。
- 向表面两侧生成射线时采用正确的 geometric-normal offset，避免立即
  自相交，同时保留向内射线对远端表面的命中。
- SurfaceInteraction 拒绝没有表面语义的 medium record。

测试使用解析结果明确的单位球和固定射线。它只验证交互契约，不评价具体
BSDF 或积分器输出。

## `nearlighter.bsdf`

对应文件：[`bsdf_tests.cpp`](bsdf_tests.cpp)

### 测试目的

验证 BSDF/BxDF 的方向空间、连续 PDF 与离散 delta 事件契约。此处的错误
往往不会导致程序失败，却会通过错误的路径吞吐量产生系统性亮度偏差。

### 测试内容与逻辑

- Lambertian 的解析值 $f = \rho / \pi$、余弦加权 PDF、半球支持范围，
  以及对半球立体角的数值归一化。
- 恒定 RGB 理想镜面（SpecularReflectionBxDF）的方向、事件 PDF、flags，以及
  `value * abs(cosine) / pdf` 对反射率的恢复。
- 理想介质在法向入射时的 Fresnel 反射/透射概率、相对折射率和 radiance
  transport 的平方折射率因子；空气到折射率 1.5 的期望反射率独立固定为 0.04，不调用被测 helper 产生期望。
- 从高折射率介质出射时的全反射回退。
- 固定容量 BSDF 对第五个分量及空指针的拒绝，失败后计数与已有响应不变。
- 两个连续分量的总函数值与平均 PDF；连续与 delta 混合时分别检查立体角密度和离散事件概率；两个同方向 delta 分量的期望吞吐量。
- 非轴对齐 ShadingFrame 下采样、求值与 PDF 查询的一致性，以及 importance transport 不带 radiance 的平方折射率修正。
- 编译期检查 BSDF 不可复制且可 noexcept 移动；运行时检查移动后的源对象为空、目标坐标系与响应不变、自移动安全、被替换分量和最终分量各析构一次。
- 测试内普通 Material 派生类组合两个分量，不修改 BSDF 分派代码；未实现材质、Isotropic 和 fuzzy Metal 直接调用时抛错；纯发光材质 Emissive 合法返回无散射，仍保持单面发光。
- 同一位置纹理材质在球面相反两点生成独立 BSDF；销毁交点、Primitive、Material 和 Texture 后，再验证两份响应的颜色与方向基。

数学测试直接构造 BSDF，以解析方向和分层立体角积分隔离求交与积分器。
材质入口及生命周期测试使用单位球和局部测试类型，不引入生产级自定义材质框架。

## `nearlighter.render_core`

对应文件：[`render_core_tests.cpp`](render_core_tests.cpp)

### 测试目的

验证 Film 的分层累积、Light 与 LightSampler 的条件/选择概率约定，以及
PathIntegrator 在开始采样前对已组装 Scene 执行的能力检查。

### 测试内容与逻辑

- Film 对 beauty、emission、direct、indirect 的逐像素样本均值，精确样本
  计数和 Welford 无偏 beauty 方差；未启用的可选 AOV 不可访问。
- 发光 Quad 由 Scene 创建 AreaLight；样本携带入射方向、辐射亮度、
  距离与条件方向 PDF。LightSampler 单独返回选择 PMF，乘积与命中灯面的查询一致。
- 非黑色背景形成均匀球面分布的 infinite-light strategy。
- finite light 与 environment 同时存在时，两者的最终 PDF 均包含 `1 / 2`
  uniform light-selection PMF。
- uniform 选择的区间分配、PMF 归一化、空列表和不属于当前集合的灯光。
- 仅包含已支持表面材质的 Scene 通过 PathIntegrator 检查；fuzzy Metal 在像素循环前被拒绝。

测试使用解析可控的单位球、平面发光体和两样本 Film fixture，不依赖图像
文件或随机统计阈值。

## `nearlighter.scene`

对应文件：[`scene_tests.cpp`](scene_tests.cpp)

验证场景组装中容易静默破坏照明的放置、身份与资源生命周期问题：

- 同一混合 BVH 经嵌套 Instance 放置两次，检查两盏灯都被发现、命中身份分别对应
  各自 AreaLight、源 Shape/Material 仍共享、输入子树没有被修改。
- 用手工组合变换的 Primitive 对照命中位置、法线和非均匀缩放后的灯光 PDF，
  固定种子样本必须命中正确的世界空间灯面。
- 递归计数最终可见表面，保证移出的灯面没有在原子树中重复保留；旧采样目标
  引用发光源时扩展到实际放置。
- 六个 Quad 构成的方块只有前面发光，分别从前后发射射线，确认材质和光源关联。
- ConstantMedium 的发光边界仅控制体积范围，不应被发现为可见面光源。
- 同一组嵌套灯光与显式展开场景，在 BSDF-only、light-only、MIS 下逐像素对照；
  结果须非黑、无非法贡献。改变分辨率后复用同一 Scene，world 和灯光地址不变。
- 移动 Scene 保持 world/Light 地址稳定；纯非发光实例保留原 Instance 和 BVH。
- 发光源先通过 Instance 加入，再直接加入 world，验证两处放置都可见且分别关联到灯光。
  这覆盖构建缓存已记录源对象后，直接放置仍须登记的情况。
- 不可采样的发光 Mesh 仍保留在 world 中，并按实际放置计数。
  同一源实例化两次时计数为二，灯光列表为空，新 PathIntegrator 仍明确拒绝。
- 自定义不透明求交包装保留旧路径求交能力，但不能把隐藏光源误判为已完整检查。

测试使用固定射线与固定种子，不依赖外部资产或随机统计阈值。

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
- 二维样本按 x、y 顺序消费两个连续的一维样本。

### 实现逻辑

测试构造多个固定 seed 的 Sampler，直接比较连续原始随机值；随后分别改变
path seed 的一个输入分量，确认派生结果变化；最后通过重复采样检查接口的
边界契约，并用等 seed 标量流验证二维样本的消费顺序。该测试只验证
确定性和范围，不承担随机分布的统计质量检验。

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
- `path` 模式在受支持的 diffuse + constant-environment 场景中生成 NEE
  shadow rays 和 BSDF continuation rays，并保持确定性。
- `bsdf_only` 不生成 shadow ray；`light_only` 和 `mis` 保留 NEE，从调度层
  验证三种 direct-light 策略确实走独立分支。
- Path Film 满足 `beauty = emission + direct + indirect`，每像素样本数与
  SPP 相同；默认关闭 RR，显式启用后实际终止部分路径。

### 实现逻辑

测试程序化构造一个 `13 x 11`、`3 SPP` 的微型发光球场景。小尺寸保证测试快速完成，非平方数 SPP 同时覆盖“SPP 表示实际样本数”的契约。第一次渲染记录每次行级回调，验证完成行数、图像尺寸和积分时间顺序；两次相同 seed 的结果按像素精确比较，另用不同 seed 渲染一次验证随机序列确实参与成像。

新积分器链路另使用一个 `13 x 11` 的 Lambertian 球与常量环境 fixture。
同 seed 两次渲染逐像素比较，随后检查 ray counters、AOV 恒等式和 sample
count；RR 变体把起始深度设为 1，以固定 seed 验证终止计数，而不把随机
图像质量当作单元测试判据。
