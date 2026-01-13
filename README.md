**Xpressive Sound Design Suite**

A sound synthesis toolkit built with Qt 6 and C++17 to compliment LMMS Xpressive. This suite is designed to generate complex mathematical audio expressions for the Xpressive audio engine (both Legacy and Modern parsing), allowing users to design everything from classic 8-bit SID chips to modern FM synthesis and percussive transients.

**Key Features**

The suite is divided into several tabs each targeting a specific era or style of sound design.

**Synthesis & Sequencing**

SID Architect: A multi-segment sequencer for creating complex instrument chains with custom envelopes, frequency offsets, and automated waveforms (Triangle, Square, Saw, PWM).

Bessel FM: An advanced Frequency Modulation laboratory featuring a library of 40 classic 80s presets (DX7 Electric Piano, LatelyBass, etc.).

Arp Animator: Generates mathematical expressions for polyphonic arpeggiators with customizable chords (Major, Minor, Diminished, Augmented).

Lead Stacker: Create thick, "super-saw" style leads by stacking unison voices with precise detuning.

**Percussion & Noise**

Drum Architect: A specialized tool for designing kicks, snares, and cymbals using pitch-enveloped oscillators. It includes dynamic filter suggestions based on the base frequency.

Noise Forge: Generates sample-rate-reduced noise expressions for lo-fi textures and percussion.

**Utilities & DSP**

PCM Sampler: Converts standard WAV files into optimized 4-bit mathematical strings for playback within the Xpressive engine.

Filter Forge: Designs FIR-style filters (Low-Pass/High-Pass) with adjustable tap counts. Experimental, limited success so far with FIR.

Harmonic Lab: An additive synthesis interface with 16 harmonic sliders for building complex timbres.

**Technical**

Framework: Qt 6.x (Widgets) 
Language: C++17 
Build System: CMake 3.16+ 

**Usage**

Select a Mode: Use the top tabs to choose your sound design environment (e.g., Bessel FM for pads, Drum Architect for percussion).

Configure Parameters: Adjust sliders, spin boxes, and drop-downs to shape the sound.

Generate: Click the "Generate" button to produce the mathematical expression.

Export: Use the Copy Clipboard function to take the generated string and paste it directly into your Xpressive engine environment.

XPF Packaging: Use the XPF Packager tab to wrap your raw code into an XML-based .xpf file for easy distribution.

**License**

This project is licensed under the MIT License - see the LICENSE file for details.

Copyright (c) 2026 Ewan Pettigrew
