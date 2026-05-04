# HESolveCrackPropagation Solver Walkthrough

## 1. Purpose of this solver

`HESolveCrackPropagation` is a **2D XFEM crack propagation solver** built for:

- **linear elastic fracture mechanics**
- **stationary crack analysis**
- **stress intensity factor extraction**
- **automatic crack growth point prediction**

This implementation is based on:

- **2D Quad4 elements only**
- **small-strain linear elasticity**
- **plane stress or plane strain**
- **XFEM enrichment** for discontinuity and crack-tip singularity

The main classes are:

- `HESolveCrackPropagation` — top-level execution and crack extension orchestration
- `HESolveCrackPropagationModel` — model construction, enrichment classification, assembly, solve, SIF
- `HESolveCrackPropagationElement` and derived classes — standard and enriched element formulations
- `HESolveCrackPropagationNode` and derived classes — standard / Heaviside / asymptotic node data
- `HESolveCrackPropagationTools` — geometry, level-set sign, polygon split, triangulation, Gauss integration helpers

---

## 2. What this solver actually does in the current code

Even though the model has both `Static` and `Modal` enums, the **execution flow of `HESolveCrackPropagation::onExecute()` is clearly organized around static crack propagation**:

1. build the XFEM model
2. assemble stiffness
3. solve displacement
4. recover displacement/stress
5. write one static result step
6. compute `K1`, `K2`
7. compute the next crack point
8. extend the crack path

The top-level execute path does **not** build the mass matrix before calling `solve()`, and the result is always written as `LinearStatic`, followed by crack-growth post-processing. So the current practical use of this refactored solver is **static XFEM crack propagation**, not a complete modal XFEM workflow. 

---

## 3. High-level solver flow

The complete crack-propagation flow is:

1. Read nodes and Quad4 elements from the database
2. Read the user-supplied crack polyline(s)
3. Detect which elements are cut by the crack and which contain the crack tip
4. Classify nodes as:
 - standard
 - Heaviside enriched
 - asymptotically enriched
5. Create the correct XFEM element type:
 - standard
 - crack-body
 - crack-tip
 - blended
6. Assemble the global stiffness matrix
7. Apply nodal loads and displacement constraints
8. Reduce to free DOFs and solve the linear system
9. Recover nodal displacements
10. Recover element and nodal stresses
11. Evaluate stress intensity factors $K_I$ and $K_{II}$
12. Compute the next crack-growth direction and next point
13. Extend the crack polyline

---

## 4. Governing mechanics

This solver uses **2D small-strain linear elasticity**.

The strain vector is:

$$ \boldsymbol{\varepsilon}= \begin{bmatrix} \varepsilon_{xx} \\ \varepsilon_{yy} \\ \gamma_{xy} \end{bmatrix} $$

The stress vector is:

$$ \boldsymbol{\sigma}= \begin{bmatrix} \sigma_{xx} \\ \sigma_{yy} \\ \tau_{xy} \end{bmatrix} $$

The strain-displacement relation is:

$$ \boldsymbol{\varepsilon} = \mathbf{B}\mathbf{d}_e $$

and the constitutive law is:

$$ \boldsymbol{\sigma}=\mathbf{D}\boldsymbol{\varepsilon} $$

---

## 5. Plane stress / plane strain constitutive matrix

The solver builds a $3 \times 3$ material matrix `D`.

### Plane strain

$$ \mathbf{D}= \frac{E}{(1+\nu)(1-2\nu)} \begin{bmatrix} 1-\nu & \nu & 0 \\ \nu & 1-\nu & 0 \\ 0 & 0 & \frac{1-2\nu}{2} \end{bmatrix} $$

### Plane stress

$$ \mathbf{D}= \frac{E}{1-\nu^2} \begin{bmatrix} 1 & \nu & 0 \\ \nu & 1 & 0 \\ 0 & 0 & \frac{1-\nu}{2} \end{bmatrix} $$

This is exactly how `makeDMatrix()` is built in the model constructor. 

---

## 6. Base Quad4 finite element formulation

Before XFEM enrichment is added, the underlying element is a standard **bilinear 4-node quadrilateral**.

### 6.1 Natural coordinates

The natural coordinates are $(\xi,\eta)$, both in $[-1,1]$.

The standard Quad4 shape functions are:

