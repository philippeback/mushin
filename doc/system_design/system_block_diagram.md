# System Block Diagram

MushinAudioProcessorEditor
|
|  (UI Interactions)
v
MushinAudioProcessor
|
|  (Audio Signal Flow)
+-----------------------+
|                       |
v                       v
Waveshaper             Filter
|                       |
|  - Drive              |  - Type (Clean, Vintage, Acid, Digital)
|  - Threshold          |  - Mode (Lowpass, Highpass, Bandpass, Notch)
|                       |  - Cutoff / Resonance
|                       |
v                       v
DualFilterLFOMatrix
|
|  - LFO 1
|  - LFO 2
|  - Routing (Serial/Parallel)
