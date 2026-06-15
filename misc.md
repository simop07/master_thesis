# Master's thesis notes

## Introduction
The aim of the notes that will follow is to present a comprehensive and chronological description of my master's thesis developments, which will mainly focus on the upgrades of the Vertex Tracker (VT)detector of FOOT experiment. The optimization of FOOT trigger board is outlined in the documentation folder of [vtxtrgbox](https://baltig.infn.it/firmwarenewvtx/vtxtrgbox) directory.

## Motivation
The motivation behind the VT work lies in the investigation of its pull distributions. To thoroughly understand what a **pull distirbution** is, and its connection to the **Kalman Filter** algorithm, check the folder "Material" in my Thesis directory in Windows. In next paragraphs, just a summary of these concepts will be provided to motivate my undergoing work.

### Tracking particles
In a tracking/vertexing system, parallel detector planes are employed to measure the positional information of a particle. When a particle traverses a detector (whether it is a pixel or a strip detector), it fires a sensor. Usually, the hit sensor is not a single pixel, it is rather a cluster of pixels or a family of strips. Specifically, FOOT VT detector gives a hit/no hit information (hence, it is a binary readout, not an analog one). When the VT detector fires, a positional measurement $\vec{x}_{meas}$ is given. In the particular case of the VT detector, we have $4$ parallel planes which provide $4$ $\vec{x}_{meas}$. These points, together with additional hit points fournished by the other tracking detectors (Inner Tracker, Micro Strip Detector and TOF Wall), provide a family of hits which follow the (bended) track of a particle.

These position measurements do not lie exactly on the true particle trajectory, due to finite detector uncertainties and possible plane misalignements, and particles suffering from multiple coulomb scattering happening within the detectors' bulk or in air. Hence, the collected measurements $\vec{x}_{meas,i}$ represent just an approximation of the true particle track.

### Track representation, fitting and global reconstruction
Premise: to thoroughly understand the track reconstruction in FOOT experiment, refer to Chapter 3 of **Roberto Zarrella PhD thesis**. Now, we have a family of hits representing the particle track. Of course, it is unphysical to simply connect these consecutive points via a polyline. Hence, a reconstruction/fitting algorithm is used to reconstruct the physical, smooth trajectory followed by a particle in FOOT experimental setup.

From Roberto Zarrella Chapter 3:
> The processing of all data acquired by FOOT is carried out through the main software of the Collaboration, which is called SHOE (Software for Hadrontherapy Optimization Experiment). SHOE is an object-oriented, flexible framework based on C++ and ROOT libraries, partially inherited from the FIRST experiment. The software takes care of the whole data processing, from raw data decoding of all sub-detectors to particle identification and tracking, up to the cross section evaluation.
>
> The SHOE framework has also been conceived to generate all the input files needed to perform Monte Carlo simulations in FLUKA and Geant4 for all the FOOT data acquisition campaigns. Custom user routines have also been implemented to decode the FLUKA scoring output and translate it in the ROOT format used for decoded data structures, so that the same reconstruction software can be used for both raw and simulated data sets. When working with MC generated files, SHOE also implements all the smearing of measured quantities, based on the evaluated experimental resolution of each detector in the setup.

To summarize, the input to SHOE can be either **experimental data** acquired by the DAQ system or **simulated data** produced with FLUKA and Geant4. Within SHOE, the detector geometry, calibration constants, magnetic field maps, and reconstruction parameters are loaded through dedicated detector libraries.

The reconstruction process is divided into two main steps:
1. **Local reconstruction**
   - Performed independently for each subdetector (VT, IT, MSD, TW, etc.).
   - Converts raw detector signals into physical measurements (hits, clusters, energies, times, positions).
   - Uses detector-specific libraries and calibration constants.

