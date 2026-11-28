# PineGlass

PineGlass is a fork of InfiniTime, the open firmware for the PINE64 PineTime.
It gives the watch an interface inspired by watchOS: a honeycomb app grid,
a pill-button Control Center, notification cards, and a Modular watch face.

## Direction

- Match watchOS layouts, gestures and colors as closely as the hardware
  allows. Check the real watchOS look (Apple support pages, press images)
  before changing a screen. Do not guess from memory.
- Never ship Apple material: no SF fonts, Apple icons, wallpapers, code or
  artwork. Use Inter, Font Awesome and Material icons. Repo, firmware and
  docs never use "Apple", "Apple Watch" or "watchOS" in a name. Describe the
  project as "watchOS-inspired" with the not-affiliated line in the README.
- Battery life comes first. The PineTime screen is an LCD with a fixed
  backlight, so dark pixels save nothing. What costs power is redraw work and
  CPU wake-ups. Redraw only on change (`Utility::DirtyValue`), no animations
  that run while idle, no drag scrolling (an SPI full-screen redraw takes
  about 100 ms). Page with `ScreenList` and the hardware scroll instead.
- Flash is about 474 KB for the app. Check the size after adding fonts or
  glyphs.
- Stay close to upstream so merges from InfiniTimeOrg/InfiniTime stay easy.
  Restyle existing screens in place instead of rewriting them.
- Keep the Bluetooth name "InfiniTime" so companion apps still detect it.

## Glossary

- **Control Center**: `screens/settings/QuickSettings`, opened by the button
  on the watch face. Two-column pill toggles, paged with swipe up.
- **App grid**: `screens/Tile` inside `ApplicationList`, the honeycomb of
  colored circle icons opened by swiping up on the face.
- **Modular**: `screens/WatchFaceModular`, the default face.
- **Platter**: the dark rounded card behind grouped content
  (`Colors::platter`).
- **Upstream**: InfiniTimeOrg/InfiniTime `main`.

## Building and checking

- Firmware: the `infinitime/infinitime-build` container (`docker/build.sh`).
  The `latest` image matches upstream `main` (GCC 15).
- UI checks: InfiniSim built against this tree, with screenshots of every
  changed screen.
- Run `clang-format` on changed files before committing.
