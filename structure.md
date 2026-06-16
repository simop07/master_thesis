# FOOT @ CNAF — Quick Reference Guide

---

## 1. Server access

From your local machine:

```bash
ssh foot-ui01
```

Expected output:

```bash
Welcome to ui01-foot.cloudcnaf - AlmaLinux 9.8
```

---

## 2. Environment variables

The following variables and aliases are defined in `~/.bashrc`:

```bash
export MYSWDIR=/opt/exp_software/foot/spasquin
export MYDATADIR=/storage/gpfs_data/foot/spasquin

source /opt/exp_software/foot/set_HTC_env.sh

alias SetupROOT="source /opt/exp_software/foot/root_shoe_foot.sh"
```

To inspect them:

```bash
cat ~/.bashrc
```

---

## 3. Directory structure

### Home (`~`)

```text
.ssh/
.vscode-server/
HTC_SHOEreco/       ← HTCondor job submission files
...
```

### Data area (`$MYDATADIR`)

```text
/storage/gpfs_data/foot/spasquin/
├── error/
├── miscellaneous/
├── z_analysis/
└── z_analysis_post/
```

### Software area (`$MYSWDIR`)

```text
/opt/exp_software/foot/spasquin/shoe_glbAnalysis/shoe/
├── Analysis/
├── Ancillary/
├── G4Simulation/
├── Libraries/
├── Reconstruction/
├── Simulation/
├── build/
├── cmake/
├── docs/
├── CMakeLists.txt
└── README.md
```

### Build directory

```text
shoe/build/
├── Analysis/
├── Libraries/
├── Reconstruction/
├── Simulation/
├── bin/
├── CMakeCache.txt
├── Makefile
├── setupFOOT.sh
└── setupFOOT.csh
```

> ⚠️ **Do not modify configuration files directly inside `build/`.** They may be overwritten during recompilation. Always edit the originals in the source repository (see [Section 6](#6-configuration-files)).

---

## 4. Typical workflow

### 4.1 Login and environment setup

Every new session requires sourcing the environment before running any SHOE executable:

```bash
cd $MYSWDIR/shoe_glbAnalysis/shoe/build
source setupFOOT.sh
```

### 4.2 Compilation

```bash
make -j8
```

### 4.3 Reconstruction (`DecodeGlb`)

Run from `build/Reconstruction`:

```bash
cd Reconstruction

./../bin/DecodeGlb \
  -in  $MYDATADIR/z_analysis/Decoded_CNAO2025_8040_TWBarCalib.root \
  -out $MYDATADIR/Decoded_Analysis_CNAO2025_8040_TWBarCalib_test.root \
  -exp CNAO2025 \
  -run 8040 \
  -nev 10
```

This step decodes and reconstructs data from the input ROOT file. It is computationally intensive — for large datasets, use HTCondor (see [Section 5](#5-batch-processing-with-htcondor)).

### 4.4 Analysis (`DecodeGlbAnalysis`)

Also run from `build/Reconstruction`:

```bash
./../bin/DecodeGlbAnalysis \
  -in  $MYDATADIR/z_analysis_post/Decoded_CNAO2025_8040_TWBarCalib.root \
  -out $MYDATADIR/z_analysis_post/Decoded_Analysis_CNAO2025_8040_TWBarCalib.root \
  -exp CNAO2025 \
  -run 8040
```

This step performs custom analysis and extracts the information of interest from the reconstructed data.

---

## 5. Batch processing with HTCondor

Navigate to the scripts folder:

```bash
cd $MYSWDIR/shoe_glbAnalysis/shoe/Reconstruction/scripts
```

Inspect the batch script:

```bash
less runShoeBatchT1.sh
```

Submit a batch production:

```bash
./runShoeBatchT1.sh \
  -i /storage/gpfs_data/foot/shared/DataCNAO2025sync/ \
  -o $MYDATADIR/error \
  -c CNAO2025 \
  -r 8040
```

Monitor jobs:

```bash
condor_q          # one-shot view
watch condor_q    # auto-refresh
```

Submission files are stored in `~/HTC_SHOEreco/`, organized by run:

```text
~/HTC_SHOEreco/
├── CNAO2025_8040/
├── submitDAG_CNAO2025_8040_8040.sub
├── submitDAG_CNAO2025_8040_8040.sub.condor.sub
├── submitDAG_CNAO2025_8040_8040.sub.dagman.log
├── submitDAG_CNAO2025_8040_8040.sub.dagman.out
├── submitDAG_CNAO2025_8040_8040.sub.nodes.log
└── submitDAG_CNAO2025_8040_8040.sub.metrics
```

Using Condor is nice because is like doinf `DecodeGlb` but with more computational power. Condor uses two tasks: first it separates the output so that it can process them in parallel, then it merges in a unique `ROOT` file.

---

## 6. Configuration files

Configuration files (`*.cfg`) exist in two locations:

```text
shoe/Reconstruction/config/          ← source (edit here)
shoe/build/Reconstruction/config/    ← generated copy (may be overwritten)
```

Always edit the source version. The build copy is regenerated during compilation and local changes to it may be lost.

---

## 7. Where to edit code and what to recompile

The SHOE source is split into modules with different roles. Which executables need to be rerun depends on which module was modified.

### 7.1 Modifying `Analysis/`

Example files:

```text
Analysis/TANAbase/TANAactQualityPlots.cxx
Analysis/TANAbase/TANAactQualityPlots.hxx
```

Analysis code is only used by `DecodeGlbAnalysis`. The reconstruction output files remain valid, so only recompile and re-run the analysis step:

```bash
source setupFOOT.sh
make -j8
cd Reconstruction
./../bin/DecodeGlbAnalysis ...
```

### 7.2 Modifying `Libraries/`

Example files:

```text
Libraries/TAGfoot/TAGactKFitter.cxx
Libraries/TAVTbase/TAVTactBaseNtuCluster.cxx
```

Library changes affect the reconstruction itself. The existing ROOT reconstruction files are no longer valid and must be regenerated:

```bash
source setupFOOT.sh
make -j8
cd Reconstruction
./../bin/DecodeGlb ... # Or use Condor to gain computational power
./../bin/DecodeGlbAnalysis ...
```
