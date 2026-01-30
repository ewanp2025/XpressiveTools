**Xpressive Sound Design Suite**

A sound synthesis toolkit built with Qt 6 and C++17 to compliment LMMS Xpressive. This suite is designed to make life easier when working with the Xpressive instrument by generating complex mathematical audio expressions (selectable between both Legacy and ExprTk parsing). Instead of manually working with complex XML files or trying to write math formulas from scratch, this suite lets you generate high-quality patches, percussion, and phonetic speech strings using a proper UI.

This is still very much work in progress. The intent was to create a suite of tools for creating Xpressive expressions as I had realised that Xpressive can do many things that people are asking for in instruments within the LMMS forums. However, the 'language' of the time domain may be quite strange to people who have no prior experience with signal processing.

**Key Features**

The suite is divided into several tabs each targeting a specific era or style of sound design.

**Tab Breakdown**
•  SID Architect – Build multi-segment C64-style oscillator chains with per-segment decay and frequency offsets. 
•  PCM Sampler – Converts WAV files into mathematical expressions for sample playback within Xpressive with optimized 4-bit quantisation. 
•  Console Lab – A library of classic chip-synth waveforms (NES, GB, C64) with live bit-crushing and quantisation. 
•  SFX Macro – Generates exponential pitch sweeps and glides for transitions and cinematic risers. 
•  Arp Animator – Creates rhythmic, synchronised arpeggios with 8 bit style presets and BPM-sync capabilities. 
•  Wavetable Forge – An interface for sequencing complex waveform, pitch, and PWM changes over time. 
•  Bessel FM – A dual-operator FM synthesis engine with a library of 80s inspired DX7 and bell tone presets. 
•  Harmonic Lab – A 16 harmonic additive synthesiser with real time spectrum visualization and slider based control. 
•  Drum Designer – Architects percussion by combining exponential pitch-drop envelopes with simulated filter behaviours. Unlike the other tabs, this one exports a .xpf instrument so that the instruments filter can be controlled due to limited success with FIR.
•  Velocilogic – Maps different code expressions to MIDI velocity layers. 
•  Noise Forge – Provides precise sample rate control over random noise generation for digital textures. 
•  XPF Packager – Wraps your raw expressions into a fully-formed LMMS instrument file for instant loading. Placeholder.
•  Filter Forge – An experimental lab for generating mathematical approximations of FIR filters using $last(n)$ logic. Placeholder. Limited success so far with FIR results.
•  Lead Stacker – A unison engine that generates supersaw stacks with adjustable detune, sub oscillators, and drift. 
•  Randomiser – Uses chaos algorithms to generate semi random mathematical patches for instant inspiration. 
•  Phonetic Lab – A text-to-speech engine utilizing SAM phoneme tables to generate complex vocal and speech formulas. WIP. Works in principle as SAM does not use filters. However,  results are not satisfactory yet.
•  Logic Converter – A migration tool to bridge code between Xpressive versions, converting between Time ($t$) and Samples ($s$). WIP.
•  Key Mapper – Allows keyboard splitting by assigning different logic expressions to specific MIDI key ranges. 
•  Step Gate – A 16-step rhythmic sequencer that aims to pulse sounds in sync with the project BPM. 
•  Numbers 1981 – A dual-oscillator sequencer inspired by early 80s minimal synth patterns and random streams. 
•  Delay Architect – Builds multi tap echo chains using math based delay lines with adjustable feedback and sample rates. 
•  Macro Morph – A high level dashboard that morphs complex sound "vibes" using simplified Macro knobs. 
•  String Machine – Emulates vintage Solina and Logan string ensembles with evolving filter swells and ensemble width. 
•  Hardware Lab – A direct parameter analog suite for designing patches with virtual ADSR, LFOs, and resonance. 
•  West Coast Lab – Emulates Buchla-style nonlinear waveshaping using parallel and series wavefolding topologies. 
•  Modular Grid – A visual, modular synethesiser emulator / node based environment for connecting oscillators and logic modules to "draw" your synth. 
•  Spectral Resynthesiser – Analyses external WAV stabs to reconstruct sounds through windowed harmonic additive synthesis (another way to bring external recordings into Xpressive as an expression). 


**Technical**

Framework: Qt 6.x (Widgets) 
Language: C++17 
Build System: CMake 3.16+

Panning: Due to the XML parsing, the PAN1 attributes are currently disabled to prevent crashes. You'll need to set your panning manually in the LMMS instrument editor for now (for drum designer).


**Usage**

Select a Mode: Use the top tabs to choose your sound design environment (e.g., Bessel FM).

Configure Parameters: Adjust sliders and drop-downs to shape the sound.

Generate: Click the "Generate" button to produce the mathematical expression.

Export: Use the Copy Clipboard function to take the generated string and paste it directly into your Xpressive engine environment in O1.

For Drum Generator generate the .xpf file and save it to your instruments folder. Drum generator works differently so that it can control the instrument filters external to the expression (as I have had limited success with FIR with last(n).

**License**

The phonemes were worked out by various versions of reversed engineered SAM (1982) found on github which was from the Commodore 64 and no longer exists / is assumed abandonware. Therefore due to this assumption, I cannot put my code under any specific open source software license.

Copyright (c) 2026 Ewan Pettigrew