$$ N_1 = \frac14(1-\xi)(1-\eta) $$

$$ N_2 = \frac14(1+\xi)(1-\eta) $$

$$ N_3 = \frac14(1+\xi)(1+\eta) $$

$$ N_4 = \frac14(1-\xi)(1+\eta) $$

The solver implements exactly these formulas in `shapeMatrixForOneGaussPoint()`. 

### 6.2 Standard shape matrix

For 2D displacement interpolation:

$$ \mathbf{u}(\xi,\eta)=\mathbf{N}(\xi,\eta)\mathbf{d}_e $$

with:

$$ \mathbf{N}= \begin{bmatrix} N_1 & 0 & N_2 & 0 & N_3 & 0 & N_4 & 0 \\ 0 & N_1 & 0 & N_2 & 0 & N_3 & 0 & N_4 \end{bmatrix} $$

### 6.3 Jacobian

The Jacobian maps natural derivatives to physical derivatives:

$$ \mathbf{J} = \frac{\partial(x,y)}{\partial(\xi,\eta)} $$

The solver computes `J` from the natural derivatives of the standard Quad4 shape functions and the nodal coordinates. 

### 6.4 Standard B matrix

The standard 2D small-strain block is:

$$ \mathbf{B}_i= \begin{bmatrix} N_{i,x} & 0 \\ 0 & N_{i,y} \\ N_{i,y} & N_{i,x} \end{bmatrix} $$

and the full $B$ matrix is all 4 node blocks concatenated.

The solver constructs the standard $B$ matrix in `bMatrixForOneGaussPoint()` using the Jacobian inverse. 

---

## 7. Why XFEM is needed

A standard FEM mesh has difficulty representing cracks because:

- the displacement field is **discontinuous across crack faces**
- the field near the crack tip is **singular**
- remeshing at each crack-growth step is expensive

XFEM solves this by enriching the displacement approximation **without remeshing the mesh to fit the crack**.

---

## 8. XFEM displacement approximation in this solver

The conceptual XFEM approximation is:

$$ \mathbf{u}(x) = \sum_{i \in I} N_i(x)\mathbf{u}_i + \sum_{j \in J_H} N_j(x)\big(H(x)-H(x_j)\big)\mathbf{a}_j + \sum_{k \in J_T} N_k(x)\sum_{m=1}^{4}\big(F_m(x)-F_m(x_k)\big)\mathbf{b}_{km} $$

where:

- $I$ = all standard nodes
- $J_H$ = Heaviside-enriched nodes
- $J_T$ = crack-tip enriched nodes
- $H(x)$ = discontinuous crack-face enrichment
- $F_m(x)$ = crack-tip branch enrichment functions
- $\mathbf{u}_i$ = standard nodal DOFs
- $\mathbf{a}_j$ = Heaviside enrichment DOFs
- $\mathbf{b}_{km}$ = crack-tip enrichment DOFs

This is exactly the logic implemented through the **node classes**, **element classes**, and **extra columns appended to the B/N matrices**. 

---

## 9. Enrichment functions in this solver

## 9.1 Heaviside enrichment

The Heaviside enrichment models the **jump in displacement across the crack faces**.

Conceptually:

$$ H(x)= \begin{cases} +1, & \text{one side of crack} \\ -1, & \text{other side of crack} \end{cases} $$

In this implementation, `getLevelSet()` does **not compute a signed distance magnitude**. 
Instead, it builds a polygonal side region from the crack and returns a **sign indicator**:

- `-1` on one side
- `+1` on the other side 

For a Heaviside-enriched node, the enrichment contribution uses:

$$ H(x)-H(x_i) $$

This subtraction ensures the enriched nodal DOF has the partition-of-unity property and does not duplicate the standard nodal value.

In the code, the crack-body and blended elements build extra columns using:

- the standard derivative block
- multiplied by `gptsLevelSet - nodes_[i]->levelSet()`

which is the implemented version of:

$$ H(x)-H(x_i) $$

## 9.2 Crack-tip asymptotic enrichment

Near the crack tip, the elastic field behaves like $\sqrt{r}$-type singular functions.

The solver uses 4 branch functions:

$$ F_1(r,\theta)=\sqrt{r}\sin\frac{\theta}{2} $$

$$ F_2(r,\theta)=\sqrt{r}\cos\frac{\theta}{2} $$

