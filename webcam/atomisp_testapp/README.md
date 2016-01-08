INTEL ATOM IMAGE SIGNAL PROCESSING (ISP) TEST APP
=================================================

Introduction
------------

This is the test application used to validate the Intel Atom ISP drivers
and firmware via the MIPI CSI interface. 

License
-------

This file is part of Intel Atom ISP Test App.

Intel Atom ISP Test App is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Intel Atom ISP Test App is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Intel Atom ISP Test App.  If not, see http://www.gnu.org/licenses/.

Prerequisites
-------------

On the target machine:

1. Intel Atom ISP chip.
2. Imaging sensors, or cameras, attached via MIPI CSI interface for Atom 
   ISP capturing. 
3. Supported OS:
   - Tizen1.0 with Linux kernel 3.8 (32-bit)
   - Tizen3.0 with LTSI kernel 3.10 (32-bit)
   - Fedora18 with Linux kernel 3.8 (32-bit)
   - Fedora18 with Linux kernel 3.8 (64-bit)
4. Intel Atom ISP drivers and firmware loaded properly on boot up.

On the development machine:

1. Tools
   - gcc
   - gdb (optional)
   - make
2. Libraries:
   - libdrm
   - libdrm_intel
   - wayland-client
   - wayland-egl
   - EGL
   - GLESv2
   - X11
   - V4L2
   - glibc

Notes: 
- Make sure all the libraries installed are 32-bit, or i686/i386, 
  compatible. Only 32-bit architecture is supported by the ISP drivers
  and firmware.
- Make sure the *-devel, or *-dev, packages are also installed.

Building the Test App
---------------------

To build the app for Tizen/Wayland:

> ./do_make.sh mipi-way


To build the app for Fedora/GNOME or KDE:

> ./do_make.sh mipi-x


Both will build the app named 

isp-mipi-test


The build script will print usage and supported `CFLAGS` by issuing

> ./do_make.sh

or

> ./do_make.sh --help

The supported `CFLAGS` now are:

- `-DCOLOR_CONVERSION` to support color conversion (since ISP 3.0). This will 
   enable the `-C` option.

i686 vs x86_64
--------------

The script defaults to compile as an i686 binary (32-bit). 

To enable x86_64 binary (64-bit) generation, the environment variable `ARCH` 
needs to be defined first. 

> export ARCH=x86_64

Then, run the `do_make.sh` script as stated above. You will notice the `-m64` 
is supplied to `gcc` during the build. 

Please note that to get x86_64 binary, you will need to have all the libraries
in x86_64 installed. 

Using the Test App
------------------

To get help on what are the available options:

> ./isp-mipi-test -?

The app will list the following:

```script
---hey---
isp-mipi-test.X11.2014-06-21

./isp-mipi-test 
  -d <device>                           
  -b <number_of_buffers>                            
  -g (use DMA buffer sharing)                           
  -c <out color_format>                             
  -C <in color_format>                          
  -w <width>                            
  -h <height>                           
  -p <mipi-port>                            
  -m <mipi-main>                            
  -v <mipi-viewfinder>                          
  -n <max_frame_count>                          
  -i (to enable interlace mode)                             
  -q (Turn off logging)                             
  -2 (Activate viewfinder stream on)
  -f (Do not render frames)

config.device: /dev/video0
config.mipiPort: 0
config.width: 640
config.height: 480
config.pixelFormat: YV16
config.inpixelFormat: YV16
config.isInterlaced: 0

Invalid parameters or no parameters given.

---bye---
```

The first line indicates on what desktop it can run on. In this case this
app runs only on X11, which is Fedora. The date is appended during build 
time. 

The app's default configurations are also listed. 

To simply stream from a sensor:

> ./isp-mipi-test -c YV16 -p 0

or, if `-DCOLOR_CONVERSION` is used during build: 

> ./isp-mipi-test -c YV16 -C YV16 -p 0

During execution, the app will also store a copy of its activity log in a
log file named:

    isp-mipi-test.[number].log 

Also, another log file named:

    frames.[number].fps

will be generated when the frames are streaming. This file logs all the 
performance numbers of each frame. The format of the frames log is:

```script
frame,capture_time (usec),render_time (usec),total_time (usec),fps
```
```script
              frame: frame number
capture_time (usec): the capture time from sensor to ISP (in microsecond)
 render_time (usec): the rendering time a frame (in microsecond)
  total_time (usec): capture time + render time (in microsecond)
                fps: frames per second so far
```

The `[number]` increments on each run of the app. Both the `log` and `fps` files
share the same `[number]`.

Supported Color Formats
-----------------------

- YV16
    - YUV422 planar
    - full Y, half-width for U and V
    - U and V are not packed
- RGB565/RGBP
    - 16-bit RGB
    - 6-bit for G, 5-bit for R and B components, respectively
- RGB888/RGB3
    - 24-bit RGB
    - 8-bit for all components
- NV12
    - YUV422 semi-planar
    - full Y, half-size for U and V
    - U and V are packed
- BA10
    - RAW color format for Aptina MT9M114 sensor only. 

The color formats are important because the test app will associate it the
correct GL Shader fragment programs, found in the 

    shaders/

