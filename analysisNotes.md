# Master's thesis notes

## Introduction
The aim of the notes that will follow is to present a comprehensive and chronological description of my master's thesis developments, which will mainly focus on the upgrades of the Vertex Tracker (VT)detector of FOOT experiment. The optimization of FOOT trigger board is outlined in the documentation folder of [vtxtrgbox](https://baltig.infn.it/firmwarenewvtx/vtxtrgbox) directory.

## Motivation
The motivation behind the VT work lies in the investigation of its pull distributions. To thoroughly understand what a **pull distirbution** is, and its connection to the **Kalman Filter** algorithm, check the folder "Material" in my Thesis directory in Windows. In next paragraphs, just a summary of these concepts will be provided to motivate my undergoing work.

### Tracking particles
In a tracking/vertexing system, parallel detector planes are employed to measure the positional information of a particle. When a particle traverses a detector (whether it is a pixel or a strip detector), it fires a sensor. Usually, the hit sensor is not a single pixel, it is rather a cluster of pixels or a family of strips. Specifically, FOOT VT detector gives a hit/no hit information (hence, it is a binary readout, not an analog one). When the VT detector fires, a positional measurement $x_{meas}$ is given. In the particular case of the VT detector, we have $4$ parallel planes which provide $4$ $x_{meas}$. These points, together with additional hit points fournished by the other tracking detectors (Inner Tracker, Micro Strip Detector and TOF Wall), provide a family of hits which follow the (bended) track of a particle.

These position measurements do not lie exactly on the true particle trajectory, due to finite detector uncertainties and possible plane misalignements, and particles suffering from multiple coulomb scattering happening within the detectors' bulk or in air. Hence, the collected measurements $x_{meas,i}$ represent just an approximation of the true particle track.

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

$$
\tilde{C}_k = J\, C_{k-1}\, J^T + N
$$

where:
- $J$ is the **Jacobian matrix describing error propagation**,
- $N$ is the **process noise matrix** accounting for **multiple scattering**, **energy loss**, and other propagation uncertainties.

#### Step 3: Residual Calculation
At detector plane $k$, a measurement $\mathbf{m}_k$ is available. The difference between measurement and prediction is called the **residual**:

$$
\mathbf{r}_k = \mathbf{m}_k - H_k\, \tilde{\mathbf{x}}_k
$$

The matrix $H_k$ converts the state vector into measurable detector coordinates.

#### Step 4: Update
The predicted track and the detector measurement are combined. The **Kalman Gain**:

$$
K_k = \tilde{C}_k H_k^T \left( H_k \tilde{C}_k H_k^T + V_k \right)^{-1}
$$

determines how much the measurement should influence the updated state. The updated state becomes:

$$
\mathbf{x}_k = \tilde{\mathbf{x}}_k + K_k\, \mathbf{r}_k
$$

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

## Pull distribution and alignment

The basic idea is the following. In the absence of a magnetic field, residuals and pull distributions can be investigated rather straightforwardly. By comparing the measured coordinates $(x,y)$ with the fitted coordinates $(x_0,y_0)$, one obtains residual distributions that are expected to be centered around zero and approximately Gaussian. While residuals are useful for a first evaluation of the detector performance, pull distributions provide a more rigorous diagnostic tool.

The pull is defined as

$$
\mathrm{Pull}_x = \frac{x_{\mathrm{meas}} - x_0}{\sqrt{\sigma_{\mathrm{meas}}^2 - \sigma_{\mathrm{fit}}^2}},
$$

and analogously for the $y$ coordinate. The subtraction in the denominator arises because the fit uncertainty depends on the measured uncertainty, meaning that the two quantities are correlated. With this definition, the pull distribution should ideally follow a standard normal distribution, with mean equal to zero and standard deviation equal to one.

However, the analysis of these distributions, particularly for the Vertex detector, reveals some interesting features. Several pull distributions are not centered around zero. This is a clear indication of detector misalignment. Referring to the track extrapolation equations above, if the assumed detector position differs from its true position along the beam axis, systematic shifts are introduced in the reconstructed transverse coordinates. Consequently, a non-zero pull mean directly signals a geometrical misalignment of the detector planes.