2. **Global reconstruction**
   - Combines the identification, classification and fitting procedure of all the particle tracks in an event, from the primary interaction point in the target to the downstream region of the setup.
   - Performs pattern recognition and track fitting (through tools such as GENFIT, by employing Kalman Filter algorithm $\rightarrow$ in this sense, **GENFIT is an external tracking package employed by SHOE**). Track fitting means **evaluating the most suitable set of track parameters** compatible with the measurements belonging to each of the track candidates, taking into account the uncertainties on the measurements and the interaction with materials along the trajectory. Tracks are usually parameterized using the position $\vec{x}$ and momentum $\vec{p}$ of the particle moving through the setup, but many different choices can be made. In this sense, **reconstructing a track means being able to calculate such parameters and their covariance at any given point of the trajectory of the particle**.
   - Reconstructs particle trajectories, interaction vertices, and other global event quantities.

In GENFIT, a **track representation** defines both the parameterization of a particle track and the model used to extrapolate the track through the detector. The particle state is described by a five-dimensional vector:
$$x = \left(\frac{q}{p}, u', v', u, v\right)$$
where:
- \$frac{q}{p}$ is the charge-over-momentum ratio (inverse of rigidity),
- $u$ and $v$ are the **particle coordinates in the local reference frame of a detector plane**,
- $u'$ and $v'$ describe the **track direction with respect to the plane**.
This parameterization allows the track to be represented locally on each detector plane.

To propagate the track from one detector plane to another, GENFIT uses the **equation of motion of a charged particle in a magnetic field**:

$$\frac{d\vec{p}}{ds} = \frac{q}{p} \left( \frac{d\vec{r}}{ds} \times \vec{B}(\vec{r}) \right)$$

Since the FOOT magnetic field is generally non-uniform, the trajectory cannot be computed analytically. Therefore, the GENFIT class handling track representation and fitting employs a **fourth-order Runge-Kutta-Nyström (RKN) numerical integration method** to solve the equation of motion and extrapolate the track parameters along the trajectory.

In GENFIT, a **track** class is a data structure containing:
- the detector measurements (hits),
- one or more track representations,
- the information required for track propagation and fitting.

Track **fitting algorithms use both the measurements and the extrapolation model to determine the most probable particle trajectory**. In the FOOT Global Reconstruction, the track parameters are estimated using the **Kalman Filter**, which iteratively combines the detector measurements with the predicted track trajectory to obtain the best estimate of the particle state and its uncertainties.

### Kalman Filter
Several fitting methods are available for reconstructing particle tracks. can provide the fitting algorithm:
- Helix (**global fit** method) $\rightarrow$ it neglects both inhomogeneous magnetic field and medium effects
- Pline (**global fit** method) $\rightarrow$ it includes tracking in inhomogeneous magnetic while neglecting medium effects
- Kalman (**progressive fit** method) $\rightarrow$ it performs tracking in inhomogeneous magnetic field including energy loss and multiple scattering.

The Kalman Filter is a recursive fitting algorithm that progressively updates the track parameters as new measurements become available. Suppose a particle crosses several detector planes:
```
Plane 1 → hit
Plane 2 → hit
Plane 3 → hit
Plane 4 → hit
Plane 5 → hit
```
The fit proceeds one detector at a time.

#### Step 1: Initial Seed
A first estimate of the particle trajectory is built. The state vector is initialized as:
$$\mathbf{x} = \left(\frac{q}{p},\, u',\, v',\, u,\, v\right)$$
together with its covariance matrix $C$, which describes the uncertainty on the track parameters (for instance, the uncertainty on the momentum at a detector plane, the uncertainty on the position at a detector plane, or how these two parameter are correlated (it is indeed a covariance matrix)).

#### Step 2: Prediction
Starting from detector plane $k-1$, `RKTrackRep` propagates the track to detector plane $k$:
```
State at plane k-1
        ↓
  Propagation
        ↓
Predicted state at plane k
```
The result is the predicted state vector $\tilde{\mathbf{x}}_k$ and its covariance:
$$\tilde{C}_k = J\, C_{k-1}\, J^T + N$$
where:
- $J$ is the **Jacobian matrix describing error propagation**,
- $N$ is the **process noise matrix** accounting for **multiple scattering**, **energy loss**, and other propagation uncertainties.

