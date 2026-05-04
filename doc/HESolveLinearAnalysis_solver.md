# HESolveLinearAnalysis Solver Walkthrough

## 1. Purpose of this solver

`HESolveLinearAnalysis` is a **3D linear finite element solver** for:

- **Static analysis**
- **Modal analysis**

It supports these element types:

- **Tet4** (4-node tetrahedron)
- **Prism6** (6-node wedge / prism)
- **Hex8** (8-node hexahedron)

The solver is implemented in these main classes:

- `HESolveLinearAnalysis` — top-level execute entry
- `HESolveLinearAnalysisModel` — builds the model, assembles matrices, solves
- `HESolveLinearAnalysisElement` and derived element classes — element-level FEM formulation
- `HESolveLinearAnalysisNode` — nodal coordinates, displacement, stress storage
- `HESolveLinearAnalysisTools` — volume and Gauss integration helpers

This version is integrated with your application database instead of reading CSV directly.

---

## 2. High-level workflow

The execution flow is:

1. Read nodes, elements, loads, and constraints from `DatabaseSession`
2. Build the global **stiffness matrix** `K`
3. Build the global **mass matrix** `M`
4. Reduce the system to **free DOFs**
5. Solve either:
   - **Static:** `K u = F`
   - **Modal:** `K phi = lambda M phi`
6. Recover:
   - nodal displacements
   - element stress
   - averaged nodal stress
7. Write results back into `HIResult`

---

## 3. Mathematical model

This solver uses **small-strain, linear elastic, 3D solid mechanics**.

### 3.1 Displacement field

At any point inside an element:

$$
\mathbf{u}(x,y,z)=\mathbf{N}(x,y,z)\mathbf{d}_e
$$

where:

- $\mathbf{u}$ = displacement vector
- $\mathbf{N}$ = shape function matrix
- $\mathbf{d}_e$ = element nodal displacement vector

For 3D solid elements, each node has **3 DOFs**:

$$
[u_x,\;u_y,\;u_z]
$$

So:

- Tet4 has $4 \times 3 = 12$ local DOFs
- Prism6 has $6 \times 3 = 18$ local DOFs
- Hex8 has $8 \times 3 = 24$ local DOFs

---

## 4. Constitutive matrix $D$

The material is **3D isotropic linear elastic**.

The solver computes Lamé constants:

$$
\lambda = \frac{E\nu}{(1+\nu)(1-2\nu)}
$$

$$
\mu = \frac{E}{2(1+\nu)}
$$

Then builds the 3D constitutive matrix:

$$
\mathbf{D} =
\begin{bmatrix}
\lambda+2\mu & \lambda & \lambda & 0 & 0 & 0 \\
\lambda & \lambda+2\mu & \lambda & 0 & 0 & 0 \\
\lambda & \lambda & \lambda+2\mu & 0 & 0 & 0 \\
0 & 0 & 0 & \mu & 0 & 0 \\
0 & 0 & 0 & 0 & \mu & 0 \\
0 & 0 & 0 & 0 & 0 & \mu
\end{bmatrix}
$$

This corresponds to the strain/stress ordering:

$$
[\varepsilon_x,\;\varepsilon_y,\;\varepsilon_z,\;\gamma_{xy},\;\gamma_{yz},\;\gamma_{zx}]
$$

and

$$
[\sigma_x,\;\sigma_y,\;\sigma_z,\;\tau_{xy},\;\tau_{yz},\;\tau_{zx}]
$$

---

## 5. Strain-displacement matrix $B$

Strain is computed from nodal displacements using:

$$
\boldsymbol{\varepsilon} = \mathbf{B}\mathbf{d}_e
$$

For each node $i$, if the shape function derivatives are:

$$
\frac{\partial N_i}{\partial x},\quad
\frac{\partial N_i}{\partial y},\quad
\frac{\partial N_i}{\partial z}
$$

then the standard 3D solid mechanics block inside $B$ is:

$$
\mathbf{B}_i =
\begin{bmatrix}
N_{i,x} & 0 & 0 \\
0 & N_{i,y} & 0 \\
0 & 0 & N_{i,z} \\
N_{i,y} & N_{i,x} & 0 \\
0 & N_{i,z} & N_{i,y} \\
N_{i,z} & 0 & N_{i,x}
\end{bmatrix}
$$

