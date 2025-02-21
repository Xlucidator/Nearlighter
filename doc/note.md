## note

### 向量计算

#### 反射向量计算

#### 折射向量计算

入射光线单位向量$\vec{R}$ ， 入射折射率$\eta$ ，法线$\vec{n}$ ，出射折射率 $\eta '$ ：求出射光线单位向量$\vec{R'}$

<img src="./figs/refract_vector.jpg" alt="refract_vector" style="zoom:50%;" />

### Schlick's Approximation

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

### Hollow Glass Sphere的渲染

似乎和书上不一样,难道是全反射的问题,还是之前哪里自由发挥的锅? (递归深度50, 采样500, 小电脑渲染了好久,差不多一行像素好几秒)

<img src="./figs/hollow_glass_diff.png" alt="refract_vector" style="zoom:100%;" />

深度和采样小的话,玻璃上会有很多黑点(这也是之前深度递归基忘写后测出depth爆栈的原因)

破案了：球体和光线的二元一次方程解的有问题，我说怎么之前渲染图就有些许的不同；倒退到单个玻璃球去比对然后一个一个怀疑的点注释修改查出来的
- 显然不是原则性问题，都是自以为是的优化，结果没考虑到极端浮点数运算带来的误差；具体见[hit-calculation](./hit-calculation.md)的开头

### BVH加速效率不佳

#### bug 1: 由于`bbox_cmp`引用悬垂导致的比较错误

通过将`getBoundingBox`的返回值改为`const AABB&`来保证引用对象存在，同时也减少拷贝次数（倒是不知道编译器会不会优化）

#### bug 2: 计算Node中AABB合并时莫名出现的某轴边界归零