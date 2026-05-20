# Mushin Preset Bank 0 - Specification

This document describes the technical configuration and intended creative use for the initial bank of 20 "Logical & Creative" presets.

| Preset Name | Core Concept | Routing | Key Controls | Modulation / Sidechain |
| :--- | :--- | :--- | :--- | :--- |
| **Mushin Pumper** | Classic Ducking | Serial | Filter A: LP (Open) | SC: Active, Target: Gain, Amount: -80% |
| **Acid Flashback** | Aggressive Acid | Serial | Filter A: Acid, 400Hz, Res: 0.85 | LFO 1: Saw (2.5Hz) -> Cutoff (0.7) |
| **Vocaloid A-O** | Dual Vowel Peak | Parallel | A: BP (800Hz), B: BP (1800Hz) | LFO 1: Sine (0.4Hz) -> A/B Cutoffs (Opposite) |
| **Industrial Hell** | Heavy Saturation | Serial | Drive: 20.0, Exhaust: ON, Grit: 0.9 | High Resonance on both filters |
| **Deep Sea Pressure** | Low-End Ducking | Serial | Filter A: LP (60Hz) | SC: Active, Target: Cutoff, Amount: -40% |
| **Lunar Sweep** | Wide Ambient Sweep | Serial | Filter A: HP (200Hz), Mix: 0.7 | LFO 1: Triangle (0.1Hz) -> Cutoff (0.8) |
| **Radio From Mars** | Lo-Fi Ringer | Serial | Filter A: BP (1500Hz), Grit: 1.0 | LFO 1: Random (15Hz) -> Cutoff (0.2) |
| **Warm Tape Sat** | Analog Warmth | Serial | Drive: 5.0, Exhaust: ON | Filter A: LP (8kHz), Clean |
| **Rhythmic Gater** | Percussive Gate | Serial | Gain: 1.0 | SC: Active, Target: Gain, Amount: 100% (Gate) |
| **Dual Resonator** | Static Filter Bank | Parallel | A: BP (400Hz), B: BP (2400Hz) | Res: 0.9 on both |
| **Sub Destroyer** | Harmonics Generator | Serial | Cutoff: 40Hz, Drive: 18.0 | Threshold: 0.5 (Saturation shaping) |
| **High End Sizzle** | Top-End Exciter | Parallel | A: HP (5kHz), B: HP (8kHz) | LFO 1: Sine (8Hz) -> Cutoff (0.1) |
| **Crushed Velvet** | Soft Clipping | Serial | Drive: 12.0, Grit: 0.4 | Mix: 0.5 (Dry/Wet blend) |
| **Neon Pulsar** | Rhythmic Pulsing | Serial | LFO 1: Square (4Hz) | Mod L1 -> Cutoff (0.6) |
| **Ocean Swell** | Slow Sea Motion | Serial | LFO 1: Sine (0.05Hz) | Mod L1 -> Cutoff (0.9), Mix: 0.8 |
| **Digital Grit** | Bit-Crush Texture | Serial | Filter A: Digital, Grit: 1.0 | Drive: 8.0, Exhaust: ON |
| **Parallel Phaser** | Notch Phasing | Parallel | A: Notch (1kHz), B: Notch (1.5kHz) | LFO 1: Sine (0.5Hz) -> Cutoff (Opposite) |
| **The Grater** | SC Distort | Serial | Drive: 1.0 | SC: Active, Target: Drive, Amount: 100% |
| **Bass Growler** | Resonant Growl | Serial | Filter A: Acid (150Hz), Res: 0.7 | LFO 1: Sine (0.8Hz) -> Cutoff (0.4) |
| **Airy Lift** | Subtle Clarity | Serial | Filter A: HP (12kHz), Res: 0.0 | Gain: 1.2, Mix: 0.3 |

## Design Philosophy

The presets in this bank are designed to showcase the "Dual Filter LFO Matrix" capabilities:
1.  **Interaction between SC and DSP**: Many presets use the Sidechain to modulate not just volume, but Filter Cutoff and Saturation Drive.
2.  **Parallel vs Serial**: Use of parallel routing for vowel sounds and multi-band processing.
3.  **LFO Matrix Complexity**: Utilizing LFOs to create rhythmic movement that feels organic rather than purely random.
4.  **Saturation as a Tool**: Drive and Exhaustion are used to add harmonics that the filters then shape.
