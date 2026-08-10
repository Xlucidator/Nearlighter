# Scripts

本目录提供 Nearlighter 的集成评估、benchmark 数据准备和固定 reference 生成脚本。所有命令均从项目根目录执行。

## 集成评估（`evaluate.py`）

### 涉及文件

- [`evaluate.py`](evaluate.py)：评估入口、case 执行和指标计算。
- [`evaluation.json`](evaluation.json)：构建、case 和 suite 配置。
- [`utils.py`](utils.py)：进程、构建、PFM 和报告等共享实现。
- [`../benchmark/datasets.json`](../benchmark/datasets.json)：外部 benchmark 数据与校验配置。
- `benchmark/references/`：固定线性 PFM reference。
- `build/evaluation/`：每次 evaluation 的生成结果。

### 使用方式

```bash
# 日常快速检查
python3 scripts/evaluate.py --suite quick

# 阶段性完整检查
python3 scripts/evaluate.py --suite full

# 官方 Cornell GT 与 Stanford Bunny 工作负载
python3 scripts/evaluate.py --suite benchmark

# 复用已经配置并构建的 executable
python3 scripts/evaluate.py --suite quick --skip-build

# 同时复用 executable 并跳过 CTest
python3 scripts/evaluate.py --suite quick --skip-build --skip-tests

# 查看完整参数
python3 scripts/evaluate.py --help
```

参数语义：

- `--suite <name>`：选择 `evaluation.json` 中的 suite，默认 `quick`。
- `--skip-build`：不重新配置和构建，复用配置目录中的 `Nearlighter`。
- `--skip-tests`：不执行 evaluation 前置 CTest；不影响是否构建。

当前 suite 定位：

| suite | case 设置 | 主要用途 |
|---|---|---|
| `quick` | 64×64、8 SPP、深度 8 | 快速确认构建、测试、场景加载、渲染和比较流程正常 |
| `full` | 256×256、128 SPP、深度 25 | 检查图像回归，并提供较稳定的性能样本 |
| `benchmark` | 官方 Cornell 512×512、64 SPP；Bunny 256×256、16 SPP × 3 | 外部 GT 质量比较与 Mesh/BVH 性能工作负载 |

`quick` 渲染时间很短、采样数很低，不适合严肃的性能比较或最终画质判断。
`benchmark` 依赖显式准备的数据，首次使用前运行 `python3 scripts/prepare_benchmark.py`；evaluation 本身不会联网或生成 reference。

### 配置语义

[`evaluation.json`](evaluation.json) 的顶层分为三部分：

- `build`：构建目录和 CMake configuration。
- `cases`：完整测评单元，包括 scene、render、单一 reference 和 metrics。
- `suites`：一次 evaluation 需要执行的 case 集合。

case 使用固定 seed，结果在相同实现和运行环境中可复现。测试 seed 与 reference seed 可以不同，使比较结果反映独立采样序列之间的误差，而不是复用同一组随机样本。

### 实现逻辑

```text
读取 evaluation.json
        ↓
创建独立 run 目录和初始 manifest
        ↓
配置并构建指定 configuration
        ↓
运行 CTest
        ↓
按 suite 执行各 case 的确定性渲染
        ↓
读取 result.pfm 和固定 reference.pfm
        ↓
计算线性 RGB 指标和绝对差值图
        ↓
归档日志、case 报告、manifest 和 summary
```

普通 evaluation 只读取 reference，不会生成、更新或覆盖它。reference 缺失、尺寸不匹配、构建失败、CTest 失败或渲染失败都会终止本次 evaluation，并在 `manifest.json` 中记录失败状态和原因。

### 输出结构

```text
build/evaluation/
├── latest.json
└── runs/<run-id>/
    ├── cases/<case-name>/
    │   ├── difference.pfm
    │   ├── difference-exposure-aligned.pfm  # 仅曝光对齐 case
    │   ├── metrics.json
    │   ├── preview.ppm
    │   ├── preview-exposure-aligned.ppm  # 仅曝光对齐 case
    │   ├── reference.ppm
    │   ├── resolved-config.json
    │   └── result.pfm
    ├── logs/
    │   ├── build.log
    │   ├── configure.log
    │   ├── ctest.log
    │   └── <case-name>-repeat-<n>.log
    ├── manifest.json
    └── summary.json
```

