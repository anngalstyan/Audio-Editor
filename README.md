# Audio Editor

A desktop audio editor built in C++17 with Qt6, featuring real-time effects, waveform visualization, undo/redo, and subtitle/caption support.

![screenshot](assets/screenshots/preview.png)

---

## Features

- **Waveform visualization** — zoom, seek by clicking, auto-scroll during playback
- **Real-time audio effects** — Reverb, Volume, Speed, Normalize with live preview before committing
- **Non-destructive editing** — original audio is always preserved; effects are previewed separately
- **Undo/redo** — full operation history via Command pattern
- **Caption/subtitle support** — load and export `.srt` files, synchronized with playback
- **Layered logging** — console and file logging via Composite pattern, injected throughout

---

## Architecture

The project is organized into two layers:

```
Core/               Business logic — no Qt dependency
  Adapters/         MP3 read/write via mpg123 and LAME
  Effects/          Audio processing algorithms (Strategy pattern)
  Commands/         Undo/redo infrastructure (Command pattern)
  Logging/          Console + file logging (Composite pattern)
  Services/         SRT caption parser and exporter
GUI/                Qt6 presentation layer
  AudioEngine       Playback, sample conversion, volume control
  WaveformWidget    Waveform rendering and mouse interaction
  EffectsPanel      Effect management and preview
  MainWindow        Top-level coordinator
```

### Design Patterns

| Pattern | Location | Purpose |
|---|---|---|
| **Strategy** | `Core/Effects/` | Interchangeable audio processing algorithms — add new effects without touching existing code |
| **Command** | `Core/Commands/` | Encapsulates operations so they can be undone and redone |
| **Factory** | `Core/EffectFactory` | Registry-based effect creation — no switch statements |
| **Adapter** | `Core/Adapters/` | Wraps mpg123/LAME behind a unified interface so the audio backend can be swapped |
| **Composite** | `Core/Logging/` | Console and file loggers treated uniformly, combined at runtime |
| **Observer** | GUI layer | Qt signals/slots decouple `AudioEngine` state from UI components |

---

## Dependencies

| Dependency | Version | Purpose |
|---|---|---|
| Qt | 6.x | GUI, audio playback, threading |
| libmpg123 | any | MP3 decoding |
| LAME (libmp3lame) | any | MP3 encoding |
| CMake | 3.16+ | Build system |

### macOS (Homebrew)

```bash
brew install qt@6 mpg123 lame
```

### Linux (apt)

```bash
sudo apt install qt6-base-dev libmpg123-dev libmp3lame-dev cmake
```

---

## Build

```bash
git clone https://github.com/annagalstyan14/audio-editor.git
cd audio-editor
mkdir build && cd build
cmake ..
make -j$(nproc)
```

On macOS, if Qt is not found automatically:

```bash
cmake .. -DCMAKE_PREFIX_PATH=/opt/homebrew/opt/qt@6
```

---

## Usage

1. Open an MP3 file via **File → Open** or drag and drop
2. Use the transport bar to play, pause, and seek
3. Add effects from the **Effects** panel — preview them before applying
4. Load an `.srt` caption file via **File → Load Captions**
5. Export edited audio via **File → Export**
6. Undo/redo with `Cmd+Z` / `Cmd+Shift+Z`

A sample caption file is included in `samples/` to test caption loading.

---

## What I'd Improve

- Support for more audio formats (WAV, FLAC, AAC) — the Adapter layer is already structured for this, it just needs new concrete adapters
- Export the full effects chain as a reusable preset
- Unit tests for Core — the business logic is decoupled from Qt intentionally, which makes it straightforward to test with Catch2
- Waveform rendering performance on very long files — currently peak-based but could use progressive loading

---

## License

MIT
