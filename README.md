# uvc-gadget

This is a fork of [uvc-gadget](https://gitlab.freedesktop.org/camera/uvc-gadget) that adds some extra functionality.

## Features

When the pigpio daemon is running and `uvc-gadget` is ran with options `-p "pin1 pin2 pin3"`:
- **pin1:** `HIGH` whenever `uvc-gadget` is running.
- **pin2:** `HIGH` whenever video is being streamed to host.
- **pin3:** Pauses video stream on `HIGH`, resumes on `LOW`. Meant to be used a "privacy lens".

## Installation

*The following instructions are for if you want to set up a Raspberry Pi as a webcam using this uvc-gadget. If you want to turn a Raspberry Pi Zero 2 W into a dedicated webcam check out [Webcam Pi](https://github.com/elcalzado/webcampi)*

1. Flash `Raspberry Pi OS Lite (64-bit)` onto a microSD card using the Raspberry Pi Imager
2. Log into the Pi using `SSH`
3. Update your device
```bash
   sudo apt update
   sudo apt full-upgrade
   sudo reboot
```
4. Enable OTG mode
```bash
   echo "dtoverlay=dwc2,dr_mode=otg" | sudo tee -a /boot/firmware/config.txt
```
5. Install dependencies
```bash
   sudo apt install git meson libcamera-dev libjpeg-dev pigpio
```
6. Build and install uvc-gadget
```bash
   git clone https://github.com/elcalzado/uvc-gadget.git
   cd uvc-gadget
   make uvc-gadget
   sudo meson install -C build/
   sudo ldconfig
   sudo mv scripts/uvc-gadget.sh /usr/local/bin/
```
7. Create the pigpio start-up service (If not interested in using GPIO jump to step 7)
```bash
   sudo nano /usr/lib/systemd/system/pigpio.service
```
8. Copy the following into `/usr/lib/systemd/system/pigpio.service`:
```
[Unit]
Description=Pigpio daemon
After=network.target

[Service]
ExecStart=/usr/bin/pigpiod -g

[Install]
WantedBy=multi-user.target
```
9. Create the uvc-gadget start-up service
```bash
   sudo nano /usr/lib/systemd/system/uvc-gadget.service
```
10. Copy the following into `/usr/lib/systemd/system/uvc-gadget.service`:
    - If you want to use different pins for GPIO just change the numbers after `-p`. You can use [pinout.xyz](pinout.xyz) to view the different numberings.
	- If you want to disable GPIO just remove `-p "23 24 26"`.
```
[Unit]
Description=UVC Gadget
# If not using pigpio change pigpio.service to sysinit.target
After=pigpio.service

[Service]
ExecStartPre=/usr/local/bin/uvc-gadget.sh start
ExecStart=/usr/bin/uvc-gadget -c 0 uvc.0 -p "23 24 26"

[Install]
WantedBy=multi-user.target
```
11. Start the services
```bash
   sudo systemctl daemon-reload
   sudo systemctl enable pigpio.service # Ignore if not using pigpio
   sudo systemctl start pigpio.service # Ignore if not using pigpio
   sudo systemctl enable uvc-gadget.service
   sudo systemctl start uvc-gadget.service 
```
12. Restart
```bash
   sudo reboot
```

---

*See below to simply build the uvc-gadget*

## Build instructions:

To compile:

```
$ meson build
$ ninja -C build
```

## Cross compiling instructions:

Cross compilation can be managed by meson. Please read the directions at
https://mesonbuild.com/Cross-compilation.html for detailed guidance on using
meson.

In brief summary:
```
$ meson build --cross <meson cross file>
$ ninja -C build
```