- `latest.json`：最后一次 evaluation 的 run id；后运行的 suite 会覆盖该指针。
- `summary.json`：本次运行的全部 case、指标和渲染性能汇总。
- `manifest.json`：状态、Git、配置哈希、编译器、主机和 case 产物哈希。
- `metrics.json`：单个 case 的渲染设置、指标和性能数据。
- `preview.ppm`：显示用结果，适合直接检查构图、颜色和噪声。
- `preview-exposure-aligned.ppm`：乘以报告中的全局曝光系数后的结果预览；仅在 case 请求 `exposure_scale` 时生成。
- `reference.ppm`：与结果采用相同 gamma 2.2 显示转换的 GT/reference 预览；仅供直观对照。
- `result.pfm`：未做显示变换的线性 RGB 浮点结果。
- `difference.pfm`：result 与 reference 的逐通道绝对差值。
- `difference-exposure-aligned.pfm`：全局曝光缩放后的绝对差值；仅在 case 请求 `exposure_scale` 时生成。

所有 evaluation 产物均位于被 Git 忽略的 build tree 中。

### 图像指标

设线性 RGB 输出为 $I_i$，reference 为 $R_i$，通道总数为 $N=3WH$，reference 最大通道值为 $P=\max_i R_i$。

| 指标 | 定义 | 数值方向 | 适合判断 |
|---|---|---|---|
| MSE | $\frac{1}{N}\sum_i(I_i-R_i)^2$ | $\downarrow$ | 同一 case 下的总体平方误差 |
| RMSE | $\sqrt{\mathrm{MSE}}$ | $\downarrow$ | 与线性 RGB 数值同量纲的典型误差幅度 |
| relative MSE | $\frac{\sum_i(I_i-R_i)^2}{\sum_iR_i^2}$ | $\downarrow$ | 误差能量相对 reference 能量的比例 |
| PSNR | $10\log_{10}(P^2/\mathrm{MSE})$ | $\uparrow$ | 同一 reference 下更直观的对数误差比较 |

Cornell 官方 case 另报告 `exposure_scale` 与 `exposure_aligned_*`。其中
$k=\operatorname{dot}(I,R)/\operatorname{dot}(I,I)$，对齐指标使用 $kI$ 与 $R$ 比较；它只消除全局亮度比例，不能修复颜色、几何或局部光照差异。

指标使用原则：

- 只比较同一个 case、同一个 reference 的多次结果；MSE 和 RMSE 不具有跨场景通用阈值。
- relative MSE 乘以 100% 表示误差能量占比，不是平均像素百分比误差。
- PSNR 依赖 reference peak。HDR 场景中的高亮光源会增大 peak，因此不能把不同场景的 PSNR 直接横向比较。
- MSE 为零时，控制台显示 `infinite` PSNR，JSON 中对应值为 `null`。
- 指标突然恶化时应检查 `difference.pfm`：随机散斑通常是采样误差；边缘错位、整体偏色、翻转或局部成片差异更可能是实现回归。

evaluation 当前没有自动通过阈值。`completed` 只表示构建、CTest、渲染和指标计算成功，画质是否正常仍需与可信历史结果和差异图结合判断。

### 性能参数

- `scene_load_seconds`：JSON、纹理和 Mesh 读取与 Scene 构造时间。
- `preparation_seconds`：相机预计算与 Renderer 顶层 BVH 构建时间。
- `integration_seconds`：Renderer 核心积分时间；不包括准备、进度回调和图像文件 I/O。
- `sample_count`：`width × height × samples_per_pixel`，即 primary sample 总数。
- `samples_per_second`：`sample_count / integration_seconds`；越高越好，是当前最适合比较积分性能的参数。

case 设置 `repetitions` 后，报告会保存每次测量及 minimum、median、maximum，顶层耗时字段取 median。性能必须在同一机器、同一 configuration、同一 case 和相近系统负载下比较；不要用一次极短的 `quick` 时间判断小幅变化。

### 快速检查结果

evaluation 完成后，按以下顺序检查：

1. 终端无 `ERROR`，最后输出 `Evaluation completed`。
2. `manifest.json` 的 `status` 为 `completed`，并确认 Git、configuration 和主机环境可比。
3. `logs/ctest.log` 中全部测试通过。
4. 打开 `preview.ppm`，检查构图、颜色、曝光和噪声是否存在明显异常。
5. 查看 `summary.json`：同一 case 下 MSE、RMSE、relative MSE 不应突然升高，PSNR 不应突然降低。
6. 发生指标变化时查看 `difference.pfm`，判断差异是随机噪声还是结构性错误。
7. 比较 `samples_per_second`；性能判断以相同 case 的多次 Release 结果为准。

```bash
# 查看最后一次运行的索引
python3 -m json.tool build/evaluation/latest.json

# 将终端输出的实际 run 目录填入这里
RUN_DIR="build/evaluation/runs/<run-id>"

# 查看本次状态、环境和复现信息
python3 -m json.tool "$RUN_DIR/manifest.json"

# 查看全部 case 的质量与性能汇总
python3 -m json.tool "$RUN_DIR/summary.json"

# 查看 CTest 日志
less "$RUN_DIR/logs/ctest.log"
```

