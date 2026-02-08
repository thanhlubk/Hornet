# GLViewWindow Renderers — Upload & Draw “Sheet”
*(Focus: `HRenderNode`, `HRenderElement` (faces + edges), `HRenderForce`, `HRenderCoordinateGizmo`)*

This sheet explains, **step by step**, how data is uploaded to the GPU and how each renderer issues draw calls.
It also explains **why edges can lay over faces** and **nodes can lay over both**, using the exact OpenGL state in your code.

---

## 0) Shared Concepts (used in all 4 renderers)

### VAO / VBO / EBO roles

- **VAO (Vertex Array Object)**: a “state bundle” that remembers:
  - which VBO is bound for each vertex attribute
  - how to interpret that VBO (format/stride/offset) via `glVertexAttribPointer`
  - (also) which EBO is bound as `GL_ELEMENT_ARRAY_BUFFER` *while the VAO is bound*

- **VBO (Vertex Buffer Object)**: raw vertex arrays (positions, normals, colors, …) on the GPU.

- **EBO/IBO (Element Buffer Object)**: index arrays that tell `glDrawElements()` which vertices to use and in what order.

### Your attribute layout convention

Across your shaders:

- **location 0**: position `aPos` (`vec3`)
- **location 1**: color `aCol / aColor` (`vec4`)
- **location 2**: normal `aNrm / aNormal` (`vec3`)

This is why you see:

- `glVertexAttribPointer(0, 3, GL_FLOAT, …)` for positions  
- `glVertexAttribPointer(1, 4, GL_FLOAT, …)` for colors  
- `glVertexAttribPointer(2, 3, GL_FLOAT, …)` for normals  

### What “upload” really means

In your code, “upload” always means:

1. Bind VAO
2. Bind a buffer (VBO or EBO)
3. `glBufferData(...)` to allocate/copy CPU vectors into GPU memory
4. Set attribute pointers (for VBOs)
5. Unbind VAO

Later, “draw” means:

1. Bind shader program
2. Set uniforms (`uMVP`, colors, lighting toggles…)
3. Bind VAO (+ correct EBO when needed)
4. Call a draw function (`glDrawArrays` or `glDrawElements`)
5. Unbind and restore state as needed

---

## 1) `HRenderNode` — upload + draw (points)

### 1.1 Member variables (what they represent)

- `m_vao`: VAO that stores the node attribute layout (pos + color).
- `m_vboPosition`: GPU buffer with `std::vector<QVector3D>` positions.
- `m_vboColor`: GPU buffer with `std::vector<QVector4D>` per-node colors.
- `m_iNumNode`: how many points to draw.
- `m_fNodeSize`: size in pixels → sent to shader as `uPointSize` and becomes `gl_PointSize`.
- `m_bEnablePerNodeColor`: toggles whether to use per-vertex color (`aCol`) or `uColor`.
- Selection overlay:
  - `m_eboSelection`: EBO containing indices of selected node vertices.
  - `m_iSelectionCount`: number of indices in `m_eboSelection`.
  - `m_mapIdIndex`: maps external `nodeId` → vertex index in VBO (so selection can be built by IDs).

---

### 1.2 Upload path (`HRenderNode::upload`)

Relevant code:

```cpp
glBindVertexArray(m_vao);

glBindBuffer(GL_ARRAY_BUFFER, m_vboPosition);
glBufferData(GL_ARRAY_BUFFER, pos.size()*sizeof(QVector3D), pos.data(), GL_STATIC_DRAW);
glEnableVertexAttribArray(0);
glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(QVector3D), (void*)0);

glBindBuffer(GL_ARRAY_BUFFER, m_vboColor);
glBufferData(GL_ARRAY_BUFFER, col.size()*sizeof(QVector4D), col.data(), GL_STATIC_DRAW);
glEnableVertexAttribArray(1);
glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(QVector4D), (void*)0);

glBindVertexArray(0);
```

#### Step-by-step explanation (every OpenGL call)

1) **Bind VAO**
```cpp
glBindVertexArray(m_vao);
```
- Makes `m_vao` the “current VAO”.
- Any `glVertexAttribPointer(...)` calls now “write into” this VAO.

2) **Bind position buffer as GL_ARRAY_BUFFER**
```cpp
glBindBuffer(GL_ARRAY_BUFFER, m_vboPosition);
```
- `GL_ARRAY_BUFFER` is the target used for vertex attributes.
- `m_vboPosition` is the GPU object that will hold the positions.

