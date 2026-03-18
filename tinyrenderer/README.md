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