and the full $B$ matrix is all node blocks concatenated.

---

## 6. Element stiffness matrix

For each element:

$$
\mathbf{K}_e = \int_{\Omega_e} \mathbf{B}^T \mathbf{D} \mathbf{B}\; dV
$$

This is evaluated numerically using Gauss integration, except for Tet4 where the formulation is constant-strain and can be evaluated in closed form using the element volume.

---

## 7. Element mass matrix

The solver builds a **consistent mass matrix**:

$$
\mathbf{M}_e = \int_{\Omega_e} \rho \mathbf{N}^T \mathbf{N}\; dV
$$

where:

- $\rho$ = density
- $\mathbf{N}$ = displacement interpolation matrix

Tet4 uses the standard closed-form consistent mass matrix.
Prism6 and Hex8 use Gauss integration.

---

## 8. Shape functions by element type

## 8.1 Tet4 element

### Natural coordinates

Tet4 uses barycentric-style coordinates $(r,s,t)$ with:

$$
N_1 = 1-r-s-t,\quad
N_2 = r,\quad
N_3 = s,\quad
N_4 = t
$$

### Shape matrix

The displacement interpolation matrix is:

$$
\mathbf{N} =
\begin{bmatrix}
N_1 & 0 & 0 & N_2 & 0 & 0 & N_3 & 0 & 0 & N_4 & 0 & 0 \\
0 & N_1 & 0 & 0 & N_2 & 0 & 0 & N_3 & 0 & 0 & N_4 & 0 \\
0 & 0 & N_1 & 0 & 0 & N_2 & 0 & 0 & N_3 & 0 & 0 & N_4
\end{bmatrix}
$$

### Derivatives

The Tet4 element is a **constant strain tetrahedron**, so shape derivatives in global coordinates are constant inside the element.

The solver computes them by forming:

$$
\mathbf{A} =
\begin{bmatrix}
1 & x_1 & y_1 & z_1 \\
1 & x_2 & y_2 & z_2 \\
1 & x_3 & y_3 & z_3 \\
1 & x_4 & y_4 & z_4
\end{bmatrix}
$$

and using:

$$
\mathbf{A}^{-1}
$$

to obtain the coefficients and derivatives.

### Volume

$$
V = \frac{1}{6}\left|\det(\mathbf{A})\right|
$$

### Stiffness

$$
\mathbf{K}_e = V \mathbf{B}^T \mathbf{D} \mathbf{B}
$$

### Mass

The consistent mass matrix uses the classical formula:

$$
\mathbf{M}_e = \rho \frac{V}{20}
\begin{bmatrix}
2I & I & I & I \\
I & 2I & I & I \\
I & I & 2I & I \\
I & I & I & 2I
\end{bmatrix}
$$

where $I$ is the $3\times3$ identity block.

### Integration point

Tet4 uses one integration point at the centroid:

$$
(r,s,t)=\left(\frac14,\frac14,\frac14\right)
$$

---

## 8.2 Prism6 element

### Natural coordinates

The Prism6 shape functions used by the solver are:

$$
N_1 = \frac12(1-t)(1-r-s)
$$

$$
N_2 = \frac12(1-t)r
$$

$$
N_3 = \frac12(1-t)s
$$

$$
N_4 = \frac12(1+t)(1-r-s)
$$

$$
N_5 = \frac12(1+t)r
$$

$$
N_6 = \frac12(1+t)s
$$

This is a linear wedge / prism element:
- linear in the triangular $(r,s)$ plane
- linear in the thickness direction $t$

### Jacobian

The Jacobian is:

$$
\mathbf{J} =
\frac{\partial(x,y,z)}{\partial(r,s,t)}
$$

The solver first computes derivatives in natural coordinates, then maps them to global coordinates using:

$$
\frac{\partial N}{\partial x} = \mathbf{J}^{-1}\frac{\partial N}{\partial(r,s,t)}
$$

### Stiffness and mass

