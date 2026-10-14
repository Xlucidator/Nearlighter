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
- `Mat4<T>` 的列向量映射、行列式与一般逆矩阵。
- Transform 的右手旋转约定，以及仿射矩阵的点映射。
- Primitive 平移、Transform 组合、恒等法线保真、非均匀缩放法线与方向 PDF Jacobian。

### 实现逻辑

测试使用解析结果已知的固定向量、几何和射线，通过绝对误差比较向量运算、`t`、命中点、两类法线、UV 与 PDF。ONB 使用对角方向、极短同向向量和接近负 z 极点的方向，检查正交性、右手性、数值稳定性和方向缩放不变性。纯 Shape 测试只检查局部几何；Primitive 测试额外检查世界空间变换、`front_face` 和 Material 绑定。Triangle 与 Box 的固定 seed 样本必须落在各自表面；由两个 indexed Triangle 构成的 Mesh 覆盖共享数据、私有 BVH 和生成法线的组合语义。

## `nearlighter.hit`

对应文件：[`hit_tests.cpp`](hit_tests.cpp)

### 测试目的

验证表面着色坐标与新射线起点的契约。相关错误通常表现为方向相关的
BSDF 偏差、自相交或漏光，无法由求交测试单独发现。

### 测试内容与逻辑

- ShadingFrame 的正交性、右手性、法线映射及世界/局部方向往返。
- SurfaceRecord 对命中点、出射方向和源 Primitive 的保留。
- HitRecord 的共同访问与具体记录访问贯穿 Primitive、Instance 和 BVH。
- 真实表面命中保持材质与基元绑定，t 与场景时刻分别传递。
- 向表面两侧生成射线时采用正确的 geometric-normal offset，避免立即
  自相交，同时保留向内射线对远端表面的命中。
- MediumRecord 独立保存介质事件，spawnRay 不使用表面偏移并保留场景时刻。

混合记录的最近事件选择、类别切换和 miss 保留行为由 `nearlighter.bvh` 覆盖。
介质记录在非均匀变换下的位置、单位出射方向和绑定由 `nearlighter.medium` 覆盖。

测试使用解析结果明确的单位球和固定射线。计数 Shape、Texture 与显式 HitMethod 再经 Instance / BVH 查询，确认只调用一次事件方法和几何查询、候选记录不求值纹理、表面不消费随机数。
显式方法在 transformed 后保留；材质与方法不兼容时在 Primitive 构造阶段拒绝。

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
- 固定容量 BSDF 拒绝第五个分量，防止数组越界；失败后计数与已有响应不变。
- 两个连续分量的总函数值与平均 PDF；连续与 delta 混合时分别检查立体角密度和离散事件概率；两个同方向 delta 分量的期望吞吐量。
- 非轴对齐 ShadingFrame 下采样、求值与 PDF 查询的一致性，以及 importance transport 不带 radiance 的平方折射率修正。
- 同一 Dielectric 同时生成 Radiance 与 Importance 响应，验证方向和概率相同、权重相差 1.5²，且后创建的响应不改变已有响应。
- 通过 Material 引用调用统一工厂，检查表面结果为 BSDF、纯发光结果为 monostate。
- 指定分支测试通过固定种子选择事件，不注入另一套生产采样器。
- BSDF 移动后保持坐标系与响应，被替换分量和最终分量各析构一次。
- 测试材质直接继承 Material，以 kind() 声明表面家族，并组合两个分量；验证 Primitive 接受它，且不需要修改 BSDF 分派代码。
- 粗糙金属可求值，纯 Emissive 返回无散射并保持单面发光。
- GGX 在三个宽度和三种观察方向下检查 sample / PDF 一致、反射互易和能量不增益。
- GGX 在 alpha=1、法向观察时有解析 PDF `1/(4*pi)`，上半球接受概率为 1/2；固定 32768 样本验证该分布质量，容差 0.01 超过三个标准差。
- 同一位置纹理材质在球面相反两点生成独立 BSDF；销毁交点、Primitive、Material 和 Texture 后，再验证两份响应的颜色与方向基。

数学测试直接构造 BSDF，以解析方向和分层立体角积分隔离求交与积分器。
材质入口及生命周期测试使用单位球和局部测试类型，不引入生产级自定义材质框架。

## `nearlighter.render_core`

对应文件：[`render_core_tests.cpp`](render_core_tests.cpp)

### 测试目的

验证 Film 的分层累积，以及 Light 与 LightSampler 的条件/选择概率约定。
积分器的能力边界由 scene 与 renderer_smoke 中的实际调用覆盖。

### 测试内容与逻辑

- Film 对 beauty、emission、direct、indirect 的逐像素样本均值，精确样本
  计数和 Welford 无偏 beauty 方差；未启用的可选 AOV 不可访问。
