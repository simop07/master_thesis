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
/opt/exp_software/foot/spasquin/shoe/
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
cd $MYSWDIR/shoe/build
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
cd $MYSWDIR/shoe/Reconstruction/scripts
```

or directly do:
```bash
cd $MYCDRDIR
```

Inspect the batch script:

```bash
less runShoeBatchT1.sh
```

Submit a batch production:

```bash
./runShoeBatchT1.sh -i /storage/gpfs_data/foot/shared/DataCNAO2025sync/ -o $MYDATADIR/{folder name} -c CNAO2025 -r 8040
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

# 7. SHOE Build and Development Workflow

## 7.1 Environment setup

The SHOE source directory is:

```text
$MYSWDIR/shoe/
```

The current build directory is:

```text
$MYSWDIR/shoe/build/
```

The build uses GCC 14.2.0 and ROOT 6.30.02. Load the FOOT/ROOT environment before configuring or compiling:

```bash
source /opt/exp_software/foot/root_shoe_foot.sh
```

Alternatively, if the `SetupROOT` alias is available in the current shell:

```bash
SetupROOT
```

Check that the correct compiler and CMake versions are available:

```bash
which gcc
which g++
g++ --version
which cmake
root-config --version
```

**Important:** CMake remembers the compiler selected when a build directory is first configured. Loading a different compiler environment afterward does not automatically change that cached selection. The current ROOT installation requires using the GCC 14.2.0 toolchain rather than the system default GCC 11.5.0.

## 7.2 Compiling the entire project from scratch

Use this procedure when creating a new build directory or rebuilding the project from scratch.

### Step 1: Load the environment and enter the source directory

```bash
source /opt/exp_software/foot/root_shoe_foot.sh
cd "$MYSWDIR/shoe"
```

### Step 2: Configure a new build directory

Create a separate build directory using the correct compilers:

```bash
cmake -S . -B build \
  -DCMAKE_C_COMPILER="$(which gcc)" \
  -DCMAKE_CXX_COMPILER="$(which g++)" \
  -DCMAKE_BUILD_TYPE=Debug \
  -DFILECOPY=ON
```

The options mean:

- `CMAKE_C_COMPILER` and `CMAKE_CXX_COMPILER`: explicitly select GCC 14.2.0 for C and C++.
- `CMAKE_BUILD_TYPE=Debug`: configure a debug build.
- `FILECOPY=ON`: enable copying the configuration files into the build tree.

### Step 3: Verify the selected compilers

```bash
grep -E '^CMAKE_(C|CXX)_COMPILER(:[^=]+)?=' \
  build/CMakeCache.txt
```

Both compiler paths must point to the GCC 14.2.0 installation.

### Step 4: Compile the project

```bash
cmake --build build --parallel 8
```

Where 8 is the number of parallel jobs. If 8 is too much, select `4`.

### Step 5: Check the build results

If the build finishes successfully, check that the expected executables exist in the generated `bin/` directory and that the configuration files are present.

For example:

```text
shoe/build/bin/
shoe/build/Reconstruction/config/
```

**Important:** A new build directory does not automatically inherit local edits made only to generated files in an older build directory. Reapply or copy any required run-specific settings, and verify them before running reconstruction.

## 7.3 Modifying configuration and geometry files

Configuration files are maintained in two locations:

```text
shoe/Reconstruction/config/          # Source configuration files
shoe/build/Reconstruction/config/  # Generated/copied runtime files
```

Always edit the source configuration files first. Files in the build directory may be overwritten when CMake copies the source configurations.

### Step 1: Edit the source file

For example, to modify a configuration file:

```text
shoe/Reconstruction/config/CNAO2025/TANAcrossSection.cfg
```

Edit that source file, not the generated copy.

Geometry files must likewise be edited in their original source location. The exact location depends on which geometry file is being modified.

### Step 2: Copy the updated configuration into the build tree

From the main source directory, rerun CMake with `FILECOPY=ON`:

```bash
cd "$MYSWDIR/shoe"
source /opt/exp_software/foot/root_shoe_foot.sh

cmake -S . -B build \
  -DFILECOPY=ON
```

This reuses the existing `build` configuration and its cached compiler selection. Do not specify a different compiler when reconfiguring an existing build. This command, practically, copies just the geo and config parameters from the Reconstruction to the build/Reconstruction folder.

### Step 3: Verify the generated file

For example:

```text
shoe/build/Reconstruction/config/CNAO2025/TANAcrossSection.cfg
```

Check that the generated file contains the intended changes before running reconstruction.

If a geometry change also affects compiled geometry code or requires regenerating geometry libraries, rebuild the affected targets or the whole project as appropriate.

## 7.4 Modifying C++ source files

For ordinary changes to an existing C++ source file, there is usually no need to rerun the full CMake configuration. A simple `make` command either in the `build` or `build/Reconstruction` folders is sufficient.

## 7.5 Run-specific reconstruction parameters

The generated parameter file for the CNAO2025 run is:

```text
shoe/build/Reconstruction/config/CNAO2025/FootGlobal.par
```

For run 8040 without a magnetic field, the settings currently used in this setup are:

```text
IncludeDI: n
IncludeKalman: y
Tracking Systems Considered: VT MSD TW
```

These settings are run-specific, not universal defaults.

Verify them before reconstruction, particularly after configuring a new build directory or regenerating copied configuration files.

---

## 8. `TANADetector.cfg` settings for QualityPlots

The detector analysis configuration is stored in:

```text
shoe/Reconstruction/config/CNAO2025/TANADetector.cfg
```

The relevant section of the configuration file is:

```text
// -+-+-+-+-+--+-+-+-+-+--+-+-+-+-+--+-+-+-+-+--+-+-+-+-+--+-+-+-+-+--+-+-+-+-+-
// -+-+-+-+-+--+-+-+-+-+--+-+-+-+-+--+-+-+-+-+--+-+-+-+-+--+-+-+-+-+--+-+-+-+-+-
//
// This is a Configuration File for FOOT analysis
//
// Campaign CNAO2024

// -+-+-+-+-+--+-+-+-+-+--+-+-+-+-+--+-+-+-+-+--+-+-+-+-+--+-+-+-+-+--+-+-+-+-+-
// Parameters for Analysis
// -+-+-+-+-+--+-+-+-+-+--+-+-+-+-+--+-+-+-+-+--+-+-+-+-+--+-+-+-+-+--+-+-+-+-+-
//
// Using DecodeGlb file as input:
PtReso:        0
ThetaReso:     0
MSDetaFunc:    0
DataReduction: 1

// Using DataReduction file as input:
MassReso:      0
CrossSection:  0
```

The parameters to enable depend on the type of analysis being performed:

- **QualityPlots analysis**:
  ```text
  DataReduction: 1
  ```

- **MSD η-function analysis**:
  ```text
  MSDetaFunc: 1
  ```

Only the required option should be enabled for the corresponding analysis.

---

## 9. Where the 'cuts' are defined (Exp_cuts)
The cuts for experimental data are stored in this file:

```text
shoe/Reconstruction/config/CNAO2025/TANAcrossSection.cfg
```

## 9. Update the Dissertation folder
To update the dissertation folder, simply run this command in bash:

```bash
cp -r /mnt/d/Simo/UniBo/Magistrale/2\ Anno/Thesis/Dissertation/ ~/desktop/magistrale/master_thesis/
```