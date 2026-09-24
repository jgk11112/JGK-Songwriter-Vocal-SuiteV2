# JGK Songwriter Vocal Suite

A single monorepo containing **separate Windows 64-bit VST3/Standalone plugins** designed around a simple singer-songwriter workflow:

- **TUNE** – monophonic real-time pitch correction
- **EQ** – vocal-focused low cut, body, presence and air
- **COMP** – easy vocal compression with amount/glue/output
- **DE-ESS** – sibilance control
- **SAT** – warmth, texture and drive
- **SPACE** – vocal reverb
- **ECHO** – vocal delay
- **LIMITER** – final level control
- **VOCAL CHAIN** – one-window chain combining the suite workflow

## Design direction

Warm cream/off-white upper panel, graphite advanced area, light-blue audio and metering accents, a black guitar-pick JGK mark, and a subtle `By JGK` signature. The front panels stay intentionally simple; deeper controls belong in Advanced views as development continues.

## Current status

**0.1 development foundation.** The repository is structured to build all nine products as independent VST3 binaries from shared code. EQ, compression, de-essing, saturation, reverb, delay and limiting have functional first-pass DSP. TUNE contains an original monophonic pitch detector + real-time correction prototype. It is intentionally labelled development software until listening tests, artifact reduction, formant work, latency testing and DAW validation are complete.

Do not market the current TUNE algorithm as equivalent to mature commercial pitch-correction products yet. The goal is to iterate it into a strong, natural singer-songwriter tool rather than hide unfinished DSP behind a polished UI.

## Build on Windows

Requirements:

- Visual Studio 2022 with **Desktop development with C++**
- CMake 3.24+
- Git

PowerShell:

```powershell
git clone <your-repo-url>
cd JGK-Songwriter-Vocal-Suite
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --parallel
```

The VST3/Standalone products are created inside the CMake build tree. The repo does not automatically install them into the system VST3 folder.

## Architecture

```text
Source/Common/   visual language shared by the suite
Source/DSP/      DSP modules and parameter layouts
Source/Plugin/   shared JUCE plugin shell/editor
.github/         Windows CI build
```

Each target gets a compile-time product kind, so the same high-quality shared shell can produce independent plugins while their DSP and parameter sets remain separate.

## Commercial ownership

No open-source licence has been applied to this repository. See `LICENSE.txt`. JUCE has its own licensing terms and must be used under a licence appropriate to how the final plugins are distributed.

## Product principles

1. Singer first, engineer second.
2. Main screen controls should be immediately understandable.
3. Advanced controls can exist without making the default experience intimidating.
4. Light-blue visuals represent live audio/activity across the entire suite.
5. The JGK guitar-pick mark is the consistent suite identifier.
6. Presets should start from natural, restrained singer-songwriter vocals.
