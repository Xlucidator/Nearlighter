# Hit、Material、Medium 与渲染流程

本文描述当前实现，配合 [测试说明](../tests/README.md) 阅读。

## 1. 三层职责

| 概念 | 回答的问题 | 代码 |
|---|---|---|
| HitMethod | 这个 Primitive 沿射线在哪里产生事件？ | `render/hit/` |
| Material | 事件发生处怎样散射或发光？ | `material/surface/`、`material/medium/` |
| HitRecord | 这次找到的事件是什么，发生在哪里？ | `render/hit/hit_record.h` |

`Shape::hit()` 仍只计算几何边界。Sphere 的形状可以同时被玻璃 Primitive 和雾 Primitive 共享；前者把边界交点作为事件，后者在球内采样自由程。

一个 Primitive 保存 Shape、Material、Transform 和 HitMethod。默认根据材质选择方法，也允许程序化构造时提供兼容的方法。

- 表面材质默认使用 `SurfaceIntersection`，只调用一次 `Shape::hit()`。
- `Homogeneous` 默认使用 `ExponentialFreeFlight`，先查进入和离开边界，再采样内部事件。
- 构造时检查方法与材质是否兼容。
- `Primitive::transformed()` 保留所选方法。

`HitRecord` 是一个值类型，内部保存唯一的 `std::variant<SurfaceRecord, MediumRecord>`。
共同操作通过成员函数访问，不在每次求交时分配记录。

```cpp
record.t();
record.point();
record.outgoing();
record.material();
record.primitive();
record.spawnRay(direction, ray.time());
```

`isSurface()` 判断记录类别；`surface()` 和 `medium()` 访问具体记录。
访问错误类别会抛出 `std::bad_variant_access`。
`material()` 和 `primitive()` 返回借用引用，要求记录已经由成功的对象查询完成绑定。

| 字段 | SurfaceRecord | MediumRecord |
|---|---|---|
| point、t、outgoing | 有 | 有 |
| Primitive、Material 借用指针 | 有，材质指向 Material | 有，材质指向 Material |
| 几何／着色法线、UV、front_face | 有 | 无 |
| spawnRay | 按几何法线偏移起点 | 从事件点直接出发 |
| BSDF、相函数实例、ShadingFrame | 不保存 | 不保存 |

记录采用调用方坐标空间；最外层 `world.hit()` 的结果才是世界坐标。`outgoing` 是从事件点朝向上一条路径顶点的单位方向。

`t` 满足 `point = origin + t * direction`。`Ray::time()` 是场景时刻，两者不是同一含义。继续射线通过 `record.spawnRay(direction, ray.time())` 保留场景时刻。

## 2. 一条射线怎样获得记录

```text
PathIntegrator
  -> Scene::world().hit()
  -> BVH / LinearAggregate
  -> Instance（若有，先将射线变换到源空间）
  -> Primitive
  -> HitMethod
  -> Shape::hit()
  <- SurfaceRecord 或 MediumRecord
  <- 最近的 HitRecord
```

容器和实例可以嵌套。各层分工如下：

1. BVH 检查包围盒，叶节点调用已有 Intersectable 接口。
2. LinearAggregate 和 BVH 都只比较 `record.t()`，收紧后续查询区间。
3. Instance 查询源对象，并调用 `record.transform()` 将事件转回父空间。
4. Primitive 调用所选方法，转换局部事件并绑定自身和材质。
5. 只有积分器确定最终命中后，才读取纹理并创建散射响应。

逆变换不归一化射线方向，因此 `t` 不变。
`record.transform()` 对两类记录都更新位置和 outgoing；只有表面需要处理法线、朝向反转和 front_face。
Primitive 用 `record.bind(material, primitive)` 统一完成两种记录的绑定。

miss 不改变调用方记录。默认构造的记录不代表有效命中，只能在 `hit()` 返回 true 后使用。

## 3. 介质拆分后保留了什么

旧 ConstantMedium 的三项职责分别进入：

- Shape 和 Transform：限定介质占据范围。
- Homogeneous：保存消光系数、反照率纹理和相函数。
- ExponentialFreeFlight：在范围内采样事件距离。

Homogeneous 表示**消光恒定**，反照率仍可使用空间纹理。Isotropic 现在只表示球面各向同性相函数，不再是一种表面 Material。

自由程使用：

$$
s = -\log(U) / \sigma_t, \qquad t_{event} = t_{entry} + s / |d|
$$

这里的方向长度是 **Primitive 输入射线**的长度。内部 Transform 缩放 Shape 会改变雾的范围，不自动改变消光系数；外层 Instance 则放置整个介质实体，保留源空间的密度单位。这延续了原实现的计量方式。

