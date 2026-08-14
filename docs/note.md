# Self-Note

每个像素样本的采样过程

```
  Camera 生成主光线
      |
  world.hit() 查找最近交点
      |
  读取材质发光信息 material.emitted()
      |
  获取材质散射信息 material.scatter()
      |
      +-- Metal/Dielectric： 直接生成确定或近似确定的方向
      |
      `-- Lambertian/Isotropic： 返回材质 PDF
              |
              +-- 无 sampling targets： 按材质 PDF 生成方向
              |
              `-- 有 sampling targets：
                      50% Surface target PDF + 50% Material PDF
                              |
                      生成一条次级光线
                              |
                      递归 trace()
```

## 1. 向量计算

### 反射向量计算

略

### 折射向量计算

入射光线单位向量$\vec{R}$ ， 入射折射率$\eta$ ，法线$\vec{n}$ ，出射折射率 $\eta '$ ：求出射光线单位向量$\vec{R'}$

<img src="./figs/note/refract_vector.jpg" alt="refract_vector" style="zoom:50%;" />

### 法线的协变变换

法线不是普通的方向向量，而是作用在切向量上的协向量。设正向线性
变换为 $A$，切向量和法线满足：

$$
\boldsymbol{n}^{\mathsf T}\boldsymbol{v}=0
$$

切向量变换为 $\boldsymbol{v}'=A\boldsymbol{v}$。为了保持正交关系，
变换后的法线应满足：

$$
\boldsymbol{n}'=A^{-\mathsf T}\boldsymbol{n}
$$

因此同一个正向空间变换对不同几何量的作用不同：

- 点：使用完整仿射矩阵；
- 向量：使用线性部分 $A$；
- 法线：使用逆转置 $A^{-\mathsf T}$，再归一化。

逆转置只保证法线仍垂直于变换后的切平面。若 $\det(A)<0$，表面
定向还会反转；是否翻转法线取决于代码采用的有向表面语义。

## 2. Schlick's Approximation

对菲涅尔反射系数的近似。菲尼尔反射系数指，光从某介质进入另一介质时光被反射的比率。

完整的菲涅尔方程和反射系数公式

- 入射光的senkrecht偏振反射率：$R_s(\theta_i, \theta_t) = (\frac{\eta_1\cos\theta_i-\eta_2\cos\theta_t}{\eta_1\cos\theta_i+\eta_2\cos\theta_t})^2$   //垂直偏振
- 入射光的parallel偏振反射率：$R_p(\theta_i, \theta_t) = (\frac{\eta_1\cos\theta_t-\eta_2\cos\theta_i}{\eta_1\cos\theta_t+\eta_2\cos\theta_i})^2$   // 平行偏振
  - $\theta_i$为入射角，$\theta_t$为折射后出射角
  - $\eta_1, \eta_2$分别为入射和出射介质的折射率
- 对于无偏的光，可认为两种偏振等量，则有 $R = \frac{R_s+R_p}{2}$

近似后反射率公式为 $R(\theta_i) = R_0 + (1 - R_0)(1-\cos\theta_i)^5$

- $R(\theta_i)$ 是入射角为 $\theta_i$ 的反射率
- $R_0$ 是法向入射时的反射率，计算公式为 $R_0 = (\frac{\eta_1-\eta_2}{\eta_1+\eta_2})^2$

https://sparkfengbo.github.io/post/gl-fei-nie-er-fang-cheng-he-schlick-jin-si/

Brewster's angle 布儒斯特角：起偏振角，反射光与折射光分为互相垂直的线偏振光

- 此时反射光和折射光成90°垂直，所以有
  - $\eta_1\sin\theta_i = \eta_2\sin\theta_t$，$\theta_i + \theta_t = \pi - \frac{\pi}{2} = \frac{\pi}{2}$ $\implies$ $\eta_1\sin\theta_i = \eta_2\sin(\frac{\pi}{2}-\theta_i) = \eta_2\cos\theta_i$
  - 从而有 $\theta_B = \theta_i = \arctan(\frac{\eta_2}{\eta_1})$
- 在菲涅尔方程中，此时的平行偏振反射率为0，即$R_p = 0$，所有平行偏振光都透射进入另一介质

## 3. Hollow Glass Sphere的渲染

似乎和书上不一样,难道是全反射的问题,还是之前哪里自由发挥的锅? (递归深度50, 采样500, 小电脑渲染了好久,差不多一行像素好几秒)

<img src="./figs/hollow_glass_diff.png" alt="refract_vector" style="zoom:100%;" />

深度和采样小的话,玻璃上会有很多黑点(这也是之前深度递归基忘写后测出depth爆栈的原因)

破案了：球体和光线的二元一次方程解的有问题，我说怎么之前渲染图就有些许的不同；倒退到单个玻璃球去比对然后一个一个怀疑的点注释修改查出来的

- 显然不是原则性问题，都是自以为是的优化，结果没考虑到极端浮点数运算带来的误差；具体见[hit-calculation](./hit-calculation.md)的开头

