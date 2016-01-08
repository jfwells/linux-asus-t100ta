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

#include <stdbool.h>

#include <intel_bufmgr.h>

#include "utilities.h"

#ifndef VIDEO_H_
#define VIDEO_H_

typedef enum IO_METHOD_S {
	IO_METHOD_READ,
	IO_METHOD_MMAP,
	IO_METHOD_USERPOINTER,
	IO_METHOD_DMABUF
} IOMethod_t;

typedef struct RESOLUTION_S {
	int width, height;
} Resolution;

typedef struct VIDEO_BUF_S {
	void *start;
	size_t length;
} VideoBuffer;

typedef struct FIFO_BUF_S {
	int fd;
	void *frameBuf1;
	void *frameBuf2;
	int	width;
	int	height;
	int format;
	int	size;
	int	count;	/* Reserved */
	int	bayerOrder;
	int	bitDepth;
} FIFOBuffer;

typedef struct DMA_BUF_S {
	unsigned int index;
	drm_intel_bo *bo;
	int prime_fd;
} DMABuffer;

typedef struct DRM_S {
	int fd;
	unsigned int width;
	unsigned int height;
	unsigned int format;
	dri_bufmgr *bufmgr;
} DRMContext;

typedef struct VIDEO_S {
	char *device;
	int port;
	Resolution size;
	PixelFormat_t pixelFormat;
	PixelFormat_t inpixelFormat;
	IOMethod_t ioMethod;
	bool isFIFO;
	bool isInterlaced;
	bool hasViewFinder;
	bool isFromViewFinder;

	FILE *hAppLog;

	char *rawFile;
	FIFOBuffer *rawFileInfo;
	int fifoFd;
	long frameCount;
	long frame;
	int fd;
	char *error;
	unsigned int videoBuffersCount;
	unsigned int requestedBuffersCount;
	VideoBuffer *videoBuffers;
	DMABuffer *dmaBuffers;
	unsigned char *lastVideoBuffer;

	DRMContext *drm;

	void (*setLoggerWith) (struct VIDEO_S *, FILE *);
	void (*setIOMethodTo) (struct VIDEO_S *, IOMethod_t);
	void (*setBufferCountTo) (struct VIDEO_S *, int);
	void (*setIsFromViewFinder) (struct VIDEO_S *, bool);
	void (*setHasViewFinder) (struct VIDEO_S *, bool);
	int (*openDevice) (struct VIDEO_S *);
	int (*initDevice) (struct VIDEO_S *);
	int (*startStream) (struct VIDEO_S *);
	int (*stopStream) (struct VIDEO_S *);
	int (*dequeue) (struct VIDEO_S *);
	int (*autoDequeue) (struct VIDEO_S *);
} Video;

#ifdef COLOR_CONVERSION
Video *Video_newWith(const char *, int, int, PixelFormat_t, PixelFormat_t, int, bool);
#else
Video *Video_newWith(const char *, int, int, PixelFormat_t, int, bool);
#endif
void Video_dispose(Video *);

#endif /* VIDEO_H_ */
