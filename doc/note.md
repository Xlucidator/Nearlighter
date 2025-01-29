## note



### 向量计算

#### 反射向量计算



#### 折射向量计算

入射光线单位向量$\vec{R}$ ， 入射折射率$\eta$ ，法线$\vec{n}$ ，出射折射率 $\eta '$ ：求出射光线单位向量$\vec{R'}$ 

 <img src="./refract_vector.jpg" alt="refract_vector" style="zoom:50%;" /> 



### Schlick's Approximation

对菲涅尔反射系数的近似。菲尼尔反射系数指，光从某介质进入另一介质时光被反射的比率。

完整的菲涅尔方程和反射系数公式

- 入射光的senkrecht偏振反射率：$R_s(\theta_i, \theta_t) = (\frac{\eta_1\cos\theta_i-\eta_2\cos\theta_t}{\eta_1\cos\theta_i+\eta_2\cos\theta_t})^2$   //垂直偏振
- 入射光的parallel偏振反射率：$R_p(\theta_i, \theta_t) = (\frac{\eta_1\cos\theta_t-\eta_2\cos\theta_i}{\eta_1\cos\theta_t+\eta_2\cos\theta_i})^2$   // 平行偏振
  -  $\theta_i$为入射角，$\theta_t$为折射后出射角
  -  $\eta_1, \eta_2$分别为入射和出射介质的折射率
- 对于无偏的光，可认为两种偏振等量，则有 $R = \frac{R_s+R_p}{2}$ 

近似后反射率公式为 $R(\theta_i) = R_0 + (1 - R_0)(1-\cos\theta_i)^5$ 

-  $R(\theta_i)$ 是入射角为 $\theta_i$ 的反射率
-  $R_0$ 是法向入射时的反射率，计算公式为 $R_0 = (\frac{\eta_1-\eta_2}{\eta_1+\eta_2})^2$ 

https://sparkfengbo.github.io/post/gl-fei-nie-er-fang-cheng-he-schlick-jin-si/

Brewster's angle 布儒斯特角：起偏振角，反射光与折射光分为互相垂直的线偏振光

- 此时反射光和折射光成90°垂直，所以有 
  -  $\eta_1\sin\theta_i = \eta_2\sin\theta_t$，$\theta_i + \theta_t = \pi - \frac{\pi}{2} = \frac{\pi}{2}$ $\implies$ $\eta_1\sin\theta_i = \eta_2\sin(\frac{\pi}{2}-\theta_i) = \eta_2\cos\theta_i$ 
  - 从而有 $\theta_B = \theta_i = \arctan(\frac{\eta_2}{\eta_1})$ 
- 在菲尼尔方程中，此时的平行偏振反射率为0，即$R_p = 0$，所有平行偏振光都透射进入另一介质

