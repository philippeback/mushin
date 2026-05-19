# Mushin: Modulation Matrix Manual

The Modulation Matrix is the brain of Mushin's dynamic movement. It allows you to route two independent Low-Frequency Oscillators (LFOs) to various DSP parameters of the dual filter system.

---

## 1. The Matrix Layout
The matrix is displayed as a grid of horizontal sliders located in the **MOD MATRIX (L1 / L2)** panel.

-   **Vertical Columns:** Each column represents a **Target** (the knob being moved).
-   **Stacked Sliders:** Each cell contains two sliders:
    -   **Top Slider:** Amount of **LFO 1** sent to the target.
    -   **Bottom Slider:** Amount of **LFO 2** sent to the target.

---

## 2. Modulation Targets

| UI Label | Target Parameter | Effect |
| :--- | :--- | :--- |
| **A-Cut** | Filter A Cutoff | Creates rhythmic filter sweeps or "wubs" on Filter A. |
| **A-Res** | Filter A Resonance | Dynamically changes the "peakiness" or squelch of Filter A. |
| **B-Cut** | Filter B Cutoff | Creates rhythmic filter sweeps on Filter B. |
| **B-Res** | Filter B Resonance | Dynamically changes the resonance level of Filter B. |

---

## 3. How to Use the Sliders

The sliders are **Bipolar** (they go from negative to positive).

-   **Middle Position (0.0):** No modulation. The parameter stays at its base knob value.
-   **Right Side (Positive):** The LFO will *add* to the knob value. (e.g., LFO goes up, Cutoff goes up).
-   **Left Side (Negative):** The LFO will *subtract* from the knob value. (e.g., LFO goes up, Cutoff goes down).

---

## 4. Understanding LFO Sources

You can configure the sources in the **LFO 1** and **LFO 2** panels:

-   **Freq (Frequency):** Sets the speed of the movement (from very slow 0.1Hz to fast 20Hz).
-   **Wave (Waveform):** 
    -   **Sine:** Smooth, rounded movement.
    -   **Triangle:** Linear rise and fall.
    -   **Saw:** Sharp ramp (great for rhythmic "stabs").
    -   **Square:** Instant jumps between high and low.
    -   **Random:** Unpredictable "jitter" or sample-and-hold style movement.

---

## 5. Pro Tips

### Complex Rhythms (Polyrhythms)
Set LFO 1 to a slow speed (e.g., 0.5Hz) and LFO 2 to a faster speed (e.g., 3.2Hz). Route both to **A-Cut**. The two LFOs will combine to create a complex, non-repeating pattern of filter movement.

### Phase Inversion
Route **LFO 1** to **A-Cut** with a positive amount, and to **B-Cut** with a negative amount. When Filter A opens up, Filter B will close down, creating a wide, shifting stereo or serial effect.

### Self-Oscillation Jitter
If Filter A is set to `Vintage` mode, try routing a **Random** waveform LFO at a high frequency to **A-Res**. If the base Resonance is high, this will create a "glitchy," unstable analog feedback sound.