## 4. BVH加速效率不佳

### bug 1: 由于 `bbox_cmp`引用悬垂导致的比较错误

通过将 `getBoundingBox`的返回值改为 `const AABB&`来保证引用对象存在，同时也减少拷贝次数（倒是不知道编译器会不会优化）

### bug 2: 计算Node中AABB合并时莫名出现的某轴边界归零

问题出在 `AABB::empty`和 `AABB::universe`的初始化上；由于写成了多文件应用，所以在给AABB这两个静态变量赋值时，可能Interval的静态变量尚未初始化（两个.o呢），所以就被初始化为零了。

如果全部为头文件的话到还可以，多文件这种有依赖的静态变量，还是写成静态成员函数，等待函数别调用时，依赖项必然已经完成初始化

```cpp
const AABB& AABB::empty() {
    static AABB instance(Interval::empty, Interval::empty, Interval::empty);
    return instance;
}
const AABB& AABB::universe() {
    static AABB instance(Interval::universe, Interval::universe, Interval::universe);
    return instance;
}
```

## 5. 纹理映射

### Sphere纹理坐标

对单位球面上的点$(x, y, z)$进行映射，最终得到$(u,v) \in [0, 1]$。

即用球坐标表示，正好两个参数$(\theta, \phi)$，然后再将参数归一化得到纹理坐标；此处球坐标稍微与标准球坐标系有所不同，$\phi$ 为与-x轴的夹角，使得值均大于0，详见下图

<img src="./figs/note/sphere-uvmap.png" alt="sphere_coordinate" style="zoom:80%;" />
$$
(x, y, z) \xrightarrow{(1)} (\theta, \phi) \xrightarrow{(2)} (u, v)
$$

其中有