$$ F_3(r,\theta)=\sqrt{r}\sin\frac{\theta}{2}\sin\theta $$

$$ F_4(r,\theta)=\sqrt{r}\cos\frac{\theta}{2}\sin\theta $$

These are implemented in `HESolveCrackPropagationCrackTipElement::branch()`. 

Their derivatives with respect to the physical coordinates are computed in `branchDerivative()`, using a local crack-tip frame rotated by the crack-tip angle `alpha`. 

The asymptotic enrichment contribution uses:

$$ F_m(x)-F_m(x_i) $$

again to maintain the partition-of-unity style enrichment.

---

## 10. Node classes and DOFs

The solver classifies each node into one of three classes:

- `Standard`
- `Heaviside`
- `Asymptotic` 

### DOF count by node class

Each node always has standard translational DOFs $(u_x,u_y)$, then may receive enrichment DOFs.

- **Standard node**: 2 DOFs
- **Heaviside node**: 4 DOFs total = 2 standard + 2 Heaviside
- **Asymptotic node**: 10 DOFs total = 2 standard + 8 crack-tip DOFs

That is exactly how `dofs_` is accumulated in the model constructor. 

---

## 11. How nodes and elements are classified

The model reads all Quad4 elements, then tests whether a crack polyline intersects the element polygon.

If an element is intersected:

- if the number of intersection points is **2**, its nodes are marked as **Heaviside** unless already asymptotic
- otherwise, its nodes are marked as **Asymptotic**

So the current logic is:

- **cut body of crack** → Heaviside enrichment
- **tip-containing region / special intersection case** → asymptotic enrichment

This classification is done before element objects are created. 

---

## 12. Element types in this XFEM solver

Once node classes are known, the model creates one of four element classes:

### 12.1 Standard element
All 4 nodes are standard. 
No enrichment. 
Local DOFs = 8. 

### 12.2 Crack-body element
All 4 nodes are Heaviside-enriched. 
Used where the crack fully cuts the element interior. 
Local DOFs = 16. 

### 12.3 Crack-tip element
All 4 nodes are asymptotically enriched. 
Used in the crack-tip region. 
Local DOFs = 40. 

### 12.4 Blended element
Used for mixed configurations where standard and enriched nodes coexist, or where enrichment needs a transition zone. 
Its local DOF count is:

- 8 base DOFs
- +2 for each Heaviside node
- +8 for each asymptotic node

This is computed dynamically in the constructor. 

---

## 13. Standard element formulation

For the standard Quad4 element:

$$ \mathbf{K}_e = \int_{\Omega_e} t\,\mathbf{B}^T\mathbf{D}\mathbf{B}\;d\Omega $$

$$ \mathbf{M}_e = \int_{\Omega_e} t\rho\,\mathbf{N}^T\mathbf{N}\;d\Omega $$

where:

- $t$ = thickness
- $\rho$ = density

The standard element uses a **2×2 Gauss rule** from `getQuadGaussPoint(1, 2)`. 

---

## 14. Crack-body element formulation

The crack-body element is used when the crack splits the element interior into two subdomains.

### 14.1 Domain split

The solver builds two sub-polygons:

- `firstSubPolygon`
- `secondSubPolygon`

separated by the crack segment inside the element.

These polygons are converted from global coordinates to natural coordinates, triangulated by `getTriMesh()`, and then integrated using triangle Gauss points. 

### 14.2 Heaviside-enriched B matrix

The total $B$ matrix is:

$$ \mathbf{B}_{XFEM} = [\mathbf{B}_{std}\;\;\mathbf{B}_{H}] $$

For one Heaviside-enriched node $i$, the extra block is based on:

$$ \nabla\big(N_i(H(x)-H(x_i))\big) $$

In this implementation, because the crack interior is integrated as separate subdomains with constant sign, the enrichment block is assembled from:

- standard shape derivatives
- multiplied by $H(x)-H(x_i)$

This is implemented by the factor:

$$ gptsLevelSet - nodes_[i]->levelSet() $$

inside `HESolveCrackPropagationCrackBodyElement::bMatrixForOneGaussPoint()`. 

### 14.3 Heaviside-enriched shape matrix

Similarly, the enriched displacement interpolation uses:

$$ N_i(H(x)-H(x_i)) $$

This is implemented in `shapeMatrixForOneGaussPoint()`. 

---

## 15. Crack-tip element formulation

The crack-tip element adds asymptotic near-tip functions.

### 15.1 Local crack-tip frame

The crack-tip local coordinate system is built using the crack-tip angle:

$$ \alpha = \tan^{-1}\left(\frac{y_{tip}-y_{prev}}{x_{tip}-x_{prev}}\right) $$

The solver rotates points into this frame using:

$$ \mathbf{Q}= \begin{bmatrix} \cos\alpha & \sin\alpha \\ -\sin\alpha & \cos\alpha \end{bmatrix} $$

This is how $r,\theta$ are measured relative to the local crack-tip direction. 

### 15.2 Tip-enriched approximation

For one enriched node $i$ and one branch function $F_m$, the added field is:

$$ N_i(F_m(x)-F_m(x_i)) $$

The derivative is:

$$ \nabla\left(N_i(F_m-F_{m,i})\right) = \nabla N_i(F_m-F_{m,i}) + N_i\nabla F_m $$

That exact structure appears in the implementation:

- first term from standard $B$
- second term from `dBdx`, `dBdy`

inside `HESolveCrackPropagationCrackTipElement::bMatrixForOneGaussPoint()`. 

### 15.3 Crack-tip integration

The crack-tip element subdivides the integration domain into triangles around the crack-tip geometry and uses **triangle order-3 integration**. 

---

## 16. Blended element formulation

Blended elements are transition elements between standard and enriched zones.

They may contain:

- standard nodes
- Heaviside nodes
- asymptotic nodes

So the element formulation appends only the enrichment blocks required by the node class of each node.

Conceptually:

$$ \mathbf{B}_{blend}=[\mathbf{B}_{std}\;\mathbf{B}_{H}\;\mathbf{B}_{tip}] $$

but with only the necessary columns actually present.

This is implemented in `HESolveCrackPropagationBlendedElement::bMatrixForOneGaussPoint()` and `shapeMatrixForOneGaussPoint()`. 

### Why blended elements are needed

Without blended elements, the approximation would jump abruptly between enriched and unenriched regions, causing poor transition behavior and reduced accuracy. 
Blended elements provide a gradual transition from standard FEM to enriched XFEM.

---

## 17. Geometry tools used by XFEM

The XFEM formulation needs extra geometry utilities beyond standard FEM.

The tools file provides:

- segment intersection
- point-in-polygon test
- crack-polygon intersection extraction
- polygon ordering
- ear-clipping triangle meshing
- natural/global coordinate conversion
- quad subdivision and Gauss point generation
- triangle Gauss integration
- level-set side classification 

These are essential because cut elements and tip elements are integrated over **subdomains**, not just over the full original Quad4 area.

---

## 18. Global assembly

Once element objects are created, each element forms its local stiffness matrix and contributes it to the global sparse stiffness matrix.

The assembly uses `boolean_`, which maps local element DOFs to global DOFs.

### Standard base mapping

For node index `a`, the base DOFs are:

$$ [2a,\;2a+1] $$

### Heaviside enriched DOFs

Heaviside extra DOFs are placed after all standard nodal DOFs:

$$ 2N + 2j,\quad 2N + 2j+1 $$

where:
- $N$ = total node count
- $j$ = Heaviside enriched node index in the Heaviside list

### Asymptotic enriched DOFs

Asymptotic DOFs are placed after standard + Heaviside enriched DOFs:

$$ 2N + 2N_H + 8k + d $$

where:
- $N_H$ = number of Heaviside nodes
- $k$ = asymptotic node index in the asymptotic list
- $d=0,1,\dots,7$

This is exactly how the model builds the global DOF map. 

---

## 19. Loads and constraints

### 19.1 Loads

The solver reads `HILbcForce` and fills the global load vector:

$$ \mathbf{F} $$

using nodal forces in $x$ and $y$. 

### 19.2 Constraints

The solver reads `HILbcConstraint` and marks standard translational DOFs as:

- `0.0` = fixed
- `1.0` = free

These are stored in `boundaryCondition_`. 

### Important implementation note

In this code, the constraints are directly applied only to the **standard translational DOFs** `idx*2` and `idx*2+1`. 
This is one reason XFEM systems can become singular if enrichment DOFs are not sufficiently supported by the formulation or by the crack topology. 