3) **Upload position data**
```cpp
glBufferData(GL_ARRAY_BUFFER, pos.size()*sizeof(QVector3D), pos.data(), GL_STATIC_DRAW);
```
Parameters:
- target: `GL_ARRAY_BUFFER` (the currently bound array buffer)
- size: byte size of the data
- data: pointer to CPU array (`pos.data()`)
- usage: `GL_STATIC_DRAW` (data rarely changes)

4) **Enable attribute array 0**
```cpp
glEnableVertexAttribArray(0);
```
- Turns on attribute slot 0 (position).

5) **Describe attribute format for location 0**
```cpp
glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(QVector3D), (void*)0);
```
Parameters:
- index: `0` → matches `layout(location=0) in vec3 aPos;`
- size: `3` → vec3
- type: `GL_FLOAT` → your QVector3D stores floats
- normalized: `GL_FALSE` → no normalization for floats
- stride: `sizeof(QVector3D)` → tightly packed vec3 array
- pointer: `(void*)0` → offset from start of the bound VBO

6) **Repeat for colors (attribute location 1)**
Same logic as above, but `size=4` and `index=1`.

7) **Unbind VAO**
```cpp
glBindVertexArray(0);
```
- Stops accidentally modifying it later.

8) **Build `nodeId -> index` map**
```cpp
m_mapIdIndex[nodeIds[i]] = i;
```
- This is NOT OpenGL: it is your lookup table so selection can build indices later.

9) **Selection EBO created once**
```cpp
if (!m_eboSelection) glGenBuffers(1, &m_eboSelection);
```
- Selection is stored as indices (so you can highlight some points without re-uploading all nodes).

---

### 1.3 Draw path (`HRenderNode::draw`)

Main pass:

```cpp
glEnable(GL_DEPTH_TEST);
glGetIntegerv(GL_DEPTH_FUNC, &_oldDepthFunc);
glDepthFunc(GL_LEQUAL);
glEnable(GL_POLYGON_OFFSET_POINT);
glPolygonOffset(-1.0f, -1.0f);

glDrawArrays(GL_POINTS, 0, m_iNumNode);
```

#### What this achieves (and why nodes appear “on top”)

- **Render order** in `HRenderModel::draw()` is:
  1) faces
  2) edges
  3) nodes

- In addition, nodes use:
  - `glDepthFunc(GL_LEQUAL)` so if depth is equal it still passes.
  - `glPolygonOffset(-1, -1)` for points (pulls them slightly toward the camera)
    - Negative offset moves generated depth closer → helps nodes “win” against edges/faces at identical geometry depth.

#### Shader parameters
- `uMVP`: transforms world → clip space.
- `uPointSize`: written into `gl_PointSize` in the VS.
- `uUseVertexColor`: picks between `aCol` and uniform color.

#### Selection overlay pass
- Uses `m_eboSelection` and `glDrawElements(GL_POINTS, ...)` to draw **only selected indices**.
- Enables blending for a softer overlay.

---

## 2) `HRenderElement` — upload + draw faces + draw edges

### 2.1 Member variables (what they represent)

GPU objects:
- `m_vao`: VAO for element mesh
- `m_vboPosition`: positions (shared by faces + edges)
- `m_vboNormal`: normals (used for lit faces)
- `m_eboTri`: triangle index buffer (faces)
- `m_eboEdge`: line index buffer (edges)

Batches & selection:
- `m_vecElementBatches` (`FaceBatch`): `{first,count,color,elementId}`
  - lets you draw per-element blocks with possibly different colors
- `m_mapElementTriRanges`: `elementId -> [(first,count), ...]`
  - used for selection overlay: draw only triangles belonging to selected element IDs
- `m_iTriCount`, `m_iEdgeCount`: total index counts

---

### 2.2 Upload (`HRenderElement::upload`)

Code:

```cpp
glBindVertexArray(m_vao);

glBindBuffer(GL_ARRAY_BUFFER, m_vboPosition);
glBufferData(GL_ARRAY_BUFFER, pos.size()*sizeof(QVector3D), pos.data(), GL_STATIC_DRAW);
glEnableVertexAttribArray(0);
glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(QVector3D),(void*)0);

glBindBuffer(GL_ARRAY_BUFFER, m_vboNormal);
glBufferData(GL_ARRAY_BUFFER, nrm.size()*sizeof(QVector3D), nrm.data(), GL_STATIC_DRAW);
glEnableVertexAttribArray(2);
glVertexAttribPointer(2,3,GL_FLOAT,GL_FALSE,sizeof(QVector3D),(void*)0);

glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_eboTri);
glBufferData(GL_ELEMENT_ARRAY_BUFFER, triIdx.size()*sizeof(uint32_t), triIdx.data(), GL_STATIC_DRAW);

glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_eboEdge);
glBufferData(GL_ELEMENT_ARRAY_BUFFER, lineIdx.size()*sizeof(uint32_t), lineIdx.data(), GL_STATIC_DRAW);

glBindVertexArray(0);
```

