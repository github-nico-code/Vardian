# Vardian

Vardian is a Windows desktop viewport for **Rokid Max** glasses. It displays a native-resolution
region of a larger Windows virtual desktop and moves that region using the glasses' 3DOF IMU.

> [!IMPORTANT]
> Vardian is alpha software. Test new builds before relying on them for daily work.

## Highlights

- Detects the Rokid Max display and USB HID interface.
- Preserves text clarity with pixel-to-pixel `BitBlt` capture—no desktop scaling.
- Displays a native 1920×1080 region of a larger desktop, such as 3840×2160.
- Provides smooth, frame-time-aware horizontal and vertical viewport tracking.
- Filters IMU noise with configurable exponential smoothing and a dead zone.
- Supports independent horizontal and vertical sensitivity.
- Can lock vertical movement for stable reading and coding.
- Supports recentering from the tray menu, settings window, or `Ctrl+Alt+R`.
- Clamps the viewport to the Windows virtual desktop, including layouts with negative coordinates.

## How it works

Vardian places its output window on the Rokid Max display. Every frame, it copies the visible
desktop region into that window using `BitBlt` with `SRCCOPY`. Source and destination pixels are
kept at the same size, so Vardian does not squeeze an entire 4K desktop into the glasses' display.

The Rokid Max gyroscope controls a floating-point target viewport. A time-aware low-pass filter,
dead zone, and a second target-follow step reduce jitter while retaining responsive head tracking.
Only the final source rectangle is rounded to integer pixels; subpixel movement remains in the
tracking state for later frames.

## Requirements

- Windows 10 or Windows 11, x64.
- Rokid Max connected as an active Windows display and USB HID device.
- An extended virtual desktop larger than the Rokid viewport for panning.
- For building: Visual Studio 2022 with the **Desktop development with C++** workload and a
  Windows 10/11 SDK.

Additional desktop space can come from physical monitors, an HDMI dummy plug, or a virtual display
driver such as [IddSampleDriver](https://github.com/roshkins/IddSampleDriver). Driver installation
requirements depend on the selected solution; Vardian itself is designed to run as a normal user.

## Build

Open `Vardian_Project.sln` in Visual Studio, select `Release | x64`, and build the `Vardian`
project.

From a Visual Studio Developer Command Prompt, the application project can also be built with:

```powershell
msbuild Vardian.vcxproj /m /p:Configuration=Release /p:Platform=x64
```

The expected output is:

```text
x64\Release\Vardian.exe
x64\Release\vardian.ini
```

The separate setup project requires the appropriate Visual Studio Installer Projects extension.

## Run

1. Connect the Rokid Max and configure it as an extended Windows display.
2. Make sure the desktop region excluding the Rokid display is at least as large as the viewport.
3. Start `Vardian.exe`.
4. Right-click the Vardian tray icon to open tracking settings or recenter the viewport.
5. Turn your head left or right to navigate the wider desktop.

Vardian moves the Rokid display to the right side of the Windows virtual desktop during
initialization. Windows display changes cause the application to detect and initialize the device
again.

## Tracking settings

Open **Tracking settings...** from the tray icon. Numeric changes take effect after pressing
**Apply** and are saved to `vardian.ini` next to the executable.

| Setting | Default | Valid range | Description |
| --- | ---: | ---: | --- |
| Horizontal sensitivity | `1.0` | `0.0`–`10.0` | Controls yaw-driven left/right movement. |
| Vertical sensitivity | `0.4` | `0.0`–`10.0` | Controls pitch-driven up/down movement. |
| Smoothing | `0.15` | `0.01`–`1.0` | Lower values are smoother; higher values respond faster. |
| Dead zone | `12.0` | `0.0`–`10000.0` | Ignores small mapped movement, in viewport pixels per second. |
| Lock vertical movement | `false` | Boolean | Keeps the viewport's vertical position fixed. |

The smoothing value describes the nominal response at 60 Hz. Vardian adjusts the effective filter
coefficient using the measured frame time, so behavior remains similar if the update interval
varies.

## Configuration file

Default `vardian.ini`:

```ini
[tracking]
horizontal_sensitivity=1.0
vertical_sensitivity=0.4
smoothing=0.15
dead_zone=12.0
lock_vertical=false
```

Missing, malformed, non-finite, or out-of-range numeric values are replaced or clamped to safe
values. A damaged configuration file must not prevent the application from starting.

## Recenter

Recenter makes the current head orientation the new tracking center while keeping the visible
viewport in place. It also clears pending filtered movement so the viewport does not jump.

Available controls:

- Press `Ctrl+Alt+R` from any application.
- Choose **Recenter** from the Vardian tray menu.
- Press **Recenter** in the tracking settings window.

If another application already owns `Ctrl+Alt+R`, Vardian logs the registration failure and the
tray/settings controls remain available.

## Diagnostics

Runtime errors are written beside the executable to:

```text
Vardian.exe.err.log
```

Debug builds additionally log the raw and filtered yaw/pitch movement plus target and current
viewport coordinates. Tracking diagnostics are rate-limited and excluded from Release builds.

## Current limitations

- Rokid Max is identified by its current display name and USB VID/PID.
- Tracking uses incremental IMU data rather than an absolute orientation pose. The dead zone limits
  sensor bias, and Recenter provides an explicit correction when needed.
- Vardian intentionally shows only a native-resolution desktop region; it does not provide a
  scaled overview of the entire virtual desktop.
- This project does not integrate with SteamVR or OpenXR.
- Hardware behavior depends on the Windows display layout, USB connection, and Rokid firmware.

## Acknowledgements

Vardian incorporates ideas or code derived from:

- [Monado](https://monado.freedesktop.org/)
- Microsoft Windows samples
- [Void Computing's Rokid protocol notes](https://voidcomputing.hu/blog/good-bad-ugly/#the-rokid-air-mcu-protocol)

The original Vardian project is maintained at
[github-nico-code/Vardian](https://github.com/github-nico-code/Vardian).

## License

Vardian is distributed under the [Boost Software License 1.0](LICENSE.txt).
