# Kirby: Nightmare in Dream Land Native PC Port (Recompilation) v1.0.0

A native 64-bit Windows port of **Kirby: Nightmare in Dream Land** (USA, A7KE).

Unlike conventional emulation, this project statically recompiles the original Game Boy Advance ARM/Thumb machine code into native C++ instructions. The resulting binary executes natively on modern PC hardware with ultra-low latency, full analog movement, 1080p custom bezel framing, integer scaling, crystal-clear crackle-free audio, and instant save state controls.

---

## 1. Features & Upgrades

- **Native Static Recompilation**: 52,343 functions statically recompiled and compiled natively with Clang/LLVM for 64-bit Windows. Zero JIT compilation overhead, micro-stutter, or frame pacing issues.
- **Crystal-Clear Crackle-Free Audio**: High-precision audio pipeline with band-limited anti-aliased resampling bridge running at 44.1 kHz / 48 kHz. Completely eliminates buffer underruns, popping, and crackling.
- **Custom 1080p Decorative Bezel**: Handcrafted 16:9 bezel featuring official Kirby: Nightmare in Dream Land artwork (Kirby, Meta Knight, Waddle Dee, Fountain of Dreams, and starry sky).
- **F10 Bezel / Clean Fullscreen Toggle**: Press **F10** at any time to switch dynamically between the decorative artwork border and a pristine Clean Fullscreen mode with authentic 3:2 GBA pillarboxing.
- **Plug-and-Play Controller Support**:
  - Full automatic detection for Xbox One / Series, DualShock 4, DualSense, Nintendo Switch Pro, and standard USB controllers.
  - Left Analog Stick steering with calibrated deadzones.
  - Ergonomic secondary face buttons: **X** acts as secondary B (Inhale / Attack), **Y** acts as secondary A (Jump / Float).
  - Analog trigger support: **LT** drops ability (L), **RT** drops ability (R).
  - **F9** in-game controller preset switcher (Xbox, Nintendo Switch, Modern Trigger Action, Classic 1:1) with on-screen HUD toast notifications.
  - Interactive standalone remapping utility (`Remap Controller.bat`).
- **Modern Convenience Hotkeys**:
  - **F11**: In-game settings / overlay menu (replaces default Escape).
  - **Esc**: Instant clean exit (flushes SRAM save and closes).
  - **F1**: Instant Quick Save State (Slot 1) with on-screen HUD confirmation.
  - **F2**: Instant Quick Load State (Slot 1) with on-screen HUD confirmation.
  - **Hold 1**: Real-time rewind gameplay.
  - **Hold 2**: Real-time fast-forward gameplay.

---

## 2. Requirements & Setup

### Requirements
- 64-bit Windows 10 or Windows 11.
- Any DirectX 11 / OpenGL 3.2+ compatible GPU.
- An original USA retail ROM of *Kirby: Nightmare in Dream Land* (`A7KE`, SHA-1: `37a476567d133c146fee6b5e2eb0b07a215da6b0`).

### Quick Start
1. Double-click **`Play Kirby.cmd`** (or `KirbyRecomp.exe`).
2. The game will launch directly into borderless fullscreen with the custom 16:9 bezel and optimal audio settings.

---

## 3. Controls & Hotkeys

### Gameplay Controls
| Action | Keyboard | Xbox / PC Controller | DualShock / DualSense | Switch Pro |
| :--- | :--- | :--- | :--- | :--- |
| **Movement / Duck / Float** | Arrow Keys | **Left Analog Stick** / D-Pad | **Left Analog Stick** / D-Pad | **Left Stick** / D-Pad |
| **A (Jump / Float / Fly)** | `X` | `A` | `Cross` | `B` (Preset 1) / `A` |
| **B (Inhale / Swallow / Attack)**| `Z` | `B` | `Circle` | `A` (Preset 1) / `B` |
| **Secondary Inhale / Attack** | `S` | `X` | `Square` | `Y` |
| **Secondary Jump / Float** | `A` | `Y` | `Triangle` | `X` |
| **L (Discard / Drop Ability)** | `C` | `LB` / `LT` | `L1` / `L2` | `L` / `ZL` |
| **R (Discard / Drop Ability)** | `V` | `RB` / `RT` | `R1` / `R2` | `R` / `ZR` |
| **Start (Pause / Ability Info)**| `Enter` | `Start` | `Options` | `+` |
| **Select** | `Right Shift` | `Back` | `Share` / `Touchpad` | `-` |

### System Hotkeys
| Hotkey | Action |
| :--- | :--- |
| **`F10`** | **Toggle 1080p Bezel on / off (Clean Fullscreen mode)** |
| **`F9`** | **Cycle Controller Presets (Xbox, Switch, Trigger Action, Classic 1:1)** |
| **`F11`** | **In-game overlay menu (replaces Escape)** |
| **`Esc`** | **Instant clean exit (safely saves and closes)** |
| **`F1`** | **Quick Save State (Slot 1)** |
| **`F2`** | **Quick Load State (Slot 1)** |
| **`Hold 1`** | **Rewind gameplay** |
| **`Hold 2`** | **Fast-forward gameplay** |

---

## 4. Architecture & Technical Details

- **Binary Size**: ~74.9 MB native executable (`KirbyRecomp.exe`).
- **Recompilation Shards**: 64 balanced translation units with deterministic link order.
- **Interposer Pipeline**: `kirby_bezel.dll` hooks SDL2 DYNAPI jump table slots dynamically for zero-overhead border rendering and HUD toast notifications.
- **Save Engine**: Standard 32KB battery-backed SRAM saved directly to `saves/kirby_nid.sav`.

---

## 5. Legal Notice

This project is a static recompiler and runtime engine. It does not contain or distribute copyrighted game assets or proprietary Nintendo BIOS code. All trademarks, character names, and game titles are copyright and property of HAL Laboratory, Inc. and Nintendo.