$$
(1) 
\left\{ \begin{aligned} y &= -\cos(\theta) \\ x &= -\sin(\theta)\cos(\phi) \\ z& = \sin(\theta)\sin(\phi) \end{aligned} \right.
    \implies
\left\{ \begin{aligned} \theta &= \arccos(-y) \\ \phi &= \arctan(-\frac{z}{x}) + \pi \end{aligned} \right.

\ \ \ 

(2)
\left\{
\begin{aligned}
u &= \frac{\phi}{2\pi} \\
v &= \frac{\theta}{\pi}
\end{aligned}
\right.
$$

关于第一个变换中的$\phi$，原本应该是$\phi = \arctan(-\frac{z}{x})$，不过对应函数 `atan2(z,-x)`的取值范围是$0 \to \pi, -\pi \to 0$，即$[-\pi, \pi]$，并不是我们想要的$[0, 2\pi]$ ...

啧，那其实还不如最原始的球坐标，然后得到这个$\phi' \in [-\pi, \pi]$，然后再做个偏移后归一化呢。反正相当于 $\phi = \phi' + \pi$，而$\phi' = \arctan(-\frac{z}{x})$，则$\phi = \arctan(-\frac{z}{x}) + \pi$

## 6. 插值

插值函数：多项式插值，分段插值，三角插值；证明n+1个节点确定n阶多项式插值函数：即x_i构成范德蒙德矩阵。

- 拉格朗日插值，牛顿插值。不过全面反映被插值函数的性态，存在龙格现象(两端震荡)
- 分段二次插值

Hermite插值：节点的函数值和n阶导数值都需要相同。直接Hermite插值得到的多项式次数高，也存在龙格现象。实际运用中常用分段三次Hermite插值多项式PCHIP

### 三线性插值

就是两层双线性插值再插个值

![bilinear-interpolation](./figs/note/bilinear_interp.png)

### Hermitian平滑

只用三线性插值的话，结果还是有明显的网格特征，且存在Mach Bands马赫带。因而对u, v, w进行一个三次Hermite插值，似乎就是GLSL中smoothstep的插值。smoothstep可以用来生成0到1的平滑过渡值，称为平滑梯度函数，由分段三次Hermite插值公式推导而来

$$
\textrm{smoothstep}(t) = t^2\cdot(3-2t) = -2t^3 + 3t^2
$$

本质是针对$P_0$和$P_1$点进行三次Hermite插值： $P(t) = (2t^3 - 3t^2 + 1)P_0 + (t^3 - 2t^2 + t)M_0 + (t^3 - t^2)M_1 + (-2t^3 + 3t^2)P_1$。
其中$M_0$和$M_1$是两点处的方向，也即导数，应该均为0。
这里我们已经将u,v,w控制在了$[0，1]$之间，所以只需在0~1之间做平滑，即这里设置$P_0 = (0, 0), P_1 = (1, 1)$，而方向$\vec{M_0}$和$\vec{M_1}$则都为$(0,1)$横向，即两侧区间外绝对平滑。“平滑梯度”的名称很形象

![smoothstep](./figs/note/smoothstep.png)

再细点，其实上述公式是个x,y关于t的参数方程

$$
\newcommand{\matrix}[1]{\left[ \begin{matrix} #1 \end{matrix} \right]}

\matrix{x\\y} = (2t^3 - 3t^2 + 1) \matrix{x_0\\y_0} + (t^3 - 2t^2 + t) \matrix{a_0\\b_0} + (t^3 - t^2) \matrix{a_1\\b_1} + (-2t^3 + 3t^2) \matrix{x_1\\y_1}  \\
= (2t^3 - 3t^2 + 1) \matrix{0\\0} + (t^3 - 2t^2 + t) \matrix{1\\0} + (t^3 - t^2) \matrix{1\\0} + (-2t^3 + 3t^2) \matrix{1\\1} \\
= \matrix{t \\ -2t^3 + 3t^2}
$$

所以有 $y = -2t^3 + 3t^2 = -2x^3 + 3x^2$ 这一简化的公式

http://www.cnitblog.com/luckydmz/archive/2014/06/23/89615.html
https://zhuanlan.zhihu.com/p/157758600

## 7. Perlin噪声改进

在基本的噪声算法中，每个网格点通常会被分配一个随机浮点数值。然后，在这些网格点之间使用插值（如三次插值）进行平滑。但是，如果这些浮点数直接作为噪声值，那么它们的最小值和最大值会总是刚好出现在整数的 x/y/z 位置上。

- 噪声图案的极值总是发生在固定的整数网格上，可能导致可见的规则性。
- 过渡不够自然，最终会看起来有点“块状”（Blocky）

改进：不是在网格点上放置随机浮点数，而是放置随机单位向量。这样之后使用了点积运算，极值会偏移，打破了规则性；相比直接使用浮点数，这种方法能消除明显的网格结构，使噪声更加平滑、细腻；由于单位向量可以朝向任意方向，噪声图案不再表现出轴对齐的特征，避免了规则的条纹或格子状伪影

## 8. 渲染时间

Cornell Box (SPP = 200, depth = 50, 400px * 400px, WSL)

- 带Transform： 632633ms = 632.633s = 10min
- smoke：840265ms = 840.265s = 14min

stage2-achievement

- SPP = 100, depth = 25, 400px * 400px, WSL：431556ms = 431.556s = 7min
- SPP = 250, depth = 25, 400px * 400px, WSL：1053010ms = 1053.010s = 17.5min = 7min * 2.5

## 9. 分层采样 Stratified Sampling

stratified sampling：对于指定的pixel，原采样是随机投射spp次光线；分层采样是再将这个pixel划分为spp个格子，随机投射将均匀分布于每个格子中

渲染时间还稍快。对于spp = 64，depth=50的Cornell Box，原采样用时203188ms，分层采样用时191859ms

## 10. 蒙特卡洛积分 Monte Carlo Integration

### 总结

对于 $I = \int_a^b f(x) dx$ 进行Monte Carlo积分

- 采样用的随机变量 $X \sim f_X(x), x \in [a, b]$ ，进行N次独立同分布采样，即得 $X_1, X_2, ..., X_N$ ，样本值为  $x_1, x_2, ..., x_N$ .
  - $f_X(x)$ 即为随机变量X的概率密度函数pdf，离散后则为 $P\{X = x\}$ ，书中记法是 $p(x)$ ，我觉得很难看，还是希望记成 $\mathrm{pdf}(x)$ . 
- 则积分结果为  $I = \frac{1}{N} \sum\limits_{k=1}^{n} \frac{f(x_k)}{f_X(x_k)}$ 即 $\frac{f(x)}{f_X(x)}$的均值

### 一些技巧

- 简化表示 $f_X(x) = \frac{x}{2}$ ：可知 $P\{ X \leq \sqrt{2} \} = 0.5$ 所以分两段 $[0, \sqrt{2}]$ 和 $[\sqrt{2}, 2]$ ，每段再用均匀分布简化模拟
- 对于任意曲线pdf，求**cdf半分位点**：假设采样N个点，然后按x升序排列，之后从前向后加 $f_X(x)$ (即**前缀和**)直到超过0.5，此时的x就是所求分位点
  - 可以多次求半分位点，递归二分，复杂度上限即排序O(nlogn)：即分治策略产生非均匀分布
  - 更高效：Metropolis-Hastings
- Metropolis-Hastings Algorithm：
- 确定cdf：产生近似分布去靠近真正的cdf
  - 有 $y = F_X(x)$ ,则有 $x = F_X^{-1}(y)$ ；CDF的逆称为ICD
  - 随机均匀采样y，可得多组x = icd(y)去，这样的(x,y)即可近似真正的cdf：**怎么感觉就是在说废话** d

### 实现举例

举例泛化说明，计算最开始的$I$

```cpp
/* 选择的分布 */
float pdf(float x) { return ...; }
float icd(float y) { return ...; }  // y in [0, 1]

/* 被积函数 */
float f(float x) { return ...; }

/* Monte Carlo Intergration */
void main() {
    int N = xxxx;  // 采样数量
    auto res = .0; // 积分结果
    for (int i = 0; i < N; ++i) {
        auto x = icd(random_float());  // y是均匀分布，则x = icd(y)就满足cdf表示的分布
        sum += f(x) / pdf(x);
    }
    res = sum / N;
}
```

- 若用的是[a,b]的均匀分布，则有 `pdf(x) = 1 / (b - a)`，从而 `cdf(x) = x / (b - a)` ，从而 `icd(y) = y * (b - a)`
- 自定义分布：会引导样本分布在pdf大的地方，可在噪声大的场景增加pdf降噪，噪声小的地方减少pdf提高性能；好“样”用在刀刃上
  - 这样总比均匀分布更快速的收敛
  - 称为importance sampling
  - the perfect importance sampling：对于最合适的pdf，则采样数仅需1，当然本身就是答案了
    - 升维后也一样，总能找到**最正确的一个点**，配合pdf进行权重操纵

### 单位球面上的MC积分

前提：随机方向

- 产生1维样本：之前的方法，即使用了指定cdf的反函数，因而称为反演法 inversion method
- 产生2维样本：即**单位球上的随机方向**
  - 随机方向 $\iff$ 球面上随机一点(3维样本约束在2维)
  - 若沿球面均匀分布，即可用之前实现**生成随机方向的方法**：空间中均匀随机生成，然后去除球外部分，剩下的在归一化，称为拒绝法 rejection method

e.g. 如果用MC法求 $\iint_{\Omega} \cos^2(\theta) d\vec{r}$ ，这是个曲面积分：采样沿球面均匀分布，则需要被采样均值的是 $\frac{f(\theta, \phi)}{p(\vec{r})} = \frac{f(\theta,\phi)}{p(d(\theta,\phi))}$

- 对于采样分布，则有 $p(\vec{r}) = p(\vec{d}) = \frac{1}{4\pi}$ ， $\vec{d}$ 表示单位球上随机方向
- 对于被积函数，可以发现 $f(\theta, \phi) = \cos^2(\theta) = d_z^2$ ，其中记 $\theta$ 为与z轴的夹角，0到 $\pi$ 范围
- 所以MC法可写为如下代码

  ```cpp
  float f(const vec3& d) { return d.z()*d.z(); }
  float pdf(const vec3& d) { return 1 / (4*pi); }
  
  void main() {
      int N = 1000000;
      auto res = 0.0;
      for (int i = 0; i < N; i++) {
          vec3 d = random_unit_vector();  // 可以发现二维变量可划归到d上，对应之前的x
          auto f_d = f(d);
          res += f_d / pdf(d);
      }
      res = res / N;
  }
  ```

一些定义

- 在3d空间中表示方向：单位球上的点
- 在3d空间中表示方向范围：方向区域，即空间角
  - 1d角度 - θ, 2d及以上角度 - sr

## 11. 渲染模型更换：渲染方程

### 散射概率模型

(1) 反照率 albedo ：重新定义为 被散射的概率，不被散射scattered即被吸收asorbed

从光子的角度思考：RGB $\to$ 光子波长，如300nm , 350nm, 400nm, ... 700nm ：可以认为RGB是特殊波长的代数线性混合

- 近似的自然解释：人类视觉系统中3组特殊的色彩视锥细胞cones，敏感的波长即类似RGB，称为long / medium / short cones，这是根据敏感波长命名的
- 颜色可以表示为 在L/M/S 空间中激发对应视锥夕宝的程度

(2) 散射方向分布 pScatter：定义为散射光线在**立体角**上分布的**概率密度函数** ，书上称为**散射PDF** 

- 这个函数可能与出射方向、入射方向、光的波长(e.g. 彩虹)、散射位置有关，可记为$pScatter(\vec{x}, \omega_i, \omega_o, \lambda)$ .
  - 书中例子显示，这里入射角度incident angle指的是viewing angle，也就是ray tracing方向的入射角度，即真实光线传播的出射角度（代码中主要递归函数 `getRayColor` 中也是这么认为的，`ray_in` 是从视线侧来的，`scatter` 光线是另一侧，希望打到光源），这和后文理论上用蒙特卡洛积分描述又是反的
  - **albedo的因变量也是这些**，可以记为 $A(\vec{x}, \omega_i, \omega_o, \lambda)$ .
- 对于Lambertian，pScatter仅与出射角$\theta_o$ 有关，$pScatter(\vec{x}, \omega_i, \omega_o, \lambda) = C \cdot \cos(\theta_o)$ .

(3) 最终得到平面上某点的颜色：将该点单位半球面上所有入射方向的值积起来，即沿立体角积分

- 公式为 $Color_o(\vec{x}, \omega_o, \lambda) = \iint_{H^2} A(\vec{x}, \omega_i, \omega_o, \lambda) \cdot pScatter(\vec{x}, \omega_i, \omega_o, \lambda) \cdot Color_i(\vec{x}, \omega_i, \lambda) d\omega_i$ .
  - 【实际情况】往眼睛那儿是出射，从环境和光源那儿是入射
  - $d\omega$ 是立体角的微分，有公式定义 $d\omega = \frac{dA}{r^2} = \sin\theta d\theta d\phi$ , 其中 $dA$ 是球面上面积微元
    - 书上似乎默认单位球， **将立体角记作了dA，我觉得很不合适** .
  - 写成蒙特卡洛积分形式： $Color_o(\vec{x},\omega_o,\lambda) = \sum{\frac{A()\cdot pScatter() \cdot Color_i()}{p(\vec{x},\omega_i,\omega_o, \lambda)}}$ .
- Color即是递归得出的，即类似 `getRayColor()` 函数

(4) BRDF与书中pScatter的关系

BRDF的定义

- 辐射度量学
  - 能量 $Q$ ，单位 - 焦耳J
  - **辐射通量** $\Phi$ , Radiant Flux :  $\Phi = \frac{dQ}{dt}\ (\textrm{W})$ ，**单位时间**穿过截面的光能
    - 又称光通量，单位又记作 lm，流明
  - 辐射强度 $I$ , Radiant Intensity : $I = \frac{d\Phi}{d\omega}\ (\textrm{W}/\textrm{sr})$ ，单位**立体角**的辐射通量
    - 又称发光强度，单位又记作 lm/sr = cd，坎德拉
  - 辐照度 $E$ , Irradiance : $E = \frac{d\Phi}{dA} = \iint_H L(\omega) \cos \theta d\omega\ (\textrm{W}/\textrm{m}^2)$ ，单位**面积**的辐射通量
    - 又称辉度，单位又记作 lm/m2 = lux，勒克斯
  - 辐射率 $L$ , Radiance : $L = \frac{d^2\Phi}{dA \cdot \cos\theta \cdot d\omega}\ (\textrm{W}/\textrm{sr}\cdot \textrm{m}^2)$ ，单位**投影面积**和单位**立体角**的辐射通量
    - 又称光亮度 Luminance，单位又记作 lm/sr m2 = cd/m2 = nit，尼特
    - <font color="red">注意</font> ：Irradiance中用的就是寻常面积，而Radiance中关注实际投影垂直的距离，即 $dA\cdot \cos{\theta} = dA_{\perp}$ .
- 双向反射分布函数：描述表面如何反射光线，即**从某方向入射**然后**反射到各个方向**的**能量分布**
  - 定义为**反射辐射率**和**入射幅照度**的比值： $f(\omega_i, \omega_o) = \frac{dL_o(\omega_o)}{dE_i(\omega_i)} = \frac{dL_o(\omega_o)}{L_i(\omega_i)\cos\theta_i d\omega_i}$ .
    -  $\omega_i, \omega_o$ 都是从反射点出发，指向入射方向和出射方向的单位方向向量
    -  ！对应到 **光追实现中** ：o是从视线侧trace过来的，无法变更；i是scatter出的光线，可调整，越发往有效光源处，渲染收敛速度越快
    -   $d\omega$ 方向向量的微分即立体角微分 $\to$ 似乎很不严谨
  -  <img src="./figs/note/brdf-model.png" alt="brdf-model" style="zoom:70%;" /> 
  - 从不同方向入射的光，都可以反射到指定出射方向，因此出射部分用指定方向的辐射率，入射部分用不指定方向的辐照度
  - **出射的角标**：这里是遵循书中的写法用了 $L_o$ ，但这其实只是**反射**，用 $L_r$ 才合适
  - 其实BRDF都暗含了针对某个反射点/反射平面，所以其实因变量还包含 位置 $\vec{x}$ ，即 $f(\vec{x}, \omega_i, \omega_o)$ 
- 渲染方程
  - 即 出射光 = 自发光 + 反射光
  -   $L_o(\vec{x}, \omega_o) = L_e(\vec{x}, \omega_o) + L_r(\vec{x}, \omega_o) = L_e(\vec{x}, \omega_o) + \iint_H f(\vec{x}, \omega_i, \omega_o) \cdot L_i(\vec{x}, \omega_i) \cos\theta_i d\omega_i$ 
  - 其中 $\cos\theta_i = (\vec{n} \cdot \omega)$ 即点积形式 

书中两者关系

- 书中给出 $f(\vec{x}, \omega_i, \omega_o, \lambda) = \frac{A(\vec{x}, \omega_i, \omega_o, \lambda) \cdot pScatter(\vec{x}, \omega_i, \omega_o, \lambda)}{\cos\theta_o}$ ，则有 $pScatter(\vec{x}, \omega_i, \omega_o, \lambda) = \frac{f(\vec{x}, \omega_i, \omega_o, \lambda)\cdot\cos\theta_o}{A(\vec{x}, \omega_i, \omega_o, \lambda)}$ . 【？】
  - 基本确定了：这里书上的 $\cos\theta_o$ 就是前面描述的 $\cos\theta_i$ ，所以换算应该写为 $pScatter(\vec{x}, \omega_i, \omega_o, \lambda) = \frac{f(\vec{x}, \omega_i, \omega_o, \lambda)\cdot\cos\theta_i}{A(\vec{x}, \omega_i, \omega_o, \lambda)}$
  - pScatter - 散射方向的分布函数(PDF)， 仅包含**方向**
  - BRDF - 散射方向的**能量**分布函数(PDF)，除了方向外还包含能量/颜色
    - 给BRDF增加了一个光波长的因变量，合理
- pScatter去除了BRDF中的颜色值反照度A，又将其出射的“**不仅关于立体角还相对于投影平面**的Radiance”转换为“**绝对的只关乎立体角的**辐射亮度Radiant Intensity”


### 具体的散射PDF

| 材质           | 散射PDF                    | BRDF            | 解释                                                         |
| -------------- | -------------------------- | --------------- | ------------------------------------------------------------ |
| Lambertian     | $\frac{\cos\theta_o}{\pi}$ | $\frac{A}{\pi}$ | 散射分布见Lambertian散射图<br />仅与出射角有关 $C\cdot\cos(\theta_o)$ <br />同时下半平面不出射为0，因而pdf积半球面为1 <br /> $\iint_{H^2} C \cdot \cos\theta d\omega = 1$ , 根据球面微分$d\omega = \sin\theta d\theta d\phi$ <br />则有$C\int_0^{2\pi}\int_{0}^{\frac{\pi}{2}}\cos\theta\sin\theta d\theta d\phi = 1$,解得 $C = \frac{1}{\pi}$ |
| Normal Diffuse | $\frac{1}{2\pi}$           |                 | 散射在半球面上均匀分布                                       |

关于**立体角/单位球面的微分** $d\omega = \sin\theta d\theta d\phi$ 的推导（回顾一型曲面积分）

- 这本书的定义： $\theta$ 为向量(x, y, z)与z轴的夹角， $\phi$ 为向量投影到xoy平面后与x轴的夹角
  - 这与传统数学书上符号选择**相反**
  - 此时坐标系摆放可仍按传统数学书上的来：意识到两者其实**都是右手系**，只不过摆放位置不同
  - 此时有 **笛卡尔坐标系** 与 **球坐标系** 变换关系为 $\left\{ \begin{aligned} x =& r\sin\theta\cos\phi \\y =& r\sin\theta\cos\phi \\ z =& r\cos\theta \end{aligned} \right.$ , $\theta\in[0,\pi], \phi\in[0,2\pi]$ .
  
- 稍微不严谨的**微元法** ：$dA = (rd\theta)(r\sin\theta d\phi) = r^2\sin\theta d\theta d\phi$ 
  
   <img src="./figs/note/solid-angle.jpg" alt="solid-angle-view" style="zoom:60%;" /> 

再看 **Lambertian 反射** 

 <img src="./figs/note/Lambert6.gif" alt="lambertian" style="zoom:50%;" /> 

- 光线入射材质后，从**不同角度**观察的反射光线： **Radiant Intensity** 值（单位立体角）如图成一个圆，即 $C\cdot\cos\theta$ ，而 **Radiance** 值（单位立体角单位投影面积）则为常数。

## 12. 渲染模型的计算

### MC法采样PDF选择 - Importance Sampling

已知：采样PDF与待采样函数（真实光线情况）越接近，效果越好收敛速度越快

从实际光线情况出发，构造PDF

- 光线集中向光源的PDF：记为pLight
- 光线从表面反射的PDF：记为pSurface

可以合成某个不错的采样PDF，如 $p(\omega_o) = \lambda\cdot pSurface(\omega_o) + (1-\lambda)\cdot pLight(\omega_o)$ ，取 $\lambda = 0.5$ .

- 最终级的目标是让pdf和最终正确的颜色 $pScatter() \cdot Color_i()$ 相近
- 对于漫反射材质，Color比较重要，即有效光线来源比较重要（pLight权重大些？）；对于镜面材质，pScatter比较重要，即要看观察方向o在不在散射对的位置（pSurface权重大些）

## 优化前记录

WSL2 - Ubuntu 24.04 - 9955HX - Single Core : SPP = 100, max_depth = 25, 600x600, Render Time = 10m13s

WSL2 - Ubuntu 24.04 - 9955HX - Single Core : SPP = 64, max_depth = 25, 400x400, Render Time = 2m54s
WSL2 - Ubuntu 24.04 - 9955HX - Single Core : SPP = 1000, max_depth = 25, 400x400, Render Time = 23m26s

## 13. 随机方向生成

### Rejection Method

不必提，不可定制采样概率

### Inversion Method

简化版：将z轴视为表面法线，**生成绕z轴对称的随机方向**。则只与法线的夹角 $\theta$ 有关，所以有球面分布 $p_\Omega(\omega) = f(\theta)$。$\omega$ 是方向随机变量，在球面坐标系中可表示为 $(\sin\theta\cos\phi, \sin\theta\sin\phi, \cos\theta)$，$d\omega$ 在整个球面上的积分为 $4\pi$。
- **需满足** $\iint_{S^2}p_\Omega(\omega)d\omega = \int_0^{2\pi}\int_{0}^{\pi}f(\theta)\sin\theta d\theta d\phi = 1$，即有联合概率密度函数 $p_{\Theta,\Phi}(\theta, \phi) = f(\theta)\sin\theta$。
- 转化到两个参数的一维分布
  - $\phi$：分布**在绕z轴的方向上均匀分布**。说明 $\phi$ 在 $[0, 2\pi)$ 上有均匀分布，概率密度为 $p_\Phi(\phi) = \frac{1}{2\pi}$。
  - $\theta$：分布**与z轴的夹角由函数指定**。从联合概率密度求边缘概率密度，可得 $p_\Theta(\theta) = \int_0^{2\pi}p_{\Theta,\Phi}(\theta, \phi)d\phi = 2\pi f(\theta)\sin\theta$；也可利用 $\theta$ 与 $\phi$ 独立，由 $p_{\Theta,\Phi}(\theta,\phi) = p_\Theta(\theta)p_\Phi(\phi)$ 求得。
- 分别对累积分布函数求逆，将均匀随机数变换为满足目标分布的随机变量
  - $\phi$：$F_\Phi(\phi) = \int_0^{\phi}p_\Phi(t)dt = \frac{\phi}{2\pi} = r_1$，所以 $\phi = F_\Phi^{-1}(r_1) = 2\pi r_1$。
  - $\theta$：$F_\Theta(\theta) = \int_0^{\theta}p_\Theta(t)dt = 2\pi\int_0^{\theta}f(t)\sin t\,dt$。
     - 均匀采样整个球面：$p_\Omega(\omega) = \frac{1}{4\pi}$，$F_\Theta(\theta)=\frac{1-\cos\theta}{2} = r_2$，所以 $\cos\theta = 1 - 2r_2$。
     - 均匀采样半球面：$p_\Omega(\omega) = \frac{1}{2\pi}$，$F_\Theta(\theta) = 1-\cos\theta = r_2$，所以 $\cos\theta = 1-r_2$。
     - 余弦采样半球面：$p_\Omega(\omega) = \frac{\cos\theta}{\pi}$，$F_\Theta(\theta) = 1-\cos^2\theta = r_2$，所以 $\cos\theta = \sqrt{1-r_2}$。
  -  以球面上均匀分布为例：将值代回球坐标 $(\sin\theta\cos\phi, \sin\theta\sin\phi, \cos\theta)$ 中即为 $(\sqrt{1-(1-2r_2)^2}\cos(2\pi r_1), \sqrt{1-(1-2r_2)^2}\sin(2\pi r_1), 1-2r_2)$ ，化简下即为 $(2\sqrt{r_2(1-r_2)}\cos(2\pi r_1), 2\sqrt{r_2(1-r_2)}\sin(2\pi r_1), 1-2r_2)$ 。这样就可以用两个均匀分布随机数，生成球面上均匀分布(定制分布)的方向样本了

| 采样方式       | $p_\Omega(\omega)$       | $F_\Theta(\theta)$       | x                                  | y                   | z              |
| -------------- | ------------------------ | ------------------------ | ---------------------------------- | ------------------- | -------------- |
| 均匀采样球面   | $\frac{1}{4\pi}$         | $\frac{1-\cos\theta}{2}$ | $2\sqrt{r_2(1-r_2)}\cos(2\pi r_1)$ | .. $\sin(2\pi r_1)$ | $1-2r_2$       |
| 均匀采样半球面 | $\frac{1}{2\pi}$         | $1-\cos\theta$           | $\sqrt{r_2(2-r_2)}\cos(2\pi r_1)$  | .. $\sin(2\pi r_1)$ | $1-r_2$        |
| 余弦采样半球面 | $\frac{\cos\theta}{\pi}$ | $1-\cos^2\theta$         | $\sqrt{r_2}\cos(2\pi r_1)$         | .. $\sin(2\pi r_1)$ | $\sqrt{1-r_2}$ |

完整版：对任意平面的法线生成。其实就是将法线方向当作'z'轴，再作一组标准正交基 Orthonormal Basis 

- 标准正交基的生成：已经有一个 $\vec{n}$ 了，需要再随便找一个与 $\vec{n}$ 不平行的 $\vec{a}$，然后就可用叉乘找了 $\vec{s} = \textrm{normalize}(\vec{n}\times\vec{a})$， $\vec{t} = \vec{n}\times\vec{s}$ . 这里的 $\vec{a}$ 我们就从标准的x,y,z基向量上依次找即可（检查 $\vec{n}$ 的值即可）
  - 这里视 n, s, t 为 z, y, x，那么t的算法就是左手系而不是右手系了呀，我觉得要反一反


## 14. 根据光源采样

对P点上散射的光线进行采样

- 指向光源的立体角微分满足 $d\omega = \frac{dA}{r^2} = \frac{dS \cdot \cos\theta}{\Vert PQ \Vert^2}$，其中 $S$ 为光源面积，$Q$ 为其上任意一点，面积微分为 $dS$，$dS\cdot \cos\theta$ 即为垂直于连线的投影；在灯面上均匀采样时，面积概率密度为 $p_Q(q) = \frac{1}{S}$。
- 从 P 点散射的光线打在 Q 点的情况满足 $p_\Omega(\omega)d\omega = p_Q(q)dS$，从而可解得 $p_\Omega(\omega) = \frac{\Vert PQ \Vert^2}{\cos\theta \cdot S}$。

直接把不被光源直接照到的地方给pass掉了，就少了间接光照，渲染出图也验证了这一点，无光处全黑

 ![sample_only_to_light](./figs/optim/cb_spp10_md25_400-sample_only_to_light.png) ![sample_only_to_light-2](./figs/optim/cb_spp10_md25_400-sample_only_to_light-correct.png) 

 
### 渲染问题

图中红色墙面的椭圆状明暗分布与第15节的共面求交问题不同。该场景的灯面位于 $y=554$，天花板位于 $y=555$，两者没有重合，因而不会发生 ceiling 与 light 在同一参数 $t$ 上竞争命中的问题。

- 有限矩形面光源在侧墙上的直接照明本来就随距离、入射余弦和灯面出射余弦连续变化，等照度线呈椭圆状是合理现象。
- 图像只有 `10 SPP`，且经过显示映射和8位量化，平滑梯度中的噪声与色阶容易表现为分层。红色材质的通道分布更集中，因此视觉上比绿色墙明显。
- 灯面低于完整天花板 `1` 个单位，虽然不会发生共面求交竞争，但改变了灯具边缘的可见关系。对于仅向下发光的灯面，顶部天花板不会直接接收其背面辐射，不能仅凭高度差把图2中的亮圈判定为正常照明。
- 将 Lambertian 的 `scattering_pdf` 强制改为 $1/\pi$ 会丢失余弦项，不能作为正确性修复。图2的亮圈还受到错误估计器和间接光照的影响，需要在恢复正确的 BRDF、余弦项与采样 PDF 后单独验证。

第15节的开孔方案会消除共面命中竞争，并改变灯具边缘的局部可见关系；但不会消除侧墙上由面光源几何造成的椭圆状照度分布，也不能单独证明图2的亮圈已经修复。若仍出现离散的硬色带，应恢复正确估计器、提高 SPP、使用浮点输出并检查 tone mapping，而不是继续移动灯面。

补充实验：仅保留红绿墙并交换颜色后，明显的分层仍跟随红色材质，进一步说明两侧差异主要来自颜色与显示过程，而不是左右墙的几何不对称。

## 15. Cornell Box共面灯具造成横向光带

### 现象

灯面和完整天花板都位于 $y=548.8$，灯具区域因而完全重合。渲染结果在后墙和天花板上出现了稳定的横向光带，而非普通的低 SPP 随机噪声。

| ceiling与light共面重叠 | light临时下移到$y=548.79$ |
| --- | --- |
| ![共面重叠产生横向光带](./figs/note/cornell-coplanar-overlap.png) | ![光源下移后横向光带消失](./figs/note/cornell-light-offset.png) |

### 原因

从表面采样灯具方向后，次级射线会在几乎相同的参数 $t$ 上同时命中白色 ceiling 和 emissive light。BVH 遍历顺序、严格的最近交点区间和浮点舍入共同决定最终保留哪个交点：

- 命中 light 时，路径取得发光贡献并终止。
- 命中 ceiling 时，路径被当成普通 Lambertian 表面继续递归。

相邻扫描行的射线方向和浮点行为相近，因此错误形成连贯光带。两次独立 seed 的逐行高频残差相关系数约为 `0.913`，也说明它属于固定的几何/求交偏差。

将灯具临时下移 `0.01` 后，条纹消失，曝光对齐 relative MSE 从约 `5.76%` 降至 `0.277%`。场景同时明显变亮，说明原设置中大量灯光路径被共面 ceiling 错误遮挡；这项位移实验只用于定位问题，不是最终建模方式。

### 解决

最终场景使用四个 Quad 拼接天花板并留出矩形灯具开口，再用一个发光 Quad 填充开口。ceiling 与 light 只共享边界，不再占据同一片面积：

- 几何语义与 Cornell Box 灯具开口一致。
- 避免共面交点竞争，不依赖人为高度偏移。
- 单个灯面可直接进行均匀面积采样，无需先选择两个 Triangle。

修复几何后，当前 RGB light radiance 仍会使原始结果偏亮。evaluation 同时保留原始指标和曝光对齐指标，并输出 `preview-exposure-aligned.ppm` 供直观比较；后续应根据官方辐射数据或明确的 RGB 近似重新确定绝对光强。

正式开孔场景的曝光对齐 relative MSE 为约 `0.277%`，PSNR 为约 `50.91 dB`，与临时下移光源的诊断结果一致。
