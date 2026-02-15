# About BRDF
## Fundamentals
### Halfway vector
$$
\textbf{h} = \frac{\textbf{l} + \textbf{v}}{|\textbf{l} + \textbf{v}|}
$$

### Gram-Schmidt Orthogonization
$$
\textbf{t}' = \textbf{t} - (\textbf{t} \cdot \textbf{n})\textbf{n}
$$

$$
\textbf{t}'_{normalized}=\frac{\textbf{t}'}{|\textbf{t}'|}
$$

## Fundamental
### Cook-Torrance Microfacet Model

$$
f(\textbf{l}, \textbf{v}) = C_{diffuse} + \frac{D(\theta_{\textbf{h}})F(\theta_{\textbf{d}})G(\theta_l, \theta_{\textbf{v}})}{4cos\theta_{\textbf{l}}cos\theta_{\textbf{v}}}
$$

$\theta_{\textbf{v}}$, $\theta_{\textbf{l}}$ are the angles of incidence of the **l** and **v** vectors with respect to the normal, $\theta_{\textbf{h}}$ is the angle between the normal and the half vector.  $\theta_{\textbf{d}}$ is the "difference" angle between **l** and the half vector **h**. 


## Diffuse
### Lambert
$$
C_{diffuse} = \frac{C_{baseColor}}{\pi}
$$


### Disney's Empirical Model

$$
C_{diffuse} = \frac{C_{baseColor}}{\pi}(1 + (F_{D90} - 1)(1 - cos\theta_{\textbf{l}})^5) (1 + (F_{D90} - 1)(1 - cos\theta_{\textbf{v}})^5)
$$


This model describes the pheonomenon that diffuse has sensible retroreflection if view vector closing the grazing angle.  

$F_{D90}$ is grazing fresnel value. $F_{D90} = 0.5 + 2 \alpha cos^2\theta_{\textbf{d}}$

$\alpha$ is roughness. 

## Distribution

### Normalized Blinn (Unreal Implementation)

$$
D(\textbf{h}) = \frac{\epsilon + 2}{2 \pi} (\textbf{n} \cdot \textbf{h})^\epsilon
$$

$\epsilon$ is smoothness exponent: $\epsilon = \frac{2}{\alpha^2}$

$\frac{n + 2}{2 \pi}$ is normalize factor, to make sure integration amount less equal to 1.

$\textbf{n}$ is marco surface normal, $\textbf{h}$ is half vector. 

### GGX (TR)

$$
D(\textbf{m}) = \frac{\alpha_g^2 \chi^+(\textbf{m} \cdot \textbf{n})}{\pi cos^4(\theta_\textbf{m})(\alpha_g^2 + tan^2\theta_\textbf{m})}
$$

When $m = h$ (means this ray will be exactly reflect into the view) , $cos\theta_m = cos\theta_h$, i.e $cos\theta_h = normal \cdot half$.

Can also represented as:

$$
D(\textbf{h}) = \frac{c}{(\alpha^2cos^2\theta_{\textbf{h}} + sin^2\theta_{\textbf{h}})^2}
$$

$c$ is a scaling constant, here $\gamma = 2$, it's constanct $c = \frac{(\gamma - 1)(1 - \alpha^2)}{\pi(1 - \alpha^2)}$, reducing to $c = \frac{1}{\pi}$.

$\alpha$ is presented $roughness^2$ parameter with values between 0 (perfectly smooth) and 1 (fully rough).

Can also represented formula as (Unreal Engine):
$$
D(\textbf{h}) = \frac{\alpha^2}{\pi({(\textbf{n} \cdot \textbf{h})^2(\alpha^2 -1) + 1})^2}
$$

### GTR
> Genelized Trowbridge-Reitz Distribution Function

$$
D(\textbf{h}) = \frac{c}{(\alpha^2cos^2\theta_{\textbf{h}} + sin^2\theta_{\textbf{h}})^\gamma}
$$

$\gamma$ controls "tail" of specular peak, the $\gamma$ smaller, the tail longer.


### Anisotropic GGX

$$
D(\textbf{h}) = \frac{1}{\pi}\frac{1}{\alpha_x\alpha_y}\frac{1}{((\frac{\textbf{x} \cdot \textbf{h}}{ \alpha_x })^2 + (\frac{\textbf{y} \cdot \textbf{h}}{ \alpha_y })^2 + (\textbf{n} \cdot \textbf{h})^2) ^ 2}
$$

As [GGX (TR)](#ggx-tr) metioned above, $\frac{1}{\pi}$ is scaling constant $c$.

$\textbf{n}$ is marco surface normal, $\textbf{h}$ is half vector.   
$\textbf{x}$ is tangent vector, $\textbf{y}$ is bitangent vector.  
$\alpha_x, \alpha_y$ are roughness in there anisotropic direction.


## Geometric Shadowing
### Generic Form of Shadowing and Masking Function (G2)

$$
G_2(\textbf{v}, \textbf{l}) = G(\textbf{v})G(\textbf{l})
$$

### Schlick Approximation

$$
G(\textbf{v}) = \frac{\textbf{n} \cdot \textbf{v}}{\textbf{n} \cdot \textbf{v} - k(\textbf{n} \cdot \textbf{v}) + k}
$$


With $k = \sqrt{\frac{2m^2}{\pi}}$; $m$ is root mean square (RMS) slope of the microfacets (simply understood as roughness).

Parameter can be $\textbf{v}$ or $\textbf{l}$, both of them are suit for this function.

### Smith Separable Masking and Shadiowing
$$
G_2(\omega_o, \omega_i, \omega_m) = G_1(\omega_o, \omega_m)G_1(\omega_i, \omega_m) = \frac{\chi^+(\omega_o \cdot \omega_m)}{1 + \Lambda(\omega_o)} \frac{\chi^+(\omega_i \cdot \omega_m)}{1 + \Lambda(\omega_i)} 
$$

$\chi^+$ is heaviside function, satisfied with $\chi^+(x) = \begin{cases} 1, x > 0 \\0, x \leq  0\end{cases}$ .

$\Lambda$ meansures the ratio of the invisble masked microfacet area to the visible mircofacet area for a given direction.  

In Beckmann Distribution, $\Lambda(\omega_o) = \frac{erf(a) - 1}{2} + \frac{1}{2a\sqrt\pi}exp(-a^2)$. Where $a = \frac{1}{\alpha tan\theta_o}$. Walter el al. propose an accurate rational approximation for $G_1 = \frac{1}{1 + \Lambda(\omega_o)}$, which we can use to approximate $\Lambda(\omega_o) \approx \begin{cases} \frac{1-1.259a+0.396a^2}{3.535a+2.181a^2}, x < 1.6 \\0, otherwise. \end{cases}$  
In GGX Distribution, $\Lambda(\omega_o) = \frac{-1+\sqrt{1+\frac{1}{a^2}}}{2}$, where $a = \frac{1}{\alpha tan\theta_o}$.


### Smith Height-Correlated Masking and Shadowing
$$
G_2(\omega_o, \omega_i, \omega_m) = \frac{\chi^+(\omega_o \cdot \omega_m)\chi^+(\omega_i \cdot \omega_m)}{1 + \Lambda(\omega_o) + \Lambda(\omega_i)}
$$

### Smith Height-Direction-Correlated Masking and Shadowing
$$
G_2(\omega_o, \omega_i, \omega_m) = \frac{\chi^+(\omega_o \cdot \omega_m)\chi^+(\omega_i \cdot \omega_m)}{1 + max(\Lambda(\omega_o) + \Lambda(\omega_i)) + \lambda(\omega_o, \omega_i)min(\Lambda(\omega_o) + \Lambda(\omega_i))}
$$

Here masking and shadowing are fully correlated when the outgoing and incident directions are parallel and $\lambda = 0$. The correlation decreases as the angle between the directions increases, and as $\lambda$ increases up to 1.

Ginneken et al. proposed an empirical factor $\lambda = \frac{4.41\phi}{4.41 \phi + 1}$; which depends on $\phi$, the azimuthal angle difference between $\omega_i$ and $\omega_o$.

## Fresnel
### Fresnel Equation

$$
R_s = |\frac{n_1 cos\theta_i-n2cos\theta_t}{n_1 cos\theta_i+n2cos\theta_t}|^2
$$

$$
R_p = |\frac{n_1 cos\theta_t-n2cos\theta_i}{n_1 cos\theta_t+n2cos\theta_i}|^2
$$

$R_s$ and $R_p$ describe reflection of different polarized light. $\theta_i$ is the angle between light incident direction and normal, $\theta_t$ is the angle between light refractive direction and normal.

Unpolarized form:
$$
R = \frac{R_s + R_p}{2}
$$



### Schlick's Fresnel Approximation

$$
F = F_0 + (1 - F_0)(1 - cos\theta_{\textbf{d}})^5
$$

$F_0$ represents reflectance at normal incidence and is achromatic for dielectirics and chromatic for matals. $F_0 = (\frac{n_1 - n_2}{n_1 + n2})^2$, $n_1$ is IOR of light source material, $n_2$ is IOR of light destination material.

$cos_d$ also can be replaced as $\textbf{v} \cdot \textbf{h}$.


## Image Based Lighting
### Approximation Intergation from Unreal Engine

$$
\int_{\Omega}L_i(\textbf{l})f(\textbf{l}, \textbf{v}) cos\theta_l \mathrm{d}\textbf{l} \approx \frac{1}{N}\sum^N_{k=1} \frac{L_i(\textbf{l}_k)f(\textbf{l}_k, \textbf{v})cos\theta_{\textbf{l}_k}}{p(\textbf{l}_k, \textbf{v})}
$$

Left side: $\Omega$ is Hemisphere, $L_i$ is incident light in light direction $\textbf{l}$, $f$ is fresnel term.

Right side: $N$ is total samples count, $p(\textbf{l}_k, \textbf{v})$ is Probability Density Function (PDF).  
> TODO: Where the approximation was derived from?

Also, a inexpensive (related to above) split sum approximation is provided:

$$
\frac{1}{N}\sum^N_{k=1} \frac{L_i(\textbf{l}_k)f(\textbf{l}_k, \textbf{v})cos\theta_{l_k}}{p(\textbf{l}_k, \textbf{v})} \approx (\frac{1}{N}\sum^N_{k=1}L_i(\textbf{l}_k))(\frac{1}{N}\sum^N_{k=1}\frac{f(\textbf{l}_k, \textbf{v}cos\theta_{\textbf{l}_k})}{p(\textbf{l}_k, \textbf{v})})
$$


# About BSSRDF

## Empirical Models
> Reference Model: Monte Carlo simulation [Kalos and Whitlock 1986; Wang et al. 1995]
- Dipole diffusion model [Jensen el al. 2001]
- Quantized diffusion model [d'Eon and Irving 2011]
-  Photon beam diffusion [Habel el al. 2013]