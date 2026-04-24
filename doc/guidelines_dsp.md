# MUST HAVE

* Il faut utiliser des LinearSmoothedValue partout pour les paramètres de gain, ça permet de lisser les sauts de volume.
* Il faut le créer le drybuffer une fois dans la fonction prepareToPlay, avec une taille max définie par le paramètre samplesPerBlock, et ensuite ne pas refaire d'allocation mémoire dans la fonction process, car c'est le genre de choses qui bouffe des ressources ou génère des craquements en mode Debug

# BEST PRACTICES

* **Zero Allocations in Process:** Never use `new`, `malloc`, or resize containers (like `std::vector` or `juce::AudioBuffer`) inside `processBlock`. All memory must be pre-allocated in `prepareToPlay`.
* **Avoid Standard Library I/O:** Do not use `std::cout`, `printf`, or file streams in the audio thread. These can cause priority inversion and audio glitches.
* **SIMD Optimization:** Use `juce::dsp::AudioBlock` and the `juce::dsp` module whenever possible. They are optimized for SIMD (Single Instruction, Multiple Data) on most platforms.
* **State Consistency:** If you have multiple related parameters, update them at the start of the block and use smoothed values to interpolate. Ensure that `reset()` is called on all DSP objects in the processor's `reset()` or `prepareToPlay()` to avoid "clicks" on playback start.
* **Denormal Handling:** Use `juce::ScopedNoDenormals` at the top of `processBlock` to prevent performance drops caused by extremely small floating-point numbers in recursive filters.
* **Thread Safety:** Only use `std::atomic` or `juce::AudioProcessorValueTreeState` for communication between the UI and Audio threads. Never use `juce::CriticalSection` (mutexes) in the audio thread if it can be avoided.

# ON FILTERS

* dans JUCE en plus du TPTSVF, tu as aussi un filtre Moog qui est plus coloré.


# ON WAVESHAPER

* ton tanh tu peux trouver une formule pour lui appliquer le paramètre du threshold aussi, pas juste sur le clamp:

    samples[i] = std::tanh(samples[i] / (threshold + 1e-3f)) * (threshold + 1e-3f);