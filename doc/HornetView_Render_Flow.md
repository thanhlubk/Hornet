# GLViewWindow.cpp / GLViewWindow.h — OpenGL Rendering Flow (step‑by‑step)

This document explains **how the code in `GLViewWindow` renders** your mesh, forces, and coordinate gizmo using OpenGL (via Qt’s `QOpenGLWindow`), with **line-by-line / call-by-call** explanations of the most important functions:

- `GLViewWindow::initializeGL()`
- `GLViewWindow::setMesh(...)`
- `GLViewWindow::paintGL()`

…and the helper classes that participate in that flow:

- `HRenderModel` → `HRenderNode` + `HRenderElement`
- `HRenderForce`
- `HRenderCoordinateGizmo`
- `HViewCamera`
- `HViewLighting`
- `HViewSelectionManager`
- Picking helpers: `convertWorldToScreen`, `convertScreenToWorld`, `getHitPosition`, `selectAtPoint`, `selectInRect`

---

## 0) The big picture: what renders, in what order?

Each frame (`paintGL`) renders in this order:

1. **Clear** color + depth buffers.
2. Compute camera matrices:
   - `P` = projection matrix
   - `V` = view matrix
3. Draw main mesh (**model**):
   - element faces (triangles)
   - element edges (lines)
   - nodes (points)
4. Draw **forces** (arrow meshes).
5. Draw **coordinate gizmo** (mini axes) in a small bottom-left viewport.
6. Draw **2D marquee rectangle** using `QPainter` if the user is dragging selection.

---

## 1) Qt → OpenGL lifecycle (when do these functions run?)

### 1.1 Construction (`GLViewWindow::GLViewWindow`)
- Creates the “controller” objects:
  - `m_pCamera` (`HViewCamera`)
  - `m_pLighting` (`HViewLighting`)
  - `m_pSelection` (`HViewSelectionManager`)
- Creates the “renderer” objects:
  - `m_pRenderModel` (`HRenderModel`) for mesh nodes + elements
  - `m_pRenderForce` (`HRenderForce`) for force arrows
  - `m_pRenderCoordinate` (`HRenderCoordinateGizmo`) for mini axes
- **No OpenGL objects are created here yet**. At construction time, there is usually **no current OpenGL context**.

### 1.2 OpenGL context becomes available (`initializeGL`)
Qt calls `initializeGL()` once, when the OpenGL context for this window is created and made current.

This is where you may safely call:
- `glGenBuffers`, `glGenVertexArrays`, shader compilation, etc.

### 1.3 Data comes in (`setMesh`)
Your app calls `setMesh(nodes, elements)` whenever a new mesh should be shown.
This builds CPU caches for picking and calls the renderers to upload the mesh data to the GPU.

### 1.4 Rendering loop (`paintGL`)
Qt calls `paintGL()` whenever the window needs repainting:
- after `update()`
- after resizing / exposing
- after camera/lighting/selection signals trigger an `update()`

### 1.5 Cleanup (`destroyGLObjects` and destructors)
When the OpenGL context is about to be destroyed (or the window is destroyed),
you must delete VAOs/VBOs/programs **while a context is current**.

---

## 2) Shader attribute/uniform mapping (important for understanding VAO/VBO setup)

Your GLSL uses explicit attribute locations:

### Nodes (`VSNode.glsl`)
```glsl
layout(location=0) in vec3 aPos;
layout(location=1) in vec4 aCol;
```
So the C++ binds:
- attrib 0 → position VBO (vec3)
- attrib 1 → color VBO (vec4)

### Elements (`VSElement.glsl`)
```glsl
layout(location=0) in vec3 aPos;
layout(location=2) in vec3 aNrm;
```
So the C++ binds:
- attrib 0 → position VBO
- attrib 2 → normal VBO

### Forces (`VSForce.glsl`)
```glsl
layout(location=0) in vec3 aPos;
layout(location=1) in vec4 aColor;
layout(location=2) in vec3 aNormal;
```

### Coordinate gizmo (`VSCoordinate.glsl`)
```glsl
layout(location=0) in vec3 aPos;
```