## Benchmark 准备（`prepare_benchmark.py`）

### 涉及文件

- [`prepare_benchmark.py`](prepare_benchmark.py)：显式下载、校验、转换与 reference 准备入口。
- [`utils.py`](utils.py)：RGBE/PFM、构建、渲染和报告共享实现。
- [`../benchmark/datasets.json`](../benchmark/datasets.json)：来源 URL、SHA-256、目标路径和生成设置。
- [`../benchmark/README.md`](../benchmark/README.md)：数据依据、场景适配、许可和指标限制。

### 使用方式

```bash
# 完整准备：Cornell 官方 GT、Stanford Bunny Mesh 和 Bunny reference
python3 scripts/prepare_benchmark.py

# 只准备官方数据，不生成自有 Bunny reference
python3 scripts/prepare_benchmark.py --skip-bunny-reference

# 复用现有 Release executable 生成 Bunny reference
python3 scripts/prepare_benchmark.py --skip-build

# 重新下载并替换已经准备的产物
python3 scripts/prepare_benchmark.py --force

# 查看完整参数
python3 scripts/prepare_benchmark.py --help
```

### 实现逻辑

脚本只在被显式调用时访问网络。Cornell RGBE 和 Stanford archive 首先下载到临时文件，SHA-256 正确后才替换缓存；Bunny 只提取指定 PLY member，并验证 PLY 的顶点数与三角形数。

Cornell RGBE 被解码为线性 PFM。Bunny 使用官方 Mesh 与固定 Nearlighter fixture 生成高 SPP 回归 reference。每个 reference 目录写入独立 manifest，记录来源、哈希、设置和 reference 类型。具体适配和限制见 [benchmark/README.md](../benchmark/README.md)。

## Reference 生成（`generate_reference.py`）

### 涉及文件

- [`generate_reference.py`](generate_reference.py)：单个固定 reference 的显式生成入口。
- [`utils.py`](utils.py)：构建、渲染命令、PFM 读取和日志等共享实现。
- `benchmark/references/`：约定的固定 reference 存放目录。

### 使用方式

```bash
# quick case reference
python3 scripts/generate_reference.py \
    --scene assets/scenes/cornell_box_rtow.json \
    --output benchmark/references/cornell_box_rtow_quick_v1/reference.pfm \
    --preview benchmark/references/cornell_box_rtow_quick_v1/preview.ppm \
    --width 64 \
    --height 64 \
    --spp 512 \
    --max-depth 25 \
    --seed 1

# full case reference
python3 scripts/generate_reference.py \
    --scene assets/scenes/cornell_box_rtow.json \
    --output benchmark/references/cornell_box_rtow_full_v1/reference.pfm \
    --preview benchmark/references/cornell_box_rtow_full_v1/preview.ppm \
    --width 256 \
    --height 256 \
    --spp 1024 \
    --max-depth 25 \
    --seed 1

# 查看完整参数
python3 scripts/generate_reference.py --help
```

主要参数：

- `--scene`：源场景 JSON 路径。
- `--output`：线性 PFM reference 路径，必须使用 `.pfm` 后缀。
- `--preview`：显示用 PPM 路径；省略时使用 output 的同名 `.ppm`。
- `--width`、`--height`、`--spp`、`--max-depth`、`--seed`：固定渲染设置。
- `--build-directory`、`--configuration`：构建目录和 configuration，默认 `build/release` 与 `Release`。
- `--skip-build`：复用已有 executable。
- `--force`：显式允许替换已有 reference 和 preview。

### 实现逻辑

脚本默认配置并构建 Release，但不运行 CTest。渲染首先写入临时 PFM 和 PPM；PFM 能被正常读取且尺寸与参数一致后，临时文件才替换目标文件。目标已存在时默认拒绝覆盖，避免日常 evaluation 意外改变基准。

reference 和 preview 写入各自指定路径，构建与渲染日志写入 output 父目录下的 `logs/`。固定 reference 位于被 Git 忽略的 `benchmark/references/`；Git 保留 benchmark 配置、专用场景、说明文档和目录骨架。

### Reference 使用原则

- 只从已经人工确认正确、工作区干净且可追溯的实现生成正式 reference。
- 使用明显高于 evaluation case 的 SPP，降低 reference 自身的 Monte Carlo 噪声。
- 使用独立 seed，避免 result 与 reference 复用同一采样序列。
- 替换 reference 属于显式基线变更，应同步检查场景、参数和评价指标，不要用 `--force` 掩盖未知回归。
- 自生成 reference 只能验证相对历史行为，不能独立证明渲染结果在物理意义上正确；正式 benchmark 应优先使用可信外部 Ground Truth。
