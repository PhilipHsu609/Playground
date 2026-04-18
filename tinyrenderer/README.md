# tinyrenderer

My implementation of [ssloy/tinyrenderer](https://github.com/ssloy/tinyrenderer), following the [haqr.eu tutorial](https://haqr.eu/tinyrenderer/).

## Lesson 0: Getting Started

- TGA image reader/writer with RLE compression
  - [TGA Specs](http://tfc.duke.free.fr/coding/tga_specs.pdf)

## Lesson 1: Bresenham’s Line Drawing Algorithm

- Parametric line → x-iteration → transpose for steep lines → integer-only Bresenham
- Track an integer error term; step $y$ when error exceeds threshold
- Wireframe rendering of OBJ models via orthographic projection

## Lesson 2: Triangle Rasterization and Back-Face Culling

- Bounding-box iteration with barycentric point-in-triangle test
- Barycentric coordinates: $P = \alpha A + \beta B + \gamma C$, $\alpha + \beta + \gamma = 1$
  - Each weight = sub-triangle area / total area
  - Inside test: all three weights $\geq 0$
  - Enables smooth interpolation of any vertex attribute across the triangle
- Back-face culling: skip triangles with negative signed area (facing away from camera)

## Lesson 3: Hidden Faces Removal (Z-Buffer)

- Per-pixel depth buffer: interpolate $z$ via barycentric coords, paint only if $z > \text{zbuffer}[pixel]$
- Order-independent — no triangle sorting needed
- Back-face culling alone leaves artifacts; painter’s algorithm can’t handle all cases

## Lesson 4: Naive Camera Handling

Rotate the scene instead of moving the camera — apply a modeling transform before projection. Y-axis rotation matrix:

$$R_y(\theta) = \begin{pmatrix} \cos\theta & 0 & \sin\theta \\\ 0 & 1 & 0 \\\ -\sin\theta & 0 & \cos\theta \end{pmatrix}$$

Perspective projection via intercept theorem (camera at $(0, 0, c)$, image plane at $z = 0$):

$$x' = \frac{x}{1 - z/c}, \quad y' = \frac{y}{1 - z/c}$$

- Camera pose is **baked into the formula** — this is the "naive" part
- Changing $c$ changes perspective *strength* (foreshortening), not overall scale
- Points with $z \geq c$ are behind the camera → divisor flips sign → broken projection
- Pipeline: `model.vert → rotate → perspective → viewport → screen`
- Introduced `Mat<T, R, C>` class with matrix-matrix and matrix-vector multiply
