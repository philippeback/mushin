# Multi-GPU Configuration for Local LLMs

This document outlines how to utilize a dual-GPU setup (GTX 1660 6GB + GTX 980 Ti 6GB) to host significantly more powerful coding models for C++ development.

---

## 1. The Strategy: VRAM Pooling
By utilizing both cards, you increase your available VRAM from 6GB to **12GB**. This shifts your capability from running "efficient" models to running "powerhouse" models.

*   **Total VRAM:** 12GB (minus ~1GB for Windows/Display overhead) $\approx$ 11GB effective.
*   **Layer Splitting:** The LLM engine (LM Studio, Ollama, or llama.cpp) splits the model's neural network layers across both GPUs. 

---

## 2. Recommended Models (12GB VRAM Target)

With 12GB, you can move beyond 7B models and enter the **14B parameter class**, which offers a massive jump in C++ reasoning and architectural understanding.

### A. The "New King": Qwen2.5-Coder-14B-Instruct
*   **Quantization:** `Q4_K_M` (Recommended) or `Q5_K_M` (High Precision).
*   **Size:** ~9.1 GB (for Q4).
*   **Why:** Currently the best-in-class for open-source coding. It handles complex C++ templates and JUCE-style abstractions much better than the 7B version.
*   **Context Window:** With 9.1GB for the model, you have ~2GB left for context, allowing for roughly **8,192 to 16,384 tokens**.

### B. The "Reliable Alternative": Mistral-Nemo-12B-Instruct
*   **Quantization:** `Q6_K` or `Q8_0`.
*   **Why:** A joint venture between NVIDIA and Mistral. It is highly optimized for NVIDIA hardware and has a native 128k context window (though VRAM will limit you to less).

---

## 3. Configuration in LM Studio

1.  **Detection:** Ensure both GPUs are recognized in the **Settings > GPU** section.
2.  **GPU Offload:** 
    *   Set **GPU Override** to "Max" or manually set the number of layers (e.g., 48 or 64 depending on the model).
    *   LM Studio will automatically fill the primary card (typically the 1660) and offload the remaining layers to the secondary card (980 Ti).
3.  **Cross-Device Communication:** 
    *   Data must travel between the two cards via the PCIe bus. 
    *   Ensure "Flash Attention" is enabled to minimize the memory footprint.

---

## 4. Hardware Requirements & Constraints

### A. Power Supply (PSU)
*   **GTX 1660:** ~120W
*   **GTX 980 Ti Strix:** ~250W - 300W (OC version is thirsty).
*   **Recommendation:** Minimum **750W Gold-rated PSU**. The 980 Ti can have transient power spikes that trip lower-quality units.

### B. PCIe Bandwidth
*   The model will only be as fast as the slowest link. If your second GPU is in a PCIe 2.0 or 3.0 x4 slot, you will see a delay in "Time to First Token" (TTFT). 
*   **Optimization:** Place the card you want to handle the "display" in the secondary slot, and the primary compute card in the main x16 slot.

### C. Thermal Management
*   The 980 Ti Strix is a triple-fan beast. It will dump a lot of heat into the case.
*   Ensure there is at least one slot of "breathing room" between the cards if possible.
*   Increase your case intake fan speeds to provide enough cool air for both heatsinks.

---

## 5. Summary Table

| Model Size | Quantization | VRAM Used | Performance Level | Recommendation |
| :--- | :--- | :--- | :--- | :--- |
| **7B** | Q8_0 | ~7.5 GB | Fast / Good | Use for quick autocomplete |
| **14B** | Q4_K_M | ~9.1 GB | Moderate / Excellent | **Ideal for complex C++ refactoring** |
| **14B** | Q6_K | ~11.5 GB | Slow / Superior | Maximum logic, but very little context space |