#### Important subtlety: EBO binding is VAO state

- When VAO is bound, binding `GL_ELEMENT_ARRAY_BUFFER` stores that EBO in the VAO.
- But your code binds two different EBOs back-to-back (`m_eboTri` then `m_eboEdge`).
- Therefore the VAO *would end up remembering the last one* (`m_eboEdge`).

That’s why in draw you explicitly re-bind the EBO you want:

- in `drawElement()` you call `glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_eboTri);`
- in `drawEdges()` you call `glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_eboEdge);`

So: **VAO contains attribute layout**, while **draw decides which index buffer to use**.

---

### 2.3 Draw faces (`HRenderElement::drawElement`)

Face pass:

```cpp
glEnable(GL_DEPTH_TEST);
glDepthMask(GL_TRUE);

GLboolean hadCull = glIsEnabled(GL_CULL_FACE);
if (hadCull) glDisable(GL_CULL_FACE);

glEnable(GL_POLYGON_OFFSET_FILL);
glPolygonOffset(2.f, 2.f);

... bind shader, set uniforms ...

glBindVertexArray(m_vao);
glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_eboTri);
glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

for (FaceBatch b : m_vecElementBatches)
    glDrawElements(GL_TRIANGLES, b.count, GL_UNSIGNED_INT, offset(b.first));
```

#### Why polygon offset is used here
`glPolygonOffset(2,2)` pushes filled triangles *slightly away from the camera in depth*.

That reduces **z-fighting** when you later draw wire edges on the same surface.

- If you draw edges at exactly the same depth as faces:
  - some pixels may randomly show face or edge depending on rasterization and precision.
- By offsetting faces back:
  - edges tend to pass depth test more consistently.

#### Batch drawing
Each `FaceBatch` stores:
- `first`: starting index inside the triangle index buffer
- `count`: how many indices to draw (must be multiple of 3 for triangles)
- `color`: per-element RGBA from your model (or default if disabled)
- `elementId`: used for selection mapping

`offset(b.first)` is computed as:

```cpp
const void* off = (void*)(sizeof(uint32_t) * b.first);
```

Because OpenGL expects a byte offset into the bound EBO.

---

### 2.4 Draw edges (`HRenderElement::drawEdges`)

Edge pass:

```cpp
glEnable(GL_DEPTH_TEST);
glDepthMask(GL_FALSE);
glGetIntegerv(GL_DEPTH_FUNC, &oldDepthFunc);
glDepthFunc(GL_LEQUAL);

... bind shader, set unlit ...

glBindVertexArray(m_vao);
glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_eboEdge);
glLineWidth(m_fEdgeWidth);
glEnable(GL_LINE_SMOOTH);

glDrawElements(GL_LINES, b.count, GL_UNSIGNED_INT, offset(b.first));
```

#### Why edges “lay over” faces in your code

**Three cooperating mechanisms:**

1) **Render order**
- Faces are drawn first, then edges.
- So the depth buffer already contains face depths.

2) **Faces were pushed back**
- `glPolygonOffset(2,2)` moved faces slightly farther.

3) **Edges use `GL_LEQUAL` + don’t write depth**
- `glDepthFunc(GL_LEQUAL)` allows edge fragments at *equal* depth to pass.
- `glDepthMask(GL_FALSE)` prevents edges from replacing depth values.
  - That keeps depth buffer “owned by faces”, which is usually what you want.

Result:
- edges are visible on top of faces where they coincide,
- but edges still correctly disappear behind other geometry that is closer.

---

## 3) `HRenderForce` — rebuild/upload + draw (arrow mesh)

Forces are more complex because the geometry is **generated** (cylinder + cone) and may be rebuilt to preserve constant screen size.

### 3.1 Key member variables

GPU:
- `m_vao`
- `m_vboPosition` (vec3)
- `m_vboNormal` (vec3)
- `m_vboColor` (vec4)
- `m_ebo` (indices)

