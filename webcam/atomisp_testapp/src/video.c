/**
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
	along with Intel Atom ISP Test App.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "video.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/time.h>

#include <drm/drm.h>
#include <drm/drm_mode.h>
#include <intel_bufmgr.h>
#include <linux/videodev2.h>
#include <linux/v4l2-mediabus.h>
#include <linux/v4l2-subdev.h>

#include <xf86drm.h>
#include <xf86drmMode.h>

#include "utilities.h"
#include "str_struct.h"

#define PAGE_ALIGN(x) ((x + 0xfff) & 0xfffff000)
#define ERRSTR strerror(errno)
#define CLEAR(x) memset(&(x), 0, sizeof(x))
#define BYE_ON(cond, ...) \
do { \
    if (cond) { \
        int errsv = errno; \
        fprintf(stderr, "ERROR(%s:%d) : ", \
            __FILE__, __LINE__); \
        errno = errsv; \
        fprintf(stderr,  __VA_ARGS__); \
        abort(); \
    } \
} while(0)

#define OUTPUT_MODE_FILE 0x0100
#define OUTPUT_MODE_TEXT 0x0200

//#define COLOR_CONVERSION

#define _ISP_MODE_PREVIEW	0x8000
#define _ISP_MODE_STILL     0x2000
#define _ISP_MODE_VIDEO     0x4000

#define DMABUF_COUNT	4
#define FRMBUF_COUNT	6

#define DRM_DEV "emgd"
#define FIFO_DEV_PATH "/dev/video2"

static int bytesperlineFactor = 2; // default = (bits per pixel / bits per byte) = 16 / 8

static int getV4L2FourCC(PixelFormat_t _format) {
	switch (_format) {
	case RGBP:
		bytesperlineFactor = 2;
		return V4L2_PIX_FMT_RGB565;
	case RGB3:
		bytesperlineFactor = 3;
		return V4L2_PIX_FMT_RGB24;
	case NV12:
		bytesperlineFactor = 2;
		return V4L2_PIX_FMT_NV12;
	case BA10:
		bytesperlineFactor = 2;
		return V4L2_PIX_FMT_SGRBG10;
	case YUYV:
		bytesperlineFactor = 2;
		return V4L2_PIX_FMT_YUYV;
	default: // YV16
		bytesperlineFactor = 2;
		return V4L2_PIX_FMT_YUV422P;
	}
}

static void setLoggerWith(Video *self, FILE *_hAppLog) {
	self->hAppLog = _hAppLog;
}

static void setIOMethodTo(Video *self, IOMethod_t _ioMethod) {
	self->ioMethod = _ioMethod;
}

static void setBufferCountTo(Video *self, int _bufferCount) {
	if (_bufferCount > DMABUF_COUNT) {
		self->requestedBuffersCount = _bufferCount;
	}
}

static void writeToLog(Video *self, const char *_fmt, ...) {
	bool hasLogFileHandle = true;

	char buffer[1024];
	va_list args;
	va_start(args, _fmt);
	vsprintf(buffer, _fmt, args);
	va_end(args);

	if (self->hAppLog == NULL) {
		hasLogFileHandle = false;
	}

	// log to screen
	fprintf(stdout, "%s\n", buffer);
	fflush(stdout);

	// log to file
	if (hasLogFileHandle) {
		fprintf(self->hAppLog, "%s\n", buffer);
		fflush(self->hAppLog);
	}
}

static int readRawFileForFIFO(Video *self) {
	if (self->rawFile == NULL || strlen(self->rawFile) <= 0) {
		sprintf(self->error, "Raw image file not set.");
		return 0;
	}

	self->rawFileInfo = (FIFOBuffer *) calloc(1, sizeof(FIFOBuffer));
	struct stat st;
	CLEAR(st);

	FILE *handle = fopen(self->rawFile, "r");
	if (handle == NULL) {
		sprintf(self->error, "Raw image file cannot be opened.");
		return 0;
	}

	self->rawFileInfo->fd = fileno(handle);

	if (0 > fstat(self->rawFileInfo->fd, &st)) {
		sprintf(self->error, "Raw image file cannot be fstat-ed.");
		fclose(handle);
		return 0;
	}

	self->rawFileInfo->format = getV4L2FourCC(self->pixelFormat);
	self->rawFileInfo->width = self->size.width;
	self->rawFileInfo->height = self->size.height;
	self->rawFileInfo->size = PAGE_ALIGN(st.st_size);

	self->rawFileInfo->frameBuf1 = calloc(1, PAGE_ALIGN(self->rawFileInfo->size));
	size_t readin = 0;
	char *ptr = (char *) self->rawFileInfo->frameBuf1;
	while(!feof(handle)) {
		readin = fread (ptr, self->rawFileInfo->size, 1, handle);
		ptr += readin;
	}

	fclose(handle);
	return 1;
}

static int openDevicePath(Video *self, const char *_devicePath) {
	if (_devicePath == NULL || strlen(_devicePath) <= 1) {
		return -1;
	}

	struct stat st;
	int fd = -1;

	if (-1 == stat(_devicePath, &st)) {
		sprintf(self->error, "Cannot stat %s: %s", _devicePath, ERRSTR);
		return -1;
	}

	if (!S_ISCHR(st.st_mode)) {
		sprintf(self->error, "%s is not a device.", _devicePath);
		return -1;
	}

	fd = open(_devicePath, O_RDWR | O_NONBLOCK, 0);
	if (fd < 0) {
		sprintf(self->error, "DEV OPEN: %s", ERRSTR);
		return -1;
	}

	return fd;
}

static int openDevicesForFifo(Video *self) {
	if (self->device) {
		free(self->device);
		self->device = NULL;
	}

	// open stream device for stream
	self->fd = openDevicePath(self, self->device);
	if (0 > self->fd) {
		return 0;
	}

	// open FIFO device for injection
	// this is hard-coded paths from Intel Atom ISP driver
	Str *deviceInput = Str_newWith(FIFO_DEV_PATH);
	self->fifoFd = openDevicePath(self, deviceInput->str);
	if (0 > self->fifoFd) {
		return 0;
	}
	Str_dispose(deviceInput);

	return 1;
}

static int openDevice(Video *self) {
	self->fd = openDevicePath(self, self->device);

	if (0 > self->fd) {
		return 0;
	}
//	struct stat st;
//
//	if (-1 == stat(self->device, &st)) {
//		sprintf(self->error, "Cannot stat %s: %s", self->device, ERRSTR);
//		return 0;
//	}
//
//	if (!S_ISCHR(st.st_mode)) {
//		sprintf(self->error, "%s is not a device.", self->device);
//		return 0;
//	}
//
//	self->fd = open(self->device, O_RDWR | O_NONBLOCK, 0);
//	if (self->fd < 0) {
//		sprintf(self->error, "DEV OPEN: %s", ERRSTR);
//		return 0;
//	}

	return 1;
}

static int initFifoDeviceWithRawImage(Video *self) {
	struct v4l2_format fmt;
	struct v4l2_streamparm parm;
	struct v4l2_capability cap;
	struct v4l2_requestbuffers req;

	int ret = -1;

	CLEAR(fmt);
	CLEAR(parm);
	CLEAR(cap);
	CLEAR(req);

	ret = ioctl(self->fifoFd, VIDIOC_QUERYCAP, &cap);
	if (0 > ret) {
		sprintf(self->error, "%s is not a device.", FIFO_DEV_PATH);
		return -1;
	}

	if (!(cap.capabilities & V4L2_CAP_VIDEO_OUTPUT)) {
		sprintf(self->error, "%s is not a FIFO capable device.", FIFO_DEV_PATH);
		return -1;
	}

	CLEAR(parm);
	parm.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	parm.parm.output.outputmode = OUTPUT_MODE_FILE;
	return 1;
}

#ifdef COLOR_CONVERSION
static int getMbuscode(int inpixelformat_fourcc, int outpixelformat_fourcc){

	switch(inpixelformat_fourcc){
	case V4L2_PIX_FMT_RGB565:
		return V4L2_MBUS_FMT_RGB565_2X8_LE;
		break;
	case V4L2_PIX_FMT_YUV422P:
		return V4L2_MBUS_FMT_UYVY8_1X16;
		break;
	case V4L2_PIX_FMT_SGRBG10:
		return V4L2_MBUS_FMT_SGRBG10_1X10;
		break;
	case V4L2_PIX_FMT_YUYV:
		return V4L2_MBUS_FMT_YUYV8_1X16;
		break;
	default:
		fprintf(stdout, "setting mbus code same as output pixel format\n");
		fflush(stdout);
		fprintf(stderr, "\n\n : Unrecognized mbuscode :setting mbus code same as output pixel format.\n\n");
		fflush(stderr);
		switch(outpixelformat_fourcc){
			case V4L2_PIX_FMT_RGB565:
				return V4L2_MBUS_FMT_RGB565_2X8_LE;
				break;
			case V4L2_PIX_FMT_YUV422P:
				return V4L2_MBUS_FMT_UYVY8_1X16;
		}
		break;
	}
	return 0;
}
#endif

static int initDevice(Video *self) {
	struct v4l2_streamparm parm;

    CLEAR(parm);
	parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	parm.parm.capture.capturemode = _ISP_MODE_STILL;

	if (self->hasViewFinder) {
		parm.parm.capture.capturemode = _ISP_MODE_VIDEO;
	}

	if (self->isFIFO) {
		parm.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
		parm.parm.output.outputmode = OUTPUT_MODE_FILE;

		if (0 >= readRawFileForFIFO(self)) {
			return 0;
		}
	}

	if (self->fd < 0) {
		if (self->isFIFO) {
			/**
			 * NOTE for FIFO: Open /dev/video2 for FIFO capture and /dev/video0 for normal
			 */
			if (0 >= openDevicesForFifo(self)) {
				return 0;
			}

			// Init /dev/video2 with raw image
			if (0 >= initFifoDeviceWithRawImage(self)) {
				return 0;
			}
		} else {
			/**
			 * Open like normal
			 */
			if (0 >= openDevice(self)) {
				return 0;
			}
		}
	}

	struct v4l2_capability caps;
	CLEAR(caps);
	int ret;

    if (!self->isFromViewFinder) {
		if (!self->isFIFO) {
			ret = ioctl(self->fd, VIDIOC_S_INPUT, &self->port);
		} else {
			ret = ioctl(self->fifoFd, VIDIOC_S_INPUT, -1);
		}
		ret = ioctl(self->fd, VIDIOC_S_PARM, &parm);

		ret = ioctl(self->fd, VIDIOC_QUERYCAP, &caps);
		if (ret) {
			sprintf(self->error, "VIDIOC_QUERYCAP: %s", ERRSTR);
			return 0;
		}
    }

	struct v4l2_format fmt;
	CLEAR(fmt);
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ret = ioctl(self->fd, VIDIOC_G_FMT, &fmt);
    if (ret < 0) {
    	sprintf(self->error, "PRE VIDIOC_G_FMT: %s", ERRSTR);
    	return 0;
    }

    writeToLog(self, "BASE VIDIOC_G_FMT: %dx%d, %.4s, %d, %d, %d", fmt.fmt.pix.width,
																   fmt.fmt.pix.height,
																   (char *)&fmt.fmt.pix.pixelformat,
																   fmt.fmt.pix.bytesperline,
																   fmt.fmt.pix.sizeimage,
																   fmt.fmt.pix.field);

    fmt.fmt.pix.width = self->size.width;
    fmt.fmt.pix.height = self->size.height;
    fmt.fmt.pix.pixelformat = getV4L2FourCC(self->pixelFormat);
    fmt.fmt.pix.bytesperline = self->size.width * 2;
    fmt.fmt.pix.sizeimage = fmt.fmt.pix.bytesperline * self->size.height;
    fmt.fmt.pix.field = V4L2_FIELD_NONE;

    // reset height if interlace is set
    if (self->isInterlaced) {
    	fmt.fmt.pix.height = self->size.height / 2;
    	fmt.fmt.pix.field = V4L2_FIELD_ALTERNATE;
    }

    writeToLog(self, "PRE VIDIOC_S_FMT: %dx%d, %.4s, %d, %d, %d", fmt.fmt.pix.width,
																  fmt.fmt.pix.height,
																  (char *)&fmt.fmt.pix.pixelformat,
																  fmt.fmt.pix.bytesperline,
																  fmt.fmt.pix.sizeimage,
																  fmt.fmt.pix.field);

