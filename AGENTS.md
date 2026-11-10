# God Hand Windows source port

## User requirements

- Port the existing PS2 game; preserve the original gameplay and assets.
- Follow the OpenGOAL style of recovering readable source and replacing platform dependencies with native implementations.
- Do not substitute a packaged PS2 emulator or a generic instruction-translated runtime for the requested source port.
- Useful source, reference inputs, installed portable tools, evidence and project documentation belong in `E:\godhand_windows`.
- Build outputs, object files, test executables, temporary downloads, caches, headless projects and bulky logs belong in `G:\middle_thing\godhand_windows`.
- The user has authorized installing needed tools. Do not ask again for routine tool installation or reversible implementation work.
- Public GitHub repository: `https://github.com/idantony/godhand-windows`. Keep local commits for rollback; push only meaningful execution/integration milestones, not every small module. Exclude original game data, raw instruction/decompiler dumps, dependencies and caches. Publish curated progress separately from private analysis.
- After each meaningful GitHub push, review the overall execution frontier, remaining blockers and critical path; adjust the next work and independent agent assignments when evidence warrants it. Keep this review short and actionable. Local commits retain rollback points; formal overall reviews use the GitHub push milestone, following the user's latest clarification.
- After milestone pushes, give the user a rough workload-completion estimate and remaining largest items in chat only. Do not publish that subjective percentage to GitHub or confuse it with upstream matching coverage.
- Maintain the reusable `skill/ps2-source-port` throughout the work. Preserve a resumable project state there; label unverified techniques and never generalize God Hand-specific facts into all PS2 games.
- Development agents: use at most `gpt-6.1-sol`; use `gpt-6-luna` for mechanical bounded work. Do not enable Codex Fast.

## Ground truth and dependencies

- Actual input is `God Hand (USA)(Uncensor Patch v1.0).iso`, boot ID SLUS-21503, NTSC-U 1.00.
- The supplied patched ELF is not byte-identical to the upstream retail target, including its loaded image. The local hash and bytes in `analysis/disc-baseline.json` are ground truth.
- `vendor/god-hand-decomp` is pinned at `eaae18d0dfab1f9a0df276ae5ca1581ad326faf3`. Preserve its working tree and license. Its matching report is not our native-port progress.
- All 81 disc overlays match the hashes committed in the upstream manifest. This is not proof that the entire ISO or boot ELF matches retail.
- Standalone Ghidra + EmotionEngine Reloaded analyzes the ELF. REA 6.8.0's Windows backend does not admit PS2 ELF; do not bypass its gate by misclassifying the binary.

## Verification

- `python tools/build-native.py` builds and runs native C module tests, writing `analysis/native-verification.json` and G: build outputs.
- `python -B tools/verify_rng.py` compares two recovered RNG functions with a restricted, development-only model of the actual MIPS routines. This is not PS2 hardware validation and is never shipped as game runtime.
- Numerical adaptations must preserve defined 32-bit wrap and relevant PS2 floating-point behavior. Do not assume Windows default float conversion matches EE.
- Original assets are read-only. Avoid bulk extraction, duplicated ISOs and all destructive cleanup. Move only verified task-generated intermediates to the dedicated G: tree.
- Report separately: readable source recovered, native modules built, modeled differential checks, and actual gameplay tested. Only mark playable after a real original-game scene has been observed and verified.
