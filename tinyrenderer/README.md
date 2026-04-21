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

## Lesson 5: Better Camera Handling

Decouple the camera from the projection. The full pipeline is now a composition of four standard matrices:

$$\text{screen} = V \cdot P \cdot M_{\text{view}} \cdot v$$

Homogeneous coordinates (4D) handle translation as part of matrix multiplication. The perspective divide happens explicitly after projection.

**View matrix** — change of basis from world to camera coordinates. Given `eye`, `center`, `up`, build the camera's local axes and inline the translation:

$$M_{\text{view}} = \begin{pmatrix} x_x & x_y & x_z & -x \cdot \text{eye} \\\ y_x & y_y & y_z & -y \cdot \text{eye} \\\ z_x & z_y & z_z & -z \cdot \text{eye} \\\ 0 & 0 & 0 & 1 \end{pmatrix}$$

where $z = \widehat{\text{eye} - \text{center}}$, $x = \widehat{\text{up} \times z}$, $y = z \times x$.

**Projection matrix** — standard OpenGL `gluPerspective`:

$$P = \begin{pmatrix} \frac{1}{a \tan(\theta/2)} & 0 & 0 & 0 \\\ 0 & \frac{1}{\tan(\theta/2)} & 0 & 0 \\\ 0 & 0 & -\frac{f+n}{f-n} & -\frac{2fn}{f-n} \\\ 0 & 0 & -1 & 0 \end{pmatrix}$$

The bottom row puts $w' = -z$, which triggers the perspective divide. Row 2 maps $z \in [-n, -f]$ non-linearly to NDC $z \in [-1, +1]$.

**Viewport matrix** — maps NDC $[-1, 1]$ to pixel coords $[0, w] \times [0, h]$.

- **Z-buffer convention**: OpenGL NDC puts *closer objects at smaller* $z$ (near plane $\to -1$). Depth test becomes `if (new_z < stored_z)` with z-buffer initialized to `+∞`.
- **Gotcha**: when `eye - center` is parallel to `up`, `cross(up, z)` degenerates. Avoid straight-up/down views or switch `up` dynamically.
- **Changing one eye coordinate** moves the camera along that world axis — to orbit the model, parameterize `eye` on a sphere around `center`.