//setting of subdev format for input
#ifdef COLOR_CONVERSION
    struct v4l2_subdev_format subdev_fmt;
    subdev_fmt.pad = 0;
    subdev_fmt.which = V4L2_SUBDEV_FORMAT_ACTIVE;

    if (self->hasViewFinder) {
		subdev_fmt.format.code = V4L2_MBUS_FMT_SGRBG10_1X10;
		subdev_fmt.format.width = fmt.fmt.pix.width;
		subdev_fmt.format.height = fmt.fmt.pix.height;

		writeToLog(self, "=== viewfinder active ===");
		writeToLog(self, "Setting subdev_fmt.format to RAW @ %dx%d", subdev_fmt.format.width,
				                                                     subdev_fmt.format.height);
		writeToLog(self, "=== viewfinder active ===");
    } else {
		subdev_fmt.format.code = getMbuscode(getV4L2FourCC(self->inpixelFormat), getV4L2FourCC(self->pixelFormat));
		subdev_fmt.format.width = fmt.fmt.pix.width;
		subdev_fmt.format.height = fmt.fmt.pix.height;
    }

    subdev_fmt.format.field = V4L2_FIELD_NONE;

	if (!self->isFromViewFinder) {
		ret = ioctl(self->fd, VIDIOC_SUBDEV_S_FMT, &subdev_fmt);

		if (ret < 0) {
			sprintf(self->error, "VIDIOC_SUBDEV_S_FMT: %s", ERRSTR);
			return 0;
		}

		writeToLog(self, "POST VIDIOC_SUBDEV_S_FMT: %dx%d, %.4s", subdev_fmt.format.width,
				                                                  subdev_fmt.format.height,
				                                                  (char *)&subdev_fmt.format.code);
	}