---

## 20. Static solve

The reduced linear system is:

$$ \mathbf{K}_{red}\mathbf{u}_{red}=\mathbf{F}_{red} $$

The solver:

1. identifies free DOFs
2. builds reduced stiffness matrix
3. builds reduced load vector
4. solves with:
 - `SimplicialLDLT`
 - fallback to `SparseLU`

Then it reconstructs the full DOF vector `root_` by scattering the reduced solution back into free positions. 

---

## 21. About modal analysis in this XFEM model

The model class contains a modal branch using:

$$ \mathbf{K}_{red}\phi = \lambda \mathbf{M}_{red}\phi $$

and solves it with a **dense generalized self-adjoint eigensolver** after converting reduced sparse matrices to dense matrices. 

However, the current top-level crack-propagation execution path does **not** assemble the mass matrix before calling `solve()`, and it immediately proceeds into static-style stress recovery and SIF evaluation. So, in the current integrated application flow, the meaningful path is **static crack propagation**. 

---

## 22. Displacement recovery

After solving:

- `root_` holds the global DOF values
- each element extracts its local displacement vector using `boolean_`
- each node receives:

$$ u_x = d[2i],\quad u_y = d[2i+1] $$

For enriched elements, `createDisplacement()` may additionally store sub-polygon displacements for split domains, especially in crack-body and blended elements. 

---

## 23. Stress computation

The elemental stress is:

$$ \boldsymbol{\sigma}=\mathbf{D}\mathbf{B}\mathbf{d}_e $$

with:

$$ \boldsymbol{\sigma}= \begin{bmatrix} \sigma_{xx} \\ \sigma_{yy} \\ \tau_{xy} \end{bmatrix} $$

### 23.1 Von Mises stress

The implementation uses:

$$ \sigma_{vm} = \sqrt{ \frac{ (\sigma_{xx}-\sigma_{yy})^2 + \sigma_{xx}^2 + \sigma_{yy}^2 + 6\tau_{xy}^2 }{2} } $$

and stores:

$$ [\sigma_{xx},\sigma_{yy},\tau_{xy},\sigma_{vm}] $$

### 23.2 Nodal stress averaging

Each node stores a list of stress contributions from neighboring elements. 
The final nodal stress is the arithmetic average:

$$ \sigma_{node} = \frac{1}{n}\sum_{i=1}^{n}\sigma^{(i)} $$

This smoothing is done in `HESolveCrackPropagationNode::createStress()`. 

---

## 24. Stress intensity factor extraction

After the static solution is known, the solver computes SIFs using an **interaction integral / J-integral style formulation** around each crack tip.

### 24.1 Circular extraction region

For each crack tip:

1. find nodes inside a radius
2. find elements around that circle
3. keep elements crossing the circle boundary

This creates the integration domain for SIF extraction. 

### 24.2 Interaction integral idea

The solver computes two interaction integrals, one for each auxiliary mode:

- mode I auxiliary field
- mode II auxiliary field

At Gauss points it evaluates:

- real strain/stress field
- real displacement gradients
- auxiliary stress field
- auxiliary displacement gradients
- weight-function gradient

and accumulates an integral vector:

$$ \mathbf{I} = [I_1, I_2] $$

### 24.3 Convert interaction integrals to $K_I$, $K_{II}$

For plane strain:

$$ K_I = I_1 \frac{E}{2(1-\nu^2)} $$

$$ K_{II} = I_2 \frac{E}{2(1-\nu^2)} $$

For plane stress:

$$ K_I = I_1 E $$

$$ K_{II} = I_2 E $$

This conversion is implemented directly in `createSIF()`. 

---

## 25. Crack growth direction

The solver computes the crack-growth angle using a **maximum circumferential stress style formula** based on $K_I$ and $K_{II}$:

$$ \theta = 2\tan^{-1} \left( \frac{-2K_{II}/K_I}{1+\sqrt{1+8(K_{II}/K_I)^2}} \right) $$

Then:

$$ \gamma = \alpha + \theta $$

and the next point is:

$$ x_{next}=x_{tip}+\Delta a\cos\gamma $$

$$ y_{next}=y_{tip}+\Delta a\sin\gamma $$

