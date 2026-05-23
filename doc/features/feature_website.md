# Feature: Mushin Product Website

## Overview

A static marketing website for the Mushin audio plugin, hosted on Firebase Hosting. The site communicates the core philosophy, demonstrates the plugin's character, and funnels visitors toward a download/install action.

The primary UX principle mirrors the plugin itself: **opinionated, immersive, zero decision fatigue**. The visitor should feel the aesthetic immediately, understand the concept within 30 seconds, and know exactly how to get it.

---

## 1. Philosophy & Content Strategy

### The "Mushin" Concept (Hero Copy)
Mushin (無心) — "no-mind" in Japanese Zen — is the mental state of a master craftsman operating without hesitation, ego, or overthinking. Applied to music production: **stop tweaking, start making**. Mushin as a plugin is built around this ethos. It is opinionated by design. It does the heavy lifting so the producer can stay in flow.

Key angles to hit:
- **Decision Fatigue Reduction:** Every knob has one clear purpose. No modulation routings buried in sub-menus. The plugin decides the aesthetic; the producer decides the intensity.
- **The Shokunin Ethos:** Reference to the Japanese concept of the artisan who has internalised mastery. The plugin embodies its own expertise so the user doesn't have to.
- **Signal Corruption as Art:** Mushin is not a utility. It is an instrument. Saturation, filtering, bitcrushing, and gating are treated as expressive dimensions, not technical afterthoughts.
- **The Market Physics Origin:** Mushin's DSP signal path is a direct metaphor for market exhaustion physics — S-curve waveshaping, kinetic energy choking, quantization entropy, and the Air Gap pause. The story of *why* these specific effects exist is the differentiator.

---

## 2. Page Architecture

### Section 1 — Hero
- **Headline:** `Mushin. No Mind. Pure Signal.`
- **Subheadline:** `A signal corruptor built for producers who are done tweaking.`
- **Visual:** Animated waveform or shader-inspired background (matches the plugin's visual identity).
- **CTA:** `Download for Windows` (primary), `Learn More` (secondary scroll).

### Section 2 — The Concept (Why It Exists)
- Short prose block (~100 words) on the Mushin / Shokunin ethos.
- 3-column icon grid:
  - 🧠 **No-Mind Flow** — Stay in the creative moment. The plugin has opinions so you don't need to.
  - 🔩 **Shokunin Craft** — Every DSP stage is engineered with intentionality. Nothing is arbitrary.
  - 📉 **Signal Entropy** — Corruption is musical. Saturation, decimation, and quantization noise are expressive tools.

### Section 3 — Feature Highlights
Highlight the key features with short descriptions. Visual: screenshot or animated GIF of the plugin UI.

| Feature | Description |
|---|---|
| **S-Curve Waveshaper** | Soft-saturation with Hard Clip Exhaustion mode. Shapes transients with surgical warmth. |
| **Dual Filter + LFO Matrix** | Two independently configurable state-variable filters. Modulation matrix ties LFOs to cutoff, resonance, grit, and more. |
| **Info Decay (Quantization Error)** | Continuous bit-depth reduction from 24-bit to 2-bit. Post-filter placement keeps the staircase edges crisp and musical. |
| **Trance Gate** | Tempo-synced gate with attack/hold/decay envelopes. Preset patterns from Straight 16th to Euclidean 5. |
| **Noise/Generator** | White noise, pink noise, sine, triangle, sawtooth, square — routable pre-distortion, pre-filter, or post-filter. |
| **Sidechain Engine** | Internal, external, or test-signal sidechain. Targets drive, cutoff, or output gain. |
| **Delay** | Ping-pong stereo delay, tempo-synced or free-running. |
| **WebView2 UI** | Plugin interface is a live HTML/CSS/JS web app. Reactive shader backgrounds, animated waveform display. |

### Section 4 — The Signal Path (Story Section)
A visual diagram or prose section explaining the DSP signal path as a narrative:

> The signal enters as raw market data. The waveshaper imprints exhaustion. The filters apply kinetic resistance. Information decays under quantization entropy. What emerges is the truth of the signal — stripped of noise, full of character.

Optional: Mermaid or SVG diagram of the signal chain (Distortion → Filter A → Filter B → Info Decay → Mix → Gain).

### Section 5 — Download & Install
- **Platform:** Windows 10/11 (VST3, Standalone)
- **Requirement:** Microsoft Edge WebView2 Runtime (evergreen, typically already installed)
- **Download Button:** Links to latest GitHub Release or Firebase Storage asset.
- **Install steps (numbered, minimal):**
  1. Download the installer.
  2. Run as Administrator.
  3. Open your DAW and scan for VST3 plugins.
  4. Load Mushin on any audio track.
- Note: macOS support is planned for a future release.

### Section 6 — Footer
- Links: GitHub · License · Contact
- Tagline: `Built with JUCE. Driven by a clue.`

---

## 3. Design & Aesthetic

### Visual Language
- **Dark theme only.** Background: deep near-black (`#0a0a0f` range), matching the plugin's glassmorphic UI.
- **Accent colour:** Cyan/teal (`#00e5ff` range) for primary CTAs. Red-to-orange gradient for "danger/exhaust" states.
- **Typography:** `Inter` or `Outfit` from Google Fonts. Clean, slightly technical feel.
- **Animations:** CSS-only or lightweight canvas. Avoid heavy JS frameworks. A subtle animated shader background on the hero (re-uses the plugin's own aesthetic).
- **Glassmorphism panels** for feature cards, matching the plugin UI.

### Responsive Breakpoints
- Mobile-first. Single-column on small screens.
- Two-column feature grid on tablet.
- Three-column or full-width hero on desktop.

---

## 4. Technical Stack

| Concern | Choice |
|---|---|
| **Hosting** | Firebase Hosting (static, CDN-backed) |
| **Build** | Plain HTML + CSS + minimal vanilla JS. No framework. |
| **Assets** | Plugin UI screenshots, animated GIF demos, SVG icons |
| **Fonts** | Google Fonts CDN (Inter / Outfit) |
| **Analytics** | Firebase Analytics (optional, GDPR note needed) |
| **Forms** | None at launch. Contact via GitHub Issues. |

Firebase project: existing `mushin` project (same as used by cloud preset sharing feature).

---

## 5. Implementation Steps

1. **Content Draft:** Write final hero copy and concept prose (can reference `features-concept.md` and `usp/` docs for source material).
2. **Design Mockup:** Generate hero and feature section mockups.
3. **Build Static Site:** `index.html`, `style.css`, assets. No build pipeline needed for v1.
4. **Screenshot Assets:** Capture clean plugin UI screenshots for the features section.
5. **Firebase Deploy:**
   ```bash
   firebase init hosting
   firebase deploy --only hosting
   ```
6. **Custom Domain (optional):** Configure DNS for a domain like `mushin.audio` or `getmushin.com`.
7. **Download Link:** Wire up the download button to the latest GitHub Release tag or a Firebase Storage bucket.

---

## 6. Future Iterations

- **Preset Browser Demo:** Embed a read-only web preset browser that loads factory presets and shows parameter snapshots.
- **Audio Demo Player:** Short before/after audio clips demonstrating each DSP stage.
- **macOS section:** Once macOS build is complete, add AU/VST3 download variant.
- **Community Profiles:** If cloud preset sharing lands, expose a public gallery of community-shared "Corruption Profiles."
