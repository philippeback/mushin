
**4. Quantization Error (Information Decay)**
If exhaustion is the degradation of signal integrity, bitcrushing is the purest DSP representation of a fading signal.
* **The Math:** Audio quantization reduces the number of possible amplitude values. The formula is $y = \frac{\text{round}(x \cdot \text{steps})}{\text{steps}}$, where `steps` dictates the bit depth (e.g., $2^{16}$ for 16-bit).
* **The Logic:** Tie the `steps` variable to the statistical confidence of the move. As the "last buyer" enters and statistical capacity drains, dynamically reduce the `steps` from $2^{24}$ down to $2^8$ or $2^4$.
* **The Exhaustion Event:** The audio doesn't just get quieter; it gets fundamentally destroyed by quantization noise. The signal breaks apart under its own weight, exactly how a trend fractures at the top of a run.