# Window w-bwtinv-20260912T064334Z attestation (PR-4 v1.1)

- label: measured (parallel; attestation attached)
- window_start_utc: 2026-09-12T06:43:34Z
- window_end_utc: 2026-09-12T07:03:53Z
- binaries: unbwt_bench_ref.exe sha256 E47D0A51006E3679553EBA5648545A57F91B5741BF944DF18EB96A8CB4316DE3 (build-i9-bwtinv); unbwt_bench_omp.exe sha256 FC345062411664FD94B97DB09FC2B671FF8E2B9023B6A2496FBB4E77902558FE (build-i9-bwtinv)
- commands: & build-i9-bwtinv/unbwt_bench_ref.exe --reps=7 --warmup=1 --csv=prototypes/i9-bwtinv/measure/w-bwtinv-20260912T064334Z-degraded.csv dickens webster enwik8 ; same for _omp
- inputs (sha256 computed below)
- reps: 7 (raw per-rep values in the CSV reps_ms column and in w-bwtinv-20260912T064334Z-*-stdout.txt)
- host load (1 Hz Get-Counter \Processor(_Total)\% Processor Time): pre-window median 31.0007404946679 %, max 71.3050529127588 %; during-window median 36.3374678126328 %, max 90.8898640448409 %; own bench CPU share column in prototypes/i9-bwtinv/measure/w-bwtinv-20260912T064334Z-load.csv
- pinning/priority: no affinity mask set (harness sets HIGH_PRIORITY_CLASS); make targets use all logical processors
- threads: ref exe rows = 1; omp exe rows = per-variant t in the variant name (t1/t4/t8/t16); libsais internal = explicit num_threads
- concurrent measurement jobs: none started by this lane; see host-load column
B24C37886142E11D0EE687DB6AB06F936207AA7F2EA1FD1D9A36763C7A507E6A  C:\Users\<user>\Documents\ANVIL\scratch\ratio-first\corpora\silesia\dickens
6A68F69B26DAF09F9DD84F7470368553194A0B294FCFA80F1604EFB11143A383  C:\Users\<user>\Documents\ANVIL\scratch\ratio-first\corpora\silesia\webster
2B49720EC4D78C3C9FABAEE6E4179A5E997302B3A70029F30F2D582218C024A8  C:\Users\<user>\Documents\ANVIL\scratch\ratio-first\corpora\enwik8dir\enwik8