- 发光 Quad 由 Scene 创建 AreaLight；样本携带入射方向、辐射亮度、
  距离与条件方向 PDF。LightSampler 单独返回选择 PMF，乘积与命中灯面的查询一致。
- 非黑色背景形成均匀球面分布的 infinite-light strategy。
- finite light 与 environment 同时存在时，两者的最终 PDF 均包含 `1 / 2`
  uniform light-selection PMF。
- uniform 选择的区间分配、PMF 归一化、空列表和不属于当前集合的灯光。
- 同一个 AreaLight 接受表面和介质受光点，样本 PDF 与查询一致。

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
- Homogeneous Primitive 的 Shape 只控制体积范围，不应被发现为面光源。
- 同一组嵌套灯光与显式展开场景，在 ScatteringOnly、light-only、MIS 下逐像素对照；
  结果须非黑、无非法贡献。改变分辨率后复用同一 Scene，world 和灯光地址不变。
- 移动 Scene 保持 world/Light 地址稳定；纯非发光实例保留原 Instance 和 BVH。
- 发光源先通过 Instance 加入，再直接加入 world，验证两处放置都可见且分别关联到灯光。
  这覆盖构建缓存已记录源对象后，直接放置仍须登记的情况。
- 不可采样的发光 Mesh 仍保留在 world 中，并按实际放置计数。
  同一源实例化两次时计数为二，灯光列表为空，NEE 模式明确拒绝；纯散射模式仍可处理。
- 自定义不透明求交包装保留普通求交能力，但不能把隐藏光源误判为已完整检查。

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

测试在测试可执行文件旁生成三个最小 fixture，检查顶点坐标、扇形三角形索引、OBJ 法线和 UV 对齐，以及 binary PLY 浮点字节序。
这些检查能够发现数量正确但导入内容错误的情况。
Debug 与 Release 使用各自构建目录，测试结束后删除 fixture。

## `nearlighter.bvh`

对应文件：[`bvh_tests.cpp`](bvh_tests.cpp)

### 测试目的

验证 BVH 只改变求交效率，不改变 Intersectable 集合的最近命中语义，并验证 Instance 能够共享和整体放置同一 BVH。

### 测试内容

- 多组固定射线的命中与未命中状态。
- 最近命中的 `t`、命中点和法线。
- 命中的 front-face 与 Material 状态。
- 同一 BVH 的两个 Instance 具有独立命中结果和世界包围盒。
- 固定 t 的表面／介质测试叶子交替作为最近结果，验证两种容器选择相同的 variant 分支。
- 上下界裁剪和 miss 不修改原记录；采用确定性叶子隔离随机数消费顺序。

### 实现逻辑

同一组 Sphere Primitive 同时构造为 `LinearAggregate` 和 `BVH`。BVH 在私有对象副本上按 centroid 中位数构建二叉层次，单对象叶只求交一次。对每条射线分别使用相同 seed 的独立 Sampler 求交，并将线性聚合结果作为参考。实例测试让两个 Instance 共享同一底层 BVH，只改变各自 Transform。

## `nearlighter.medium`

对应文件：[`medium_tests.cpp`](medium_tests.cpp)

### 测试目的与逻辑

以固定种子独立计算指数自由程期望值，验证介质 Primitive 的事件参数、位置、绑定和包围盒。

- 非单位方向保持物理事件位置；内部起点按 t=0 裁剪；miss 保留原记录。
- 分别验证 Primitive 内部缩放和外层 Instance 的密度单位，非均匀缩放后 outgoing 回到调用方空间。
- Isotropic 在 20000 个固定样本下验证单位方向、匹配 PDF、零一阶方向矩与 `E[z²]=1/3`。
- 用归一化但非均匀的测试相函数配合均匀提议，检查 PhaseResponse 没有把响应值误当成 PDF。
- 通过 Material 根接口构造 PhaseResponse，再通过共同 SampleableFunction 接口验证求值和样本值只乘一次 albedo，条件 PDF 不乘 albedo，零反照率仍返回有效的零贡献样本。
- 检查介质吞吐量只乘反照率和相函数比值，不乘表面余弦；检查无穷消光被拒绝。

统计矩的容差为 0.01–0.015，覆盖该样本量下的数个标准差。

## `nearlighter.sampler`

对应文件：[`sampler_tests.cpp`](sampler_tests.cpp)

### 测试目的

验证确定性采样的核心契约。随机序列错误可能只表现为无法复现、像素相互影响或未来并行后结果漂移，正常渲染时不容易定位。

### 测试内容