算法要求闭合凸形状，每条射线只有一段内部区间。JSON 当前允许 Sphere 和 Box；不能把任意 Mesh 或开放平面当成同样的体积范围。

材质在最终事件处返回 `PhaseResponse`，封装反照率与相函数的求值和采样。相函数借用自材质，不在每次事件处分配。空间纹理收到事件位置，UV 约定为零；MediumRecord 自身没有伪造的 UV 字段。

相函数提供 `evaluate / sample / PDF`。方向约定与 BSDF 一致，但介质权重没有表面余弦：

```text
surface: f * abs(cosine) / proposal_pdf
medium:  albedo * phase_value / proposal_pdf
```

自由程抽样已经包含同段的事件概率和透射概率，积分器不会再无条件乘一次指数衰减。

## 4. 表面材质与金属模型

根 `Material` 提供家族识别、`isEmissive()`、`scattering()` 与 `emission()`。
散射和发光接口都接收完整 HitRecord，具体材质在内部取得对应的记录。
具体材质直接继承 Material，并覆盖 kind() 声明表面或介质家族。
两类记录都借用 Material 指针；表面方法按 kind() 检查兼容性。
ExponentialFreeFlight 仍要求 Homogeneous，因为该算法需要恒定消光参数。

`scattering()` 返回 `ScatteringResponse = std::variant<std::monostate, BSDF, PhaseResponse>`：

- 表面材质返回 BSDF。
- 介质材质返回 PhaseResponse。
- 当前纯 Emissive 返回 monostate，表示没有散射。

这份结果按值保存，variant 自身不分配堆内存。
积分器通过 `std::visit` 调用两种响应共同的 SampleableFunction 接口。
`sample()` 的 optional 结果只表达本次方向采样失败，与材质没有散射是两个层次。

BSDF 在 `material/surface/bsdf.h/.cpp`，继续独占最多四个 BxDF。具体 BxDF 留在对应材质文件内，新增模型无需修改 BSDF 的中央类型表。

Metal 使用各向同性 GGX、Smith 遮蔽和可见法线采样。`albedo` 作为 RGB 正入射反射率，角度变化采用 Schlick 近似；`alpha = roughness²`。为避免极窄分布的数值问题，alpha 小于 0.001 时使用相同反射率的理想镜面极限。

旧 fuzz 的经验方向扰动已删除。旧 JSON 的 fuzz 会给出明确迁移错误，示例已改成 roughness；两个参数不保证相同数值对应相同外观。`SpecularReflectionBxDF` 仍是可复用的恒定 RGB 理想反射分量。

Dielectric 默认继续使用新路径原有的精确反射率和 Radiance 折射权重。`reflectanceExact()` 与 `reflectanceSchlick()` 都是私有方法，均处理全反射；选择由内部调用决定，没有 JSON 或 CLI 开关。