Indeed, during the June data-taking campaign in Bologna, a misalignment of the second Vertex detector plane was identified through this analysis and subsequently corrected. After the correction, the pull means moved significantly closer to zero, confirming the effectiveness of the alignment procedure.

A second issue concerns the width of the pull distributions. While the pull standard deviation should ideally be equal to one, values around two are observed for carbon ions and for most fragmentation products. A pull width larger than one indicates that the uncertainties assigned to the measurements are underestimated. In other words, the actual fluctuations of the residuals are larger than expected from the error model currently used in the reconstruction.

Before the alignment correction, an even stronger dependence of the pull width on the fragment charge was observed. For the lightest fragments, the pull standard deviation could reach values as large as six. This behavior was likely related to the broader angular distributions of low-charge fragments. Since lighter fragments are emitted with larger scattering angles, their trajectories deviate more significantly from the beam direction, making the reconstruction more sensitive to imperfections in the detector geometry and uncertainty modeling.

The alignment correction substantially reduced the charge dependence of the pull widths, representing an important improvement in the overall reconstruction performance. Nevertheless, the remaining excess width still indicates that the uncertainty model requires further refinement.

Several approaches can be considered to address this issue. A straightforward solution is the introduction of scale factors applied to the estimated uncertainties, effectively rescaling the pull distributions toward unit width. This constitutes one of the main topics that I will investigate in the next phase of the project. However, such scale factors should be regarded only as temporary corrections. A deeper understanding of their physical origin is required.

Ultimately, the goal is to develop more realistic, data-driven estimators for both hit positions and their associated uncertainties. In particular, customized three-dimensional estimators based on the detector geometry, the measured hit distributions, and the corresponding uncertainties in all spatial coordinates could provide a more accurate description of the detector response and significantly improve the pull distributions.

This represents the main idea behind my thesis work. If time permits, an additional analysis of fragmentation data and fragment identification will also be carried out starting in September.

## Chi-squared p-value distribution

An additional indication that the uncertainties are currently underestimated comes from the distribution of the $\chi^2$ p-value. In an ideal reconstruction, where the residuals are correctly modeled and the uncertainties are properly estimated, the p-value distribution should be uniform in the interval $[0,1]$. At present, however, the reconstructed $\chi^2$ values tend to be systematically larger than expected, causing the p-value distribution to accumulate near zero. This behavior is consistent with the observations from the pull distributions: a larger-than-expected $\chi^2$ indicates that the residuals are not fully accounted for by the assigned uncertainties, providing further evidence that the current error estimates are underestimated.

As a first step toward understanding these effects, I developed a $\chi^2$ minimization procedure with respect to the detector position along the $z$ axis. In this approach, $z$ represents the longitudinal position of the Vertex detector planes. By minimizing the track $\chi^2$ with respect to this parameter, it is possible to estimate corrections to the detector geometry directly from the data. The resulting distributions exhibit several interesting features and provide an independent handle on the detector alignment.

The corresponding estimate of the longitudinal displacement is given by

$$
\Delta z = z - z_0 = \frac{\displaystyle\sum_i \frac{(x_i - x_{i,0})\, t_{x,i}}{\sigma_i^2}}{\displaystyle\sum_i \frac{t_{x,i}^2}{\sigma_i^2}},
$$

where $x_i$ and $x_{i,0}$ denote the measured and fitted positions, respectively, $t_{x,i}$ is the track slope, and $\sigma_i$ is the measurement uncertainty.

A complication arises from the fact that the software alignment in the transverse coordinates ($x$ and $y$) depends on the assumed detector positions along $z$. Since a correction to the $z$ coordinates has recently been introduced, the entire alignment procedure must be repeated using the updated geometry. For this reason, residual signatures of misalignment can still be observed in some of the current Vertex detector distributions. A complete reprocessing of the data with the updated geometry is therefore required before drawing definitive conclusions regarding the detector alignment performance.

The next step of the project will be the introduction and study of scale factors applied to the estimated uncertainties. The objective is to bring the pull distributions closer to the ideal standard normal behavior, namely a mean of zero and a standard deviation of one. Once these scale factors have been determined, it will be important to understand whether they can be justified by the detector response and reconstruction model, or whether a more sophisticated and physically motivated uncertainty estimation procedure is required.