- 相同 seed 产生相同 PCG32 序列。
- 不同 seed 能改变随机序列。
- render seed、像素坐标和 sample index 都参与 path seed 派生。
- 浮点样本和闭区间整数样本始终处于约定范围。
- 球面、旋转余弦半球和 1:3 混合分布，用独立立体角网格积分验证归一化，用 32768 个固定样本验证方向一阶矩。
- MixturePDF 用生成方向所在半球检查组件频率，用解析式检查混合密度。
- Sphere、CosineHemisphere、Surface 和 Mixture 的方向密度不随正比例缩放改变；固定方向覆盖 `1e-9` 到 `1e9` 的非单位尺度。

### 实现逻辑

测试比较等 seed 原始随机值，并逐项改变 path seed 的输入。
PDF 使用独立网格积分，避免只以生成器验证自身。
方向缩放测试用同一射线朝向一个固定 Quad，检查数值归一化及几何查询阈值都不依赖输入向量的长度。
方向矩容差为 0.015，组件频率容差为 0.01；它们检验分布变换与组合，不承担 PCG 的全面统计质量检验。

## `nearlighter.renderer_smoke`

对应文件：[`renderer_smoke_tests.cpp`](renderer_smoke_tests.cpp)

### 测试目的

验证 `Scene -> Camera -> Sampler -> Renderer -> Image` 的最小核心渲染链路。它不评价图像质量，而是捕获各模块单独正确、组合后却产生空图、非有限值或不可复现结果的集成错误。

另用确定性介质事件验证体积发光的边界：当前积分器必须明确拒绝源项，不能把它直接加作表面辐射亮度。

### 测试内容

- 输出 Image 的宽高与 RenderSettings 一致。
- RenderSettings::aovs 控制输出图层；同 seed 的 beauty-only 与全部 AOV 渲染逐像素一致。
- 图像不是全黑，所有通道均为有限值。
- 相同 Scene、设置和 seed 两次渲染逐像素一致。
- 修改 seed 后采样结果发生变化。
- `stats.counters.sample_count` 等于宽、高和实际 SPP 的乘积。
- 行级进度回调按顺序覆盖每一行，并报告正确的图像尺寸和单调积分时间。
- `path` 模式在受支持的 diffuse + constant-environment 场景中生成 NEE
  shadow rays 和 BSDF continuation rays，并保持确定性。
- `scattering` 不生成 shadow ray；`light_only` 和 `mis` 保留 NEE，从调度层
  验证 NEE 与方向继续策略确实走独立分支。
- Path Film 满足 `beauty = emission + direct + indirect`，每像素样本数与
  SPP 相同；默认关闭 RR，显式启用后实际终止部分路径。

### 实现逻辑

测试程序化构造一个 `13 x 11`、`3 SPP` 的微型发光球场景。小尺寸保证测试快速完成，非平方数 SPP 同时覆盖“SPP 表示实际样本数”的契约。第一次渲染记录每次行级回调，验证完成行数、图像尺寸和积分时间顺序；两次相同 seed 的结果按像素精确比较，另用不同 seed 渲染一次验证随机序列确实参与成像。

新积分器链路另使用一个 `13 x 11` 的 Lambertian 球与常量环境 fixture。
同 seed 两次渲染逐像素比较，随后检查 ray counters、AOV 恒等式和 sample
count；RR 变体把起始深度设为 1，以固定 seed 验证终止计数，而不把随机
图像质量当作单元测试判据。

### 解析场景均值

- 均匀环境照亮平面，漫反射率 0.5 的解析亮度为 0.5。
- 漫反射分量 0.2 加镜面分量 0.3 的解析亮度同为 0.5。
- 四种策略各使用 32768 固定样本，容差 0.015；同时检查 direct AOV，捕获 Mixture 离散 PMF 遗漏或重复 NEE。
- 相机位于雾球中心时，纯吸收的解析亮度为 `exp(-0.7)`；反照率为 1 时均匀环境亮度保持 1。
- 反照率 0.6 对照两种介质策略的均值，检查真实介质计数与 AOV 恒等式。
- 含介质的 LightOnly / MIS 在相机采样前被拒绝。

## `nearlighter.scene_loader`

对应文件：[`scene_loader_tests.cpp`](scene_loader_tests.cpp)

临时 JSON 场景经完整 Loader 和 Renderer 执行，测试后删除 fixture：

- JSON 几何、消光和反照率的结果与直接构造的 core 场景逐像素一致。
- Homogeneous 材质被识别为介质，体积范围不会生成灯光。
- 省略 phase 与显式 Isotropic 的渲染逐像素相等。
- 旧对象级 medium 没有 dummy surface material 时仍映射到相同渲染结果。
- 重复介质来源、未实现相函数、未消费相函数参数、开放体积范围和旧 fuzz 字段明确报错。

这些错误可能使格式合法的场景静默采用错误模型或参数，因此保留加载层的独立回归覆盖。
fixture 写入各自测试可执行文件旁，Debug 与 Release 并行测试不会覆盖对方文件。
