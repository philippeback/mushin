# Feature: Moog-Style Rotary Knob Controls

## 1. Overview & Requirements
To elevate the user experience of **Mushin** from a generic web interface to a premium, tactile, and hardware-inspired instrument, we need to introduce high-fidelity custom rotary controls. 

Linear sliders are functional but do not reflect the premium quality of traditional analog audio processors, nor do they make optimal use of screen real estate. The **Moog-Style Knob** will serve as a direct, aesthetically stunning, and highly ergonomic replacement for the linear range sliders—beginning with the **Drive** control.

### Core Goals
* **Tactile Hardware Aesthetic:** Recreate the iconic, heavy-skirted fluted design of classic Moog modular/Minimoog knobs with brushed metal caps, high-contrast indicator lines, and glowing dynamic tracks.
* **Flawless Ergonomics:** Implement standard pro-audio plugin mouse interactions (vertical dragging, fine adjustments, scroll wheel, double-click to reset) rather than awkward circular mouse paths.
* **Dynamic Theme Integration:** Utilize the CSS variables injected from C++ (`skin.css`) to ensure the knobs seamlessly match active themes (e.g., Synthwave neon, Acid lime, Ocean Deep cyan) with zero code modifications.
* **Bidirectional Integration:** Seamlessly integrate with the existing C++ parameter model, ensuring remote updates from native presets or host automation rotate the UI knob in real-time.

---

## 2. Visual & Structural Design (Aesthetics)
The Moog-style knob will be rendered using resolution-independent **inline SVG** combined with **Vanilla CSS** animations and filters. This guarantees sharp rendering at all display scale factors.

### Anatomy of the Mushin Moog Knob
```
            .-'""'-.
          .'   __   '.     <-- Outer markings / Glowing Value Track
         /   .'  '.   \
        |   |      |   |   <-- Heavy Black Bezel (Ribbed/Fluted)
        |   |  .---|   |   <-- Machined Metallic Cap (Center Dome)
        |   | /    |   |   <-- Indicator Line (Rotates with value)
         \   '.__.'   /
          '.        .'
            '-....-'
```

1. **Outer Markings / Value Arc:** A subtle, circular track surrounding the knob that dynamically fills with the `--primary` theme color to represent the current parameter value.
2. **Knob Bezel (Base):** A heavy, dark-gray to deep-black circular body with radial ridges, giving a physical, plastic fluted appearance.
3. **Machined Cap (Dome):** A center silver cap with a concentric brushed-metal gradient (radial and linear CSS gradients) that creates dynamic lighting reflections when the knob is turned.
4. **Indicator Line:** A high-contrast line starting from the center cap extending outwards, glowing in the active theme's `--primary` color.

### CSS Styling & Theme Support
By utilizing CSS variables, the knob adapts instantly to the active workspace theme:
* **Track Fill & Indicator:** `--primary`
* **Glow/Drop Shadows:** `rgba(var(--primary-rgb), 0.2)`
* **Active State Accents:** `--secondary`
* **Tick Marks:** `--marking`

---

## 3. Interaction Mechanics (UX)
To match the usability of industry-leading audio plugins (like FabFilter, U-he, or Soundtoys), the interaction model will strictly adhere to professional desktop audio conventions:

| Action | Control Behaviour | Why |
| :--- | :--- | :--- |
| **Left-Click & Drag Vertically** | Dragging **up** increases the value; dragging **down** decreases it. Circular dragging is ignored. | Standard VST standard; prevents erratic value jumps and keeps mouse movement intuitive. |
| **Shift + Drag** | Enters **Fine-Adjustment Mode**, reducing mouse sensitivity by 10x. | Crucial for fine-tuning precise parameters (e.g., getting exactly `1.03` Drive). |
| **Double-Click** | Resets the parameter to its designated **default value**. | Allows instant recall of standard, safe settings without guessing. |
| **Mouse Wheel / Scroll** | Rotating the scroll wheel increases/decreases the value incrementally. | Excellent for quick adjustments when hovering. |
| **Keyboard Arrows** | `Up`/`Right` to increment, `Down`/`Left` to decrement. | Provides full accessibility and keyboard navigation. |

---

## 4. Technical Architecture & Approach

To implement this without rewriting the entire GUI framework, we will create a reusable, lightweight Javascript class `MushinKnob` that wraps standard `<input type="range">` elements or replaces them dynamically.

### Component Design Pattern
We will replace the static HTML input with a semantic `<div class="mushin-knob-container">` container. This container holds:
1. **The Visual SVG Dial:** Renders the actual knob body, center metal dome, and rotating indicator.
2. **Hidden Control Layer:** A standard `<input type="range" class="hidden-param">` containing the true raw values, allowing standard forms, event dispatchers, and existing code to remain 100% compatible.
3. **Interactive Overlay:** Listens to mouse/touch gestures and translates them into smooth mathematical coordinate deltas.