directory. If the listed color formats do not include the one you are 
looking for, you will need to provide your own Shader fragment program 
and put them into the 

    shaders/fragment/

directory. Then, proceed to update the following source files:

- utilities.h
- shader.c
- isp-mipi-test.c

The following are how the Shader fragment programs are stored:

```script
shaders
|---fragment
|   |---weston
|   |   |---nv12.c
|   |   |---rgb_passthru.c
|   |   |---uyvy.c
|   |   |---vyuy.c
|   |   |---yuyv.c
|   |   |---yv16.c
|   |   +---yvyu.c
|   +---x
|       |---nv12.c
|       |---rgb_passthru.c
|       |---uyvy.c
|       |---vyuy.c
|       |---yuyv.c
|       |---yv16.c
|       +---yvyu.c
+---vertex
    +---default.c
```

Make sure your fragment is put in the right directory. 

Supported MIPI ports (on the ValleyView CRB)
--------------------------------------------

|Port | Sensor     | Kernel Module |
|-----|------------|---------------|
|0    | OmniVision | OV5640_1.ko   |
|1    | Aptina     | MT9M114.ko    |
|2    | OmniVision | OV5640_2.ko   |

Main and Viewfinder Views
-------------------------

These options are required when streaming from more than one sensors. ISP 
needs to know where to temporarily hold the video data without interfering
with the data from other sensors. 

When no main and viewfinder provided, the app will default them to 

    /dev/video0

for any sensor. This will cause dead-lock and timeouts when streaming from
multiple sensors. 

The test app has been tested on the ValleyView CRB with the supported 
sensors. To stream from multiple sensors, these commands are used:

- streaming from any two sensors:

```script
./isp-mipi-test -c YV16 -p 0 -m /dev/video4 -v /dev/video6 &> /dev/null &
./isp-mipi-test -c YV16 -p 2 -m /dev/video8 -v /dev/video10 &> /dev/null &
```

- streaming from all sensors:

```script 
./isp-mipi-test -c YV16 -p 0 -m /dev/video4 -v /dev/video6 &> /dev/null &
./isp-mipi-test -c YV16 -p 2 -m /dev/video8 -v /dev/video10 &> /dev/null &
./isp-mipi-test -c YV16 -p 1 &> /dev/null &
```

Add the `-C` option if the app is tested against ISP 3.0. 

On the ValleyView CRB, the mains and viewfinders for the sensors are
as follows:

| Sensor   | Port | Main          | Viewfinder    |
|----------|------|---------------|---------------|
| OV5640_1 | 0    | `/dev/video4` | `/dev/video6` |
| OV5640_2 | 2    | `/dev/video8` | `/dev/video10`|
| MT9M114  | 1    | `/dev/video0` | `/dev/video1` |

Streaming With Viewfinder Turned On
-----------------------------------

By default, the app will not activate the viewfinder. To try to stream with 
viewfinder turned on, the command is 

`./isp-mipi-test -g -c nv12 -C ba10 -p 1 -w 1280 -h 720 -m /dev/video0 -v /dev/video1 -2`

The following are the limitations for when viewfinder is activated:

- BA10 (raw) is the only color format supported for viewfinder. 
- Only MT9M114 sensor can support BA10. 
- Only `/dev/video0` for `main` and `/dev/video1` for `viewfinder` is supported. 
- Only a single stream is supported. 
- The app does not render the viewfinder frames. 
- The app shows only corrupted frames due to rendering engine not updated. 

The app automatically reduce the resolutions, width and height, by 12 pixels, 
respectively, when viewfinder is activated. 

Known Issues
------------

1. No File Injection, or FIFO, are enabled by the test app. This feature will
   be added in the future. 
2. ISP acceleration APIs not exercised. This will be added in the future. 
3. Cannot save frames to still images. This is not the requirement of ISP. This
   is a requirement of customers' camera app. 
4. Cannot record frames streamed to a video. This is not the requirement of ISP. 
   This is a requirement of customers' camera app.

Troubleshooting
---------------

1. In the event the app fails with `Error Compiling Fragment Shader` error, it is 
   possible the GLSL version used is different from the one where the app is 
   developed on. Check the `log` file and see what OpenGL is complaining about 
   the shader language, and double check the GLSL specification at 
   http://en.wikipedia.org/wiki/OpenGL_Shading_Language.
   
   The shader codes found in the `shaders/` directory can be updated to comply 
   with the targeted specification.
   
Contacts
--------

- Wong, Ken S'ng <ken.sng.wong@intel.com>
- Bandi, Kushal <kushal.bandi@intel.com>
- Poluri, Sarat Chandra <sarat.chandra.poluri@intel.com>

Release Notes
-------------

[Aug 4, 2014]
- Added `-f` option to turn-off rendering to check that ISP is working.

[Jul 9, 2014]
- Added viewfinder stream on and off with `/dev/video0` as main and `/dev/video1` 
  as viewfinder.
- Updated README.
- Added GPL v3 license and preambles.

[Jun 6, 2014]
- Added this README.
- Made Makefile to be as generic as possible.
- Made do_make.sh to supply the correct Makefile macros to build the right
  version of the test app.


