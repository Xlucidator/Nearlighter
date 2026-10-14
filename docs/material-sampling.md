# Material 与采样接口

本文说明材质评审后的当前实现。
求交、场景组装及自由程算法见 [Hit、Material、Medium 与渲染流程](hit-material-medium.md)。

## 1. 从命中到下一条射线

```text
world.hit()
  -> HitRecord：SurfaceRecord / MediumRecord
  -> Material::emission(record, outgoing)
  -> Material::scattering(record, mode)
       ScatteringResponse：monostate / BSDF / PhaseResponse
  -> integrator 选择方向提议
       响应自身的 sample(outgoing, sampler)
       或目标物体的 PDF::generate(sampler)
  -> 响应值 / 实际提议概率 -> 路径权重
       表面额外乘 abs(cosine)
  -> record.spawnRay(incoming, time)
```

Material 保存可共享的参数、纹理和模型选择。
`scattering()` 在最终命中点解析参数，按值返回本次事件的响应。
积分器决定采样策略、更新路径权重并追踪下一条射线。

积分器通过 `std::visit` 处理具体响应，两种响应都提供 `evaluate / PDF / sample`。
表面的余弦投影仍由积分器根据事件类型计算，介质不伪造表面法线或余弦。

## 2. sampling 中的三种对象

| 对象 | 职责 | 接口 |
|---|---|---|
| Sampler | 保存并推进一条路径的随机序列 | `next1D / next2D` 等 |
| PDF | 表达一个方向提议分布 | `value(direction)`、`generate(sampler)` |
| SampleableFunction | 表达目标函数 F 及其默认提议 q | `evaluate(condition, point)`、`PDF(condition, point)`、`sample(condition, sampler)` |

`SampleableFunction<Condition, Point, Value, Result>` 不依赖材质、颜色或渲染枚举。
`sample()` 直接返回 `Result`，由具体实例决定结果结构及是否允许失败。

| 实例 | 函数值 | sample 的结果类型 | 方向空间 |
|---|---|---|---|
| BxDF | Color | `std::optional<ScatteringSample>` | 局部坐标 |
| BSDF | Color | `std::optional<ScatteringSample>` | 世界坐标 |
| PhaseResponse | Color | `std::optional<ScatteringSample>` | 世界坐标 |
| PhaseFunction | float | `std::optional<PhaseSample>` | 与调用者一致；当前使用世界坐标 |
| 测试标量函数 | float | 普通结果结构 | 标量定义域 |

非渲染测试用均匀提议积分 `F(x)=2x`，验证接口不要求 RGB、方向或 optional。
F 和 q 不必相等，也不要求成比例。
如果积分器改用其他提议，分母必须采用实际提议的概率。

### 独立方向分布

抽象类和具体 PDF 集中声明在 `sampling/pdf.h`，实现放在 `src/nearlighter/sampling/pdf.cpp`。
各类只公开构造及 `value / generate`，不公开局部采样算法、组件选择或缓存密度的辅助入口。

| 分布 | 含义与用途 |
|---|---|
| SpherePDF | 均匀球面；Isotropic 和常量环境光共用 |
| CosineHemispherePDF | 指定法线的余弦半球；Lambertian 在自己的局部坐标中使用 |
| SurfacePDF | 从参考点朝目标表面集合采样方向；不判断材质是否发光 |
| MixturePDF | 按构造时给定的概率混合两个连续分布；借用组件 |

MixturePDF 不拥有组件，两个被借用分布必须比它活得久。
分布组合不创建逐次 shared_ptr 链。
方向密度只取决于朝向，不取决于输入向量长度。
非零有限方向先按最大分量缩放再归一化，避免极大或极小长度导致浮点溢出、下溢或几何查询阈值变化。
零向量和非有限方向的密度为零。

积分器的 mixture 策略还需要处理镜面事件，所以直接选择散射响应或目标 PDF。
它不使用普通 MixturePDF 反向识别组件，也不把完整散射样本压成一个方向。
策略选择概率、连续混合密度及离散事件概率在积分器内集中计算。

## 3. 连续查询、镜面事件与采样失败

接口区分连续查询和完整事件采样：

