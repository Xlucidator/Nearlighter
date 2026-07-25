# Scripts

本目录提供 Nearlighter 的集成评估与固定参考图生成工具。

## Evaluation

主要文件：

- `evaluate.py`：评估入口与指标计算。
- `evaluation.json`：build、cases 和 suites 配置。
- `utils.py`：构建、进程、PFM 和报告等共享实现。

日常快速检查：

```bash
python3 scripts/evaluate.py --suite quick
```

阶段性完整检查：

```bash
python3 scripts/evaluate.py --suite full
```

可选参数：

- `--suite <name>`：选择 suite，默认 `quick`。
- `--skip-build`：复用现有 executable。
- `--skip-tests`：跳过 CTest 前置检查。

执行流程：

```text
读取 evaluation.json
        ↓
创建独立 run 目录
        ↓
配置并构建 Release
        ↓
运行 CTest
        ↓
执行 suite 中的 cases
        ↓
读取固定 reference PFM
        ↓
计算指标和差异图
        ↓
归档日志、manifest 和 summary
```

`evaluation.json` 顶层语义：

- `build`：构建目录与 CMake configuration。
- `cases`：完整测评单元，包括 scene、render、唯一 reference 和 metrics。
- `suites`：一次 evaluation 执行的 case 集合。

当前指标：

- MSE：所有 RGB 通道的平均平方误差。
- RMSE：MSE 的平方根。
- relative MSE：误差能量与参考图像能量的比值。
- PSNR：以参考图像最大线性通道值为 peak。
- `difference.pfm`：逐通道绝对差值图。

普通 evaluation 只读取 reference，不会生成、更新或覆盖它；reference 缺失时直接失败。每次运行的图像、日志与报告写入独立目录：

```text
build/evaluation/
├── runs/<run-id>/
│   ├── cases/
│   ├── logs/
│   ├── manifest.json
│   └── summary.json
└── latest.json
```

所有生成内容均位于被 Git 忽略的 build tree 中。

## Generate

主要文件：

- `generate_reference.py`：固定回归 reference 的显式生成入口。
- `utils.py`：构建、渲染命令与 PFM 校验等共享实现。

quick 回归参考：

```bash
python3 scripts/generate_reference.py \
    --scene assets/scenes/cornell_box_rtow.json \
    --output benchmark/references/cornell_box_rtow_quick_v1/reference.pfm \
    --preview benchmark/references/cornell_box_rtow_quick_v1/preview.ppm \
    --width 64 \
    --height 64 \
    --spp 512 \
    --max-depth 25 \
    --seed 1
```

full 回归参考：

```bash
python3 scripts/generate_reference.py \
    --scene assets/scenes/cornell_box_rtow.json \
    --output benchmark/references/cornell_box_rtow_full_v1/reference.pfm \
    --preview benchmark/references/cornell_box_rtow_full_v1/preview.ppm \
    --width 256 \
    --height 256 \
    --spp 1024 \
    --max-depth 25 \
    --seed 1
```

脚本默认配置并构建 Release，不运行 CTest，然后执行确定性渲染、校验 PFM 尺寸并发布 reference 与 preview。目标文件已存在时拒绝覆盖；有意替换时必须显式增加 `--force`。

固定 reference 位于 `benchmark/references/`。Git 只保留目录骨架，具体图片由本地生成或外部 benchmark 数据提供。