#### Step 3: Residual Calculation
At detector plane $k$, a measurement $\mathbf{m}_k$ is available. The difference between measurement and prediction is called the **residual**:
$$\mathbf{r}_k = \mathbf{m}_k - H_k\, \tilde{\mathbf{x}}_k$$
The matrix $H_k$ converts the state vector into measurable detector coordinates.

#### Step 4: Update
The predicted track and the detector measurement are combined. The **Kalman Gain**:
$$K_k = \tilde{C}_k H_k^T \left( H_k \tilde{C}_k H_k^T + V_k \right)^{-1}$$
determines how much the measurement should influence the updated state. The updated state becomes:
$$\mathbf{x}_k = \tilde{\mathbf{x}}_k + K_k\, \mathbf{r}_k$$
Conceptually:
```
Prediction  = 10.0 ± 1.0 mm
Measurement = 10.3 ± 0.1 mm
→ trust the measurement more

Prediction  = 10.0 ± 0.05 mm
Measurement = 10.3 ± 1.0 mm
→ trust the prediction more
```
The Kalman Gain automatically computes the optimal weighting.

#### Step 5: Repeat
The updated state becomes the starting point for the next detector plane:
```
Predict → Measure → Update → Predict → Measure → Update → ...
```
This process continues until all detector measurements have been processed.

#### Summary
Kalman Filter can be divided in three steps:
1. **Extrapolation**: predict track at next plane, which yields $\tilde{\mathbf{x}}_k$
2. **Filtering**: combine prediction and measurement, which yields $\mathbf{x}_k$
3. **Smoothing**: refine states using all measurements. Basically if the Kalman Filter before this stage did someting like this: Plane 1 → Plane 2 → Plane 3 → Plane 4, now the idea is that it does something like this: Plane 4 → Plane 3 → Plane 2 → Plane 1.
---

### Why Kalman Filtering is Better than a Global Least-Squares Fit
A traditional least-squares fit tries to determine one trajectory using all measurements simultaneously. The Kalman Filter instead updates the track at every detector plane.
This is particularly advantageous because particles:
- loose energy,
- undergo multiple Coulomb scattering,
- slightly change direction while crossing materials.

These effects are naturally included through the process noise matrix $N$. As a result, the Kalman Filter provides a more realistic description of particle trajectories, especially for low-momentum fragments.

---

### Global Reconstruction Workflow

The FOOT Global Reconstruction consists of three main components.

#### 1. Uploader

The Uploader converts SHOE detector information into GENFIT-compatible measurements.

For each detector, the reconstructed hit positions are transferred to GENFIT structures:

| Detector | Measurement type |
|----------|-----------------|
| VT       | 2D              |
| IT       | 2D              |
| MSD      | 1D strip        |
| TW       | 2D              |

#### 2. Track Selector

The Track Selector performs **pattern recognition**. Its task is to determine which detector hits belong to the same particle and create track candidates.

The selector:

1. Creates seed tracklets.
2. Propagates them through the detector.
3. Assigns compatible hits.
4. Rejects poor candidates.

Track propagation is performed using `RKTrackRep`. Track candidates are built before any final fit is attempted.

#### 3. Track Fitter

The Track Fitter receives the selected track candidates and performs the final Kalman fit.

For each candidate:

1. The Kalman Filter is executed.
2. Forward and backward iterations are performed.
3. **Track convergence** is checked by looking at the **p-value** and **reduced chi-square**.
4. The final fitted parameters are stored.

Candidates that fail the fit quality criteria are rejected as fake tracks.

#### FOOT algorithms

There are several algorithms built in FOOT which provide the tracking of particles, each well suited for a specific purpose. For instance, the Linear algorithm is designed for **runs without a magnetic field**. Instead of RKN propagation, straight-line extrapolation is used:

$$x(z) = x_0 + t_x\,(z - z_0)$$
$$y(z) = y_0 + t_y\,(z - z_0)$$

where $(x,y)$ and $(x_0,y_0)$ represent respectively the measured position and the fit position. This is the starting point of my analysis work.

## Pull distribution and alignmen