$$
\mathbf{K}_e = \sum_{gp} \det(\mathbf{J})\,w_{gp}\,\mathbf{B}^T\mathbf{D}\mathbf{B}
$$

$$
\mathbf{M}_e = \sum_{gp} \rho\,\det(\mathbf{J})\,w_{gp}\,\mathbf{N}^T\mathbf{N}
$$

### Gauss rule

The solver uses a **3 × 2** integration rule:
- 3 integration points on the triangle
- 2 integration points along the prism axis

So each Prism6 element uses **6 Gauss points** total.

---

## 8.3 Hex8 element

### Natural coordinates

Hex8 uses the standard trilinear shape functions:

$$
N_i(\xi,\eta,\zeta)=\frac18(1+\xi\xi_i)(1+\eta\eta_i)(1+\zeta\zeta_i)
$$

where $(\xi_i,\eta_i,\zeta_i)$ are the natural coordinates of the 8 corner nodes:

$$
(-1,-1,-1),\;
(1,-1,-1),\;
(1,1,-1),\;
(-1,1,-1),\;
(-1,-1,1),\;
(1,-1,1),\;
(1,1,1),\;
(-1,1,1)
$$

### Jacobian

$$
\mathbf{J} = \frac{\partial(x,y,z)}{\partial(\xi,\eta,\zeta)}
$$

Derivatives are computed in natural coordinates and transformed to global coordinates through:

$$
\frac{\partial N}{\partial x} = \mathbf{J}^{-1}\frac{\partial N}{\partial(\xi,\eta,\zeta)}
$$

### Stiffness and mass

$$
\mathbf{K}_e = \sum_{gp} \det(\mathbf{J})\,w_{gp}\,\mathbf{B}^T\mathbf{D}\mathbf{B}
$$

$$
\mathbf{M}_e = \sum_{gp} \rho\,\det(\mathbf{J})\,w_{gp}\,\mathbf{N}^T\mathbf{N}
$$

### Gauss rule

Hex8 uses a standard **2 × 2 × 2** Gauss rule:
- 2 points in $\xi$
- 2 points in $\eta$
- 2 points in $\zeta$

So each Hex8 element uses **8 Gauss points** total.

---

## 9. Global assembly

Each element contributes local matrices to the global system using a boolean / DOF map.

For each element:

- build local stiffness matrix $\mathbf{K}_e$
- build local mass matrix $\mathbf{M}_e$
- map local DOFs to global DOFs
- assemble with sparse triplets

If node $a$ is the global node index, then its 3 global DOFs are:

$$
[3a,\;3a+1,\;3a+2]
$$

So the element map is:

$$
[u_{x1},u_{y1},u_{z1},u_{x2},u_{y2},u_{z2},\dots]
$$

This map is stored in `boolean_`.

The global matrices are:

$$
\mathbf{K} = \sum_e \mathbf{A}_e^T \mathbf{K}_e \mathbf{A}_e
$$

$$
\mathbf{M} = \sum_e \mathbf{A}_e^T \mathbf{M}_e \mathbf{A}_e
$$

where $\mathbf{A}_e$ is the local-to-global DOF mapping operator.

---

## 10. Loads and boundary conditions

### 10.1 Nodal loads

Loads are read from `HILbcForce` and assigned directly into the global load vector:

$$
\mathbf{F}
$$

Each node contributes:

$$
[F_x,\;F_y,\;F_z]
$$

### 10.2 Constraints

Constraints are read from `HILbcConstraint`.

For constrained DOFs, the solver marks:

- `0.0` = fixed
- `1.0` = free

So `boundaryCondition_` acts like a mask.

### 10.3 Reduced system

Only free DOFs are kept.

If the free DOF set is indexed by `freeDofs`, then the solver builds reduced matrices:

$$
\mathbf{K}_{red}
$$

$$
\mathbf{M}_{red}
$$

$$
\mathbf{F}_{red}
$$

This is done by scanning the sparse matrices and keeping entries whose row and column are both free DOFs.

---

## 11. Static analysis

The static system is:

$$
\mathbf{K}_{red}\mathbf{u}_{red}=\mathbf{F}_{red}
$$

The solver tries:

1. `Eigen::SimplicialLDLT`
2. fallback to `Eigen::SparseLU`