Common uniforms:
- `uMVP` = `P * V` (or `P * V * M`)
- `uView` = `V` (used to transform normals/positions into view space)
- Lighting uniforms: `uAmbOn`, `uAmbColor`, `uAmbI`, etc. via `HViewLighting::applyTo`

---

## 3) `GLViewWindow::initializeGL()` — step-by-step

Code:

```cpp
void GLViewWindow::initializeGL()
{
    GLenum err = glewInit();
    if (err != GLEW_OK)
        return;

    initializeOpenGLFunctions();
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_PROGRAM_POINT_SIZE);

    if (auto *ctx = context()) {
        connect(ctx, &QOpenGLContext::aboutToBeDestroyed,
                this, [this]() { destroyGLObjects(); },
                Qt::DirectConnection);
    }

    m_pRenderModel->initialize();
    m_pRenderCoordinate->initialize();
    m_pRenderForce->initialize();

    connect(m_pLighting, &HViewLighting::lightingChanged, this, [this]() { update(); });
    connect(m_pCamera, &HViewCamera::viewChanged, this, [this]() { update(); });
    connect(m_pSelection, &HViewSelectionManager::selectionChanged, this, [this]() { update(); });
    connect(m_pRenderCoordinate, &HRenderCoordinateGizmo::dataChanged, this, [this]() { update(); });
    connect(m_pRenderForce, &HRenderForce::dataChanged, this, [this]() { update(); });
    connect(m_pRenderModel, &HRenderModel::settingChanged, this, [this]() { update(); });
}
```

### Step 1 — Initialize GLEW
1. `glewInit()`
   - GLEW loads modern OpenGL function pointers for your current context.
   - If this fails, you return early and nothing will render.

### Step 2 — Initialize Qt’s OpenGL function wrappers
2. `initializeOpenGLFunctions()`
   - Provided by `QOpenGLExtraFunctions`.
   - Ensures Qt has resolved function pointers for the context too (so you can call `gl*` via the class).

### Step 3 — Enable global OpenGL states
3. `glEnable(GL_MULTISAMPLE);`
   - Turns on MSAA (anti-aliasing) **if** your context was created with samples > 0.

4. `glEnable(GL_DEPTH_TEST);`
   - Enables depth testing so closer fragments win.

5. `glEnable(GL_PROGRAM_POINT_SIZE);`
   - Allows shaders to set `gl_PointSize` (used for node rendering).

### Step 4 — Connect context destruction to cleanup
6. `connect(ctx, &QOpenGLContext::aboutToBeDestroyed, ...)`
   - Ensures `destroyGLObjects()` runs while the context still exists.
   - Uses `Qt::DirectConnection` so cleanup happens immediately in the context’s thread.

### Step 5 — Initialize renderers (this is where VAOs/VBOs and shaders are created)
7. `m_pRenderModel->initialize();`
8. `m_pRenderCoordinate->initialize();`
9. `m_pRenderForce->initialize();`

Each of those calls:
- compiles shaders
- allocates VAO/VBO/EBO IDs with `glGen*`
- sets up attribute pointers
- (for the model) uploads initial empty buffers

### Step 6 — Hook signals to request redraw
All those `connect(..., update())` lines mean:
- camera changes (mouse move, wheel, key movement) → `paintGL` again
- lighting change → redraw
- selection change → redraw
- renderer settings/data change → redraw

---

## 4) `GLViewWindow::setMesh(...)` — step-by-step

Code:

```cpp
void GLViewWindow::setMesh(const std::vector<Node> &nodes,
                           const std::vector<Element> &elements)
{
    m_mapNodeIdPos.clear();
    for (const auto &n : nodes)
        m_mapNodeIdPos[n.id] = QVector3D(n.x, n.y, n.z);

    m_elements = elements;
    for (size_t i = 0; i < m_elements.size(); ++i)
        if (m_elements[i].id < 0)
            m_elements[i].id = int(i);

    m_pRenderModel->setMesh(nodes, elements);

    fitView();
    m_pCamera->setFocus(m_pRenderModel->center());

    update();
}
```

