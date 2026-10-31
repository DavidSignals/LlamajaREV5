# Llamaja REV5 - Digital Reverberator

![Llamaja REV5 Interface](assets/rev5_interface.png) <!-- Asegúrate de subir una captura de tu interfaz y actualizar esta ruta -->

**Llamaja REV5** is a digital reverberation unit inspired by classic 80s hardware. This plugin combines vintage algorithms with modern FDN (Feedback Delay Network) architecture, providing incredibly dense, natural, and highly customizable spaces for your mixes.

Developed in **C++** using the **JUCE Framework (v7/v8)**.

## 🚀 Features

*   **Advanced FDN Reverb Engine:** 8x8 Feedback Delay Network with lossless Hadamard mixing for exponentially growing echo density.
*   **Independent Early Reflections (Moorer):** 8 distinct reflection taps per channel using prime-number-based delay times to prevent phase cancellation.
*   **Pre-Diffuser & Stereo Modulation:** 4 Schroeder Allpass filters in series to smooth transients, coupled with independent LFO modulation (Sine/Cosine) for wide stereo decorrelation.
*   **Dual 3-Band EQ System:** 
    *   **Pre-EQ:** Clean up the signal before it hits the reverb tank.
    *   **Prog-EQ:** Shape the tone of the wet tail exclusively.
*   **Visual Feedback:** Features a dynamic spectrum pixel animation on the LCD display.
*   **Hardware-Style Workflow:** Quick access to 24 Factory Presets (Large Halls, Rooms, Plates, etc.) and 8 User memory slots.
*   **Robust DSP:** 32-bit floating point internal processing with a built-in 1-pole high-pass DC Blocker to prevent offset accumulation on long tails.

## 🛠️ Technical Specifications

*   **Format:** VST3, AU, Standalone
*   **OS Compatibility:** macOS (11.0+, Intel & Apple Silicon), Windows (64-bit)
*   **Sample Rates:** 44.1kHz to 192kHz
*   **Channels:** True Stereo Processing

## ⚙️ Installation & Build

### Download Pre-compiled Binaries
You can download the latest `.zip` package for Mac or Windows from the [Releases page](../../releases). 
Move the extracted files to your system's VST3 or Components folder and rescan your DAW.

### Build from Source (CMake)
```bash
git clone [https://github.com/DavidSignals/LlamajaREV5.git](https://github.com/DavidSignals/LlamajaREV5.git)
cd LlamajaREV5
mkdir build && cd build
cmake ..
cmake --build . --config Release