```
       +---------------------------------------------+
       |           MushinKnob JS Component           |
       |                                             |
       |  +-------------------+  +----------------+  |
       |  |  Interactive DOM  |  | Visual SVG/CSS |  |
       |  |  Event Handlers   |  |   Rotator      |  |
       |  +---------+---------+  +--------+-------+  |
       |            |                     |          |
       |            v                     v          |
       |  +---------------------------------------+  |
       |  |       Underlying Input (Range)        |  |
       |  |           id="drive" [0..1]           |  |
       |  +-------------------+-------------------+  |
       +----------------------|----------------------+
                              v
                Sends update to JUCE Bridge 
                 (updateParam / native)
```

---

## 5. Technical Implementation Blueprint

### A. The HTML & SVG Structure
This drops directly into the place of the old slider in `index.html`:

```html
<div class="control-group">
    <div class="label">Drive</div>
    
    <!-- Custom Knob Component -->
    <div class="mushin-knob" id="knob-drive" data-param="drive" data-default="0.05" tabindex="0">
        <svg viewBox="0 0 100 100" class="knob-svg">
            <!-- Radial Defs for Brushed Aluminum Look -->
            <defs>
                <radialGradient id="metallic-dome" cx="50%" cy="50%" r="50%" fx="30%" fy="30%">
                    <stop offset="0%" stop-color="#fdfdfd" />
                    <stop offset="30%" stop-color="#dcdcdc" />
                    <stop offset="70%" stop-color="#8a8a8a" />
                    <stop offset="100%" stop-color="#4a4a4a" />
                </radialGradient>
                <linearGradient id="brushed-reflection" x1="0%" y1="0%" x2="100%" y2="100%">
                    <stop offset="0%" stop-color="rgba(255,255,255,0.15)" />
                    <stop offset="45%" stop-color="rgba(0,0,0,0.3)" />
                    <stop offset="50%" stop-color="rgba(255,255,255,0.4)" />
                    <stop offset="55%" stop-color="rgba(0,0,0,0.3)" />
                    <stop offset="100%" stop-color="rgba(255,255,255,0.15)" />
                </linearGradient>
            </defs>

            <!-- Outer Dynamic Value Track -->
            <circle class="knob-track-bg" cx="50" cy="50" r="44" />
            <path class="knob-value-arc" d="" /> <!-- Managed via JS -->

            <!-- Outer Bezel (Fluted Plastic Ring) -->
            <circle class="knob-bezel" cx="50" cy="50" r="36" />
            
            <!-- Metallic Cap Dome -->
            <circle class="knob-dome" cx="50" cy="50" r="26" fill="url(#metallic-dome)" />
            <circle class="knob-reflection" cx="50" cy="50" r="26" fill="url(#brushed-reflection)" pointer-events="none" />
            
            <!-- Rotating Pointer Skirt -->
            <g class="knob-pointer-group">
                <line class="knob-pointer-line" x1="50" y1="50" x2="50" y2="16" />
                <circle class="knob-pointer-dot" cx="50" cy="18" r="2.5" />
            </g>
        </svg>
        <input type="range" id="drive" class="knob-hidden-input" min="0" max="1" step="0.001" style="display:none;">
    </div>
    
    <div class="value" id="drive-val">1.0</div>
</div>
```

### B. Custom CSS Variables and Styling
Add these styles to target maximum sensory aesthetic:

```css
.mushin-knob {
    position: relative;
    width: 60px;
    height: 60px;
    margin: 8px 0;
    cursor: grab;
    outline: none;
}

.mushin-knob:active {
    cursor: grabbing;
}

.mushin-knob:focus-visible .knob-bezel {
    stroke: var(--secondary);
    stroke-width: 1.5px;
}

.knob-svg {
    width: 100%;
    height: 100%;
    overflow: visible;
}

/* Background circle for the track */
.knob-track-bg {
    fill: none;
    stroke: var(--marking);
    stroke-width: 3.5;
    stroke-linecap: round;
}

/* Dynamic Active Value Arc */
.knob-value-arc {
    fill: none;
    stroke: var(--primary);
    stroke-width: 4;
    stroke-linecap: round;
    filter: drop-shadow(0 0 3px var(--primary));
    transition: stroke 0.3s ease;
}

/* Outer fluted ring */
.knob-bezel {
    fill: #151515;
    stroke: #2e2e2e;
    stroke-width: 1;
    box-shadow: inset 0 2px 4px rgba(0,0,0,0.8);
}

/* Base pointer lines */
.knob-pointer-line {
    stroke: var(--primary);
    stroke-width: 3.5;
    stroke-linecap: round;
    filter: drop-shadow(0 0 2px var(--primary));
}

.knob-pointer-dot {
    fill: #ffffff;
}

.knob-pointer-group {
    transform-origin: 50px 50px;
    will-change: transform;
}
```