### Step 1 — CPU cache for picking
1. `m_mapNodeIdPos[n.id] = QVector3D(n.x, n.y, n.z);`
   - This map is used by selection/picking code:
     - `selectAtPoint` (node distance test)
     - `getHitPosition` (ray-triangle tests using element triangles)

### Step 2 — Ensure every element has a stable ID
2. `if (m_elements[i].id < 0) m_elements[i].id = int(i);`
   - Rendering batches and selection maps use `elementId`.
   - This ensures selection works even if the caller didn’t assign IDs.

### Step 3 — Hand the mesh to GPU-side renderers
3. `m_pRenderModel->setMesh(nodes, elements);`
   - This is the main “upload mesh to GPU” entry point.
   - Internally it calls:
     - `HRenderNode::setNodes(nodes)` → uploads point data
     - `HRenderElement::setElements(nodes, elements)` → builds triangles/edges, computes normals, uploads buffers

### Step 4 — Frame the camera
4. `fitView();`
   - Uses the mesh’s computed bounding box to place the camera.

5. `m_pCamera->setFocus(m_pRenderModel->center());`
   - Sets orbit focus point to mesh center.

### Step 5 — Trigger a repaint
6. `update();`
   - Schedules Qt to call `paintGL()` soon.

---

## 5) `GLViewWindow::paintGL()` — step-by-step

Code:

```cpp
void GLViewWindow::paintGL()
{
    glClearColor(0.12f, 0.12f, 0.14f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    const QMatrix4x4 P = m_pCamera->projectionMatrix(width(), height());
    const QMatrix4x4 V = m_pCamera->viewMatrix();

    m_pRenderModel->draw(P, V, m_pLighting);
    m_pRenderForce->draw(P, V, m_pLighting, width(), height());
    m_pRenderCoordinate->draw(m_pCamera->rotation(), width(), height());

    if (m_bDragSelect)
    {
        QRectF rect(...);  // based on drag start/current
        QPainter painter(this);
        ...
        painter.drawRect(rect);
    }
}
```

### Step 1 — Clear the framebuffer
1. `glClearColor(r,g,b,a)`
   - Sets the color used by `glClear(GL_COLOR_BUFFER_BIT)`.

2. `glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)`
   - Clears the color buffer and depth buffer for a new frame.

### Step 2 — Compute camera matrices
3. `P = camera->projectionMatrix(w,h)`
   - Either perspective (`P.perspective(...)`) or orthographic (`P.ortho(...)`).

4. `V = camera->viewMatrix()`
   - `lookAt(cameraPos, cameraPos + forward, up)` computed from quaternion.

### Step 3 — Draw the model (mesh)
5. `m_pRenderModel->draw(P, V, *m_pLighting)`
   - Dispatches to element renderer and node renderer:
     - `HRenderElement::drawElement(P,V,lighting)` (faces)
     - `HRenderElement::drawEdges(P,V)` (wireframe)
     - `HRenderNode::draw(P,V)` (points)

### Step 4 — Draw forces (arrows)
6. `m_pRenderForce->draw(P,V,lighting,w,h)`
   - If constant screen size is enabled, it may rebuild geometry each frame.

### Step 5 — Draw coordinate gizmo
7. `m_pRenderCoordinate->draw(cameraRotation, w, h)`
   - Changes viewport to a small square in the bottom-left.
   - Clears depth only in that area.
   - Draws 3 colored arrows for X/Y/Z.

### Step 6 — Draw 2D marquee overlay
8. If dragging selection (`m_bDragSelect`), it uses `QPainter` to draw a semi-transparent rectangle.
   - This is not “OpenGL geometry”; it is a Qt 2D overlay rendered after the 3D pass.

---

# 6) The renderer internals (where the real OpenGL buffer setup happens)

## 6.1 `HRenderElement::initialize()` — creating VAO/VBO/EBO + shaders

Key code:

```cpp
glGenVertexArrays(1, &m_vao);
glGenBuffers(1, &m_vboPosition);
glGenBuffers(1, &m_vboNormal);
glGenBuffers(1, &m_eboTri);
glGenBuffers(1, &m_eboEdge);
```

### Example explanation (your requested style)

1. **Create a vertex array object**
   ```cpp
   glGenVertexArrays(1, &m_vao);
   ```
   - **`1`** = how many VAO names (IDs) to generate.
   - **`&m_vao`** = pointer to where OpenGL will write the generated ID.
   - After the call, `m_vao` holds an integer “handle” identifying that VAO inside the OpenGL context.

2. **Create a vertex buffer for positions**
   ```cpp
   glGenBuffers(1, &m_vboPosition);
   ```
   - **`1`** = create exactly one buffer object name.
   - **`&m_vboPosition`** = address of a `GLuint` that will receive the buffer ID.
   - Result: `m_vboPosition` becomes something like `7` (example), and that number is used in later binds.

3. **Create other buffers**
   - `m_vboNormal` stores normals
   - `m_eboTri` stores triangle indices
   - `m_eboEdge` stores edge line indices

### What `initialize()` does overall
- Compile/link `m_shaderProgram` (element shaders).
- Allocate IDs for VAO/VBO/EBO.
- Call `upload({}, {}, {}, {}, {}, {})` to set a valid VAO state even with empty data.

---

## 6.2 `HRenderElement::upload(...)` — uploading vertex/index buffers + describing layout

Key code (annotated):

```cpp
glBindVertexArray(m_vao);
```
1. **Bind VAO**
   - Makes `m_vao` the “current VAO”.
   - All subsequent `glVertexAttribPointer` / `glEnableVertexAttribArray` calls record into this VAO.

```cpp
glBindBuffer(GL_ARRAY_BUFFER, m_vboPosition);
```
2. **Bind the position VBO as the array buffer**
   - `GL_ARRAY_BUFFER` is the binding point used for vertex attributes.

```cpp
glBufferData(GL_ARRAY_BUFFER, pos.size()*sizeof(QVector3D), pos.data(), GL_STATIC_DRAW);
```
3. **Upload position data**
   - **target**: `GL_ARRAY_BUFFER` = “the buffer currently bound as array buffer”
   - **size**: number of bytes to allocate and fill
   - **data**: pointer to CPU memory (can be `nullptr` to allocate without upload)
   - **usage**: `GL_STATIC_DRAW` = “uploaded once, used many times”

```cpp
glEnableVertexAttribArray(0);
glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(QVector3D), (void*)0);
```
4. **Describe attribute 0 (position)**
   - `0` = attribute location index (`layout(location=0) in vec3 aPos;`)
   - `3` = number of components per vertex (x,y,z)
   - `GL_FLOAT` = each component is a 32-bit float
   - `GL_FALSE` = do not normalize (only matters for integer formats)
   - `stride` = `sizeof(QVector3D)` bytes between consecutive vertices
   - `pointer` = offset inside the buffer (0 = start)

Then normals are uploaded similarly at attribute location 2:

```cpp
glEnableVertexAttribArray(2);
glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(QVector3D), (void*)0);
```

Triangle indices:

```cpp
glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_eboTri);
glBufferData(GL_ELEMENT_ARRAY_BUFFER, triIdx.size()*sizeof(uint32_t), triIdx.data(), GL_STATIC_DRAW);
```

Edge indices:

```cpp
glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_eboEdge);
glBufferData(GL_ELEMENT_ARRAY_BUFFER, lineIdx.size()*sizeof(uint32_t), lineIdx.data(), GL_STATIC_DRAW);
```

**Important note:** `GL_ELEMENT_ARRAY_BUFFER` binding is part of VAO state.
This code later re-binds the desired EBO before drawing (good).

---

## 6.3 `HRenderElement::drawElement(...)` — rendering faces

High-level flow:

1. Compute MVP:
   ```cpp
   QMatrix4x4 MVP = P * V;
   ```

2. Enable depth, disable culling (two-sided), enable polygon offset:
   - Polygon offset reduces z-fighting between filled faces and later edges.

