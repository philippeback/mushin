# Codebase Block Schema Design

```
+-------------------+
| AudioProcessor    | (Plugin Core: Handles audio processing setup)
|                   | 
+-+--------------+--+
  |              |
  | Parameters   v  
  |          +-------+
  |          | UI    | <--- MushinAudioProcessorEditor
  |          +-------+
  |            
  |      +---------------+
  |      | DSP Manager   |
  |      +-------+-------+
        /         \
       v           v
+------+-----+  +----------+
| Filter System | LFO Matrix|
| (DualFilterLFOMatrix)     |
+---------------------------+
       |
       v
+-------------+
| Waveshapers  |
+-------------+
```

**Key Components:**
1. *AudioProcessor*: Central plugin logic handling audio I/O and parameter management
2. *UI Layer*: MushinAudioProcessorEditor handles user interface rendering
3. *DSP Manager*: Coordinates filter systems, LFO modulation sources
4. *Filter System*: Handles filtering operations (Clean/Vintage/ Acid/Digital modes)
5. *LFO Matrix*: Manages low-frequency oscillation modulation routing
6. *Waveshapers*: Distortion/shaping effects processing

Connections:
- Parameters from AudioProcessor control all DSP components
- LFO Matrix provides modulation values to filters/waveshapers
- Filter system outputs feed into waveshaping stages (if enabled)
```