Behavior:
- `m_vecForces`: input arrows (position, direction, size, id, color, lightingEnabled, style...)
- `m_bConstantScreenSize`: if true, arrow width/length scales so it looks the same in pixels
- `m_fSize`: base size in pixels
- `m_bRebuild`: marks that geometry must be re-generated and uploaded

Selection:
- `m_mapRangesLighting`, `m_mapRangesUnlighting`:
  - `forceId -> [(first,count), ...]` ranges inside each index sub-buffer
- `m_iLightingCount`, `m_iUnlightingCount`:
  - how many indices belong to lit/unlit subsets
- `m_vecSelectionIds`: selected force IDs
- selection overlay draws only the selected force ranges with `uAlpha`

---

### 3.2 Constant screen size math (`worldPerPixelAt`)

Goal: “How many world units correspond to 1 screen pixel at this depth?”

Steps:
1) project a world position to clip/NDC (`VP * world`)
2) nudge NDC y by `2/viewportH` (≈ one pixel in NDC)
3) unproject both points back to world (using `invVP`)
4) distance between them = world units per pixel

Then in `rebuild()`:

```cpp
px = m_fSize * A.size;
wpp = worldPerPixelAt(...);
totalLen = m_bConstantScreenSize ? px * wpp : px;
```

So:
- if constant screen size → arrow length is chosen so it measures roughly `px` pixels on screen.

---

### 3.3 Geometry generation (`HRenderForce::rebuild`)

The arrow is built in world coordinates by constructing a local orthonormal frame:

- `z` = normalized force direction
- choose `up0` not parallel to `z`
- `x = cross(up0, z)`  
- `y = cross(z, x)`

Then convert local points to world:

```cpp
toW(pL)  = tailPos + x*pL.x + y*pL.y + z*pL.z
nToW(nL) = x*nL.x + y*nL.y + z*nL.z
```

#### Shaft (cylinder)

- Build 2 rings (z=0 and z=shaftLen), each ring has `segs` vertices.
- For each segment, connect two triangles → a quad strip.

Indices are appended to either `idxLit` or `idxUnlit`.

#### Head (cone)

- Apex + rim triangle fan
- Per-triangle normals computed with `QVector3D::normal(...)`

#### Per-force index ranges

For selection later:

```cpp
baseI = outIdx.size() before building
... append indices ...
cnt = outIdx.size() - baseI
mapRef[A.id].push_back({baseI, cnt});
```

So each force ID remembers exactly which index range(s) belong to it.

---

### 3.4 Upload inside rebuild (dynamic buffers)

Because forces can change or rebuild with camera:

- Buffers use `GL_DYNAMIC_DRAW`

Example:

```cpp
glBindBuffer(GL_ARRAY_BUFFER, m_vboPosition);
glBufferData(GL_ARRAY_BUFFER, m_pos.size()*sizeof(QVector3D), m_pos.data(), GL_DYNAMIC_DRAW);
```

Same for normals, colors, and indices.

---

### 3.5 Draw (`HRenderForce::draw`)

Render flow:

1) `ensureGL()` creates objects if needed
2) If constant size or rebuild needed → call `rebuild(P,V,w,h)`
3) Bind VAO
4) Enable depth test
5) Bind shader and apply lighting uniforms
6) Draw two contiguous index ranges:
   - lit part: first `m_iLightingCount` indices
   - unlit part: next `m_iUnlightingCount` indices (offset by `m_iLightingCount`)

Why two parts?
- Some forces want lighting, some want flat color.

#### Selection overlay

- Blend enabled
- `glDepthMask(GL_FALSE)` so overlay does not disturb depth buffer
- `glPolygonOffset(-1,-1)` pulls overlay forward (reduce z-fighting)
- then per-selected ID: draw only its stored ranges in both lit/unlit maps

---

## 4) `HRenderCoordinateGizmo` — buildArrowCoordinate + draw (corner widget)

This gizmo draws a **mini 3-axis arrow** in the corner.

### 4.1 Geometry build (`buildArrowCoordinate`)

It builds one arrow pointing +Z in local space:

- cylinder shaft from z0 to z1
- cone from z1 to z2

It only stores positions (no normals, no colors), because the shader is unlit color.

#### Upload
```cpp
glBindVertexArray(m_vao);
glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);

glBufferData(GL_ARRAY_BUFFER, V.size()*sizeof(QVector3D), V.data(), GL_STATIC_DRAW);
glBufferData(GL_ELEMENT_ARRAY_BUFFER, I.size()*sizeof(uint32_t), I.data(), GL_STATIC_DRAW);

glEnableVertexAttribArray(0);
glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(QVector3D),(void*)0);
glBindVertexArray(0);
```