3. Bind shader and set uniforms:
   ```cpp
   m_shaderProgram.bind();
   m_shaderProgram.setUniformValue("uMVP", MVP);
   m_shaderProgram.setUniformValue("uView", V);
   m_shaderProgram.setUniformValue("uLit", 1);
   lighting.applyTo(m_shaderProgram);
   ```

4. Bind VAO + triangle EBO:
   ```cpp
   glBindVertexArray(m_vao);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_eboTri);
   ```

5. Draw each “face batch”:
   ```cpp
   for (const auto& b : m_vecElementBatches)
   {
       m_shaderProgram.setUniformValue("uColor", ...);
       const void* off = (void*)(sizeof(uint32_t) * b.first);
       glDrawElements(GL_TRIANGLES, b.count, GL_UNSIGNED_INT, off);
   }
   ```

### `glDrawElements(...)` parameters
- **mode**: `GL_TRIANGLES` → interpret indices as triangles (3 indices per triangle)
- **count**: how many indices to read from the EBO for this draw
- **type**: `GL_UNSIGNED_INT` → indices are 32-bit unsigned ints (`uint32_t`)
- **indices (pointer/offset)**:
  - When an EBO is bound, the last parameter is treated as a **byte offset**
    into that EBO, not a CPU pointer.
  - `b.first` is measured in “index units”; multiplying by `sizeof(uint32_t)`
    converts it to a byte offset.

Selection overlay:
- second draw pass with blending + different color
- uses precomputed index ranges per element ID (`m_mapElementTriRanges`)

---

## 6.4 `HRenderElement::drawEdges(...)` — rendering wireframe

Key differences:
- `glDepthMask(GL_FALSE)` so edges do not write depth (prevents them from blocking later draws)
- `glDepthFunc(GL_LEQUAL)` so edges at same depth as faces still show
- Draw mode is `GL_LINES`
- Uses `glLineWidth(m_fEdgeWidth)` and `glEnable(GL_LINE_SMOOTH)`

---

## 6.5 `HRenderNode` — points (nodes)

### Initialize
Creates:
- VAO
- position VBO
- color VBO
- selection EBO (created later in `upload`)

### Upload
- attribute 0: position (`vec3`)
- attribute 1: color (`vec4`)

### Draw
- `glDrawArrays(GL_POINTS, 0, m_iNumNode)`
- `GL_PROGRAM_POINT_SIZE` is enabled globally so shader’s `gl_PointSize` works.

Selection overlay:
- uses `glDrawElements(GL_POINTS, m_iSelectionCount, GL_UNSIGNED_INT, 0)`
- selection EBO stores indices of the selected point vertices.

---

## 6.6 `HRenderModel` — the “mesh renderer” that orchestrates nodes+elements

- `initialize()` calls `m_renderNode.initialize()` and `m_renderElement.initialize()`.
- `setMesh(nodes,elements)` calls both `setNodes` and `setElements`.
- It also computes:
  - `m_vCenter`
  - `m_fFrameRadius`
  used by `GLViewWindow::fitView()` / `HViewCamera::frameBounds()`.

---

## 6.7 `HRenderForce` — force arrow meshes

This renderer differs from the model in an important way:

- If **constant screen size** is enabled, it rebuilds geometry each frame so the arrow stays the same pixel size regardless of camera distance.

### Initialize
Creates VAO + buffers:
```cpp
glGenVertexArrays(1, &m_vao);
glGenBuffers(1, &m_vboPosition);
glGenBuffers(1, &m_vboNormal);
glGenBuffers(1, &m_vboColor);
glGenBuffers(1, &m_ebo);
```

### Rebuild (CPU side)
- For each `ForceArrow`:
  - chooses an orthonormal basis around the arrow direction
  - computes world-units-per-pixel at the arrow position
  - builds a cylinder (shaft) + cone (head) triangle mesh
  - splits indices into “lit” and “unlit” groups

### Upload (GPU side)
Uses `GL_DYNAMIC_DRAW` because it can change every frame.