## Cluster error update (16/06/2026)
Check Ubaldi slides in the material of my Master's thesis to check why the $1/\sqrt(2)$ factor was used and why the variance of the mean was used. He explains that this factor was used to correct the pulls of the vertex tracklet - at that time decoupled with the global tracks later developed in FOOT.

## Update 24/06/2026
After creating the function $\Delta z$ defined two paragraphs above, we are still waiting for the software-based alignment procedure. The idea is that once we update a geometric parameter, such as the distance of the first two VT planes $\Delta z_{1,2}$, we need to perform again the software alignment of the whole experimental setup. Why? Because the Global Reconstruction algorithm has to know the relative position and angular displacement of all detectors. In other words, the "digital twin" of the real experimental setup has to move with the real detectors. To perform the software-based alignment, a detector plane is firstly chosen as reference in the alignment procedure. Then, a coordinate transformation matrix **dynamically adjusts the 3D position and rotation of each detector plane** relative to the chosen reference detector. We are waiting for this software alignment procedure.

In the meanwhile, I developed code to study the cluster shape of the VT detector by following the paper on MIMOSA-5, located in my Master's thesis folder. The Global Reconstruction algorithm relies on the accurate determination of the position associated with each detector cluster. Specifically, the fitting procedure (Kalman Filter) is based on how precisely the cluster position are determined. Indeed, as described in [this section](#step-4-update), the uncertainty of the cluster prosition influence the relative weight that the measurement has on the fit: the more accurate a measured position is with respect to the fit measurement, the larger will be the weight on the measurement. In particles crossing the detector approximately orthogonally, the generated clusters are typically compact and nearly symmetric, allowing a straightforward estimation of the hit position and its uncertainty. **From now on, follow the ClusterVertex.pdf file in my mMaster's thesis to understand theoretically what I have developed**. The following paragraphs will simply provide a description of what I did on the code and the expected results.

What do we expect from a MIMOSA-28 with binary readout: charges with low Z have a small cluster size. The inferior pixel number is caused by the fact that lighter particles do not have a sufficient energy to fire the peripheral pixels needed to generate the elongated cluster shape. Therefore, the elongated structure is more prone to vanish in smaller clusters. At the same time, lighter particles scatter at larger angles, and from the paper above we expect larger incidence angle to create more elongated clusters.

Other thing I have discovered: the number of clusters processed by each vertex plane is different. For instance, the 4th plane is the least efficient one (indeed the average number of pixels per cluster is 20 and not 40 as in the other planes). But that is not important, as the track reconstruction requires 3 cluster as minimum to generate a VT tracklet.

Another problem which arose during the analysis was an underestimate of the cluster error in the VT. This was corrected by comparing the cluster error (computed from the weighted positional variance) with the intrinsic detector resolution, given by the pixel pitch divided by $\sqrt{12}$. The requirement is that the cluster uncertainty should not be lower than this limit, which is set by the finite size of the VT pixels.

For the VT, with a pixel pitch of $20.7~\mu\mathrm{m}$, the correction is implemented as

$$
\sigma_{\mathrm{cluster}} = \max\left(\sigma_{\mathrm{cluster}}, \frac{20.7~\mu\mathrm{m}}{\sqrt{12}}\right),
$$

or equivalently

$$
\sigma_{\mathrm{cluster}}^2 = \max\left(\sigma_{\mathrm{cluster}}^2, \left(\frac{20.7~\mu\mathrm{m}}{\sqrt{12}}\right)^2\right).
$$

My correction consisted of applying the lower limit directly to the standard deviation, whereas previously the lower limit was applied to the variance.

A similar procedure was already implemented in the MSD. In that case, with a strip pitch of $150~\mu\mathrm{m}$, the cluster variance was compared with the square of the intrinsic detector resolution:

$$
\sigma_{\mathrm{cluster}}^2 = \max\left(\sigma_{\mathrm{cluster}}^2, \left(\frac{150~\mu\mathrm{m}}{\sqrt{12}}\right)^2\right).
$$

The current issue, which is still not understood, is that the MSD correction significantly reduces the number of reconstructed tracks. This behavior is not expected, since comparing variances is mathematically equivalent to comparing standard deviations:

$$
\sigma_{\mathrm{cluster}}^2 >
\left(\frac{150~\mu\mathrm{m}}{\sqrt{12}}\right)^2
\quad \Longleftrightarrow \quad
\sigma_{\mathrm{cluster}} >
\frac{150~\mu\mathrm{m}}{\sqrt{12}}.
$$

Therefore, the large reduction in the track reconstruction efficiency cannot be explained simply by the choice of comparing variances instead of standard deviations.

## Update 25/06/2026
The problem concerning the MSD cluster error is produced by negative energy loss values affecting the individual strips. These values, which represent the weights of the variance computation, are negative because they are computed with a baseline/noise pedestal removal. This means that if the energy in a strip is slighly lower than the baseline, we have negative values. To address this issue, negative values are removed from the COG computation and its error calculation. Updates will follow $\rightarrow$ With this change, we found out the following: the MSD pulls, after removing the negative-energy events, remained quite similar to the previous ones. The main difference I observe is in the **pull sigma for the lighter fragments**. In practice, before the modification, the sigma values for carbon and protons were noticeably different, around 0.43 (carbon) and 0.54 (protons). After the modification, they become 0.43 (carbon) and 0.47 (protons). Therefore, the sigma becomes slightly more charge-independent, although the effect is not very large. This behavior is observed in all MSD planes. And, of course, now these computations make sense physically - no negative energy losses!

Cluster analysis update: the study of the cluster shape in FOOT is useless, at least for the electronic setup where the angular acceptance is roughly $10^\circ$ (if one considers the angles of the TOF Wall, otherwise is $6^\circ$). Now, to explain this issue briefly, you should look at the elongation plot and at the cluster plots. Even if tracks are tilted, they do not produce evident asymmetric clusters - it is not possible to eyeball them as in the MIMOSA-5 paper, despite it is possible to eyeball the track inclination in both $\theta$ and $\phi$ angles. This is connected to the fact that the elongation is not large in FOOT, due to the low values of $\theta$, which in the MIMOSA-5 paper start from $60^\circ$! The interesting plots are the elongation expressed as a function of $\sigma_L$ (increasing function) and $\sigma_T$ (constant function), proving what the MIMOSA-5 paper stated: the cluster elongation happens only in the projection of the particle direction onto the detector plane, not in the perpendicular component. Moreover, plotting the elongation as a function of $\theta$, we can attest the fact that the elongation does not very with $\theta$. This is the proof of what we were saying before: in FOOT elongation is not important. For carbon ions, there is a slight increase of the elongation with the number of pixels, showing to us the fact that as the pixels increase in size the elongation increase. If we look at the elongation plots as a function of theta with the cut in the number of pixels, the increase of the elongation with the number of pixels - which actually can be seen JUST for the carbons - does not mean that the elongation increases when the tracks are tilted. I mean, there is no connection between the number of pixels and the track inclination. There is, instead, the phenomenon we were describing before: as the number of pixel increases, it is easier to measure the elongation - this is why wiht protons we have the problem that elongation is constant: the horizontal lines of the proton suggest that the proton elongation is being computed from essentially the same values of $\lambda_L$ and $\lambda_T$, indicating that protons rarely activate peripheral pixels because of their lower energy deposition. As a result, elongation is not a particularly descriptive variable for proton and low-Z ions. In conclusion, both $\sigma_{fit}$ and $\sigma_{meas}$ increase slighly with the elognation, showing the fact that FOOT reconstruction algorithm considers the fact that the residuals on x and on y may be different; in other words, the anisotropicity of the cluster is, somehow, already considered in our code. *Last but not least, the following paragraph gives an additional reason on why elongation study is FOOT is not greatly informative.

The lateral projection of the particle track (in cx or in y) must be approximately equal to one pixel pitch:

$$
\Delta x = t \tan\theta \sim p
\Delta y = t \tan\theta \sim p
$$

therefore,

$$
\tan\theta \sim \frac{p}{t}.
$$

With:

$$
p = 20\,\mu\mathrm{m}, \qquad t = 50\,\mu\mathrm{m},
$$

we obtain:

$$
\theta \sim \arctan\!\left(\frac{20}{50}\right)
= \arctan(0.4)
\simeq 21.8^\circ.
$$

Therefore, the minimum angle at which the track is expected to cross from one pixel to the next is approximately

$$
\boxed{\theta \sim 22^\circ}
$$

with respect to the normal to the sensor plane.
For comparison:

$$
\Delta x(10^\circ) = 50 \tan 10^\circ \simeq 8.8\,\mu\mathrm{m}
$$

which is less than half a pixel.

$$
\Delta x(20^\circ) = 50 \tan 20^\circ \simeq 18.2\,\mu\mathrm{m}
$$

which is nearly one pixel.

$$
\Delta x(40^\circ) = 50 \tan 40^\circ \simeq 42\,\mu\mathrm{m}
$$

which corresponds to about two pixels. Thus, below $10^\circ$ the geometrical effect is relatively small, while above $20^\circ$ it becomes clearly observable.

Actually, to conclude with a positive message, other than the error on the MSD without the negative energy values, the elongation study has proved two things:
1. The elongation in FOOT is not a very informative parameter, due to the low inclination of tracks. Consider that a $10^\circ$ inclination makes a particle travel instead of the $50~\mu\mathrm{m}$ (VT thickness) exaclty $50.1~\mu\mathrm{m}$... which is a very small difference. In a sense, this study may be considered consistent with the MIMOSA-5 paper, which shows that at its lowest angle ($60^\circ$) the elongation is $\approx 60^\circ$.
2. The principal axis analysis of the elongation, that is, the eigenvalues of the elongation, when rotated, they yield exaclty the distribution matrix. And is the distribution matrix interesting? Yes it is: the $\sigma_{meas}$ provided by the weighted variance computation in the VT detector acutally already includes slightly the elongation description, as we can see from the plots (this error as well as the $\sigma_{fit}$ error increase with the elongation), but this variance computation does not include the $\sigma_{xy}$ instead contained in the distribution matrix. This covariance essentially describes how the x and y measurement are correlated - which needs to be considered a a sort of in information on elongation. In GENFIT we can add this covariance! This may help - not greatly as we expect the covariance not to be large, as the elongation takes on values between 1 and 2 - but still, it is something that may produce a more comprehensive analyisis of the VT cluster reconstruction. Updates will follow $\rightarrow$ the pulls vary a little, the sigma of the pull decreases slightly in most of the cases, in some other cases it increases. I mean, the code structure is now available, once the software-based alignement will be carried out, one can check again the difference with and without covariance matrix.

## Update 26/06/2026
The VA cited in the MSD is the electronic chip used to physically read out the strips. To visualize it, see `Characterization of 150 um thick MSD for FOOT experiment.pdf`. Somewhere in the code there is a VA gain correction, which is used to equalize the strip response and generate a more uniform detector response.