#endif
//end

	// decrease the main resolutions by 12px to conform with the rectangle spec it has viewfinder
	if (self->hasViewFinder && !self->isFromViewFinder) {
		fmt.fmt.pix.width = self->size.width - 12;
		fmt.fmt.pix.height = self->size.height - 12;
	}

    ret = ioctl(self->fd, VIDIOC_S_FMT, &fmt);
    if (ret < 0) {
		sprintf(self->error, "VIDIOC_S_FMT: %s", ERRSTR);
		return 0;
	}

    ret = ioctl(self->fd, VIDIOC_G_FMT, &fmt);
	if (ret < 0) {
		sprintf(self->error, "POST VIDIOC_G_FMT: %s", ERRSTR);
		return 0;
	}

    writeToLog(self, "POST VIDIOC_S_FMT: %dx%d, %.4s, %d, %d, %d", fmt.fmt.pix.width,
																   fmt.fmt.pix.height,
																   (char *)&fmt.fmt.pix.pixelformat,
																   fmt.fmt.pix.bytesperline,
																   fmt.fmt.pix.sizeimage,
																   fmt.fmt.pix.field);

	struct v4l2_requestbuffers requestBuffers;
	CLEAR(requestBuffers);

	if (self->ioMethod == IO_METHOD_DMABUF) {
		/**
		 * Begin DMABUF init
		 */

		self->drm = (DRMContext *) calloc(1,  sizeof (DRMContext));

		requestBuffers.count = DMABUF_COUNT;
		if (self->requestedBuffersCount > 0) {
			requestBuffers.count = self->requestedBuffersCount;
		}
		requestBuffers.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		requestBuffers.memory = V4L2_MEMORY_DMABUF;

		writeToLog(self, "DMA Requesting buffers for %d...", requestBuffers.count);

		ret = ioctl(self->fd, VIDIOC_REQBUFS, &requestBuffers);
		if (ret < 0) {
			sprintf(self->error, "VIDIOC_REQBUFS: %s", ERRSTR);
			return 0;
		}

		writeToLog(self, "DMA Requesting buffers for %d... done; Got %d", ((self->requestedBuffersCount <= 0) ? DMABUF_COUNT : self->requestedBuffersCount),
				                                                          requestBuffers.count);

		if (requestBuffers.count < self->requestedBuffersCount) {
			sprintf(self->error, "VIDIOC_REQBUFS: Not enough buffers allocated. %d", requestBuffers.count);
			return 0;
		}
		self->videoBuffersCount = requestBuffers.count;

		self->drm->format = getV4L2FourCC(self->pixelFormat);
		self->drm->width = self->size.width;
		self->drm->height = self->size.height;

		writeToLog(self, "DMA opening DRM...");

		self->drm->fd = drmOpen(DRM_DEV, NULL);
		if (self->drm->fd < 0) {
			sprintf(self->error, "drmOpen(%s): %s", DRM_DEV, ERRSTR);
			return 0;
		}

		int batchSize = fmt.fmt.pix.sizeimage * self->videoBuffersCount;

		self->drm->bufmgr = intel_bufmgr_gem_init(self->drm->fd, batchSize);
		if (self->drm->bufmgr == NULL) {
			sprintf(self->error, "intel_bufmgr_gem_init: %s", ERRSTR);
			return 0;
		}

		writeToLog(self, "DMA opening DRM... done");

		writeToLog(self, "DMA initializing buffers...");

		// init buffers
		self->dmaBuffers = (DMABuffer *) calloc(self->videoBuffersCount, sizeof(DMABuffer));

		int i;
		for (i=0; i < self->videoBuffersCount; i++) {
			self->dmaBuffers[i].index = i;
			self->dmaBuffers[i].prime_fd = -1;
			self->dmaBuffers[i].bo = NULL;
		}

		writeToLog(self, "DMA initializing buffers... done");
		writeToLog(self, "DMA allocating Intel BO to buffers for render...");

		// create buffers
		unsigned int name;
		struct drm_prime_handle prime;
		memset(&prime, 0, sizeof prime);

		for (i=0; i < self->videoBuffersCount; i++) {
			self->dmaBuffers[i].bo = drm_intel_bo_alloc_for_render(self->drm->bufmgr, "v4l2_surface", fmt.fmt.pix.sizeimage, 0);
			if (self->dmaBuffers[i].bo == NULL) {
				sprintf(self->error, "drm_intel_bo_alloc: %s", ERRSTR);
				return 0;
			}

			//get the prime handle and put it in prime_fd of dmaBuffers
			prime.handle = self->dmaBuffers[i].bo->handle;
			ret = ioctl(self->drm->fd, DRM_IOCTL_PRIME_HANDLE_TO_FD, &prime);
			self->dmaBuffers[i].prime_fd = prime.fd;

			if (0 != drm_intel_bo_flink(self->dmaBuffers[i].bo, &name)) {
				sprintf(self->error, "drm_intel_bo_flink: %s", ERRSTR);
				return 0;
			}
		}

		writeToLog(self, "DMA allocating Intel BO to buffers for render... done");

		/**
		 * Finish DMABUF init
		 */
	} else if (self->ioMethod == IO_METHOD_MMAP) {
		/**
		 * Begin MMAP init
		 */

		requestBuffers.count = FRMBUF_COUNT;
		if (self->requestedBuffersCount > 0) {
			requestBuffers.count = self->requestedBuffersCount;
		}
		requestBuffers.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		if (self->isFIFO) {
			requestBuffers.count = 1;
			//requestBuffers.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
		}

		requestBuffers.memory = V4L2_MEMORY_MMAP;

		ret = ioctl(self->fd, VIDIOC_REQBUFS, &requestBuffers);
		if (ret < 0) {
			sprintf(self->error, "VIDIOC_REQBUFS: %s", ERRSTR);
			return 0;
		}

		if (requestBuffers.count <= 0) {
			sprintf(self->error, "VIDIOC_REQBUFS: Not enough buffers allocated. %d", requestBuffers.count);
			return 0;
		}

		self->videoBuffers = (VideoBuffer *) calloc(requestBuffers.count, sizeof(*self->videoBuffers));
		if (!self->videoBuffers) {
			sprintf(self->error, "Not enough memory.");
			return 0;
		}

		for (self->videoBuffersCount = 0; self->videoBuffersCount < requestBuffers.count; ++self->videoBuffersCount) {
			struct v4l2_buffer buf;
			CLEAR(buf);

			buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			if (self->isFIFO) {
				buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
			}
			buf.memory = V4L2_MEMORY_MMAP;
			buf.index = self->videoBuffersCount;

			ret = ioctl(self->fd, VIDIOC_QUERYBUF, &buf);
			if (ret < 0) {
				sprintf(self->error, "VIDIOC_QUERYBUF: %s", ERRSTR);
				return 0;
			}

			self->videoBuffers[self->videoBuffersCount].length = buf.length;
#ifdef I64
			self->videoBuffers[self->videoBuffersCount].start = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_32BIT, self->fd, buf.m.offset);
#else
			self->videoBuffers[self->videoBuffersCount].start = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, self->fd, buf.m.offset);
