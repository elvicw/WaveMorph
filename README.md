# WaveMorph: Waveform Transformation Previewer (VST3 / AU / Standalone)

**WaveMorph** is an audio plugin and standalone preview tool designed for Digital Audio Workstations (DAW). It visually demonstrates the real-time transformation of audio waveforms before and after dynamic compression, equalization, saturation, and clipping.

---

## 🌟 Key Features

### 1. Dual-Waveform Transformation Display
- **Cyan Waveform (Pre / Dry)**: The original un-processed audio (or DAW sidechain reference).
- **Amber Waveform (Post / Wet)**: The transformed audio after dynamic processing, EQ, and saturation.
- **Crimson Highlight (Gain Reduction & Shaved Peaks)**: Visually highlights where peaks have been squashed by compression or chopped off by the clipper.
- **View Modes**:
  - **Overlay Mode**: Both waveforms aligned on the same center zero-crossing axis for transient and body comparison.
  - **Split Mode**: Before waveform in the top pane, After waveform in the bottom pane.
  - **Delta Mode**: Displays solely the mathematical difference `(Wet - Dry)` to inspect exactly what was added or removed.
- **Freeze Mode**: Press **FREEZE** at any moment to lock the display snapshot and inspect individual transient cycles up close.
- **Adjustable Time Window**: 50 ms (transient cycle view), 100 ms, 250 ms, 500 ms, 1s, and 2s rolling history.

### 2. Audio File & Recording Preview Mode (`[ FILE & RECORD PREVIEW ]`)
- **Drag & Drop Audio Files**: Drag any `.wav`, `.aif`, `.mp3`, `.flac`, or `.ogg` file directly into the plugin window.
- **Full File Overview**: Generates a start-to-finish waveform overview showing the original recording overlaid with the transformed waveform.
- **Live DAW Track Capture (`[ REC TRACK ]`)**: Record up to 15 seconds of audio directly from your DAW track into the previewer without needing to export files.
- **Interactive Auditioning**: Scrub along the waveform, click anywhere to position the playhead, and use **PLAY PREVIEW** with **Loop** and **A/B Audition** (Hear Dry vs Processed).
- **Transformation Metrics Card**:
  - **Peak**: Before vs After (dBFS)
  - **RMS Loudness**: Before vs After + loudness gain ($\Delta \text{dB}$)
  - **Dynamic Range / Crest Factor**: Before vs After + squash reduction ($\Delta \text{dB}$)
  - **Clipping Headroom**: Digital ceiling breach sample counter

### 3. Built-In Preview Transformation Rack
- **Dynamics Compressor / Limiter**:
  - Threshold: -60 dB to 0 dB
  - Ratio: 1:1 to 30:1 (with smooth log ballistics)
  - Attack: 0.1 ms to 100 ms
  - Release: 10 ms to 1000 ms
  - Makeup Gain: 0 dB to +24 dB
  - Soft Knee: 0 dB to 18 dB
  - Gain Reduction live readout
- **3-Band Sculpting Equalizer**:
  - Low Shelf: 80 Hz ($\pm 18\text{ dB}$)
  - Parametric Mid: 1000 Hz ($\pm 18\text{ dB}$, Q 0.5 to 5.0)
  - High Shelf: 8000 Hz ($\pm 18\text{ dB}$)
- **Saturation & Clipper**:
  - Drive: 0 dB to +24 dB
  - Ceiling: -12 dB to 0 dBFS
  - Modes: **Soft Saturation** (analogue tape/tube curve) vs **Hard Digital Ceiling**

### 4. External DAW Sidechain Compare Mode
- Want to monitor what your *own* third-party plugins (FabFilter, SSL, Universal Audio, etc.) are doing?
- Switch **Input Source** from `Internal DSP Preview` to `External DAW Sidechain`.
- Route the pre-effect track to **Sidechain (Channels 3-4)** and put WaveMorph at the end of your chain.
- WaveMorph will render your third-party plugin's transformation in real-time!

---

## 📂 Plugin Formats & Installation Paths

### macOS
| Format | File Path |
|---|---|
| **VST3 Plugin** | `~/Library/Audio/Plug-Ins/VST3/WaveMorph.vst3` |
| **AU Component** | `~/Library/Audio/Plug-Ins/Components/WaveMorph.component` |
| **Standalone App** | `./WaveMorph.app` or `build/WaveMorph_artefacts/Release/Standalone/WaveMorph.app` |

### Windows
| Format | File Path |
|---|---|
| **VST3 Plugin** | `C:\Program Files\Common Files\VST3\WaveMorph.vst3` |
| **Standalone App** | `WaveMorph.exe` (run directly without a DAW) |

---

## 💻 Building on Windows

The entire codebase is written in cross-platform standard C++17 and JUCE. To build on Windows:

1. Install **Visual Studio 2022** (with C++ Desktop Development) and **CMake**.
2. Open PowerShell or Command Prompt in the repository folder:
   ```powershell
   # Configure Visual Studio solution
   cmake -B build -G "Visual Studio 17 2022" -A x64

   # Build Release binaries
   cmake --build build --config Release --parallel
   ```
3. Copy the compiled plugin to your Windows VST3 folder:
   ```powershell
   Copy-Item -Recurse build\WaveMorph_artefacts\Release\VST3\WaveMorph.vst3 "C:\Program Files\Common Files\VST3\"
   ```
4. Or run the standalone app directly:
   ```powershell
   .\build\WaveMorph_artefacts\Release\Standalone\WaveMorph.exe
   ```

*Tip: A pre-configured GitHub Actions workflow (`.github/workflows/build.yml`) is included to automatically build Windows and macOS binaries in the cloud whenever code is pushed.*

---

## 🚀 How to Use in Your DAW

### Ableton Live / FL Studio / Cubase / Studio One / Reaper / Bitwig
1. Rescan plugins or search for **WaveMorph** under Audio Effects.
2. Insert **WaveMorph** onto any audio track or master bus.
3. Switch between **Live Stream** (real-time rolling waveform) and **File & Record Preview**.

### Logic Pro
1. Under Audio FX, navigate to `Audio Units > WaveMorphAudio > WaveMorph`.
2. Both Stereo and Sidechain auxiliary routing are fully supported and Apple `auval` verified.

### Standalone (No DAW required)
1. Double click [`WaveMorph.app`](file:///Users/ElvicLlagunoWaskin@hellobenefex.com/vst3/WaveMorph.app) directly in Finder or run:
   ```bash
   open WaveMorph.app
   ```
2. Drag and drop any song or recorded vocal/drum stem into the window to preview waveform transformations immediately!

---

## 🛠 Rebuilding from Source

WaveMorph uses CMake and Ninja:

```bash
# Configure
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release

# Build VST3, AU, and Standalone
cmake --build build --config Release

# Validate AU with Apple auval
auval -v aufx WvTr WvMr
```