| 操作 | 返回内容 |
|---|---|
| `evaluate()` | 连续部分的函数值 |
| `PDF()` | 连续部分的提议密度；方向函数以立体角为测度 |
| `sample()` | 实际事件及其函数值、概率、事件标记等信息 |

Lambertian、GGX 和当前相函数采样的是连续方向。
有效连续样本的密度与 `PDF()` 查询一致。

理想镜面的概率集中在精确方向上，不能用普通有限的立体角密度表示。
镜面 BxDF 的 `evaluate()` 和 `PDF()` 返回连续部分的零值。
它的采样结果则携带 delta 系数和离散事件概率：

- 理想反射只有一个事件，概率为 1。
- 理想玻璃按 Fresnel 概率选择反射或折射。
- BSDF 选择多个分量时，样本概率还包含分量选择概率。
- mixture 选择散射策略时，离散概率还包含外层策略选择概率。

`ScatteringSample::pdf` 因而可能是连续密度，也可能是离散事件概率。
`flags` 指明是否为镜面事件；不能用 `PDF(sample.incoming)` 覆盖镜面样本携带的概率。
定义与标准处理参考 [PBRT BSDF 表示](https://pbr-book.org/4ed/Reflection_Models/BSDF_Representation)。

渲染响应允许 `sample()` 返回 nullopt。
例如，GGX 先采微表面法线，反射方向仍可能落到宏观表面下方。
这次采样按零贡献计入样本数，不重采到成功，否则会改变实际提议分布。

函数值为零与采样失败不同。
零反照率 PhaseResponse 仍可产生正常方向及概率，只是样本贡献为零。

## 4. Material 的统一入口与生命周期

根 Material 提供家族、发光能力及统一的散射和发光接口：

```cpp
using ScatteringResponse = std::variant<std::monostate, BSDF, PhaseResponse>;

ScatteringResponse Material::scattering(
    const HitRecord& record,
    TransportMode mode = TransportMode::Radiance) const;

Color Material::emission(const HitRecord& record, const Vec3f& outgoing) const;
```

具体材质直接继承 Material，以 kind() 声明家族，不保留额外的家族基类。
具体材质内部取得适用的记录；外部可直接通过 Material 调用共同接口。

ScatteringResponse 按值保存以下三种结果：

- monostate：没有散射，例如当前纯 Emissive。
- BSDF：本次表面命中的响应。
- PhaseResponse：本次介质事件的响应。

variant 自身不为响应对象分配堆内存。
BSDF 仍独占当前命中的 BxDF，分量保存已求值的材质参数。
具体 BxDF 留在对应材质文件中。
PhaseResponse 保存当前点的反照率，并借用材质拥有的只读 PhaseFunction。
相函数必须活得比 PhaseResponse 久；当前 Scene 和路径循环满足这一约定。

`material/scattering.h` 集中放置 TransportMode、BxDFFlags 和 ScatteringSample。
这些是 BSDF、BxDF 和 PhaseResponse 共享的基础值类型，不依赖 Material 类。
Material 则依赖完整响应类型，依赖方向保持单向。

`emission()` 统一返回 Color，但记录类型决定物理含义：

- 表面返回向指定方向发出的辐射亮度。
- 介质返回需要沿距离积分的体积发光源项。

当前 Homogeneous 不发光，尚未实现体积发光积分或体积光源采样。
积分器遇到 `isEmissive()` 为 true 的介质事件会报错，不把源项直接作为表面亮度相加。
这是命中事件检查，不是整个场景的预检。

## 5. 为什么 PhaseFunction 不含 albedo

标准相函数 p 表示散射方向分布，满足：

$$
\int_{S^2} p(\omega_o, \omega_i)\,d\omega_i = 1
$$

把反照率 alpha 乘进去，积分变成 alpha，结果便不再是归一化相函数。
因此可以组合，但组合结果称为 PhaseResponse。
这个名称是项目的工程命名，不是用来替代学界已有的相函数定义。
标准定义参考 [PBRT 相函数章节](https://pbr-book.org/4ed/Volume_Scattering/Phase_Functions)。

当前指数自由程按消光系数 sigma_t 抽样。
命中事件后的完整方向响应为：

$$
F_{volume} = \alpha p,\qquad \alpha = \sigma_s / \sigma_t
$$

PhaseResponse 返回 F；其 PDF 返回方向提议 q，不乘 alpha。
介质继续权重为 `F / q`。
各向同性的默认提议恰好等于 p，因此这一步权重就是 alpha。
消光系数仍由自由程算法处理，不会在 PhaseResponse 里重复相乘。

BSDF 则定义单位入射辐照度产生多少出射辐射亮度，不要求函数积分为一。
例如 Lambertian 的 BSDF 是 `albedo / pi`，已经包含反射强度。
所以二者在代码中共享求值与采样接口，但不能把 PhaseFunction 直接叫作 BSDF。
表面继续权重为 `BSDF * abs(cosine) / q`。

## 6. sample 如何获得随机数

评审前的代码由积分器先调用 Sampler，随后把分量选择值和二维样本传入 BSDF。
BSDF/BxDF 的 sample 没有 Sampler 参数，并不代表没有使用随机数。

现在积分器把同一个路径 Sampler 传入响应：

```cpp
auto sample = response.sample(outgoing, sampler);
```

BSDF 消费分量选择值，具体 BxDF 消费自己所需的随机维度。
Lambertian、GGX、Isotropic 使用二维样本；玻璃事件选择使用一个标量；理想镜面分量不消费随机数。
`evaluate()` 和 `PDF()` 不改变随机序列。

两种传参方式都成立，本轮选择主要统一调用契约并减少无用随机维度。
它不会自动提高蒙特卡洛收敛速度，也未增加低差异采样能力。
如果未来需要严格分配 QMC 维度，应在 Sampler 层明确维度约定。
因为维度消费次序改变，同一个 seed 的噪声不保证与旧版本逐像素相同。

## 7. TransportMode 的具体作用

当前代码中，mode 只决定玻璃透射响应是否乘 `(eta_before / eta_after)²`。
它不改变反射方向、折射方向或 Fresnel 分支概率。

| 模式 | 计算的量 | 当前用途 |
|---|---|---|
| Radiance | 最终有多少光到达相机 | 当前相机路径 |
| Importance | 一份光对指定像素读数能产生多大作用 | 保留给未来光源子路径 |

可以把像素读数理解为光的一侧与接收权重的一侧共同决定的结果。
Radiance 描述有多少光，Importance 描述所选像素对这些光的接收权重。
Importance 不是另一种光能，也不是重要性采样策略。

折射会压缩或展开对应的方向范围，两种量经过折射时需要不同的局部权重。
按当前路径约定，从空气侧折入折射率 1.5 的玻璃，正入射反射概率为 0.04，透射概率为 0.96：

| 模式 | 透射系数（正入射余弦为 1） | 除以 0.96 后的条件路径权重 |
|---|---|---|
| Radiance | `0.96 / 1.5²` | `1 / 1.5²` |
| Importance | `0.96` | `1` |

平方因子来自折射前后方向尺度的转换，不表示玻璃额外吸收了光。
不能脱离方向约定理解成任意穿越玻璃都除以 2.25。
公式与标准实现见 [PBRT Dielectric BSDF](https://pbr-book.org/4ed/Reflection_Models/Dielectric_BSDF)。

模式只在构造响应时传入：

```text
Integrator 选择 mode
  -> Material::scattering(record, mode)
  -> SpecularDielectricBxDF 构造函数
  -> mode_ 成员
```

默认参数为 Radiance，当前生产积分器使用这一模式。
同一个共享材质可以同时产生不同模式的响应，互不修改。
BSDF、其他 BxDF 和 PhaseResponse 不重复保存无用 mode。
方向查询与采样接口不再传 mode，通用采样层也不认识 TransportMode。

## 8. 建议阅读顺序

1. `sampling/sampleable_function.h` 与 `sampling/pdf.h`：函数与分布的边界。
2. `material/scattering.h`：连续与离散样本、输运模式。
3. `material/material.h`：统一命中输入与响应结果。
4. `surface/lambertian` 与 `medium/phase_response`：两种简单响应。
5. `surface/bsdf`：分量组合、坐标转换与分量概率。
6. `render/path_integrator.cpp` 的 `sampleContinuation()`：策略选择与路径权重。
7. `surface/dielectric`：TransportMode 的实际公式。