GGX 数学与可见法线采样参考 [PBRT 微表面模型](https://pbr-book.org/4ed/Reflection_Models/Roughness_Using_Microfacet_Theory)。本项目使用自己的 roughness² 参数化和 RGB Schlick 反射率。

独立 PDF 分布、材质接口、PhaseResponse 与 TransportMode 的详细说明见 [Material 与采样接口](material-sampling.md)。

## 5. Scene 怎样交付 world 与 lights

目录按职责划分：

- `scene/object/`：Intersectable、Primitive、Instance 和 LinearAggregate。
- `scene/light/`：Light、AreaLight 和 EnvironmentLight。
- `render/light_sampler.h/.cpp`：跨光源集合的选择策略。
- `accel/`：BVH 等加速算法。

SceneLoader 解析 JSON，程序化构造直接提供相同的 core 对象。Scene 接收 LinearAggregate，完成以下步骤：

1. 检查采样目标，并遍历输入对象层次。
2. 遇到 Primitive 时检查材质家族；介质只记录 containsMedia。
3. 对表面材质检查 `isEmissive()`，收集发光放置。
4. 递归进入 LinearAggregate、BVH 和 Instance，避免遗漏下层发光面。
5. 将发光放置与非发光部分组成最终 world，再构建顶层 BVH。
6. 对可采样发光面创建 AreaLight，并按非黑背景创建环境光。

共享的非发光子树保持共享。发光源经过多个 Instance 放置时，各放置获得独立 Primitive 身份，但共享 Shape 和 Material。AreaLight 指向最终 world 中的那个 Primitive，使光源采样和实际命中能对应。

`lights_` 拥有光源对象；`primitive_to_arealights_` 是借用指针索引。`findAreaLights()` 在非 delta 路径命中发光面、需要 MIS 概率时使用，不创建第二份光源。
LightSampler 在 Renderer 中构造，借用该 Scene 的光源集合，只负责选灯与选择概率。
Scene 不保存 LightSampler，也没有额外的 LightSet 包装。

`sampling.targets` 是 Mixture 的方向提议集合，可以包含非发光表面。它与 Scene 自动发现的 lights 是两个用途，不应混同。没有目标时，Mixture 直接采用散射采样。

## 6. Light 与统一积分器

`sampleLi(reference, time, sampler)` 从受光点朝向灯采样方向；返回的 incoming 是“受光点到灯”。返回 radiance 沿反方向传播到受光点，可见性由积分器处理。

reference 接受统一 HitRecord，所以表面和介质都能使用同一个 Light 接口。AreaLight 的发光端仍是 SurfaceRecord；环境光不依赖 Primitive。本轮没有体积发光。

PathIntegrator 的一次循环：

1. 查询最近事件；miss 时累计环境光。
2. 表面事件先累计材质发光。
3. 检查散射深度，通过 Material 统一入口构造当前点的 ScatteringResponse。
4. 按策略采样并更新路径权重。
5. 生成下一条射线，统一处理深度、RR、计数和贡献分类。

| CLI sampling | 做法 | 本轮介质支持 |
|---|---|---|
| scattering | 只从 BSDF／相函数采样继续方向 | 支持 |
| mixture（默认） | 有目标时，以各半概率选择散射或目标方向提议 | 支持 |
| light | NEE 估计非 delta 直接光，散射采样继续路径 | 不支持 |
| mis | NEE 与继续路径命中光源按概率加权 | 不支持 |

Mixture 使用完整混合 PDF。纯 delta 响应跳过连续目标提议；同时有连续与 delta 分量时，离散样本仍计入外层策略选择概率。
策略选择直接在积分器中完成，不依赖普通 MixturePDF 识别组件。
Mixture 不额外执行 NEE，以免重复计算贡献。

介质场景的 LightOnly／MIS 在像素循环前明确报错，因为连接段的体积透射率估计尚未实现。不可采样发光面和无法检查的自定义 Intersectable 也只在 NEE 模式下被拒绝；散射采样仍可正常处理它们。

## 7. 配置与使用

最大深度只有 `RenderSettings::path.max_depth` 一个来源，JSON `render.max_depth` 和 CLI `--max-depth` 都写入它。

```cpp
RenderSettings settings = scene.defaultRenderSettings();
settings.path.sampling = PathSamplingMode::Mixture;
settings.path.max_depth = 16;
settings.aovs = FilmAOV::All;
RenderResult result = Renderer(settings).render(scene);
const IntegratorCounters& counters = result.stats.counters;
```

RenderSettings 统一保存本次任务的图像尺寸、AOV、采样数、种子和 path 参数。
PathSamplingMode 与 PathIntegratorOptions 同样在 `render_settings.h` 中声明；算法参数保留 `settings.path` 分组。
默认 AOV 为 beauty；启用其他图层不改变路径采样配置。
RenderContext 借用 Scene、最终设置、准备好的相机和 LightSampler；它没有第二份 Scene 或配置。
Renderer 不隐式读取 Scene 默认值，调用方按上例显式传入。
RenderStats 保存 Renderer 的计时，并组合 IntegratorCounters；所有光线、事件和终止计数通过 `stats.counters` 读取。

JSON 的标准介质配置：

```json
{
  "materials": {
    "fog": {
      "type": "homogeneous",
      "extinction": 0.2,
      "albedo": [0.2, 0.4, 0.9],
      "phase": { "type": "isotropic" }
    }
  },
  "objects": [
    {
      "id": "fog_ball",
      "geometry": { "type": "sphere", "center": [0, 0, 0], "radius": 1 },
      "material": "fog"
    }
  ]
}
```

phase 可省略，默认 Isotropic；未实现类型和无效参数会报错。介质反照率也可用 texture 引用。旧对象级 `medium:{density, albedo}` 只在 Loader 中转换为相同的 Homogeneous Primitive，不需要虚构表面材质，且不能与显式介质材质同时出现。

## 8. 扩展边界与阅读顺序

建议按 `hit_record` → `hit_method` → `primitive` → `instance` → 两类 Material → `path_integrator` → `scene` / `light` 阅读。

后续方向采样、光源选择与 ReSTIR 实验应进入相应采样或积分器层；HitMethod 只定位事件。带额外距离采样权重的算法、异质介质、体积 NEE 和次表面散射仍需自己的输运契约，不能仅替换 bool hit 算法就认为已经支持。
