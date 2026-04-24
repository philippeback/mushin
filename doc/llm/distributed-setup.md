# Distributed Inference: Linking Two Machines

If physical constraints (case size, PSU, or PCIe slots) prevent you from installing both the GTX 1660 and the GTX 980 Ti in a single chassis, you can use **Distributed Inference** to pool their VRAM over your local network.

---

## 1. Requirements

*   **Primary Machine (Host):** The PC you code on (with the GTX 1660).
*   **Worker Machine (Remote):** A second PC housing the GTX 980 Ti.
*   **Networking:** A **Gigabit Ethernet** connection is mandatory. Wi-Fi latency will make the model feel "stuttery" and slow.
*   **Software:** `llama.cpp` (The engine behind LM Studio and Ollama supports this via RPC).

---

## 2. Setup Guide (using llama.cpp RPC)

This method allows the Host to "rent" the VRAM of the Worker machine.

### A. On the Worker Machine (GTX 980 Ti)
1.  Download the latest `llama.cpp` binaries for Windows.
2.  Open a terminal and run the RPC server:
    ```powershell
    .\rpc-server.exe -p 50052 -g 0
    ```
    *This tells the machine to listen for LLM layers on port 50052 and use the first GPU.*

### B. On the Primary Machine (GTX 1660)
1.  Identify the IP address of your Worker Machine (e.g., `192.168.1.50`).
2.  Start the main inference engine, pointing it to the remote worker:
    ```powershell
    .\llama-cli.exe -m qwen2.5-coder-14b-instruct.Q4_K_M.gguf --rpc 192.168.1.50:50052
    ```
3.  The engine will now distribute the model's layers across the local 1660 and the remote 980 Ti.

---

## 3. Alternative: API-Based Distribution (Lower Latency)

If the RPC method feels too slow due to network overhead, a more "modular" approach is better for coding:

1.  **Host a "Heavy" Model on the Worker:** Run a 14B or 32B model on the Worker machine using **Ollama** or **LM Studio** in "Server Mode."
2.  **Access from Primary:** Configure your coding assistant (Aider, Continue.dev, or VS Code extension) to point to the **IP address** of the Worker machine instead of `localhost`.

**Benefit:** This offloads 100% of the AI work to the second PC, leaving your primary PC's 1660 completely free for your IDE, compilation, and UI responsiveness.

---

## 4. Performance Expectations

| Metric | Single GPU (7B) | Distributed (14B) |
| :--- | :--- | :--- |
| **Logic/Intelligence** | Good | **Excellent** |
| **Tokens per Second** | ~40-60 t/s | **~5-10 t/s** |
| **Network Impact** | None | High (Heavy traffic during prompt processing) |

---

## 5. Summary Recommendation

For the best C++ development experience:
1.  Use the **GTX 980 Ti machine as a dedicated AI Server**.
2.  Run **Ollama** on it with `qwen2.5-coder:14b`.
3.  On your dev machine, set your environment variable:
    ```powershell
    $env:OLLAMA_HOST = "192.168.1.50:11434"
    ```
4.  Run Aider or your IDE tools. They will talk to the 980 Ti over the network, providing high-end intelligence without slowing down your main machine.
