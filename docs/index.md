---
layout: default
---

This software is still in alpha status.

Vardian is a program to use the Rokid Max device to look at a multi monitor setup 
for Windows PCs. It uses the 3DOF functionality to navigate
a Window opened at the Rokid Max screen about the virtual windows
monitors.

# Releases

Releases can be found at [Vardian Github Releases Page](https://github.com/github-nico-code/Vardian/releases)

# Add virtual Screen to Windows

To add more virtual monitors to my setup without real monitors and
without Admin access I take a HDMI dummy plug like 
[One Dummy HDMI example device](https://www.amazon.de/dp/B07YMTKJCR?ref=ppx_yo2ov_dt_b_product_details&amp;th=1).

An alternative approach can be virtual monitor drivers like
[IddSampleDriver](https://github.com/roshkins/IddSampleDriver).

# Used Code Sources

I have taken some files from [Monado](https://monado.pages.freedesktop.org/monado/index.html)
for this project. Thanks for that.

I have taken some files from microsoft samples. Thanks for that.

# Software functionality

What does this software do:
1. Detect whether Rokid Max is connected.
1. Move Rokid Max screen to the most right in Windows Virtual screen
   setup.
1. Place a window at the Rokid Max screen.
1. Copy Windows screen regions to the window at the Rokid Max screen.
1. Move the copied screen region with the help of Rokid Max 3DOF
        functionality.

# TODOs

This program has some open TODOs. If you want to take over project or do
      some changes then please write me: nico.kasprzyk@gmx.de

Next steps could be:
* Implement a lot of comfort functionality
* Stability
* Use additional HID commands to get and set Rokid Max screen mode

# Web resources

[Void Computing Rokid Max information](https://voidcomputing.hu/blog/good-bad-ugly/#the-rokid-air-mcu-protocol)

![Branching](docs/ar_glasses.jpeg)
```
The final element.
```
