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

## Lesson 6: Shading

Programmable shader architecture — rasterizer becomes generic, per-vertex/pixel work moves into a shader (mirrors OpenGL).

- `vertex(face, corner)` → screen space, stashes per-vertex "varyings" as member state
- `fragment(bary)` → barycentric-interpolates varyings, returns pixel color

**Phong reflection model**:

$$I = I_a + I_d \max(0, \hat{n} \cdot \hat{l}) + I_s \max(0, \hat{r} \cdot \hat{v})^e, \quad \hat{r} = 2\hat{n}(\hat{n} \cdot \hat{l}) - \hat{l}$$

- **Ambient** constant floor · **Diffuse** $\hat{n}\cdot\hat{l}$ (Lambert) · **Specular** $(\hat{r}\cdot\hat{v})^e$ (shininess $e$)

**Varyings trick** — store per-vertex normals as *columns* of a `Mat3f`; `normals * bary` interpolates in one matrix-vector multiply.

- **Model**: parse OBJ `vn` lines + normal index into `normals_`; faces store `{vertIdx, normalIdx}`.
- **Specular shortcut** $r \cdot v \approx r_z$ assumes the eye is on the z-axis — otherwise needs per-pixel view direction.

> Refined to **Blinn-Phong**: $\hat{n} \cdot \hat{h}$ with $\hat{h} = \widehat{\hat{l} + \hat{v}}$, view direction from a `worldPositions` varying.

## Lesson 7: More Data!

Textures parsed from `.tga`: **diffuse** (`_diffuse`, surface color), **normal** (`_nm`, RGB→xyz), **specular** (`_spec`, highlight intensity). This lesson: diffuse.

- **Model**: parse `vt u v`; `FaceCorner` becomes `{vertIdx, texIdx, normalIdx}` (the OBJ `v/t/n` format).
- **UV varying** — columns of a `Mat<float, 2, 3>`; `texCoords * bary` → interpolated `Vec2f`.
- **Sampling v-flip** — TGA is top-down after load, OBJ UVs are bottom-left origin: `y = (1 - uv.y) * h`.
- **Modulation** — texel color × lighting factor (texture = albedo, lighting modulates).

**Material struct** — `BlinnPhongShader` carries a `Material` (`baseColor`, `shininess`, optional `diffuse`/`glow`/`specular`/`normalMap`); fragment branches on which maps are present. Separates shading *model* from surface *description* — solid color vs. textured is a Material change, not a shader change.

## Lesson 8: Tangent Space Normal Mapping

Per-pixel surface detail without geometry. World-space normal maps break under rotation; **tangent-space** maps encode the normal in a local surface frame, so one texture survives any pose (characteristically blue — most texels near $(0,0,1)$).

**TBN frame**: $\hat{n}$ geometric normal, $\hat{T}$ along texture $+u$, $\hat{B}$ along $+v$.

**Derive T, B from the triangle** — world edges $E = [e_1 \mid e_2]$ ($3\times2$), UV deltas $U$ ($2\times2$):

$$[T \mid B] = E \cdot U^{-1}$$

**Reconstruct world normal** — sample $t$, decode $[0,255]\to[-1,1]$, then:

$$\hat{n}_{world} = \widehat{t_x \hat{T} + t_y \hat{B} + t_z \hat{n}}$$

- **Normalize T and B** — the formula scales them by UV stretch; the map assumes $|T|=|B|=1$.
- **T, B not generally orthogonal** — reflects UV shear/stretch; we skip MikkTSpace, fine for learning.
- **2×2 inverse on `Mat`** — closed form, gated by `requires(R == 2 && C == 2)`, returns `std::expected` (`MatrixError::SINGULAR` on degenerate UV).

## Lesson 9: Shadow Mapping

Hard shadows via **two passes**, reusing the depth buffer:

1. **Depth pass** — render from the *light's* view; the depth buffer is the **shadow map** (nearest surface to the light per texel).
2. **Shading pass** — render from the camera; transform each fragment's world position into light space and compare. Farther than the stored depth ⇒ occluded ⇒ ambient only.

- **World-space lookup** — our shader already carries a `worldPos` varying, so we go world → light-screen directly, skipping the reference tutorial's $N \cdot M^{-1}$ inversion.
- **Orthographic light** — a directional light has parallel rays, so the light projection is orthographic (leaves $w = 1$; the lookup needs no perspective divide).
- **Shadow acne → bias** — the two passes round depth differently, so a lit surface shadows itself. Compare `lz > stored + bias`. Too much bias → *peter-panning* (shadow detaches from the feet).

**Rasterizer refactor** (shipped here):

- **Perspective-correct interpolation** — screen-space barycentrics interpolate attributes wrong (textures warp on the floor). Reweight by $1/w$ and renormalize. Depth is exempt: NDC $z$ is already linear in screen space.
- **Rasterizer owns interpolation** — shaders return clip-space corners via `primitive(face)` and shade via `fragment(Varyings)`; the rasterizer does the divide, viewport, depth test, and $1/w$-correct blend.
- **`IShader` → `Shader` concept** — the varying type now flows through the call, so dispatch is a template + concept, not runtime polymorphism. TBN is constant per triangle, so it's computed once in `primitive()` and baked into all three `Varyings` corners.
