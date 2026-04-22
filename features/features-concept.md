# DSP architecture for the "mushin" plugin

We need to translate the market physics directly into signal processing algorithms. You aren't automating a UI; you are writing the math that alters the waveform based on the data feed.

The datafeed is the original source waveform.

Here is how the trading logic maps directly to DSP concepts for your code:

**1. The S-Curve Waveshaper (Saturation & Hard Clipping)**
The S-Curve at 50 is the mathematical decay from Alpha to Beta. In DSP, an S-Curve is literally a waveshaping transfer function.
* **The Math:** Implement a soft-clipping waveshaper using a hyperbolic tangent function: $y = \tanh(x \cdot \text{drive})$. 
* **The Logic:** Tie the `drive` variable to the length/strength of the trend. As price extends, `drive` increases, rounding off the peaks of the waveform (Information Saturation).
* **The Exhaustion Event:** When the Leledc exhaustion triggers, bypass the $\tanh$ function and switch the algorithm to a hard-clip threshold: if $x > \text{threshold}$, $x = \text{threshold}$. This physically squares off the audio waveform, creating harsh, odd-order harmonics. It is the mathematical absolute of "zero dynamic range left."

**2. State-Variable Biquad Filter (The LBR Kinetic Choke)**
The LBR 3-10 tracks the relationship between Potential Energy and Kinetic Energy. This maps perfectly to a dynamic biquad filter.
* **The Math:** Implement a standard 2-pole Low-Pass Filter (LPF). 
* **The Logic:** Map the LBR Slow Line (Potential Energy) to the base cutoff frequency ($f_c$). Map the LBR Fast Line (Kinetic Energy) to the modulation depth of the cutoff. As momentum slows, the cutoff frequency drops, choking the high frequencies.
* **The Exhaustion Event:** Map the exhaustion signal to the filter's Resonance ($Q$). Right as exhaustion prints, multiply the $Q$ value exponentially for a few milliseconds, then immediately drop $f_c$ to a low Hz value (e.g., 200 Hz). This creates a resonant "scream" (the last buyer) before the energy is entirely choked out.

**3. The Circular Buffer (The Air Gap Suspension)**
The Air Gap represents an existential pause where forward progress stops and the opposing side takes inventory.
* **The Math:** Create a circular delay buffer (e.g., 50 to 100 milliseconds long) with a read pointer and a write pointer. 
* **The Logic:** During normal trend conditions, the write pointer constantly updates the buffer with incoming audio, and the read pointer plays it back. 
* **The Exhaustion Event:** When exhaustion is flagged and price enters the Air Gap, halt the write pointer but let the read pointer continue looping over the existing buffer with a Hanning window applied to smooth the edges. The forward time of the audio freezes into a static, granular drone. Once the HMA 34 confirms the slide in the opposite direction, clear the buffer and resume the write pointer.

**4. Quantization Error (Information Decay)**
If exhaustion is the degradation of signal integrity, bitcrushing is the purest DSP representation of a fading signal.
* **The Math:** Audio quantization reduces the number of possible amplitude values. The formula is $y = \frac{\text{round}(x \cdot \text{steps})}{\text{steps}}$, where `steps` dictates the bit depth (e.g., $2^{16}$ for 16-bit).
* **The Logic:** Tie the `steps` variable to the statistical confidence of the move. As the "last buyer" enters and statistical capacity drains, dynamically reduce the `steps` from $2^{24}$ down to $2^8$ or $2^4$.
* **The Exhaustion Event:** The audio doesn't just get quieter; it gets fundamentally destroyed by quantization noise. The signal breaks apart under its own weight, exactly how a trend fractures at the top of a run.