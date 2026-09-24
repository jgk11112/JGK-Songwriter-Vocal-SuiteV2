# JGK Songwriter Vocal Suite Roadmap

## Phase 1 — Foundation (current)
- One monorepo, separate VST3 targets
- Shared cream / graphite / light-blue JGK visual language
- Separate parameter state per product
- Functional first-pass DSP for each product
- TUNE monophonic correction prototype
- Windows CI configuration

## Phase 2 — TUNE quality
- Improve fundamental tracking (YIN/MPM hybrid + confidence)
- Voiced/unvoiced detection so breaths and consonants are not pitch shifted
- Better note-transition logic and hysteresis
- Low-latency high-quality pitch shifter
- Formant preservation
- Natural vibrato analysis rather than simple LFO enhancement
- Latency reporting and host compensation
- Stress tests at 44.1/48/88.2/96/176.4/192 kHz

## Phase 3 — Vocal processors
- Dynamic EQ and spectrum interaction
- Compressor auto-gain and detector shaping
- Split-band de-essing
- Oversampled saturation modes
- Reverb pre-delay/decay/diffusion/tone controls
- Tempo-synchronised delay from host BPM
- True-peak oversampled limiter + LUFS metering

## Phase 4 — Vocal Chain
- Use the exact same DSP engines as the individual plugins
- Per-module bypass and routing
- Natural singer-songwriter presets
- Macro controls mapped to safe parameter ranges
- Save/load presets

## Phase 5 — Release QA
- FL Studio validation first
- VST3 validator / pluginval tests
- Automation and state recall tests
- Mono/stereo tests
- CPU/denormal tests
- Installer, versioning and signed Windows binaries
- Licensing/branding review before commercial release