### Why not compute $K^{-1}$ explicitly?

Because in FEM that is inefficient and numerically worse.

Instead of:

$$
\mathbf{u} = \mathbf{K}^{-1}\mathbf{F}
$$

we solve the linear system directly through factorization.

### Recovery of full displacement vector

After solving reduced displacements, the solver reconstructs the full vector:

$$
\mathbf{u}
$$

by placing solved values back into free DOF positions and leaving constrained DOFs at zero.

---

## 12. Modal analysis

The modal problem is:

$$
\mathbf{K}_{red}\boldsymbol{\phi} = \lambda \mathbf{M}_{red}\boldsymbol{\phi}
$$

where:
- $\lambda = \omega^2$
- $\omega$ = circular natural frequency
- $\boldsymbol{\phi}$ = mode shape

### 12.1 Solver choice

The code uses **Spectra** with:

- `SparseSymMatProd` for $K_{red}$
- `SparseCholesky` for $M_{red}$
- `SymGEigsSolver<..., GEigsMode::Cholesky>`

This is much faster than converting the sparse matrices to dense and solving all eigenpairs.

### 12.2 What it computes

It solves only the requested number of lowest modes:

- `nev = numberOfMode`
- `ncv` = subspace size used by Spectra

The solver requests:

$$
\text{SmallestAlge}
$$

meaning the smallest algebraic eigenvalues.

### 12.3 Natural frequency

From each eigenvalue:

$$
\lambda_i
$$

the natural frequency is computed as:

$$
f_i = \frac{\sqrt{\lambda_i}}{2\pi}
$$

These frequencies are stored in `frequency_`.

### 12.4 Mode shapes

Spectra returns reduced mode shapes:

$$
\boldsymbol{\phi}_{red}
$$

Then each mode is expanded back to the full DOF space:

$$
\boldsymbol{\phi}_{full}
$$

by inserting zero for constrained DOFs.

These are stored in `modeShapes_`.

### 12.5 Displacement for a mode

When `createDisplacement(mode)` is called in modal analysis:

- `displacement_ = modeShapes_[mode]`

So the displacement shown for modal results is the selected **mode shape**, not a static displacement under load.

---

## 13. Displacement recovery

Once the global displacement vector is known (either static displacement or one modal shape), each element extracts its local displacement vector from the global DOF map.

Then for each node:

$$
u_x = d[3i],\quad
u_y = d[3i+1],\quad
u_z = d[3i+2]
$$

and nodal displacement magnitude is:

$$
u_{mag} = \sqrt{u_x^2 + u_y^2 + u_z^2}
$$

---

## 14. Stress computation

Stress at an evaluation point is:

$$
\boldsymbol{\sigma} = \mathbf{D}\mathbf{B}\mathbf{d}_e
$$

with components:

$$
[\sigma_x,\sigma_y,\sigma_z,\tau_{xy},\tau_{yz},\tau_{zx}]
$$

### 14.1 Von Mises stress

The solver computes:

$$
\sigma_{vm} =
\sqrt{
\frac12\left[
(\sigma_x-\sigma_y)^2 +
(\sigma_y-\sigma_z)^2 +
(\sigma_z-\sigma_x)^2
\right]
+
3\left(
\tau_{xy}^2 + \tau_{yz}^2 + \tau_{zx}^2
\right)
}
$$

and stores a 7-component vector:

$$
[\sigma_x,\sigma_y,\sigma_z,\tau_{xy},\tau_{yz},\tau_{zx},\sigma_{vm}]
$$

---

## 15. Stress recovery by element type

### 15.1 Tet4

Tet4 is constant strain / constant stress, so the solver computes one stress state for the element and pushes the same value to all 4 nodes.

### 15.2 Prism6 and Hex8

For Prism6 and Hex8, stress is evaluated at nodal natural coordinates using the element $B$-matrix and current displacement vector, then appended to each connected node’s stress list.

---

## 16. Nodal stress averaging

Each node stores a `stressList_` containing stress contributions from all connected elements.

Final nodal stress is computed by simple arithmetic averaging:

