**Xpressive Sound Design Suite**

A sound synthesis toolkit built with Qt 6 and C++17 to compliment LMMS Xpressive. This suite is designed to generate complex mathematical audio expressions for the Xpressive audio engine (selectable between both Legacy and ExprTk parsing), allowing users to design everything from classic 8-bit SID chip produced sounds to modern FM synthesis and percussive transients.

**Key Features**

The suite is divided into several tabs each targeting a specific era or style of sound design.

**Synthesis & Sequencing**

SID Architect: A multi segment sequencer for creating complex instrument chains with custom envelopes, frequency offsets, and automated waveforms (Triangle, Square, Saw, PWM).

Bessel FM: An advanced Frequency Modulation laboratory featuring a library of 40 classic 80s presets (DX7 Electric Piano, LatelyBass, etc.).

Arp Animator: Generates mathematical expressions for polyphonic arpeggiators with customizable chords (Major, Minor, Diminished, Augmented).

Lead Stacker: Create thick, "supersaw" style leads by stacking unison voices with precise detuning.

**Percussion & Noise**

Drum Architect: A specialised tool for designing kicks, snares, and cymbals using pitch-enveloped oscillators. It includes filter suggestions based on the base frequency. - Very early version, the aim is to control the filter within the GUI by exporting an .xpf as most drums require filters.

Noise Forge: Generates sample-rate-reduced noise expressions for lofi textures and percussion.

**Utilities & DSP**

PCM Sampler: Converts standard WAV files into optimised 4 bit mathematical strings for playback within the Xpressive engine.

Filter Forge: Designs FIR style filters (Low-Pass/High-Pass) with adjustable tap counts. Experimental, limited success so far with FIR.

Harmonic Lab: An additive synthesis interface with 16 harmonic sliders for building complex timbres.

Text to Speech: Text to Speech based on phonemes from Software Automatic Mouth

**Technical**

Framework: Qt 6.x (Widgets) 
Language: C++17 
Build System: CMake 3.16+ 

**Usage**

Select a Mode: Use the top tabs to choose your sound design environment (e.g., Bessel FM for pads, Drum Architect for percussion).

Configure Parameters: Adjust sliders and drop-downs to shape the sound.

Generate: Click the "Generate" button to produce the mathematical expression.

Export: Use the Copy Clipboard function to take the generated string and paste it directly into your Xpressive engine environment.

**License**

The phonemes were worked out by various versions of reversed engineered SAM (1982) found on github which was from the Commodore 64 and no longer exists / is assumed abandonware. Therefore due to this assumption, I cannot put my code under any specific open source software license.

Copyright (c) 2026 Ewan Pettigrew
