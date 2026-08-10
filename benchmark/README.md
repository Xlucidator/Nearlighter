# Benchmark

本目录保存可复现 benchmark 的静态配置。下载的数据、固定 reference 和运行日志不提交到 Git，需通过脚本显式准备；evaluation 不会隐式联网或改写 reference。

## 使用方式

```bash
# 下载并校验官方数据，同时生成 Bunny 回归 reference
python3 scripts/prepare_benchmark.py

# 数据已准备后，运行官方/标准工作负载 suite
python3 scripts/evaluate.py --suite benchmark

# 只准备官方 Cornell GT 和 Bunny Mesh，不生成 Bunny reference
python3 scripts/prepare_benchmark.py --skip-bunny-reference

# 查看完整参数
python3 scripts/prepare_benchmark.py --help
python3 scripts/evaluate.py --help
```

网络访问只发生在 `prepare_benchmark.py` 的显式调用中。下载文件与解包文件均按 [`datasets.json`](datasets.json) 中的 SHA-256 或元素数量校验。

## 目录语义

- [`datasets.json`](datasets.json)：来源、校验值、目标路径、使用条件和 reference 生成设置。
- [`scenes/`](scenes/)：benchmark 专用场景；与通用示例场景分开维护。
- `data/`：下载缓存和解包后的官方数据；Git 忽略。
- `references/`：固定线性 PFM reference 及其 manifest；Git 忽略。
- `build/evaluation/`：每次 evaluation 的结果、差值图、日志和报告；Git 忽略。

## Cornell Box

### 数据来源

- 场景尺寸与物体顶点来自 [Cornell Program of Computer Graphics 发布的测量数据](https://web.archive.org/web/20230512133419/https://www.graphics.cornell.edu/online/box/data.html)。
- GT 为 Cornell 发布的 `512 × 512` synthetic RGBE 图像，经逐像素 RGBE 解码后保存为线性 RGB PFM，不做曝光、gamma 或色调映射。
- 源 URL、下载 SHA-256 与生成后 reference SHA-256 分别记录在 [`datasets.json`](datasets.json) 和生成的 `references/cornell_box_official_v1/manifest.json` 中。

### Nearlighter 适配

- 官方四边形面按相同顶点拆为三角形；长方体底面被地面遮挡，未重复加入。
- 地面和天花板使用完整平面。官方模型中被长方体或灯具遮挡的小孔未单独切分，因为当前结果不可见，但该几何并非逐多边形完全等价。
- 相机使用官方位置与视向；`vertical_fov = 39.307648°` 由 `25 mm` 垂直成像面和 `35 mm` 焦距换算。Nearlighter 当前使用理想针孔相机。
- Nearlighter 尚未实现光谱渲染。墙面反射率使用常见 Cornell Box linear RGB 代理值，灯具使用固定暖色 RGB radiance；它们不是官方光谱数据的严格 CIE/D65 积分结果，也没有复现官方绝对辐射定标。
- 因材质、光源标定、相机响应和几何孔洞均存在近似，本 case 用于标准构图下的质量趋势检查，不能声明为官方物理配置的像素级复现。

### 指标解释

报告同时保留原始指标和曝光对齐指标。曝光对齐只对 Nearlighter 结果乘一个全局标量：

```text
k = dot(result, reference) / dot(result, result)
```

`exposure_scale` 为该标量，其余 `exposure_aligned_*` 指标使用 `k × result` 计算。该处理只消除单一全局亮度比例，不能补偿色差、构图偏移、材质误差或局部光照误差。原始指标仍需保留并优先用于观察绝对输出变化。

本阶段不设置通过阈值。`completed` 仅表示数据、构建、CTest、渲染和指标计算成功；应结合预览、差值图和历史同 case 结果判断是否正常。

## Stanford Bunny

### 数据来源

- Mesh 使用 [Stanford 3D Scanning Repository](https://graphics.stanford.edu/data/3Dscanrep/) 的官方 zippered reconstruction：`35,947` 个顶点、`69,451` 个三角形。
- 使用归档内的 ASCII PLY，保留原始顶点和连接关系；SHA-256 与元素数量均在准备阶段验证。
- 数据需注明 Stanford University Computer Graphics Laboratory 来源；可用于研究和带署名的免费再分发，商业使用需取得许可。以官方 repository 的最新说明为准。

### Nearlighter 适配

- PLY 不含顶点法线，加载后用相邻三角形的面积加权法线生成平滑法线。
- 场景中的地面、相机、材质和面光源是 Nearlighter 的固定 benchmark fixture，不属于 Stanford 数据集。
- 场景形成两级加速结构：Renderer 的顶层 BVH 管理 Scene 对象，Mesh 内部 BVH 管理三角形。
- reference 由 Nearlighter 以固定场景、`256 × 256`、`256 SPP`、深度 `12`、seed `1` 显式生成。它只用于检测历史回归，不是 Stanford 提供的 GT，也不能独立证明渲染正确。
- evaluation 使用独立 seed、`16 SPP`，并重复三次。三次线性结果哈希必须一致；加载、准备、积分和吞吐量报告最小值、中位数与最大值。

## Mesh 支持范围

- OBJ：位置、纹理坐标、法线、正负索引和多边形扇形三角化。
- PLY：ASCII 与 binary little-endian 1.0；位置、可选法线、常见 UV 名称和多边形面。
- OBJ 的对象、分组、平滑组和材质库记录不改变几何；一个 Scene Mesh 当前绑定一个 Material。
- 不支持 binary big-endian PLY、OBJ/PLY 内嵌材质、切线、骨骼、实例和通用场景层级。
- 多边形使用扇形三角化，适合凸多边形；导入器不负责修复自交、非凸或退化拓扑。
