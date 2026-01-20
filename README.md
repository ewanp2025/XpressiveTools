**Xpressive Sound Design Suite**

A sound synthesis toolkit built with Qt 6 and C++17 to compliment LMMS Xpressive. This suite is designed to make life easier when working with the Xpressive instrument by generating complex mathematical audio expressions (selectable between both Legacy and ExprTk parsing). Instead of manually working with complex XML files or trying to write math formulas from scratch, this suite lets you generate high-quality patches, percussion, and phonetic speech strings using a proper UI.

This is still very much work in progress.

**Key Features**

The suite is divided into several tabs each targeting a specific era or style of sound design.

**Tab Breakdown & Progress**

SID Architect	- Build complex C64-style oscillator chains with decay and frequency offsets. STABLE but will benefit from more features.

PCM Sampler	- Converts WAV files into optimized math expressions for playback. WIP modern and legacy mode.
* Under closer inspection I can only get the PCM Sampler to work in 1.5-bit (Ternary) logic. My Original Matlab script was 4 bit but I just can't get this right yet*

Console Lab - Generate console chip sounds. WIP requires more features.

SFX Macro - Generate sweeps. STABLE.

Arp Animator - Generate synchronised arpeggios. STABLE.

Wavetable Forge	- An interface for sequencing waveforms and pitches. STABLE but will benefit from more features.

Bessel FM - Generate FM sounds. STABLE.

Harmonic Lab - Generate harmonics with sliders. STABLE.

Drum Designer	- Architect classic percussion (Kicks, Snares, Hats) using code based envelopes and internal LMMS filters. WIP.

Velocilogic	- Maps different expressions to MIDI velocity layers for dynamic playing. WIP only tested with legacy.

Noise Forge - Sample rate control of noise. STABLE.

XPF Packagaer - Placeholder. WIP

Filter Forge	Generates mathematical approximations of FIR filters using last(n) logic. WIP limited success with FIR.

Lead Stacker - Generate Supersaws etc. WIP only tested with legacy.

Randomiser - Semi random expressions. WIP working.

Phonetic Lab	- A text-to-speech engine using SAM phoneme tables to generate vocal formulas. WIP.

Logic Converter	- Migrates code between Xpressive versions (e.g., Time t to Samples s). WIP only works for PCM samples and only with very short ones.

Key Mapper	- Splits the keyboard into zones so you can play different logic on different keys. WIP only tested with legacy.

Step Gate	- A 16-step rhythmic gate to pulse your sounds in sync with the BPM. WIP tested working on legacy.

**Technical**

Framework: Qt 6.x (Widgets) 
Language: C++17 
Build System: CMake 3.16+

Panning: Due to the XML parsing, the PAN1 attributes are currently disabled to prevent crashes. You'll need to set your panning manually in the LMMS instrument editor for now.

Filter Silence: If you select an LPF and set the frequency to 0, you won't hear a thing. The app now defaults to 100Hz minimum to help avoid this.

ADSR: The drum generator sets the internal Volume Envelope Amount (AMT) to 0. This is intentional so that the mathematical envelopes in your code have total control over the "snappiness" of the sound.

**Usage**

Select a Mode: Use the top tabs to choose your sound design environment (e.g., Bessel FM).

Configure Parameters: Adjust sliders and drop-downs to shape the sound.

Generate: Click the "Generate" button to produce the mathematical expression.

Export: Use the Copy Clipboard function to take the generated string and paste it directly into your Xpressive engine environment in O1.

For Drum Generator generate the .xpf file and save it to your instruments folder. Drum generator workss differently so that it can control the instrument filters external to the expression (as I have had limited success with FIR with last(n).

**License**

The phonemes were worked out by various versions of reversed engineered SAM (1982) found on github which was from the Commodore 64 and no longer exists / is assumed abandonware. Therefore due to this assumption, I cannot put my code under any specific open source software license.

Copyright (c) 2026 Ewan Pettigrew