#endif

			if (MAP_FAILED == self->videoBuffers[self->videoBuffersCount].start) {
				sprintf(self->error, "Failed to MMAP videoBuffers[%d].", self->videoBuffersCount);
				return 0;
			}
		}

		/**
		 * Finish MMAP init
		 */
	} else if (self->ioMethod == IO_METHOD_READ) {
		/**
		 * Begin READ init
		 */

		// TODO: implement io_read here

		/**
		 * Finish READ init
		 */
	} else if (self->ioMethod == IO_METHOD_USERPOINTER) {
		/**
		 * Begin USERPOINTER init
		 */

		// TODO: implement io_userpointer here

		/**
		 * Finish USERPOINTER init
		 */
	}

	return 1; // all good
}

static int startStream(Video *self) {
	int ret;
	enum v4l2_buf_type type;

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (self->isFIFO) {
		/**
		 * FIFO only need to queue once.
		 */
		struct v4l2_buffer buf;
		CLEAR(buf);

		int i;
		for (i=0; i < self->videoBuffersCount; i++) {
			buf.index = i;
			buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
			buf.memory = V4L2_MEMORY_MMAP;

			memcpy(self->videoBuffers[i].start, self->rawFileInfo->frameBuf1, self->rawFileInfo->size);

			ret = ioctl(self->fd, VIDIOC_QBUF, &buf);
			if (ret < 0) {
				sprintf(self->error, "VIDIOC_QBUF: %s", ERRSTR);
				return 0;
			}
		}
	} else {
		struct v4l2_buffer buf;
		CLEAR(buf);

		int i;
		for (i=0; i < self->videoBuffersCount; i++) {
			buf.index = i;
			buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buf.memory = V4L2_MEMORY_MMAP;

			if (self->ioMethod == IO_METHOD_DMABUF) {
				buf.memory = V4L2_MEMORY_DMABUF;
				buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
				buf.m.fd = self->dmaBuffers[i].prime_fd;
			}

			ret = ioctl(self->fd, VIDIOC_QBUF, &buf);
			if (ret < 0) {
				sprintf(self->error, "VIDIOC_QBUF: %s", ERRSTR);
				return 0;
			}
		}
	}

	ret = ioctl(self->fd, VIDIOC_STREAMON, &type);
	if (ret < 0) {
		sprintf(self->error, "VIDIOC_STREAMON: %s", ERRSTR);
		return 0;
	}

	// reset frame counters
	self->frame = -1;
	self->frameCount = 0;

	return 1; // all good
}

