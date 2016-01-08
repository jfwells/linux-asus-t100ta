INTEL ATOM IMAGE SIGNAL PROCESSING (ISP) LINUX DRIVER
=====================================================

Last Updated On
----------------

2014-09-08

----

Introduction
-------------

The release is in the form of a kernel patch. Please note that the release patch is specific to BYT D0 CPU.

---

Prerequisites
-------------

1. Intel Atom with ISP enabled.
2. Imaging sensors, or cameras, attached via MIPI CSI interface for Atom ISP capturing. 
3. Supported OS:
   - Tizen3.0 with Linux kernel 3.10 LTSI (32-bit)
   - Tizen1.0 with Linux kernel 3.8 (32-bit)
   - Fedora18 with Linux kernel 3.8 (32-bit)
   - Fedora18 with Linux kernel 3.8 (64-bit)

---

Hardware Tested
---------------

Sensors:

| Sensor  | Vendor     | Supported Max. Res. & Fps       | Supported Color Format |
|---------|------------|---------------------------------|------------------------|
| OV5640  | OmniVision | 1920x1080p @ 30fps (eff. 28fps) | YUV422, RGB565         |
| MT9M114 | Aptina     | 1280x720p @ 30fps               | YUV422, RGB565, BA10   |

Interface: Intel Annville AIC via MIPI CSI connector.

| Sensor  | Port | Data Lanes | Pin Count |
|---------|------|------------|-----------|
| OV5640  | 0    | x2         | 40        |
| OV5640  | 2    | x2         | 40        |
| MT9M114 | 1    | x1         | 24        |


---

Building the Driver
-------------------

To build the driver, you will need to patch the Linux kernel with both the ISP patch(es) provided here, and patches
from IO and graphics. The following detailed instructions give you the steps to build the kernel with all the required
patches put in:

[https://intelpedia.intel.com/BYT_Kernel](https://intelpedia.intel.com/BYT_Kernel)

While patching the IO patches, you will need to also patch ISP patch(es) to load ISP modules. Patch them like any 
other patches found in the instructions. 

---

Building the Driver as Built-in or as Loadable Modules
------------------------------------------------------

See "Building for Tizen3.0 with Kernel 3.10 LTSI" if you are building for kernel 3.10 LTSI.

While patching the kernel in the previous section, you may be prompted how the ISP modules to be build:

1. If you decided to have ISP loaded as a built-in driver, press "y" and then Enter.
2. If you decided to have ISP loaded as module, press "m".
   - The following files will need to be loaded after a fresh boot:
      - `videobuf2-core.ko` (NOTE: may not be available.)
      - `videobuf2-memops.ko` (NOTE: may not be available.)
      - `videobuf2-dma-contig.ko` 
      - `ov5640_1.ko` (Onmivision MIPI lane x2 sensor (Port 0))
      - `ov5640_2.ko` (Onmivision MIPI lane x2 sensor (Port 2))
      - `mt9m114.ko` (Aptina MIPI lane x1 sensor (Port 1))
      - `atomisp.ko` (Atom ISP CSS2.0 driver)

When prompted with how other drivers to be build, press "Enter" and continue.

---

Building for Tizen3.0 with Kernel 3.10 LTSI
-------------------------------------------

The modules NEED to be build as Modules ("m") due to an issue with UDEV not able to load the driver 
as built-in.

Instead of using the `git am` command to apply the ISP provided patches, the normal `patch -p` command 
is used. The command to use is

`patch -p1 < [/path/to/ISP/patch/file]`

The same steps applied in the previous section to build the modules.

---

Install ISP Firmware
--------------------

1. Copy the firmware files into /lib/firmware. The files required are:
   - `iaisp_2400_css.bin.big`
   - `iaisp_2400_css.bin.small`
2. Reboot the machine. 

---

Enable ISP for Linux in BIOS
----------------------------
When booting Bayley Bay target machine, the BIOS must be set with the right ISP PCI device.
In the BIOS settings under "Uncore Configuration" set:

1. "ISP Enable/Disable" to Enable
2. "ISP PCI Device" to B0D3F0

---

Load the Modules
----------------

If you have decided to build the ISP as modules, you will need to load them manually in 
the following sequence:

1. `insmod videobuf2-core.ko` (NOTE: if available)
2. `insmod videobuf2-memops.ko` (NOTE: if available)
3. `insmod videobuf2-dma-contig.ko`
4. `insmod ov5640_1.ko`
5. `insmod ov5640_2.ko`
6. `insmod mt9m114.ko`
7. `insmod atomisp.ko`

---

Check ISP Drivers 
-----------------

To check whether the ISP drivers were properly loaded:

1. Type
  
`dmesg | grep atomisp | grep success`

You should see the ISP has been successfully loaded. 

---

Atom ISP Fastboot 2-stage Firmware Load
---------------------------------------

To enable two stage load of the firmware for fastboot purposes please change the 
Atomisp driver `Makefile` and enable `ATOMISP_FASTBOOT` option.

---

Enabling Streams with Resolution Beyond 1920x1080
-------------------------------------------------

For enabling streams of size greater than 1920x1080 please enable the following kernel config options:
- `CONFIG_CMA`
- set `CMA_SIZE_MBYTES` to 64MB

---

Supported Color Formats
-----------------------

The following are the supported and tested color formats by Atom ISP:

| In (from Sensors to ISP) | Out (from ISP to display) |
|--------------------------|---------------------------|
| YUV422                   | YV16                      |
| YUV422                   | NV12                      |
| RGB565 (RGBP)            | RGB565 (RGBP)             |
| BA10 (Bayer Raw)         | NV12                      |

The RGB888 (RGB3) color format is also supported, but the sensors used do not support RGB888 input. 

---

Streaming Frames Using ISP
--------------------------

There are two ways to stream frames from ISP:

- Using the included atomisp_testapp.
- Using GStreamer.

---

atomisp_testapp
---------------

This is a sample app written to stream frames from ISP. It uses the following APIs:

- V4L2: for streaming frames on Linux-based SUT.
- OpenGL: for rendering the frames to screen.

Please see the included `atomisp_testapp` directory for more details. It has README.md and README.md.html with better 
details about how one can use the app. 

---

gstreamer
---------

Gstreamer has been tested by engineering team on Tizen 1.0. The color-format supported between GStreamer & AtomISP is
UYVY with maximum resolution of 720p. The following are sample GStreamer commands that were used for testing:

For remote streaming:

- Target (Tizen): 

	```
	gst-launch v4l2src device="/dev/video4" num-buffers=-1 ! video/x-raw-yuv,format=\(fourcc\)UYVY, width=640,height=480 ! jpegenc ! tcpserversink port=5000
	```
- Host (Fedora): 

	```
	gst-launch tcpclientsrc host=10.2.56.113 port=5000  ! jpegdec ! autovideosink
	```

For saving a single buffer to a file on target: 

```
gst-launch v4l2src device="/dev/video0" num-buffers=1 ! video/x-raw-yuv,format=\(fourcc\)UYVY,width=640,height=480 ! jpegenc ! filesink location=atomisp.jpg
```

For gstreamer-1.x support please contact PED SW engineering team.

---

Known Issues and Limitations
----------------------------

1. Aptina MT9M114 is a one lane (x1) sensor and thus can only support up to 1280x960 resolution. 

2. Only YV16, NV12, RGB565, and RGB888 are supported as output streams from the firmware.

3. ISP FRAME BUFFER WIDTH 32 MULTIPLE: ISP requires a number of frame buffers for processing incoming images.  The frame 
   buffers can be allocated by an application and passed as buffer pointers to ISP firmware 
   through AtomISP v4l2 driver. The frame memory size is calculated based on incoming image resolution width and height.
   Note that ISP frame memory width needs to be a multiple of 32. 
   For example, when an incoming image width is x and height is y, the application is responsible to allocate a memory
   size:

	`Frame buffer size = ( (int) x/32 + 32: 0 if x%32 > 0) * 32 * y * bytes/pixel`
	
4. When three simultaneous streams are running after about 20,000 frames all three streams timeout. The issue is 
   suspected to be a performance issue due to memory reads in firmware and is under investigation.

5. The Preview, or Viewfinder, is only supported with Aptina sensor on NV12 and BA10 colour format combination.

6. Only EMGD driver dated August 5, 2014, or older, works in this release. 
   - Preferred EMGD is July 22, 2014 release.

---

Release Notes
-------------

2014-09-08

- Updated README with mentions of atomisp_testapp and sample commands using GStreamer.

2014-08-27

- Added kernel 3.8 Fedora 18 support.

- HSDs implemented:
  - HSD 4994712

2014-08-22

- Added kernel 3.10 LTSI release.

- New Features Added:
  1. Support for 5MP (2560 x 1920) UYVY format.

- HSDs implemented:
  - HSD 4994664

2014-07-24

- New Features Added in ISP 3.0 release:
  1. EXPBUF ioctl enabled. Now buffers allocated by Atomisp can be shared with EMGD using the DMA Buf sharing framework. 
     Atomisp driver is now capable of being the DMA exporter.
  2. New `VIDIOC_SUBDEV_S_FMT` ioctl enabled to let application to individually set the input and output formats.
  3. Video pipe and Viewfinder pipe enabled.
  4. fastboot and multistream_enable are now module parameters and enable users to startup the driver in several 
     different ways to save time or enable all features at startup depending on requirement.
  5. Special ioctl called `ATOMISP_FW_LOAD` added to switch the firmware from small to big in the case of fastboot.
  6. Optimized ia_css_init and ia_css_uninit. Reduces time taken by atomisp_reset from ~1.2s to ~50 ms. 

- DCNs and HSDs implemented:
  - DCN 75240
  - DCN 75277
  - DCN 75244
  - DCN 75245
  - DCN 75205
  - DCN 75132
  - DCN 75311
  - HSD 4634959
  - HSD 4994596
  - HSD 4994597
  - HSD 4994598

- Released new test app, and EOL-ed old app.
