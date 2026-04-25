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

Introduce a **programmable shader architecture** — the rasterizer becomes generic, and per-vertex/per-pixel work moves into a shader. Mirrors the OpenGL pipeline.

**`IShader` interface**:

```cpp
struct IShader {
    virtual Vec3f vertex(size_t faceIdx, size_t cornerIdx) = 0;
    virtual std::optional<TGAColor> fragment(const Vec3f &bary) const = 0;
};
```

- `vertex()` transforms a face's corner to screen space, and (as a side effect) stores per-vertex values ("varyings") as member state
- `fragment()` reads the varyings, barycentric-interpolates them via the shader's own math, and returns the pixel color — or `nullopt` to discard

**Pipeline flow** (per triangle):

```
for j in 0..2:           pts[j] = shader.vertex(faceIdx, j)
                         Triangle tri{pts}
rasterizer iterates:     for each pixel inside bbox:
                             bary = barycentric(pixel, tri)
                             color = shader.fragment(bary)
                             if color: z-test, write framebuffer
```

**Phong reflection model** — the fragment shader combines three terms:

$$I = I_a + I_d \max(0, \hat{n} \cdot \hat{l}) + I_s \max(0, \hat{r} \cdot \hat{v})^e$$

where $\hat{r} = 2\hat{n}(\hat{n} \cdot \hat{l}) - \hat{l}$ is the reflected light direction and $e$ is the shininess exponent.

- **Ambient**: constant background light — prevents pure black in unlit areas
- **Diffuse** (Lambert): dot product of normal and light direction — brightest when surface faces the light
- **Specular**: reflected-light direction dotted with view direction, raised to shininess — tight highlights on shiny surfaces

**Varyings trick** — per-vertex normals are stored as *columns* of a `Mat3f`, so `normals * bary` gives the interpolated normal in a single multiply instead of three separate scales and adds.

**Model extension** — OBJ's `vn x y z` lines and the normal index of `v/t/n` face entries are now parsed into a parallel `normals_` buffer; faces store `{vertIdx, normalIdx}` per corner.

- **Discard order**: the z-buffer is updated only *after* the fragment returns a color, so discarded pixels don't pollute depth.
- **Specular shortcut**: using $r \cdot v \approx r_z$ assumes the camera looks down world $-z$. True only when the eye is on the z-axis — otherwise need to compute view direction per pixel or move lighting to camera space.

> Later refined to **Blinn-Phong** ($\hat{n} \cdot \hat{h}$ where $\hat{h} = \widehat{\hat{l} + \hat{v}}$) with a true per-pixel view direction interpolated from a `worldPositions` varying.

## Lesson 7: More Data!

Three new types of texture, all parsed from `.tga` files:

| Texture | Suffix | Purpose |
|---------|--------|---------|
| Diffuse | `_diffuse.tga` | Per-pixel surface color |
| Normal | `_nm.tga` | Per-pixel normal (RGB → xyz) |
| Specular | `_spec.tga` | Per-pixel specular intensity |

This lesson covers the **diffuse** map; normal and specular come later.

**Model extension** — parse `vt u v` lines into a `texCoords_` buffer and store the texture index in each face corner. `FaceCorner` is now `{vertIdx, texIdx, normalIdx}`, matching the OBJ `v/t/n` format exactly.

**UV varying** — texture coordinates are a third per-vertex value (after normals and world positions). Stored as columns of a `Mat<float, 2, 3>`; `texCoords * bary` interpolates to a `Vec2f` UV in the fragment shader. Same column-per-vertex pattern lets matrix-vector multiply do the barycentric blend in one operation.

**Sampling** — convert `[0, 1]²` UV → pixel coords. Watch the v-axis convention: TGA images are stored top-down internally after `flipVertically()` on load, but OBJ UVs use bottom-left origin, so `v` flips when indexing:
```cpp
const int y = std::clamp(static_cast<int>((1.f - uv.y()) * h), 0, h - 1);
```

**Modulation** — sampled texel color is multiplied per-channel by the Blinn-Phong lighting factor. Texture provides surface *albedo*; lighting modulates it.

**Architecture refactor** — `IShader` and concrete shaders moved out of `main.cpp` into a dedicated `Shader.{hpp,cpp}` module. Two implementations:

- `BlinnPhongShader` — single base color, no texture
- `BlinnPhongTexturedShader` — diffuse map sample modulating the lighting

Switching between them is a one-line change in `main()` — the rasterizer is unaware. That's the payoff of the `IShader` polymorphism.

**Range-based iteration** — `IShader` exposes a public `triangles()` view (built on `std::views::iota | std::views::transform`) that lazily produces post-vertex-processing triangles. The face count is supplied via a private virtual `faceCount()` (NVI / Template Method pattern). Caller becomes:
```cpp
for (const auto &tri : shader.triangles()) {
    rasterize(tri, shader, zbuffer, image);
}
```

- **`fragment()` no longer returns `std::optional`** — neither shader uses discard; YAGNI applied. Add it back when alpha cutout / masking arrives.
