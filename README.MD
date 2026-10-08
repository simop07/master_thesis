<p align="center"> <img src="./FOOT_logo.png" width="90%"> </p>

# Optimization of the tracking system for the FOOT experiment

[![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.19881390.svg)](https://doi.org/10.5281/zenodo.19881390)
[![GitHub license](https://img.shields.io/github/license/simop07/data_trigger_gen_FPGA?cache=1)](https://github.com/simop07/data_trigger_gen_FPGA/blob/main/LICENSE)

## Abstract

Modeling nuclear fragmentation is essential for improving treatment planning in Particle Therapy (PT) and assessing radiation-induced hazards for both astronauts and electronics in Space Radiation Protection (SRP). However, scarce experimental data on target and projectile fragments hinders the benchmarking of nuclear models in deterministic codes and Monte Carlo simulations. The FOOT (FragmentatiOn Of Target) experiment addresses this by measuring double-differential fragmentation cross sections with $5$-$10\%$ precision, using an innovative inverse kinematic approach. This thesis optimizes the FOOT global tracking system in the hardware and software aspects. On the hardware side, a dedicated `VHDL` firmware is implemented for the DE10-Nano trigger board of the MIMOSIS-2.1 Vertex Tracker (VT), exploiting its faster readout (frame time of $5\,\mu\text{s}$). The firmware features a configurable show-ahead FIFO and improved event-building synchronization, plus `C++` software interfacing it to the DAQ. Tests at LNF show reliable operation at trigger rates up to $20\,\text{kHz}$. On the software side, data from the CNAO 2025 campaign (C-C collisions at $200\,\text{MeV/u}$, $5\,\text{mm}$ graphite target) is used to optimize event reconstruction. Pull-distribution studies of the current MIMOSA-28 VT and the Microstrip Silicon Detector (MSD) reveal residual misalignments and inaccurate positional uncertainties. A dedicated $\chi^2$ minimization algorithm for the longitudinal displacements of VT planes, combined with software-based alignment, significantly improves the VT position reconstruction. For the MSD, the original center-of-gravity clustering, which overestimates positional uncertainties, is replaced by a non-linear algorithm based on the $f(\eta)$ integral method, improving uncertainty estimates and yielding more uniform $p$-value distributions with fewer badly reconstructed tracks. The final GENFIT momentum resolution is $3.34\%$ for 12C at $200\,\text{MeV/u}$, $5\,\text{mm}$ and $2.96\%$ for 4He fragments, consistent with the FOOT requirement of $3$-$5\%$.

---

## Repository structure

```
.
├── Dissertation/               # LaTex source files for the Master's thesis
├── plots/                      # Output plots, macros and ROOT files from the analysis
├── analysisNotes.md            # Workflow of analsis
└── analysisStructure.md        # Structure of analysis files on TIER1 at CNAF
```