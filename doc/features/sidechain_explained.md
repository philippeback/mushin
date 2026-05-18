# Mushin: Sidechain System Explained

The Sidechain system in Mushin allows the audio processor to be modulated by a secondary signal. This is a powerful tool for rhythmic ducking, dynamic distortion, and frequency-dependent filtering.

---

## 1. How It Works
The sidechain module extracts the volume **envelope** of a control signal and uses that envelope to dynamically "wiggle" one of Mushin's main DSP parameters.

### Detection Path:
1.  **Input:** The control signal is received from either the internal audio path or an external DAW track.
2.  **Filter:** The signal passes through a Highpass and Lowpass filter (SC HPF/LPF). This allows you to make the sidechain sensitive only to specific frequencies (e.g., just the "thump" of a kick drum).
3.  **Envelope Follower:** The signal is converted into a smooth control value (0.0 to 1.0) based on your **Attack** and **Release** settings.
4.  **Threshold:** The system only generates modulation when the envelope exceeds the **Threshold**.
5.  **Modulation:** The resulting value is scaled by the **Amount** and sent to the selected **Target**.

---

## 2. Routing Options

### Internal Sidechain
-   **Source:** Uses the main audio entering the plugin (pre-distortion).
-   **Use Case:** "Self-modulation." For example, use the low-end of your bass guitar to drive the filter cutoff higher as you play harder.

### External Sidechain
-   **Source:** Uses audio from a different track in your DAW.
-   **DAW Setup:** You must route audio to Mushin's "Sidechain Input" (Bus 1) within your DAW's routing matrix or sidechain dropdown.
-   **Use Case:** Traditional "EDM ducking." Route a Kick drum to the sidechain so the Mushin signal gets quieter (or more distorted) every time the kick hits.

---

## 3. Control Parameters

| Parameter | Function |
| :--- | :--- |
| **Active** | Master On/Off switch for the sidechain module. |
| **Source** | Select between **Internal** (self) or **External** (DAW routing). |
| **Target** | The knob that will be moved: **Drive**, **Filter Cutoff**, or **Master Gain**. |
| **Threshold** | The "floor" level. Audio below this level will not trigger any modulation. |
| **Attack** | How fast the modulation reacts when the sidechain signal hits the threshold. |
| **Release** | How fast the modulation returns to zero after the sidechain signal stops. |
| **Mode** | **Peak** (fast, reactive) or **RMS** (smooth, based on average power). |
| **Amount** | **Positive:** Increases the target value. **Negative:** Decreases the target value (ducking). |
| **HPF / LPF** | Filters the sidechain signal *before* the envelope detection. Does not affect the output sound directly. |

---

## 4. Modulation Targets

### Target: Master Gain
-   **Classic Ducking:** Set **Amount** to negative (e.g., -100%). The volume will drop when the sidechain signal is loud.
-   **Gating:** Set **Amount** to positive. The volume will only "open up" when the sidechain signal is loud.

### Target: Drive
-   **Dynamic Grit:** Set **Amount** to positive. Your distortion will get "angrier" and more saturated during loud peaks of the sidechain signal.

### Target: Cutoff
-   **Filter Sweep:** Moves both Filter A and Filter B Cutoff frequencies. 
-   **Tip:** Use a high **HPF** on the sidechain to make the filter only sweep when high-frequency elements (like snares or hats) hit the sidechain.