Another important issue of the MSD is the $f(\eta)$ function. To thoroughly understand this concept, see the paper `Turchetta - Spatial Resolution in Silicon MSD.pdf` in the thesis material. The key point is that the MSD strips are analog. Within the $150~\mu\mathrm{m}$ distance between two adjacent readout strips, there are two floating strips. These floating strips generate an $\eta$ signal, which is described in `FOOT_XVI_General_MSD_Zarrella.pdf`. Zarrella applied the $\eta$ correction, which essentially solved the following issue. When a particle deposits its energy directly on the readout strips, nearly all of the charge is collected. However, when the same particle, with the same kinetic energy, passes through a floating strip, part of the charge is lost, mainly due to diffusion processes and capacitive coupling. The $\eta$ correction compensates for this effect. Nevertheless, an important problem still remains in the MSD: we do not know precisely how to compute its spatial resolution. In `poster_PM2021_Silvestre.pdf`, there is an interesting plot showing that it is possible to achieve a spatial resolution better than the digital readout limit of both $150~\mu\mathrm{m}$ (the readout strip pitch) and $50~\mu\mathrm{m}$ (the pitch between two adjacent strips, including floating strips). This result was obtained, at least for carbon ions, which release more energy than protons and therefore allow a more precise charge measurement. A possible approach to better understand the $f(\eta)$ function is to use laser scans, as was done on page 9 of `FOOT_XVI_General_MSD_Zarrella.pdf`. Ideally, this characterization should be performed for every group of four strips, since different strip groups may not exhibit identical behavior. Another possible approach is to implement the algorithm described in `Turchetta - Spatial Resolution in Silicon MSD.pdf`. Finally, unlike the Vertex detector, the MSD currently has no Monte Carlo implementation of the $\eta$ function. The Vertex instead models the relationship between the number of fired pixels and the deposited energy using the parameterization described in `Response of the Mimosa-28 pixel sensor.pdf`. Christian implemented this model in the Monte Carlo using a spiral algorithm that starts from the central pixel and progressively extends outward, distributing the deposited charge among neighboring pixels. Next step: build $f(\eta)$ function. Updates will follow $\rightarrow$ Check [here](#update-13072026).

## Update 13/07/2026
Something that I found difficult to understand is the fact that the $\eta$ variable is iven by the signal of two adjacent readout strips, specifically the ones which have maximum signal.

Now, I have finally implemented the $\eta$ function. To understand the physics behind it, follow the Turchetta paper absolutely. Effectively, the $\eta$ algorithm provides a uniformly distributed $x_{\eta}=P\cdot f(\eta)$, which is our objective. The interesting plots I had are the linearization of the spatial distribution and the actual $f(\eta)$ functions. At the moment **I have implemented this $\eta$ algorithm for each of the 6 MSD planes** (SO AT THE MOMENT I DON'T HAVE AN $\eta$ FOR EACH $150~\mu\mathrm{m}$-PITCH). Through this integral method we wanted to linearize the spatial distribution. It is also possible to implement a laser-based algorithm - follow my presentation for the PhD admission (backup slides) - but of course it was impossible to implement it. The integral method offers the advantage that we can find the $x_{\eta}=P\cdot f(\eta)$ directly from data, and the fact that we can associate a $\sigma_{x_{\eta}}$ to the spatial position $x_{\eta}$ using an algorithm already explained by Turchetta. This is really interesting for us because we are aiming to reduce the overestimated errors of the MSD in FOOT. It is expected that this $\eta$ algorithm will finally provide a better resolution for the spatial uncertainty of the MSD. Updates will follow $\rightarrow$ Check [here](#update-14072026).

## Update 14/07/2026
So, now the situation is the following: we have one $f(\eta)^p$ for each plane $p$, and an $f(\eta)^{all}$ for all planes. The parameters of $f(\eta)^{all}$ are compatible with the $f(\eta)$ defined by averaging all the parameters of $f(\eta)^p$. All these functions are a linear combinations of error function (belonging to the group of the sigmoid functions). As the discrepancy between the highest and the lowest $f(\eta)$ is about $2~\mu\mathrm{m}$ (**interesting, it could be used as systematics**), and there is compatibility between the $f(\eta)^{all}$ and the $f(\eta)$ from averaged parameters, we can directly implement the position model and the uncertainty model with $f(\eta)^{all}$. Updates will follow in the following days.

## Update 17/07/2026
### Vertex Tracker update
In Quality Plots analysis I have represented the **residuals of the VT** (for each tracking plane) (x and y views) **as a function of the cluster position** (x and y), to **check for possible rotations** of the detector plane. Four 2D histograms were produced:

   - Residual x vs x_clus (VT1x)
   - Residual x vs y_clus (VT1x)
   - Residual y vs x_clus (VT1y)
   - Residual y vs y_clus (VT1y)

The "direct" plots (*residual x vs x_clus*, *residual y vs y_clus*) **are flat**, **with mean ≈ 0**, indicating no significant scale/pitch error. The "cross" plots (*residual x vs y_clus*, *residual y vs x_clus*) instead **show a clear linear trend**, **with slopes of opposite sign and similar magnitude** (~10⁻³ rad). This is the typical signature of a small **rotation of the VT1 sensor plane around the beam axis (z)**, rather than a translation or scale error.

**Mathematical explanation (rotation misalignment)**: the sensor's local axes (x′, y′) are rotated by a small angle $\theta$ with respect to the true experimental frame (x, y). For a point at true position (x, y), the coordinate measured by the rotated sensor is obtained from the standard rotation:

$$
x' = x\cos\theta + y\sin\theta
$$

$$
y' = -x\sin\theta + y\cos\theta
$$

For small $\theta$ (as is the case here, $\theta \sim 10^{-3}$ rad), using $\cos\theta
\approx 1$ and $\sin\theta \approx \theta$:

$$
x' \approx x + \theta \cdot y
$$

$$
y' \approx y - \theta \cdot x
$$

The physical hit position does not move - only the axis along which the sensor measures it
is tilted. The reconstructed residual is:

$$
\text{residual}_x = x' - x \approx \theta \cdot y
$$

This explains why **residual_x grows linearly with y_clus**, and residual_y grows linearly with x_clus, with slopes of opposite sign: both are set by the same rotation angle $\theta$, extracted directly from the observed slopes (~10⁻³ rad, i.e. ~0.05–0.06°).

## Update 22/07/2026
We have finally implemented the position impact estimate through the $\eta$ algorithm and its associated uncertainty. They are defined in Turchetta paper. Currently the **pull distribution for the MSD have improved**, as for carbon ions their sigma is around 0.7 and for lighter ions it reaches 0.9. The mean of the pull distributions are still different from 0, as the software-based alignment procedure is still ongoing.

Note that the above results for the pulls are obtained considering the following:
- First, I use $1-\eta$ and not $\eta$ to perform the analysis, following the definition of Turchetta. Basically in this way is better as from low to high values of $\eta$ we are going from low to high $x$ spatial positions.
- Second, recall that the $f(\eta)$ function, always calibrated using $1-\eta$ and not $\eta$, has been obtained just with carbon ions.
- Third, the derivative of $f(\eta)$, that is $\frac{df}{d\eta}$, used to compute $\sigma_{eta}, is asymmetric due to the asymmetric $\eta$ distributions. This is something that is not right, probably due to something related to the electronics (indeed we should expect $\eta$ distributions symmetric to $0.5$). This is why the current $\frac{df}{d\eta} =$ min$(\frac{df}{d\eta},362)$. Look at the `derivative.root` plot in `eta_funct` directory to understand why this is used.
- Fourth, the current error on the position is $\sigma = \sqrt(\sigma_{eta}^2+\sigma_{diff}^2)$, where:
   1. $\sigma_{eta}$ is the error directly found in the Turchetta algorithm
   2. $\sigma_{diff} = 10 \mu$m is a constant diffusion error due to the fact that charge deposit fluctuations could occur while charge deposit energy in the strips. Instead of setting this value to a fixed number (which is equal to the typical width of the diffusion cloud in silicon strips), it would be more accurate to use a per-fragment error based on the spread of the ADC signal generated in the strips. **The diffusion error would then be the $\sigma$ of the energy deposition distrubution for each fragment**.

So, on one hand, the situation of the pull has **improved a lot**. This **improved also the chi-squared distribution** (by shifting the peak (for carbons!) from $\approx0.8$ to $\approx1.$ something and reducing the events in the tail of the chi-squared PDF (always for carbon!)). The **p-value distribution** (always analyzed for carbons due to the higher statistics) shifted a little bit towards the left (due to the fact that the chi-squared has shifted towards the right); I would say that the fact that the number of high p-values has decreased is given by the fact that now the MSD error have been decreased (reaching from tens to hundreds of micrometers (actually **MSD2 plane has an eviednt problem** that needs to be addressed: the errors reach $\approx300~\mu\mathrm{m}$, something which is completely wrong. The fact that the MSD2 reaches these errors is probably due to the $\frac{df}{d\eta}$ values, which reach 362.)).

I have added also two plots: the pull as a function of $f(\eta)$ and the $\sigma_{eta}$ as a function of the $f(\eta)$. While the first plot is interesting because it is another way to see that the $f(\eta)$ distribution is flat (valid for carbons, less valid for Helium fragments - probably because the $f(\eta)$ we are using have been calibrated on carbon ions!!), the second one is interesting because it shows the fact that the error increases with $\frac{df}{d\eta}$ (in other words, in correspondance with the readout and floating strips, the higher number of entries of the $\eta$ generate larger errors).