### C. The Interaction & Math JavaScript Engine
A dedicated controller handles scaling, pointer lock, mouse-wheel scrolling, and fine-tuning modifiers:

```javascript
class MushinKnob {
    constructor(element) {
        this.element = element;
        this.paramId = element.getAttribute('data-param');
        this.defaultValue = parseFloat(element.getAttribute('data-default') || '0.5');
        
        this.hiddenInput = element.querySelector('.knob-hidden-input');
        this.pointerGroup = element.querySelector('.knob-pointer-group');
        this.valueArc = element.querySelector('.knob-value-arc');
        
        this.minVal = parseFloat(this.hiddenInput.min || '0');
        this.maxVal = parseFloat(this.hiddenInput.max || '1');
        this.step = parseFloat(this.hiddenInput.step || '0.001');
        this.value = parseFloat(this.hiddenInput.value || this.defaultValue.toString());
        
        // Moog traditional arc spans 270 degrees (-135deg to +135deg)
        this.minAngle = -135;
        this.maxAngle = 135;
        
        this.isDragging = false;
        this.startY = 0;
        this.startValue = 0;
        this.pixelRange = 150; // Pixels needed for full 0 to 1 drag
        
        this.init();
    }
    
    init() {
        this.element.addEventListener('mousedown', e => this.onMouseDown(e));
        this.element.addEventListener('dblclick', () => this.resetToDefault());
        this.element.addEventListener('wheel', e => this.onWheel(e), { passive: false });
        this.element.addEventListener('keydown', e => this.onKeyDown(e));
        
        // Initial draw
        this.updateUI(this.value);
    }
    
    setValue(normVal, skipNative = false) {
        // Clamp and step-adjust value
        const clampedVal = Math.min(this.maxVal, Math.max(this.minVal, normVal));
        const steppedVal = Math.round(clampedVal / this.step) * this.step;
        
        if (steppedVal === this.value) return;
        
        this.value = steppedVal;
        this.hiddenInput.value = steppedVal;
        
        this.updateUI(steppedVal);
        
        // Notify the plugin parameters flow
        if (typeof updateParam === 'function') {
            updateParam(this.paramId, steppedVal, skipNative);
        }
    }
    
    updateUI(normVal) {
        // Calculate rotation angle
        const percentage = (normVal - this.minVal) / (this.maxVal - this.minVal);
        const angle = this.minAngle + percentage * (this.maxAngle - this.minAngle);
        
        // Rotate visual pointer
        this.pointerGroup.style.transform = `rotate(${angle}deg)`;
        
        // Redraw SVG value arc
        this.updateArc(percentage);
    }
    
    updateArc(percentage) {
        // Computes SVGArc coordinate paths for the dynamic outer ring
        const radius = 44;
        const cx = 50;
        const cy = 50;
        
        // Standard angle offsets to match the 270deg Moog arch
        const startRad = (this.minAngle - 90) * Math.PI / 180;
        const currentRad = ((this.minAngle + percentage * 270) - 90) * Math.PI / 180;
        
        const x1 = cx + radius * Math.cos(startRad);
        const y1 = cy + radius * Math.sin(startRad);
        const x2 = cx + radius * Math.cos(currentRad);
        const y2 = cy + radius * Math.sin(currentRad);
        
        const largeArcFlag = percentage > 0.66 ? 1 : 0;
        
        const pathData = `M ${x1} ${y1} A ${radius} ${radius} 0 ${largeArcFlag} 1 ${x2} ${y2}`;
        this.valueArc.setAttribute('d', pathData);
    }
    
    onMouseDown(e) {
        this.isDragging = true;
        this.startY = e.clientY;
        this.startValue = this.value;
        
        // Create document-wide event listeners to track movement outside element boundary
        this.mouseMoveHandler = e => this.onMouseMove(e);
        this.mouseUpHandler = () => this.onMouseUp();
        
        document.addEventListener('mousemove', this.mouseMoveHandler);
        document.addEventListener('mouseup', this.mouseUpHandler);
        
        e.preventDefault(); // Stop cursor selections
    }
    
    onMouseMove(e) {
        if (!this.isDragging) return;
        
        const deltaY = this.startY - e.clientY; // Upward motion yields positive increments
        const sensitivity = e.shiftKey ? 0.1 : 1.0; // 10x finer drag resolution
        
        const deltaVal = (deltaY / this.pixelRange) * (this.maxVal - this.minVal) * sensitivity;
        this.setValue(this.startValue + deltaVal);
    }
    
    onMouseUp() {
        this.isDragging = false;
        document.removeEventListener('mousemove', this.mouseMoveHandler);
        document.removeEventListener('mouseup', this.mouseUpHandler);
    }
    
    resetToDefault() {
        this.setValue(this.defaultValue);
    }
    
    onWheel(e) {
        e.preventDefault(); // Avoid page scrolls
        const direction = e.deltaY < 0 ? 1 : -1;
        const sensitivity = e.shiftKey ? 0.002 : 0.01;
        
        this.setValue(this.value + direction * sensitivity);
    }
    
    onKeyDown(e) {
        const step = e.shiftKey ? this.step : this.step * 10;
        if (e.key === 'ArrowUp' || e.key === 'ArrowRight') {
            this.setValue(this.value + step);
            e.preventDefault();
        } else if (e.key === 'ArrowDown' || e.key === 'ArrowLeft') {
            this.setValue(this.value - step);
            e.preventDefault();
        }
    }
}
```