Notes:
- `m_iTriCount` stores **index count** (not triangle count). It is used directly as `count` in `glDrawElements`.

---

### 4.2 Draw (`HRenderCoordinateGizmo::draw`)

This function temporarily changes OpenGL viewport/scissor to draw in a small square.

#### Steps

1) Save old viewport
```cpp
glGetIntegerv(GL_VIEWPORT, oldVP);
```

2) Set mini viewport + scissor
```cpp
glViewport(margin, margin, sizePx, sizePx);
glEnable(GL_SCISSOR_TEST);
glScissor(margin, margin, sizePx, sizePx);
```
- scissor guarantees you don’t draw outside the corner rectangle.

3) Clear depth only (inside the scissor)
```cpp
glClear(GL_DEPTH_BUFFER_BIT);
```
- This prevents the main scene depth buffer from hiding the gizmo.

4) Setup its own camera
```cpp
P.perspective(35°, 1.0, 0.01, 10);
V.translate(0,0,-2.2);
```

5) Draw 3 axes by rotating the *same arrow geometry*
```cpp
M.rotate(cameraQ);   // match current view orientation
M.rotate(axisRot);   // rotate arrow into X or Y or keep Z
M.scale(m_fScale);
MVP = P*V*M;
```

- X axis: rotate around Y by -90°
- Y axis: rotate around X by +90°
- Z axis: no extra rotation

6) Restore viewport/scissor state

---

## 5) Layering rules in **your** renderer (faces / edges / nodes / forces / gizmo)

### 5.1 What draws first vs last (actual order)

From `GLViewWindow::paintGL()` + `HRenderModel::draw()`:

1. **Faces** (`drawElement`)
2. **Edges** (`drawEdges`)
3. **Nodes** (`HRenderNode::draw`)
4. **Forces** (`HRenderForce::draw`)
5. **Coordinate gizmo** (corner viewport, depth cleared)

So in general:
- nodes appear on top of edges/faces,
- forces appear after model (but still depth-tested),
- gizmo always appears in its corner.

### 5.2 Why edges overlay faces

- faces: `glPolygonOffset(2,2)` pushes them back
- edges: `glDepthFunc(GL_LEQUAL)` and `glDepthMask(GL_FALSE)`
- plus: edges drawn after faces

### 5.3 Why nodes overlay edges/faces

- nodes drawn after edges
- nodes: `glPolygonOffset(-1,-1)` pulls points forward
- nodes: `glDepthFunc(GL_LEQUAL)` helps at equality

### 5.4 Forces vs model

- forces are drawn after the model, **with depth test enabled** and depth writes enabled.
- So forces are correctly occluded by the model if behind it.
- If you ever want “forces always visible”, you’d disable depth test (or draw twice: once depth-tested, once overlay).

### 5.5 Gizmo always visible

- scissor + dedicated viewport + depth-clear inside that scissor rectangle means:
  - it does not care about the scene depth
  - it is self-contained

---

## 6) Mini “cheat list” of critical GL calls (what to remember)

### Upload
- `glGenVertexArrays / glGenBuffers`
- `glBindVertexArray`
- `glBindBuffer(GL_ARRAY_BUFFER, vbo)`
- `glBufferData(...)`
- `glEnableVertexAttribArray(location)`
- `glVertexAttribPointer(location, ...)`
- `glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo)` + `glBufferData(...)` (when indices exist)

### Draw
- `shader.bind()`
- `setUniformValue(...)`
- `glBindVertexArray(vao)`
- `glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo)` (if needed)
- `glDrawArrays(...)` or `glDrawElements(...)`
- `glBindVertexArray(0)`
- `shader.release()`

### Overlay tricks
- `glPolygonOffset(FILL/LINE/POINT)` to avoid z-fighting or force “on top”
- `glDepthMask(GL_FALSE)` to avoid altering depth in overlay passes
- `glDepthFunc(GL_LEQUAL)` to allow equality
- `glEnable(GL_BLEND)` + `glBlendFunc(...)` for translucent overlays

---

If you want, I can also add an “annotated call stack” for a single frame:
`paintGL -> HRenderModel::draw -> drawElement/drawEdges/drawNodes -> drawForces -> drawGizmo`,
including which VAO/EBO is bound at each line.
