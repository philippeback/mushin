# UI Skinning System

The Mushin UI uses a **Theme-Injection Architecture** that allows for generic skinning without modifying the core HTML or JavaScript.

## How it Works

1.  **Virtual CSS:** The JUCE `PluginEditor` intercepts requests for a virtual file named `skin.css`.
2.  **Dynamic Generation:** Instead of reading from disk, JUCE generates the content of `skin.css` on-the-fly as a string of CSS variables.
3.  **Variable-Driven UI:** The `index.html` file links to this `skin.css` and uses its variables for all visual properties (colors, borders, backgrounds).

## Customizable Variables

| Variable | Description |
| :--- | :--- |
| `--primary` | Main highlight color (knobs, active states, text highlights). |
| `--secondary` | Accent color for secondary elements. |
| `--bg-hardware` | Background color of the main plugin chassis. |
| `--text-main` | Primary text color. |
| `--panel-border` | Border color for panels and sub-sections. |
| `--marking` | Color for subtle lines, grids, and markings. |
| `--display-bg` | Background of the visualizers (Waveform, Meters). |
| `--sc-yellow` | Highlight color for Sidechain modulation activity. |
| `--sc-input` | Color for Sidechain input peak meter. |

---

## Sample Skins

Copy and paste these blocks into the `skin.css` generator in `PluginEditor.cpp` or use them as templates for external JSON themes.

### 1. Synthwave
```css
:root {
    --primary: #ff00ff;
    --secondary: #00ffff;
    --bg-hardware: #2b0033;
    --text-main: #ffffff;
    --panel-border: #ff00ff;
    --marking: rgba(255, 0, 255, 0.2);
    --display-bg: #1a0026;
    --sc-yellow: #ffff00;
    --sc-input: #00ffcc;
}
```

### 2. Acid
```css
:root {
    --primary: #bfff00;
    --secondary: #32cd32;
    --bg-hardware: #001a00;
    --text-main: #bfff00;
    --panel-border: #32cd32;
    --marking: rgba(50, 205, 50, 0.2);
    --display-bg: #000000;
    --sc-yellow: #ccff00;
    --sc-input: #00ff00;
}
```

### 3. Firepits
```css
:root {
    --primary: #ff4500;
    --secondary: #8b0000;
    --bg-hardware: #1a0500;
    --text-main: #ffd700;
    --panel-border: #ff4500;
    --marking: rgba(255, 69, 0, 0.2);
    --display-bg: #0a0200;
    --sc-yellow: #ffcc00;
    --sc-input: #ff0000;
}
```

### 4. Ocean Deep
```css
:root {
    --primary: #00bfff;
    --secondary: #00008b;
    --bg-hardware: #00051a;
    --text-main: #e0ffff;
    --panel-border: #00bfff;
    --marking: rgba(0, 191, 255, 0.2);
    --display-bg: #00020a;
    --sc-yellow: #ffffff;
    --sc-input: #008080;
}
```

### 5. Ice World
```css
:root {
    --primary: #f0f8ff;
    --secondary: #b0c4de;
    --bg-hardware: #101820;
    --text-main: #ffffff;
    --panel-border: #afeeee;
    --marking: rgba(240, 248, 255, 0.2);
    --display-bg: #050a10;
    --sc-yellow: #87cefa;
    --sc-input: #e0ffff;
}
```

### 6. Dark Hellish
```css
:root {
    --primary: #ff0000;
    --secondary: #3d0000;
    --bg-hardware: #050000;
    --text-main: #8b0000;
    --panel-border: #3d0000;
    --marking: rgba(255, 0, 0, 0.1);
    --display-bg: #000000;
    --sc-yellow: #ff4500;
    --sc-input: #660000;
}
```

---

## Next Steps

### Generic Skin Selector
The UI can be extended with a dropdown to switch between these themes at runtime. The selector should:
1.  Emit a `mushin://setTheme?name=Synthwave` command to JUCE.
2.  JUCE updates the internal `currentTheme` object.
3.  The browser is signaled to reload or the CSS is re-injected using `evaluateJavascript`.