---

## 6. Detailed Action & Verification Plan

To execute this feature successfully, we will follow these iterative steps:

### Phase 1: Preparation & Local Web Prototyping
1. **Design Extraction:** Package the CSS and SVG layout in a standalone prototype or inject it locally in a mockup environment.
2. **Framework Alignment:** Check compatibility with current JUCE skin themes to ensure gradients are fully dynamic.

### Phase 2: Core Frontend Refactoring
1. **HTML Integration:** Replace the Drive `<input>` slider container in `Source/Web/index.html` with the custom `.mushin-knob` container.
2. **Script Incorporation:** Append the `MushinKnob` class implementation directly to the script block in `index.html`.
3. **Instantiator Loop:** Auto-instantiate class wrappers for any element marked with `class="mushin-knob"`.
   ```javascript
   document.querySelectorAll('.mushin-knob').forEach(el => new MushinKnob(el));
   ```
4. **Integration with Bridge Callback:** Ensure the native incoming updater handles knobs:
   ```javascript
   window.setParameterValue = (id, normVal) => {
       const config = params[id]; if (!config) return;
       const knobEl = document.getElementById('knob-' + id);
       if (knobEl && knobEl.knobInstance) {
           knobEl.knobInstance.setValue(normVal, true); // Update visuals without loopback
       } else {
           // Fallback to standard slider/input
           const el = document.getElementById(id); if(!el) return;
           if (config.type === 'bool') el.checked = normVal > 0.5; else el.value = normVal;
           updateParam(id, normVal, true);
       }
   };
   ```

### Phase 3: Compilation & Native Test
1. **Compilation:** Run the compile script to bundle the newly added CSS/JS code into the JUCE binary:
   ```powershell
   powershell.exe -NoProfile -ExecutionPolicy Bypass -File C:\Dev\github\philippeback\mushin/.gemini/skills/compileit/scripts/build.ps1
   ```
2. **Sanity Check:** Launch `Mushin.exe` (standalone binary in `build2`) and confirm:
   - Visual aspect ratio matches standard 60x60 container dimensions.
   - Theme changes instantly recolor the knob's value arc and glowing pointer.
   - Mouse gestures behave exactly according to the vertical-drag specifications.

### Phase 4: Expansion Plan (Post-Drive)
Once the Drive knob is successfully integrated and validated, we can roll out identical Moog knobs to other continuous parameters to build a fully cohesive analog rack style layout:
* **Threshold** (Col 1)
* **Mix** (Col 1)
* **Gain** (Col 1)
* **Filter A & B Cutoff / Resonance** (Col 2 & Col 3)

---

## 7. Validation Checklist

- [ ] **Dynamic Theme Coloring:** Verified that switching between *Acid*, *Synthwave*, and *Firepits* updates the knob color scheme instantly.
- [ ] **Mouse Drag Alignment:** Verified that dragging upwards increases the `Drive` level, and dragging downwards decreases it.
- [ ] **Precision Modification:** Verified that holding `Shift` while dragging slows the value modification increment by a factor of 10.
- [ ] **Reset Verification:** Double-clicking the knob sets the value precisely back to its default factor of `0.05` (representing minimal harmonic waveshaping).
- [ ] **No Regression on Bidirectional Flow:** Verified that modifying Drive in the host DAW or loading a preset correctly rotates the knob face to the target angle.
- [ ] **No Artifacts or Clipping:** Verified SVG rendering contains no box boundaries or clipping under resizing constraints.
