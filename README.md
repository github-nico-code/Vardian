This software is still in alpha status. Please take care. It is hosted at 
https://github.com/github-nico-code/Vardian

!!! To use Vardian you hat to replace the usbser driver to WinUSB driver for the Windows "Rokid Max (3)" 
device with the help of Zadic software (https://zadig.akeo.ie). !!!

Vardian is a program to use the Rokid Max device to look at the windows screens. It uses the 
3DOF functionality to navigate the Rokid Max screen about the windows screen.

To extend Windows virtual screen without connecting an additional real monitor I take a
HDMI dummy plug like https://www.amazon.de/dp/B07YMTKJCR?ref=ppx_yo2ov_dt_b_product_details&th=1

I have taken some files from monado for this project 
(https://monado.pages.freedesktop.org/monado/index.html). Thanks for that.

I have taken some files from microsoft samples. Thanks for that.

Thanks for the libusb library too.

This program has a lot of open TODOs. If you want to take over project or do some changes 
then please write me: nico.kasprzyk@gmx.de

Next steps from my side:
* Get the movements more smoothly.
* Maybe use "usbser" connection for reading 3DOF data from Rokid Max because this driver
  is automatically installed from Windows for Rokid Max.
* Implement a lot of comfort functionality.
* Stability.