where $\Delta a$ is `growthStepLength`. 

---

## 26. Crack extension in the top-level execute class

After `createSIF()` is called for each crack tip:

1. `K1` and `K2` are stored
2. `nextPoint()` is read
3. the result is saved into `XfemCrack`
4. the crack polyline is extended by inserting/appending the new point at the matching crack-tip end

This is handled in `HESolveCrackPropagation::extendCrack()`. 

---

## 27. Complete pipeline of this XFEM crack-propagation solver

## Step 1 — Read mesh and crack
- read all nodes
- read all Quad4 elements
- read crack polyline(s)

## Step 2 — Detect enrichment regions
- detect element/crack intersections
- classify node type:
 - standard
 - Heaviside
 - asymptotic

## Step 3 — Create element objects
- standard element
- crack-body element
- crack-tip element
- blended element

## Step 4 — Build global DOF map
- 2 standard DOFs/node
- +2 Heaviside DOFs where needed
- +8 crack-tip DOFs where needed

## Step 5 — Assemble global stiffness
- compute element Gauss points
- compute element $B$ matrices
- integrate $K_e$
- assemble sparse global $K$

## Step 6 — Apply loads and constraints
- build global $F$
- identify free DOFs
- build reduced system

## Step 7 — Solve static displacement

$$ K_{red}u_{red}=F_{red} $$

## Step 8 — Recover displacement and stress
- recover nodal displacement
- compute element stress
- average nodal stress

## Step 9 — Extract fracture parameters
- find crack-tip ring of elements
- evaluate interaction integral
- compute $K_I$, $K_{II}$

## Step 10 — Predict crack growth
- compute propagation angle
- compute next crack point
- extend crack polyline

---

## 28. File-to-responsibility map

- `HESolveCrackPropagation.cpp` 
 Top-level crack-propagation execution, result writing, crack extension.

- `HESolveCrackPropagationModel.cpp` 
 Builds the XFEM model, classifies nodes/elements, assembles stiffness, solves the system, computes SIF and next crack point.

- `HESolveCrackPropagationElement.cpp` 
 Implements standard, crack-body, crack-tip, and blended element formulations including enrichment functions.

- `HESolveCrackPropagationNode.cpp` 
 Stores nodal displacement, level-set/tip data, and averages nodal stress.

- `HESolveCrackPropagationTools.cpp` 
 Geometry tests, crack/element intersection, polygon splitting, triangulation, Gauss point generation, level-set side classification.

---

## 29. Strengths of this implementation

- No remeshing is required to represent the crack
- Crack faces can cut through Quad4 elements
- Crack-tip singularity is modeled with asymptotic enrichment
- Stress intensity factors are computed directly from the solved field
- Crack growth direction is predicted automatically
- Integrated directly into the application database and result system

---

## 30. Important assumptions and limitations

- **2D only**
- **Quad4 only**
- linear elastic material
- small strain / small displacement
- crack propagation executed through a **static** solve path
- enrichment classification is heuristic and geometry-based
- modal support exists in the model class but is not fully wired into the top-level crack-propagation execution flow
- enriched systems can be harder to constrain robustly than standard FEM systems

---

## 31. Suggested presentation structure

A good presentation order for this solver is:

1. Why standard FEM struggles with cracks
2. XFEM idea: enrich instead of remesh
3. Standard Quad4 base formulation
4. Heaviside enrichment
5. Crack-tip asymptotic enrichment
6. Node classes and extra DOFs
7. Standard / crack-body / crack-tip / blended elements
8. Split-domain integration and triangulation
9. Global assembly and sparse solve
10. Stress recovery
11. Interaction integral and SIF extraction
12. Crack-growth direction and next-point prediction
13. Integration with the application database
14. Strengths and limitations

---

## 32. Short summary

This solver is a **2D Quad4 XFEM crack propagation solver** that augments the standard FEM displacement field with:

- **Heaviside enrichment** for displacement jump across crack faces
- **asymptotic branch enrichment** for crack-tip singularity

It then:

- solves the static XFEM system
- computes stress and von Mises stress
- extracts $K_I$ and $K_{II}$
- predicts the next crack point
- extends the crack path without remeshing

That is the core advantage of this solver: **crack growth is handled by enrichment and geometric crack update, not by remeshing the mesh to fit the crack at every step**.