static int stopStream(Video *self) {
	enum v4l2_buf_type type;
	int ret;
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	switch (self->ioMethod) {
		case IO_METHOD_READ:
			break;
		default:
		{
			if (self->fd >= 0) {
				ret = ioctl(self->fd, VIDIOC_STREAMOFF, &type);
				if (ret < 0) {
					sprintf(self->error, "VIDIOC_STREAMOFF: %s", ERRSTR);
				}
			}
			break;
		}
	}

	return 1; // all good
}

static int dequeue(Video *self) {
	int ret;
	struct v4l2_buffer buf;
	CLEAR(buf);

	switch (self->ioMethod) {
		case IO_METHOD_READ:
			break;
		case IO_METHOD_DMABUF:
			buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buf.memory = V4L2_MEMORY_DMABUF;
			ret = ioctl(self->fd, VIDIOC_DQBUF, &buf);
			if (ret < 0) {
				sprintf(self->error, "VIDIOC_DQBUF: %s", ERRSTR);
				return 0;
			}

			ret = drm_intel_bo_map(self->dmaBuffers[buf.index].bo, 1);
			self->lastVideoBuffer = (unsigned char *) (self->dmaBuffers[buf.index].bo->virtual);

			self->frame += 1;
			self->frameCount += 1;
			buf.m.fd = self->dmaBuffers[buf.index].prime_fd	;

			ret = ioctl(self->fd, VIDIOC_QBUF, &buf);
			if (ret < 0) {
				sprintf(self->error, "VIDIOC_QBUF: %s", ERRSTR);
				return 0;
			}
			ret = drm_intel_bo_unmap(self->dmaBuffers[buf.index].bo);
			break;
		case IO_METHOD_USERPOINTER:
			break;
		case IO_METHOD_MMAP:
		{
			buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buf.memory = V4L2_MEMORY_MMAP;

			ret = ioctl(self->fd, VIDIOC_DQBUF, &buf);
			if (ret < 0) {
				sprintf(self->error, "VIDIOC_DQBUF: %s", ERRSTR);
				return 0;
			}

			if (buf.index >= self->videoBuffersCount) {
				sprintf(self->error, "Invalid buf.index: %d of %d: %s", buf.index, self->videoBuffersCount, ERRSTR);
				return 0;
			}

			self->lastVideoBuffer = (unsigned char *) self->videoBuffers[buf.index].start;
			self->frame += 1;
			self->frameCount += 1;

			if (self->isFIFO) {
				// only QBUF and DQBUF once for FIFO
				break;
			}

			ret = ioctl(self->fd, VIDIOC_QBUF, &buf);
			if (ret < 0) {
				sprintf(self->error, "VIDIOC_QBUF: %s", ERRSTR);
				return 0;
			}
			break;
		}
	}
	return 1;
}