$$\boldsymbol{\sigma}_{node}=\frac{1}{n}\sum_{i=1}^{n}\boldsymbol{\sigma}^{(i)}$$

This is done in `HESolveLinearAnalysisNode::createStress()`.

So nodal stress is a **smoothed average**, not a raw single-element value.

---

## 17. Result writing

After solving:

- static analysis writes one `HIResult`
- modal analysis writes one `HIResult` per mode

For each node, the result contains:

### Displacement
- $u_x$
- $u_y$
- $u_z$
- translational magnitude

### Stress
- $\sigma_x$
- $\sigma_y$
- $\sigma_z$
- $\tau_{xy}$
- $\tau_{yz}$
- $\tau_{xz}$
- von Mises

For modal analysis, each result step also stores:
- modal frequency

---

## 18. Complete solver pipeline

## Step 1 — Create model

`HESolveLinearAnalysisModel`:
- reads nodes
- reads elements
- reads loads
- reads constraints
- builds DOF map

## Step 2 — Build material matrix

Compute:

- $E$
- $\nu$
- $\lambda$
- $\mu$
- $D$

## Step 3 — Build element matrices

For each element:
- create Gauss points
- compute $B$
- compute $N$
- compute $K_e$
- compute $M_e$

## Step 4 — Assemble global matrices

Assemble:
- global sparse stiffness matrix $K$
- global sparse mass matrix $M$

## Step 5 — Apply constraints

Create reduced system:
- $K_{red}$
- $M_{red}$
- $F_{red}$

## Step 6 — Solve

### Static

$$
K_{red}u_{red}=F_{red}
$$

### Modal

$$
K_{red}\phi=\lambda M_{red}\phi
$$

## Step 7 — Recover displacement

- expand reduced vectors to full DOF vectors
- assign nodal displacement

## Step 8 — Recover stress

- compute element stress
- compute von Mises
- average to nodal stress

## Step 9 — Write results

- one result set for static
- one result set per mode for modal

---

## 19. Key strengths of this solver

- Supports **three 3D solid element types**
- Sparse global assembly
- Static + modal in the same framework
- Uses **Spectra** for fast sparse modal extraction
- Computes full 3D stress and von Mises stress
- Easy integration with your application database and result system

---

## 20. Important assumptions and limitations

- Linear geometry
- Linear material
- Small displacement / small strain
- Nodal loads only
- Essential BCs handled as fixed translational DOFs
- No nonlinear material
- No contact
- No plasticity
- No thermal strain
- No dynamic transient analysis
- Nodal stress is averaged, so it is post-processed / smoothed

---

## 21. Suggested presentation structure

If you want to present this solver, a good slide order is:

1. Solver purpose and supported analyses
2. Supported element types
3. Material constitutive law
4. Shape functions
5. B matrix and strain-stress relation
6. Element stiffness and mass matrix
7. Gauss integration by element type
8. Global sparse assembly
9. Boundary conditions and reduced system
10. Static solve
11. Modal solve with Spectra
12. Stress and von Mises recovery
13. Database input and result output
14. Strengths / limitations
15. Example models and results

---

## 22. File-to-responsibility map

- `HESolveLinearAnalysis.cpp`  
  Top-level execution, chooses static or modal flow, writes `HIResult`.

- `HESolveLinearAnalysisModel.cpp`  
  Reads model data, assembles `K` and `M`, solves the reduced system, reconstructs mode shapes and displacements.

- `HESolveLinearAnalysisElement.cpp`  
  Element formulations for Tet4, Prism6, Hex8; stiffness, mass, and stress computation.

- `HESolveLinearAnalysisNode.cpp`  
  Stores displacement and averages nodal stress.

- `HESolveLinearAnalysisTools.cpp`  
  Volume formulas and Gauss point generation.

---

## 23. Short summary

This solver is a **3D linear solid FEM engine** using:
- **Tet4 / Prism6 / Hex8**
- **sparse global matrices**
- **static solve with Eigen sparse factorizations**
- **modal solve with Spectra generalized eigen solver**
- **stress recovery and nodal averaging**
- **direct integration into your application database**

It is a solid foundation for:
- structural static analysis
- natural frequency extraction
- mode shape visualization
- nodal stress post-processing