### Draw
- One draw call for lit indices (`uLit=1`)
- One draw call for unlit indices (`uLit=0`)
- Optional overlay for selection (draw only the ranges belonging to selected force IDs)

---

## 6.8 `HRenderCoordinateGizmo` — mini axes in its own viewport

Important ideas:
- Saves the current viewport.
- Sets a small viewport + scissor in bottom-left.
- Clears depth buffer only in that mini area so it’s always visible.
- Uses its own small projection/view matrices.
- Draws 3 arrows with different rotations and colors, *also rotated by camera orientation* so the gizmo matches your view.

---

# 7) Camera + lighting (how they affect drawing)

## 7.1 `HViewCamera`
The camera doesn’t call OpenGL directly. It produces matrices used by the renderers.

- `viewMatrix()` → `lookAt(position, position+forward, up)`
- `projectionMatrix(w,h)` → perspective or ortho matrix
- Mouse/keyboard events update:
  - `m_vCameraPosition`
  - `m_quaternion`
  - `m_vFocusPosition`

`GLViewWindow::paintGL()` reads these matrices every frame and passes them to renderers.

## 7.2 `HViewLighting`
Lighting doesn’t call OpenGL directly either.
It just sets shader uniforms consistently:

```cpp
lighting.applyTo(shaderProgram);
```

This fills:
- enable flags: `uAmbOn`, `uDifOn`, `uSpeOn`
- colors: `uAmbColor`, `uDifColor`, `uSpeColor`
- intensities and shininess

Both element and force shaders use these uniforms.

---

# 8) Selection & picking (how it fits into the rendering flow)

Selection affects rendering by enabling “overlay passes”:
- `HRenderNode` draws a second pass for selected nodes.
- `HRenderElement` draws a second pass for selected element faces.
- `HRenderForce` draws a second pass for selected force arrow ranges.

### Picking functions
- `convertWorldToScreen(...)`:
  - transforms a world point by `P*V`
  - divides by `w` to get NDC
  - maps NDC to Qt pixel coordinates
- `convertScreenToWorld(...)`:
  - does the inverse using `(P*V)^{-1}` (unproject)

- `getHitPosition(point, hit, elemId)`:
  - constructs a ray in world space from mouse position
  - tests against triangles (CPU ray-triangle intersection)
- `selectAtPoint(point)`:
  - for nodes: nearest projected node within a radius
  - for elements: uses ray hit test
- `selectInRect(rect)`:
  - for nodes: select nodes whose projected pixel coordinate lies inside the marquee

After picking, it calls:
```cpp
m_pSelection->applyTo(m_pRenderModel, m_pRenderForce);
```
which pushes selection IDs into the renderers (so they draw overlays).

---

# 9) Cleanup: `destroyGLObjects()` — why it’s written this way

```cpp
void GLViewWindow::destroyGLObjects()
{
    QOpenGLContext *ctx = context();
    if (!ctx) return;

    bool make = (QOpenGLContext::currentContext() != ctx);
    if (make) makeCurrent();

    m_pRenderForce->destroy();
    m_pRenderModel->destroy();
    m_pRenderCoordinate->destroy();

    if (make) doneCurrent();
}
```

### Why `makeCurrent()`?
OpenGL object deletion must happen **in a current context** that owns those objects.
Qt may call destruction while the context is not current, so this function ensures it is.

---

# 10) One “complete frame” flow summary (end-to-end)

1. **App calls `setMesh(nodes,elements)`**
   - caches nodes/elements for picking
   - `HRenderModel::setMesh` builds and uploads VBO/EBO data
2. **Qt triggers `paintGL()`**
   - clears buffers
   - gets P and V from camera
   - draws faces, edges, nodes
   - draws forces
   - draws coordinate gizmo
   - draws marquee (if dragging)
3. **User input changes camera**
   - mouse move updates camera quaternion/position
   - emits `viewChanged`
   - connected to `update()` → `paintGL()` runs again

---

If you want, I can also produce a **call graph diagram** (ASCII or Mermaid) of which functions call which, and where each OpenGL state change occurs.
