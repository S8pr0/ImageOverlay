# ImageOverlay

*A tiny (~30 KB) Windows app that shows a semi-transparent, click-through image on top of all windows — handy for tracing and drawing from references.*

## Features

- Opening an image: via a button, by dragging and dropping a file into the window, or by **pasting from the clipboard (Ctrl+V)**.
- Sliders for **opacity** (5–100%) and **scale** (10–400%).
- **"Pin"** mode: mouse clicks pass through the image to the application underneath.
- PNG files with transparent backgrounds are displayed correctly.
- Photos from phones are automatically rotated based on EXIF ​​data.
- Files can be opened using the "Open with..." menu, or by dragging and dropping an image directly onto `ImageOverlay.exe`.

Supported formats: PNG, JPG, BMP, GIF, TIFF. Other formats, such as WebP, can be added via the clipboard: select "Copy image" in your browser, then press Ctrl+V within the application.

## Controls

| Action | Keybind |
|---|---|
| Move your image | Drag with mouse |
| Opacity | Mouse Wheel |
| Scale | Ctrl + Mouse Wheel |
| Attach | **Ctrl+Alt+L** |
| Hide | **Ctrl+Alt+H** |
| Opacity while attached | **Ctrl+Alt+Плюс / Минус** |
| Open / Paste | Ctrl+O / Ctrl+V |

## Code

The program is written in C using only the Win32 API and GDI+; it does not rely on the C runtime or any third-party libraries.

### Windows (Visual Studio)

1. Install [Visual Studio](https://visualstudio.microsoft.com/) or the Build Tools, ensuring the "Desktop development with C++" workload is selected.
2. Open the **x64 Native Tools Command Prompt for VS**.
3. Navigate to the project directory and run `build.bat`.

The output will be: `build\ImageOverlay.exe`.

### Linux / macOS (LLVM Cross-Compilation)

Requires `clang`, `lld-link`, `llvm-rc`, and `llvm-dlltool` (available via the `clang`, `lld`, and `llvm` packages). No Windows SDK is required.

```sh
./build.sh
```

### GitHub Actions

A ready-to-use `ImageOverlay.exe` is automatically built on every push: go to the **Actions** tab → select the latest build → download the *ImageOverlay* artifact.

## Structure

```
src/overlay.c        Main program source code
res/app.rc           Resources: icon, manifest, and version information
res/app.manifest     DPI-awareness and modern UI element styling
res/app.ico          Application icon
imports/*.def        Windows function definitions for building via build.sh
build.bat / build.sh Build scripts
```

## License

[MIT](LICENSE)