static int autoDequeue(Video *self) {
	while(1) {
		fd_set fds;
		struct timeval tv;
		int r;

		FD_ZERO(&fds);
		FD_SET(self->fd, &fds);

		// Timeout.
		tv.tv_sec = 2;
		if (self->isFIFO) {
			tv.tv_sec = 60;	// longer timeout since raw files can be large
		}

		tv.tv_usec = 0;

		r = select(self->fd + 1, &fds, NULL, NULL, &tv);

		if (-1 == r) {
			if (EINTR == errno) {
				continue;
			}
			sprintf(self->error, "select: %s", ERRSTR);
			return 0;
		}

		if (0 == r) {
			sprintf(self->error, "select timeout");
			return 0;
		}

		if (dequeue(self)) {
			break;
		}
	}

	return 1;
}

static void setIsFromViewFinder(Video *self, bool _isFromViewFinder) {
	self->isFromViewFinder = true;
}

static void setHasViewFinder(Video *self, bool _hasViewFinder) {
	self->hasViewFinder = true;
}

#ifdef COLOR_CONVERSION
static void Video_init(Video *self, const char *_device,
		               int _width, int _height,
		               PixelFormat_t _pixelFormat, PixelFormat_t _inpixelFormat,
		               int _port, bool _isInterlaced) {
#else
static void Video_init(Video *self, const char *_device,
					   int _width, int _height,
					   PixelFormat_t _pixelFormat,
					   int _port, bool _isInterlaced) {
#endif
	self->device = (char *) calloc(strlen(_device), sizeof(char));
	strncpy(self->device, _device, strlen(_device));

	self->port = _port;
	self->size.width = _width;
	self->size.height = _height;

	self->pixelFormat = _pixelFormat;
#ifdef COLOR_CONVERSION
	self->inpixelFormat = _inpixelFormat;
#endif
	self->isFIFO = false;
	self->isInterlaced = _isInterlaced;
	self->hasViewFinder = false;
	self->isFromViewFinder = false;
	self->fifoFd = -1;
	self->hAppLog = NULL;
	self->rawFile = NULL;
	self->rawFileInfo = NULL;
	self->frameCount = 0;
	self->frame = -1;
	self->error = (char *) calloc(256, sizeof(char));

	self->ioMethod = IO_METHOD_MMAP; // default

	self->fd = -1;
	self->videoBuffersCount = 0;
	self->requestedBuffersCount = 0;
	self->videoBuffers = NULL;
	self->dmaBuffers = NULL;
	self->lastVideoBuffer = NULL;

	self->drm = NULL;

	// methods
	self->setLoggerWith = setLoggerWith;
	self->setIOMethodTo = setIOMethodTo;
	self->setBufferCountTo = setBufferCountTo;
	self->setIsFromViewFinder = setIsFromViewFinder;
	self->setHasViewFinder = setHasViewFinder;
	self->openDevice = openDevice;
	self->initDevice = initDevice;
	self->startStream = startStream;
	self->stopStream = stopStream;
	self->dequeue = dequeue;
	self->autoDequeue = autoDequeue;
}

#ifdef COLOR_CONVERSION
Video *Video_newWith(const char *_device, int _width, int _height,
		             PixelFormat_t _pixelFormat, PixelFormat_t _inpixelFormat,
		             int _port, bool _isInterlaced) {
	Video *video = (Video *) calloc(1, sizeof(Video));
	Video_init(video, _device, _width, _height, _pixelFormat, _inpixelFormat,_port, _isInterlaced);
	return video;
}
#else
Video *Video_newWith(const char *_device, int _width, int _height,
		             PixelFormat_t _pixelFormat,
		             int _port, bool _isInterlaced) {
	Video *video = (Video *) calloc(1, sizeof(Video));
	Video_init(video, _device, _width, _height, _pixelFormat, _port, _isInterlaced);
	return video;
}
#endif

void Video_dispose(Video *self) {
	// 1. free all the buffers from memory
	writeToLog(self, "Resetting lastVideoBuffer...");
	if (self->lastVideoBuffer != NULL) {
		self->lastVideoBuffer = NULL;
	}
	writeToLog(self, "Reset lastVideoBuffer.");

	switch (self->ioMethod) {
		case IO_METHOD_MMAP:
		{

			int i;
			for (i=0; i < self->videoBuffersCount; i++) {
				writeToLog(self, "Unmapping %d of %d...", i, self->videoBuffersCount-1);
				if (-1 == munmap(self->videoBuffers[i].start, self->videoBuffers[i].length)) {
					sprintf(self->error, "Failed to UNMAP videoBuffers[%d].", i);
				}
				writeToLog(self, "Unmapped %d of %d.", i, self->videoBuffersCount-1);
			}
			break;
		}
		case IO_METHOD_DMABUF:
		{
			free(self->drm);
			/*The unmapping is done in dequeue()*/
			break;
		}
		case IO_METHOD_READ:
		case IO_METHOD_USERPOINTER:
		default:
			// TODO: implement freeing steps here.
			break;
	}

	// 2. close the device
	if (self->drm != NULL) {
		writeToLog(self, "Closing drm...");
		if (self->drm->fd >= 0) {
			drmClose(self->drm->fd);
		}
		writeToLog(self, "Closed drm.");

		writeToLog(self, "Freeing dmaBuffers...");
		if (self->dmaBuffers != NULL) {
			free(self->dmaBuffers);
		}
		writeToLog(self, "Freed dmaBuffers.");

		self->drm = NULL;
		self->dmaBuffers = NULL;
	}

	if (self->isFIFO) {
		free(self->rawFileInfo);
		self->rawFileInfo = NULL;
	}

	writeToLog(self, "Closing fd...");
	if (self->fd >= 0) {
		close(self->fd);
	}
	writeToLog(self, "Close fd.");

	// 3. free all other pointers from memory
	writeToLog(self, "Freeing error and device strings...");
	free(self->error);
	free(self->device);
	writeToLog(self, "Freed error and device strings.");

	// 4. free self
	writeToLog(self, "Freeing video self...");

	free(self);
}
