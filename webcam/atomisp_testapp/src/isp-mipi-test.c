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

#include "isp-mipi-test.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include <assert.h>
#include <signal.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/time.h>

#include "utilities.h"
#include "str_struct.h"
#include "video.h"
#include "shader.h"

#ifdef WAYLAND
#define APP_NAME "isp-mipi-test.Wayland"
#else
#define APP_NAME "isp-mipi-test.X11"
#endif

#ifndef APP_BUILD_DATE
#define APP_BUILD "unknown"
#else
#define Str(arg) #arg
#define StrValue(arg) Str(arg)
#define APP_BUILD StrValue(APP_BUILD_DATE)
#endif

#define MAX_FRAME_COUNT 500
#define OV5640_1_MAIN "/dev/video4"
#define OV5640_1_VF "/dev/video6"
#define OV5640_2_MAIN "/dev/video8"
#define OV5640_2_VF "/dev/video10"

#define SIZE_OF_SHADER_LOG 1000
#define VF_WIDTH 640
#define VF_HEIGHT 480

/**
 * Globals begin
 */

// common variables
#ifdef WAYLAND
ContextData contextData = {0};
#else
Display *x_display;
Window win;
#endif
Video *mipi, *mipi_vf;
bool gIsForever = true;
bool gIsUseViewfinder = false;
int g_VideoWidth;	// used by eglCreateSurfaceWindow
int g_VideoHeight;	// used by eglCreateSurfaceWindow
PixelFormat_t g_PixelFormat; 	// used by drawScene

// EGL variables
EGLDisplay eglDisplay;
EGLint eglDispMajor;
EGLint eglDispMinor;

EGLContext eglContext0, eglContext1;
EGLSurface eglSurface0, eglSurface1;

// GLES variables
GLint g_attr_pos = 0;
GLint g_attr_color = 1;
GLint g_attr_tex = 2;
GLint g_u_matrix_p3 = -1;
GLint g_u_matrix_sz = -1;
GLint g_texY = -1;
GLint g_texUV = -1;
GLint g_texU = -1;
GLint g_texV = -1;
GLuint g_CubeTexture = 0;
GLuint g_UVTexture = 0;
GLuint g_UTexture = 0;
GLuint g_VTexture = 0;
GLfloat g_Draw1ViewPort[16];
int g_Rotation = 0;
GLuint shaderProgram;

/**
 * Globals end
 */

void initConfigWithDefaults(AppConfig_t *_config) {
	if (_config == NULL) {
		return;
	}

	_config->appCommand = Str_newWith(APP_NAME);
	_config->device = Str_newWith("/dev/video0");
	_config->mipiMain = Str_newWith(OV5640_1_MAIN);
	_config->mipiViewFinder = Str_newWith(OV5640_1_VF);
	_config->mipiPort = 0;
	_config->width = 640;
	_config->height = 480;
	_config->pixelFormat = YV16;
#ifdef COLOR_CONVERSION
	_config->inpixelFormat = YV16;
#endif
	_config->maxFrameCount = MAX_FRAME_COUNT;
	_config->isUseDMABuf = false;
	_config->isInterlaced = false;
	_config->isQuiet = false;
	_config->isNoRender = false;
	_config->requestedBufferCount = 0;
	_config->unsafeRepeatCount = 0;
}

int parseArguments(int argc, char *argv[], AppConfig_t *_config) {
	_config->appCommand->set(_config->appCommand, "%s", argv[0]);

	bool didProcessedOptions = false;

	static const char *options = "d:c:C:w:h:p:m:v:n:iqgb:?u:2f";
	int c;
	while ((c = getopt(argc, argv, options)) != -1) {
		didProcessedOptions = true;
		switch (c) {
		case 'd':
			_config->device->set(_config->device, "%s", optarg);
			break;
		case 'c':
		{
			Str *colorFormat = Str_newWith(optarg);

			if (colorFormat->isEqualsIgnoreCase(colorFormat, Str_newWith("YV16"))) {
				_config->pixelFormat = YV16;
			} else if (colorFormat->isEqualsIgnoreCase(colorFormat, Str_newWith("RGB565")) ||
					   colorFormat->isEqualsIgnoreCase(colorFormat, Str_newWith("RGBP"))) {
				_config->pixelFormat = RGBP;
			} else if (colorFormat->isEqualsIgnoreCase(colorFormat, Str_newWith("RGB888")) ||
					   colorFormat->isEqualsIgnoreCase(colorFormat, Str_newWith("RGB3"))) {
				_config->pixelFormat = RGB3;
			} else if (colorFormat->isEqualsIgnoreCase(colorFormat, Str_newWith("YUYV8")) ||
                       colorFormat->isEqualsIgnoreCase(colorFormat, Str_newWith("YUYV"))) {
                _config->pixelFormat = YUYV;
            } else if (colorFormat->isEqualsIgnoreCase(colorFormat, Str_newWith("NV12"))){
				_config->pixelFormat = NV12;
			} else {
				fprintf(stderr, "\n\n%s : Unrecognized colorformat for Atom ISP.\n\n", colorFormat->str);
				fflush(stderr);
				return 0;
			}

			Str_dispose(colorFormat);
			break;
		}
#ifdef COLOR_CONVERSION
		case 'C':
		{
			Str *colorFormat = Str_newWith(optarg);
			if (colorFormat->isEqualsIgnoreCase(colorFormat, Str_newWith("YV16"))) {
				_config->inpixelFormat = YV16;
			} else if (colorFormat->isEqualsIgnoreCase(colorFormat, Str_newWith("RGB565")) ||
			           colorFormat->isEqualsIgnoreCase(colorFormat, Str_newWith("RGBP"))) {
				_config->inpixelFormat = RGBP;
			} else if (colorFormat->isEqualsIgnoreCase(colorFormat, Str_newWith("RGB888")) ||
					   colorFormat->isEqualsIgnoreCase(colorFormat, Str_newWith("RGB3"))) {
				_config->inpixelFormat = RGB3;
			} else if (colorFormat->isEqualsIgnoreCase(colorFormat, Str_newWith("YUYV8")) ||
			           colorFormat->isEqualsIgnoreCase(colorFormat, Str_newWith("YUYV"))) {
				_config->inpixelFormat = YUYV;
			} else if (colorFormat->isEqualsIgnoreCase(colorFormat, Str_newWith("SGRBG10")) ||
					   colorFormat->isEqualsIgnoreCase(colorFormat, Str_newWith("BA10"))) {
				_config->inpixelFormat = BA10;
			} else {
				fprintf(stderr, "\n\n%s : Unrecognized colorformat for Atom ISP.\n\n", colorFormat->str);
				fflush(stderr);
				return 0;
			}
			if(_config->inpixelFormat == RGBP || _config->inpixelFormat == RGB3){
				if(_config->pixelFormat == YV16 || _config->pixelFormat == NV12){
					fprintf(stderr, "\n\n : RGB to YUV color conversion not allowed in Atom ISP.\n\n");
					fflush(stderr);
					return 0;
				}
			}
			Str_dispose(colorFormat);
			break;
		}
#endif
		case 'w':
			_config->width = atoi(optarg);
			break;
		case 'h':
			_config->height = atoi(optarg);
			break;
		case 'p':
		{
			int port = atoi(optarg);
			switch (port) {
			case 0:
			case 1:
			case 2:
				_config->mipiPort = port;
				break;
			default:
				fprintf(stderr, "\n\n%d : Unrecognized port number.\n\n", port);
				fflush(stderr);
				return 0;
			}
			break;
		}
		case 'n':
		{
			int max = atoi(optarg);
			_config->maxFrameCount = max;
			break;
		}
		case 'm':
		{
			_config->mipiMain->set(_config->mipiMain, "%s", optarg);

			// over-ride the device with mipi main node
			_config->device->set(_config->device, "%s", optarg);
			break;
		}
		case 'v':
			_config->mipiViewFinder->set(_config->mipiViewFinder, "%s", optarg);
			break;
		case '2':
			gIsUseViewfinder = true;
			break;
		case 'i':
			_config->isInterlaced = true;
			break;
	    case 'q':
	        _config->isQuiet = true;
	        break;
		case 'g':
			_config->isUseDMABuf = true;
			break;
		case 'b':
			_config->requestedBufferCount = atoi(optarg);
			break;
		case 'u':
			_config->unsafeRepeatCount = atoi(optarg);
			break;
		case 'f':
			_config->isNoRender = true;
			break;
		case '?':
			return 0;
		default:
			return 0;
		}
	}

	if (didProcessedOptions) {
		return 1;
	}
	return 0;
}

bool validateConfig(AppConfig_t *_config) {
	Str *errorMsg = Str_new();

	// check if valid port is given
	if (_config->mipiPort < 0 || _config->mipiPort > 2) {
		errorMsg->set(errorMsg, "Only ports 0 (OV_2), 1 (Aptina) or 2 (OV_2) are supported.");
		fprintf(stdout, "%s\n", errorMsg->str);
		fflush(stdout);
		return false;
	}

	// check is if Aptina and the resolution must be max at 720p
	if (_config->mipiPort == 1 &&
		(_config->width * _config->height) > (1280*720)) {
		errorMsg->set(errorMsg, "Aptina CANNOT have resolution more than 720p.");
		fprintf(stdout, "%s\n", errorMsg->str);
		fflush(stdout);
		return false;
	}

	return true;
}

static void writeToErr(FILE *_hAppLog, const char *_fmt, ...) {
	char buffer[1024];
	va_list args;
	va_start(args, _fmt);
	vsprintf(buffer, _fmt, args);
	va_end(args);

	// log to screen
	fprintf(stderr, "%s\n", buffer);
	fflush(stderr);

	// log to file
	fprintf(_hAppLog, "%s\n", buffer);
	fflush(_hAppLog);
}

static void writeToLog(FILE *_hAppLog, const char *_fmt, ...) {
	char buffer[1024];
	va_list args;
	va_start(args, _fmt);
	vsprintf(buffer, _fmt, args);
	va_end(args);

	// log to screen
	fprintf(stdout, "%s\n", buffer);
	fflush(stdout);

	// log to file
	fprintf(_hAppLog, "%s\n", buffer);
	fflush(_hAppLog);
}

void logConfig(FILE *_hAppLog, AppConfig_t *_config) {
	if (_hAppLog == NULL || _config == NULL) {
		return;
	}

	// log and emit
	writeToLog(_hAppLog, "config.device: %s", _config->device->str);
	writeToLog(_hAppLog, "config.mipiPort: %d", _config->mipiPort);
	writeToLog(_hAppLog, "config.width: %d", _config->width);
	writeToLog(_hAppLog, "config.height: %d", _config->height);

	char *strColorFormat;
	switch (_config->pixelFormat) {
	case YV16:
		strColorFormat = "YV16";
		break;
	case RGBP:
		strColorFormat = "RGB565 / RGBP";
		break;
	case RGB3:
		strColorFormat = "RGB888 / RGB3";
		break;
	case NV12:
		strColorFormat = "NV12";
		break;
	case YUYV:
		strColorFormat = "YUYV / YUYV8";
		break;
	case BA10:
		strColorFormat = "BA10 / SGRBG10";
		break;
	default:
		strColorFormat = "invalid";
		break;
	}
	writeToLog(_hAppLog, "config.pixelFormat: %s", strColorFormat);

#ifdef COLOR_CONVERSION
	switch (_config->inpixelFormat) {
	case YV16:
		strColorFormat = "YV16";
		break;
	case RGBP:
		strColorFormat = "RGB565 / RGBP";
		break;
	case RGB3:
		strColorFormat = "RGB888 / RGB3";
		break;
	case NV12:
		strColorFormat = "NV12";
		break;
	case YUYV:
		strColorFormat = "YUYV / YUYV8";
		break;
	case BA10:
		strColorFormat = "BA10 / SGRBG10";
		break;
	default:
		strColorFormat = "invalid";
		break;
	}
	writeToLog(_hAppLog, "config.inpixelFormat: %s", strColorFormat);
#endif

	writeToLog(_hAppLog, "config.isInterlaced: %d", _config->isInterlaced);
}

#ifdef WAYLAND
/**
 * Wayland/EGL/GLES stuffs begin
 */

static void pointerHandleEnter(void *_data, struct wl_pointer *_pointer,
					    uint32_t _serial, struct wl_surface *_surface,
						wl_fixed_t _sx, wl_fixed_t _sy) {
	// TODO: nothing to do
}

static void pointerHandleLeave(void *_data, struct wl_pointer *_pointer,
						uint32_t _serial, struct wl_surface *_surface) {
	// TODO: nothing to do
}

static void pointerHandleMotion(void *_data, struct wl_pointer *_pointer,
						 uint32_t _time, wl_fixed_t _sx, wl_fixed_t _sy) {
	// TODO: nothing to do
}

static void pointerHandleButton(void *_data, struct wl_pointer *_pointer,
						 uint32_t _serial, uint32_t _time, uint32_t _button, uint32_t _state) {
	struct ContextData *d = (struct ContextData*) _data;
	if (_button == BTN_LEFT && _state == WL_POINTER_BUTTON_STATE_PRESSED) {
		wl_shell_surface_move(d->shell_surface, d->seat, _serial);
	}
}

static void pointerHandleAxis(void *_data, struct wl_pointer *_pointer,
					   uint32_t _time, uint32_t _axis, wl_fixed_t _value) {
	// TODO: nothing to do
}

static void handlePing(void *_data, struct wl_shell_surface *_shell_surface, uint32_t _serial) {
	wl_shell_surface_pong(_shell_surface, _serial);
}

static void handleConfigure(void *_data, struct wl_shell_surface *_shell_surface, uint32_t _edges,
		                    int32_t _width, int32_t _height) {
	//struct ContextData *d = (struct ContextData*) _data;
}

static void handlePopupDone(void *_data, struct wl_shell_surface *_shell_surface) {
	// TODO: nothing to do
}

static const struct wl_pointer_listener pointerListener = {
	pointerHandleEnter,
	pointerHandleLeave,
	pointerHandleMotion,
	pointerHandleButton,
	pointerHandleAxis
};

static void seatHandleCapabilities(void *_data, struct wl_seat *_seat, uint32_t _caps) {
	struct ContextData *d = (struct ContextData*) _data;
	if ((_caps & WL_SEAT_CAPABILITY_POINTER) && !d->pointer) {
		d->pointer = wl_seat_get_pointer(_seat);
		wl_pointer_add_listener(d->pointer, &pointerListener, d);
	} else if (!(_caps & WL_SEAT_CAPABILITY_POINTER) && d->pointer) {
		wl_pointer_destroy(d->pointer);
		d->pointer = NULL;
	}
}

static const struct wl_shell_surface_listener shellSurfaceListener = {
	handlePing,
	handleConfigure,
	handlePopupDone
};

static const struct wl_seat_listener seatListener = {
	seatHandleCapabilities
};

static void displayHandleGlobal(void *_data, struct wl_registry *_registry,
		                 	    uint32_t _id, const char *_interface, uint32_t _version) {
	struct ContextData *d = (struct ContextData*) _data;
	if (strcmp(_interface, "wl_compositor") == 0) {
		d->compositor = (struct wl_compositor *) wl_registry_bind(_registry, _id, &wl_compositor_interface, 1);
	} else if (strcmp(_interface, "wl_shell") == 0) {
		d->shell = (struct wl_shell *) wl_registry_bind(_registry, _id, &wl_shell_interface, 1);
	} else if (strcmp(_interface, "wl_shell_surface") == 0) {
		d->shell_surface = (struct wl_shell_surface *) wl_registry_bind(_registry, _id, &wl_shell_surface_interface, 1);
		wl_shell_surface_add_listener(d->shell_surface, &shellSurfaceListener, d);
	} else if (strcmp(_interface, "wl_seat") == 0) {
		d->seat = (struct wl_seat *) wl_registry_bind(_registry, _id, &wl_seat_interface, 1);
		wl_seat_add_listener(d->seat, &seatListener, d);
	}
}

static const struct wl_registry_listener registryListener = {
	displayHandleGlobal
};
#endif

void makeIdentity(GLfloat *m) {
	int i;
	for (i = 0; i < 16; i++) {
		m[i] = 0.0;
	}
	m[0] = m[5] = m[10] = m[15] = 1.0;
}

void matrixMult(GLfloat *p, const GLfloat *a, const GLfloat *b) {
	int i;
	for (i = 0; i < 4; i++) {
		const GLfloat ai0=DO_MATRIX(a,i,0),  ai1=DO_MATRIX(a,i,1),  ai2=DO_MATRIX(a,i,2),  ai3=DO_MATRIX(a,i,3);
		DO_MATRIX(p,i,0) = ai0 * DO_MATRIX(b,0,0) + ai1 * DO_MATRIX(b,1,0) + ai2 * DO_MATRIX(b,2,0) + ai3 * DO_MATRIX(b,3,0);
		DO_MATRIX(p,i,1) = ai0 * DO_MATRIX(b,0,1) + ai1 * DO_MATRIX(b,1,1) + ai2 * DO_MATRIX(b,2,1) + ai3 * DO_MATRIX(b,3,1);
		DO_MATRIX(p,i,2) = ai0 * DO_MATRIX(b,0,2) + ai1 * DO_MATRIX(b,1,2) + ai2 * DO_MATRIX(b,2,2) + ai3 * DO_MATRIX(b,3,2);
		DO_MATRIX(p,i,3) = ai0 * DO_MATRIX(b,0,3) + ai1 * DO_MATRIX(b,1,3) + ai2 * DO_MATRIX(b,2,3) + ai3 * DO_MATRIX(b,3,3);
   }
}

void makeYRotMatrix(GLfloat angle, GLfloat *m) {
	float c = cos(angle * M_PI / 180.0);
	float s = sin(angle * M_PI / 180.0);
	int i;
	for (i = 0; i < 16; i++) {
		m[i] = 0.0;
	}
    m[0] = m[5] = m[10] = m[15] = 1.0;
    m[0] = c;
    m[2] = -s;
    m[8] = s;
    m[10] = c;
}

void makeOrthMatrix(GLfloat left, GLfloat right,
		            GLfloat bottom, GLfloat top,
		            GLfloat znear, GLfloat zfar,
		            GLfloat *m) {
	int i;
	for (i = 0; i < 16; i++) {
		m[i] = 0.0;
	}

	m[0] = 2.0/(right-left);
	m[5] = 2.0/(top-bottom);
	m[10] = -2.0/(zfar-znear);
	m[15] = 1.0;
	m[12] = (right+left)/(right-left);
	m[13] = (top+bottom)/(top-bottom);
	m[14] = (zfar+znear)/(zfar-znear);
}

static int drawScene() {
	static int rotation = 0;
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glBindTexture(GL_TEXTURE_2D, g_CubeTexture);

	switch(g_PixelFormat) {
	case YV16: {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, g_CubeTexture);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, g_VideoWidth, g_VideoHeight, GL_LUMINANCE, GL_UNSIGNED_BYTE, mipi->lastVideoBuffer);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, g_UTexture);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, g_VideoWidth/2, g_VideoHeight, GL_LUMINANCE, GL_UNSIGNED_BYTE, mipi->lastVideoBuffer+(g_VideoWidth*g_VideoHeight));

		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, g_VTexture);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, g_VideoWidth/2, g_VideoHeight, GL_LUMINANCE, GL_UNSIGNED_BYTE, (mipi->lastVideoBuffer+(g_VideoWidth*g_VideoHeight) + ((g_VideoWidth/2)*g_VideoHeight)));

		glActiveTexture(GL_TEXTURE0);
		break;
	}
	case NV12:{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, g_CubeTexture);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, g_VideoWidth, g_VideoHeight, GL_LUMINANCE, GL_UNSIGNED_BYTE, mipi->lastVideoBuffer);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, g_UVTexture);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, g_VideoWidth/2, g_VideoHeight/2,GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, (mipi->lastVideoBuffer+(g_VideoWidth*g_VideoHeight)));

		glActiveTexture(GL_TEXTURE0);
		break;
	}
	case RGBP:
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, g_VideoWidth, g_VideoHeight, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, mipi->lastVideoBuffer);
		break;
	default:
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, g_VideoWidth, g_VideoHeight, GL_RGBA, GL_UNSIGNED_BYTE, mipi->lastVideoBuffer);
		break;
	}

	GLfloat mat[16], rot[16], scale[16], final[16];
	makeIdentity(rot);
	makeIdentity(mat);
	makeIdentity(scale);
	glBindTexture(GL_TEXTURE_2D, g_CubeTexture);
	glUseProgram(shaderProgram);
	if (g_Rotation) {
		rotation += 2;
		makeYRotMatrix(rotation, mat);
		matrixMult(final, g_Draw1ViewPort, mat);
		glUniformMatrix4fv(g_u_matrix_p3, 1, GL_FALSE, final);
	} else {
		glUniformMatrix4fv(g_u_matrix_p3, 1, GL_FALSE, g_Draw1ViewPort);
	}

	glVertexAttribPointer(g_attr_pos, 3, GL_FLOAT, GL_FALSE, 0, hmi_vtx);
	glVertexAttribPointer(g_attr_tex, 2, GL_FLOAT, GL_FALSE, 0, hmi_tex);
	glEnableVertexAttribArray(g_attr_pos);
	glEnableVertexAttribArray(g_attr_tex);
	glDrawElements(GL_TRIANGLES, 2*3, GL_UNSIGNED_BYTE, hmi_ind);
	glDisableVertexAttribArray(g_attr_pos);
	glDisableVertexAttribArray(g_attr_tex);
	glBindTexture(GL_TEXTURE_2D, 0);

	return 0;
}

#ifdef WAYLAND
// declare the callbacks for Wayland
void redraw(void *, struct wl_callback *, uint32_t);
void configureCallback(void *, struct wl_callback *, uint32_t);

// link the callbacks to the listeners
const struct wl_callback_listener frameListener = {
	redraw
};

const struct wl_callback_listener configureCallbackListener = {
	configureCallback
};

// implement the callbacks
void redraw(void *_data, struct wl_callback *_callback, uint32_t _time) {
	ContextData *ctx = (ContextData*) _data;
	drawScene();
	if (_callback) {
		wl_callback_destroy(_callback);
	}
	ctx->callback = wl_surface_frame(ctx->surface);
	wl_callback_add_listener(ctx->callback, &frameListener, ctx);
	eglSwapBuffers(eglDisplay, eglSurface0);
}

void configureCallback(void *_data, struct wl_callback *_callback, uint32_t _time) {
	struct ContextData *d = (struct ContextData*) _data;
	wl_callback_destroy(_callback);
	d->configured = 1;
	if (d->callback == NULL) {
		redraw(d, NULL, _time);
	}
}

static int waylandRun() {
	static bool isInit = false;
	if (isInit) {
		wl_display_dispatch(contextData.display);
		// printf("wl_display_dispatch()\n");
	} else {
		isInit = true;
		struct wl_callback *callback;
		callback = wl_display_sync(contextData.display);
		wl_callback_add_listener(callback, &configureCallbackListener, &contextData);
		// printf("wl_callback_add_listener()\n");
	}
	return 1;
}

/**
 * Wayland/EGL/GLES stuffs end
 */
#endif

int getAppLogFileName(char **_logFileName, bool _isFPS) {
	int count = 0;
	char *fileName, *baseName, *ext;

	if (!_isFPS) {
		baseName = "isp-mipi-test";
		ext = "log";
	} else {
		baseName = "frames";
		ext = "fps";
	}

	while (true) {
		fileName = (char *) calloc(80, sizeof(char));
		snprintf(fileName, 80, "%s.%d.%s", baseName, count, ext);

		// if the file already exists?
		FILE *fd = fopen(fileName, "r");
		if (fd <= 0) {
			// not exist; just return
			break;
		}

		// already exist
		fclose(fd);

		// increase counter and try again
		count++;

		free(fileName);
	}

	*_logFileName = (char *) calloc(80, sizeof(char));
	strncpy(*_logFileName, fileName, 80);

	free(fileName);

	return 80;
}

void finishApp(int _signal) {
	gIsForever = false;
}

int main(int argc, char *argv[]) {
	// 0. prep all the app for test
    signal(SIGABRT, &finishApp);
    signal(SIGTERM, &finishApp);
    signal(SIGINT, &finishApp);

    char *logFile;
    getAppLogFileName(&logFile, false);
    FILE *hAppLog = fopen(logFile, "w");
    writeToLog(hAppLog, "---hey---");

    // print app version
	writeToLog(hAppLog, "%s.%s\n", APP_NAME, APP_BUILD);

	// print start time
	time_t now;
	struct tm *timeInfo;
	char *strNow = (char *) calloc(20, sizeof(char));

	time(&now);
	timeInfo = localtime(&now);
	strftime(strNow, 20, "%Y-%m-%d %H:%M:%S", timeInfo);
	writeToLog(hAppLog, "start_time: %s\n", strNow);
	free(strNow);

	AppConfig_t *config, *vfConfig;

	config = (AppConfig_t *) calloc(1, sizeof(AppConfig_t));
	initConfigWithDefaults(config);

	if (parseArguments(argc, argv, config) <= 0) {
#ifdef COLOR_CONVERSION
		const char *help = "\n  -d <device> \
				            \n  -b <number_of_buffers> \
				            \n  -g (use DMA buffer sharing) \
				            \n  -c <out color_format> \
				            \n  -C <in color_format> \
				            \n  -w <width> \
							\n  -h <height> \
							\n  -p <mipi-port> \
							\n  -m <mipi-main> \
							\n  -v <mipi-viewfinder> \
							\n  -n <max_frame_count> \
							\n  -i (to enable interlace mode) \
							\n  -q (Turn off logging) \
							\n  -2 (Activate viewfinder stream on) \
				            \n  -f (Do not render frames)";
#else
		const char *help = "\n  -d <device> \
				            \n  -b <number_of_buffers> \
				            \n  -g (use DMA buffer sharing) \
				            \n  -c <out color_format> \
				            \n  -w <width> \
							\n  -h <height> \
							\n  -p <mipi-port> \
							\n  -m <mipi-main> \
							\n  -v <mipi-viewfinder> \
							\n  -n <max_frame_count> \
							\n  -i (to enable interlace mode) \
							\n  -q (Turn off logging) \
							\n  -2 (Activate viewfinder stream on) \
				            \n  -f (Do not render frames)";
#endif
		fprintf(stdout, "%s %s\n\n", config->appCommand->str, help);
		fflush(stdout);

		// just log that the app stops because of invalid parameters
		logConfig(hAppLog, config);
		writeToLog(hAppLog, "\nInvalid parameters or no parameters given.\n");

		goto CRAP_5;
		return 0;
	}

	// log the config for debugging purposes
	logConfig(hAppLog, config);

	// store width and height for used by eglCreateSurfaceWindow
	g_VideoWidth = config->width;
	g_VideoHeight = config->height;
	g_PixelFormat = config->pixelFormat;

	// validate config
	if (!validateConfig(config)) {
		goto CRAP_5;
		return 0;
	}

	// is using viewfinder?
	if (gIsUseViewfinder) {
		vfConfig = (AppConfig_t *) calloc(1, sizeof(AppConfig_t));
		initConfigWithDefaults(vfConfig);

		vfConfig->device->set(vfConfig->device, "%s", config->mipiViewFinder->str);
		vfConfig->width = VF_WIDTH;
		vfConfig->height = VF_HEIGHT;
		vfConfig->pixelFormat = NV12;

		// log the config for debugging purposes
		writeToLog(hAppLog, "=== viewfinder active ===");
		logConfig(hAppLog, vfConfig);
		writeToLog(hAppLog, "=== viewfinder active ===");
	}

	// 1. init main video
	mipi = Video_newWith(config->device->str,
				         config->width, config->height,
				         config->pixelFormat,
#ifdef COLOR_CONVERSION
				         config->inpixelFormat,
#endif
				         config->mipiPort,
				         config->isInterlaced);

	if (gIsUseViewfinder) {
		mipi_vf = Video_newWith(vfConfig->device->str,
						        vfConfig->width, vfConfig->height,
							    vfConfig->pixelFormat,
#ifdef COLOR_CONVERSION
					            vfConfig->inpixelFormat,
#endif
					            vfConfig->mipiPort,
							    vfConfig->isInterlaced);

		mipi->setHasViewFinder(mipi, true);
		mipi_vf->setIsFromViewFinder(mipi_vf, true);
		mipi_vf->setHasViewFinder(mipi_vf, true);
	}

	//Set IO Method based on the command line argument
	if (config->isUseDMABuf) {
		mipi->setIOMethodTo(mipi, IO_METHOD_DMABUF);

		if (gIsUseViewfinder) {
			mipi_vf->setIOMethodTo(mipi_vf, IO_METHOD_DMABUF);
		}
	}

	if (config->requestedBufferCount > 0) {
		mipi->setBufferCountTo(mipi, config->requestedBufferCount);
	}

	mipi->setLoggerWith(mipi, hAppLog);
	if (gIsUseViewfinder) {
		mipi_vf->setLoggerWith(mipi_vf, hAppLog);
	}


	int ret = mipi->initDevice(mipi);

	if (ret != 1) {
		writeToErr(hAppLog, "%s", mipi->error);
		Video_dispose(mipi);

		if (gIsUseViewfinder) {
			Video_dispose(mipi_vf);
		}

		goto CRAP_5;
		return 0;
	}

	if (gIsUseViewfinder) {
		ret = mipi_vf->initDevice(mipi_vf);

		if (ret != 1) {
			writeToErr(hAppLog, "%s", mipi_vf->error);
			Video_dispose(mipi_vf);
			Video_dispose(mipi);
			goto CRAP_5;
			return 0;
		}
	}

	if (!config->isNoRender) {
#ifdef WAYLAND
		// 2. init wayland
		writeToLog(hAppLog, "Initializing Wayland...");
		memset(&contextData, 0, sizeof(ContextData));
		contextData.display = wl_display_connect(NULL);
		contextData.registry = wl_display_get_registry(contextData.display);
		wl_registry_add_listener(contextData.registry, &registryListener, &contextData);
		wl_display_get_fd(contextData.display);
		wl_display_dispatch(contextData.display);
		writeToLog(hAppLog, "Initializing Wayland... done");
#else
		// 2. init X
		writeToLog(hAppLog, "Initializing X...");
		x_display = XOpenDisplay(NULL);
		if (x_display == NULL) {
			writeToErr(hAppLog, "Cannot connect to X server.\n");
			Video_dispose(mipi);
			if (gIsUseViewfinder) {
				Video_dispose(mipi_vf);
			}
			goto CRAP_5;
			return 0;
		}

		Window root = DefaultRootWindow(x_display);

		XSetWindowAttributes x_setWindowAttribs;
		x_setWindowAttribs.event_mask = ExposureMask | PointerMotionMask | KeyPressMask;

		win = XCreateWindow(x_display, root, 0, 0, config->width, config->height, 0,
							CopyFromParent, InputOutput, CopyFromParent, CWEventMask,
							&x_setWindowAttribs);

		XSetWindowAttributes x_attr;

		x_attr.override_redirect = false;
		XChangeWindowAttributes(x_display, win, CWOverrideRedirect, &x_attr);

		XMapWindow(x_display, win);
		XStoreName(x_display, win, "Atom ISP Test App");

		writeToLog(hAppLog, "Initializing X... done");
#endif

		// 3. init EGL and shaders
		writeToLog(hAppLog, "Starting EGL...");
		// start the EGL
		EGLint numEglConfigs;
		EGLConfig *matchingEglConfigs;
		EGLConfig eglConfig;

#ifdef WAYLAND
		eglDisplay = eglGetDisplay((NativeDisplayType) contextData.display);
#else
		eglDisplay = eglGetDisplay((EGLNativeDisplayType) x_display);
#endif
		eglInitialize(eglDisplay, &eglDispMajor, &eglDispMinor);

		// if RGB565 is used, will need to look for EGL profile
		if (config->pixelFormat == RGBP) {
			eglChooseConfig(eglDisplay, eglConfigAttribRGB565, NULL, 0, &numEglConfigs);
			matchingEglConfigs = (EGLConfig *) calloc(numEglConfigs, sizeof(EGLConfig));
			eglChooseConfig(eglDisplay, eglConfigAttribRGB565, matchingEglConfigs, numEglConfigs, &numEglConfigs);
			eglConfig = NULL;
			int i;
			for (i=0; i < numEglConfigs; i++) {
				EGLBoolean success;
				EGLint red, green, blue;

				success = eglGetConfigAttrib(eglDisplay, matchingEglConfigs[i], EGL_RED_SIZE, &red);
				success &= eglGetConfigAttrib(eglDisplay, matchingEglConfigs[i], EGL_GREEN_SIZE, &green);
				success &= eglGetConfigAttrib(eglDisplay, matchingEglConfigs[i], EGL_BLUE_SIZE, &blue);

				if (success == EGL_TRUE && red == 5 && green == 6 && blue == 5) {
					eglConfig = matchingEglConfigs[i];
					break;
				}
			}

			if (NULL == eglConfig) {
				eglConfig = matchingEglConfigs[0];

				// tell
				writeToErr(hAppLog, "EGL: No RGB565 EGLConfig found. Fell back to RGB888.");
			}

			free(matchingEglConfigs);
		} else {
			eglChooseConfig(eglDisplay, eglConfigAttribRGB888, &eglConfig, 1, &numEglConfigs);
		}

		eglBindAPI(EGL_OPENGL_ES_API);
		eglContext0 = eglCreateContext(eglDisplay, eglConfig, 0, eglContextAttrib);

#ifdef WAYLAND
		// create surface and shell surface first; get them from compositor
		contextData.surface = wl_compositor_create_surface(contextData.compositor);
		contextData.shell_surface = wl_shell_get_shell_surface(contextData.shell, contextData.surface);
		wl_shell_surface_add_listener(contextData.shell_surface, &shellSurfaceListener, &contextData);

		// contextData.native must be pre-populated before passing to eglCreateWindowSurface()
		contextData.native = wl_egl_window_create(contextData.surface, config->width, config->height);
		eglSurface0 = eglCreateWindowSurface(eglDisplay, eglConfig, (EGLNativeWindowType) contextData.native, NULL);
		if (eglSurface0 == EGL_NO_SURFACE) {
			fprintf(stdout, "eglCreateWindowSurface failed!\n\n");
			fflush(stdout);
			exit(-1) ;
		}
		wl_shell_surface_set_toplevel(contextData.shell_surface);
#else
		eglSurface0 = eglCreateWindowSurface(eglDisplay, eglConfig, (EGLNativeWindowType) win, NULL);
		if (eglSurface0 == EGL_NO_SURFACE) {
			writeToErr(hAppLog, "eglCreateWindowSurface failed!\n\n");
			exit(-1) ;
		}
#endif

		eglMakeCurrent(eglDisplay, eglSurface0, eglSurface0, eglContext0);
		eglSwapInterval(eglDisplay, 0);

		writeToLog(hAppLog, "Starting EGL... done");

		// init scene
		writeToLog(hAppLog, "Initializing scene...");
		switch (config->pixelFormat) {
			case YV16: {
				glGenTextures(1, &g_CubeTexture);
				glGenTextures(1, &g_UTexture);
				glGenTextures(1, &g_VTexture);

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, g_CubeTexture);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, config->width, config->height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				glUniform1i(g_texY, 0);

				glActiveTexture(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_2D, g_UTexture);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, config->width/2, config->height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				glUniform1i(g_texU, 1);

				glActiveTexture(GL_TEXTURE2);
				glBindTexture(GL_TEXTURE_2D, g_VTexture);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, config->width/2, config->height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				glUniform1i(g_texV, 2);

				glActiveTexture(GL_TEXTURE0);

				break;
			}
			case NV12:{
				glGenTextures(1, &g_CubeTexture);
				glGenTextures(1, &g_UVTexture);

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, g_CubeTexture);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, config->width, config->height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				glUniform1i(g_texY, 0);

				glActiveTexture(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_2D, g_UVTexture);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, config->width/2, config->height/2, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, NULL);
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				glUniform1i(g_texUV, 1);

				glActiveTexture(GL_TEXTURE0);
				break;
			}
			case RGBP:
			case RGB3: {
				glGenTextures(1, &g_CubeTexture);

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, g_CubeTexture);
				if (config->pixelFormat == RGBP) {
					glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, config->width, config->height, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, NULL);
				} else {
					glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, config->width, config->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
				}

				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				break;
			}
			default: {
				fprintf(stderr, "\n\nUnrecognized colorformat for Atom ISP.\n\n");
				fflush(stderr);
				goto CRAP_1;
				return 0;
			}
		}

		glClearColor(.5, .5, .5, .20);
		glViewport(0, 0, config->width, config->height);
		writeToLog(hAppLog, "Initializing scene... done");

		// load the shader for use by EGL
		writeToLog(hAppLog, "Initializing Shaders...");
		Shader *shader = Shader_new();
		shader->loadDefaultVertexShader(shader);
		switch (config->pixelFormat) {
		case RGB3:
		case RGBP:
			shader->loadBuiltInFragmentShader(shader, RGBP);
			break;
		default:
			shader->loadBuiltInFragmentShader(shader, config->pixelFormat);
			break;
		}

		// log the shader used
		writeToLog(hAppLog, "Vertex Shader: %s", shader->vertexShaderFile->str);
		writeToLog(hAppLog, "Fragment Shader: %s", shader->fragmentShaderFile->str);

		// create shaders
		// compile
		char shaderLog[SIZE_OF_SHADER_LOG];
		int shaderStat;
		GLsizei shaderLen;
		GLuint vertShader, fragShader;

		// frag shader
		fragShader = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fragShader, 1, (const char **) &shader->fragmentShader->str, NULL);
		glCompileShader(fragShader);
		glGetShaderiv(fragShader, GL_COMPILE_STATUS, &shaderStat);

		if (!shaderStat) {
			glGetShaderInfoLog(fragShader, SIZE_OF_SHADER_LOG, &shaderLen, shaderLog);
			writeToErr(hAppLog, "Error Compiling Fragment Shader %s: %s", shader->fragmentShaderFile->str, shaderLog);
			goto CRAP_1;
			return 0;
		}

		// vert shader
		vertShader = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertShader, 1, (const char **) &shader->vertexShader->str, NULL);
		glCompileShader(vertShader);
		glGetShaderiv(vertShader, GL_COMPILE_STATUS, &shaderStat);

		if (!shaderStat) {
			glGetShaderInfoLog(fragShader, SIZE_OF_SHADER_LOG, &shaderLen, shaderLog);
			writeToErr(hAppLog, "Error Compiling Vertex Shader %s: %s", shader->vertexShaderFile->str, shaderLog);
			goto CRAP_1;
			return 0;
		}

		shaderProgram = glCreateProgram();
		glAttachShader(shaderProgram, fragShader);
		glAttachShader(shaderProgram, vertShader);
		glBindAttribLocation(shaderProgram, 0, "pos");
		glBindAttribLocation(shaderProgram, 1, "color");
		glBindAttribLocation(shaderProgram, 2, "itexcoord");
		glLinkProgram(shaderProgram);
		glGetProgramiv(shaderProgram, GL_LINK_STATUS, &shaderStat);

		if (!shaderStat) {
			glGetProgramInfoLog(shaderProgram, SIZE_OF_SHADER_LOG, &shaderLen, shaderLog);
			writeToErr(hAppLog, "Error Linking Shader: %s", shaderLog);
			goto CRAP_1;
			return 0;
		}

		// dispose shader object after use
		Shader_dispose(shader);
		writeToLog(hAppLog, "Initializing Shaders... done");

		writeToLog(hAppLog, "Mapping Shaders variables...");
		glUseProgram(shaderProgram);
		g_u_matrix_p3 = glGetUniformLocation(shaderProgram, "modelviewProjection");
		g_u_matrix_sz = glGetUniformLocation(shaderProgram, "u_texsize");

		if (config->pixelFormat == YV16) {
			g_texY = glGetUniformLocation(shaderProgram, "u_textureY");
			g_texU = glGetUniformLocation(shaderProgram, "u_textureU");
			g_texV = glGetUniformLocation(shaderProgram, "u_textureV");
			glUniform1i(g_texY, 0);
			glUniform1i(g_texU, 1);
			glUniform1i(g_texV, 2);
		}
		if(config->pixelFormat == NV12){
			g_texY = glGetUniformLocation(shaderProgram, "u_textureY");
			g_texUV = glGetUniformLocation(shaderProgram, "u_textureUV");
			glUniform1i(g_texY, 0);
			glUniform1i(g_texUV, 1);
		}
		writeToLog(hAppLog, "Mapping Shaders variables... done");

		writeToLog(hAppLog, "Aligning viewport...");
		glUniform2f(g_u_matrix_sz, (float) config->width, (float) config->height);

		glEnable(GL_DEPTH_TEST);
		glDisable(GL_BLEND);
		DO_ORTH_MATRIX(1.0f, g_Draw1ViewPort);
		writeToLog(hAppLog, "Aligning viewport... done");
	} // isNoRender

	// 4. start streaming

	// prepare frames logging
	char *perfFile;
	getAppLogFileName(&perfFile, true);
	FILE *perfLog = fopen(perfFile, "w");
	if (perfLog) {
		// write header
		fprintf(perfLog, "frame,capture_time (usec),render_time (usec),total_time (usec),fps\n");
		fflush(perfLog);
	}

	// prepare clocking variables
	struct timeval captureClockIn, captureClockOut;
	struct timeval renderClockIn, renderClockOut;
	time_t frameIn, frameOut;
	long captureElapsed, renderElapsed;
	long long totalElapsed;
	double framerate = 0.000;

	long long i = 0, lastFrameCount = 0;
	writeToLog(hAppLog, "Going into main loop...");
	time(&frameIn);

UNSAFE_0:
	if (mipi->startStream(mipi) <= 0) {
		writeToErr(hAppLog, "%s", mipi->error);
		fprintf(stdout, "\n\n%s\n\n", mipi->error);
		fflush(stdout);

		Str *temp = Str_newWith(mipi->error);
		if (temp->has(temp, "VIDIOC_STREAMON")) {
			Str_dispose(temp);
			config->unsafeRepeatCount = 0;
			goto CRAP_0;
		} else {
			Str_dispose(temp);
			Video_dispose(mipi);
			if (gIsUseViewfinder) {
				Video_dispose(mipi_vf);
			}
			goto CRAP_1;

		}
		return 0;
	}
	writeToLog(hAppLog, "MIPI Started Streaming.");

	if (gIsUseViewfinder) {
		if (mipi_vf->startStream(mipi_vf) <= 0) {
			writeToErr(hAppLog, "%s", mipi_vf->error);
			fprintf(stdout, "\n\n%s\n\n", mipi_vf->error);
			fflush(stdout);

			Str *temp = Str_newWith(mipi_vf->error);
			if (temp->has(temp, "VIDIOC_STREAMON")) {
				Str_dispose(temp);
				config->unsafeRepeatCount = 0;
				goto CRAP_0;
			} else {
				Str_dispose(temp);
				Video_dispose(mipi_vf);
				goto CRAP_1;

			}
			return 0;
		}
		writeToLog(hAppLog, "=== viewfinder active ===");
		writeToLog(hAppLog, "MIPI Viewfinder Started Streaming.");
		writeToLog(hAppLog, "=== viewfinder active ===");
	}

	// make sure viewfinder did dequeue
	long long vf_frame = 0;

	while(gIsForever) {
		++i;

		// capture clocking - fence-start
		gettimeofday(&captureClockIn, NULL);
		mipi->autoDequeue(mipi);
		gettimeofday(&captureClockOut, NULL);
		// capture clocking - fence-stop

		// dequeue viewfinder too
		if (gIsUseViewfinder) {
			mipi_vf->autoDequeue(mipi_vf);
			vf_frame++;
		}

		if (!config->isNoRender) {
			// TODO: need a better way to render viewfinder in a separate window.
			//       Wayland is blocking this.
			// render clocking - fence-start
			gettimeofday(&renderClockIn, NULL);
#ifdef WAYLAND
			waylandRun();
#else
			drawScene();
			eglSwapBuffers(eglDisplay, eglSurface0);
#endif
			gettimeofday(&renderClockOut, NULL);
			// render clocking - fence-stop
		} // isNoRender

		// do performance calculations
		if (config->maxFrameCount > 0) {
			// not infinity
			if (config->maxFrameCount <= i) {
				fprintf(stdout, "\n");
				fflush(stdout);
				finishApp(0);
			} else {
				fprintf(stdout, "\r");
				fflush(stdout);
			}
		}

		time(&frameOut);

		captureElapsed = ((captureClockOut.tv_sec - captureClockIn.tv_sec)*1000000L) + (captureClockOut.tv_usec - captureClockIn.tv_usec);
		renderElapsed = 0;
		if (!config->isNoRender) {
			renderElapsed = ((renderClockOut.tv_sec - renderClockIn.tv_sec)*1000000L) + (renderClockOut.tv_usec - renderClockIn.tv_usec);
		} // isNoRender
		totalElapsed = captureElapsed + renderElapsed;

		double timeDiff = difftime(frameOut, frameIn);
		if (timeDiff >= 1) {
			framerate = (double) (i - lastFrameCount) / timeDiff;
			lastFrameCount = i;
			frameIn = frameOut;
		}

		if (perfLog) {
    		// log frame data to file
    		fprintf(perfLog, "%lld,%ld,%ld,%lld,%3.3f\n",
    				          i, captureElapsed, renderElapsed, totalElapsed, framerate);
    		fflush(perfLog);
		}

        if (!config->isQuiet) {
        	// fps on screen
    		fprintf(stdout, "frm: %lld; fps: %3.3f", i, framerate);
    		fflush(stdout);

    		// infinity
    		if (config->maxFrameCount <= 0) {
				fprintf(stdout, "\r");
				fflush(stdout);
    		}
		}
	}
	writeToLog(hAppLog, "\nGone out of main loop...");

	// close the frame log
	if (config->unsafeRepeatCount <= 0) {
		fclose(perfLog);
	}

CRAP_0:
	// 5. stop streaming
	if (gIsUseViewfinder) {
		mipi_vf->stopStream(mipi_vf);
		writeToLog(hAppLog, "=== viewfinder active ===");
		writeToLog(hAppLog, "MIPI Viewfinder Stopped Stream.");
		writeToLog(hAppLog, "=== viewfinder active ===");
	}

	mipi->stopStream(mipi);
	writeToLog(hAppLog, "MIPI Stopped Stream.");

	if (config->unsafeRepeatCount <= 0) {
		if (gIsUseViewfinder) {
			Video_dispose(mipi_vf);
			writeToLog(hAppLog, "=== viewfinder active ===");
			writeToLog(hAppLog, "Freed viewfinder video self.");
			writeToLog(hAppLog, "MIPI viewfinder object disposed");
			writeToLog(hAppLog, "MIPI viewfinder streamed %lld frames.", vf_frame);
			writeToLog(hAppLog, "=== viewfinder active ===");
		}
		Video_dispose(mipi);
		writeToLog(hAppLog, "Freed video self.");
		writeToLog(hAppLog, "MIPI object disposed");
	} else {
		// test of unsafe use of ISP driver
		config->unsafeRepeatCount--;
		if (mipi->initDevice(mipi) != 1) {
			writeToErr(hAppLog, "%s", mipi->error);
			Video_dispose(mipi);
			if (gIsUseViewfinder) {
				Video_dispose(mipi_vf);
			}
			goto CRAP_1;
		}

		if (mipi_vf->initDevice(mipi_vf) != 1) {
			writeToErr(hAppLog, "%s", mipi_vf->error);
			Video_dispose(mipi_vf);
			Video_dispose(mipi);
			goto CRAP_1;
		}

		gIsForever = true;
		i = 0;
		goto UNSAFE_0;
	}

CRAP_1:
	if (!config->isNoRender) {
#ifdef WAYLAND
		// 6. close window
		// destroy surface
		wl_egl_window_destroy(contextData.native);
		wl_shell_surface_destroy(contextData.shell_surface);
		wl_surface_destroy(contextData.surface);
		if (contextData.callback) {
			wl_callback_destroy(contextData.callback);
		}
		writeToLog(hAppLog, "Destroyed surface.");
#endif

//CRAP_3:
		// stop EGL
		eglMakeCurrent(eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
		eglDestroySurface(eglDisplay, eglSurface0);
		eglDestroyContext(eglDisplay, eglContext0);
		eglTerminate(eglDisplay);
		eglReleaseThread();

		writeToLog(hAppLog, "Stopped EGL.");

#ifdef WAYLAND
//CRAP_4:
		// destroy wayland
		if (contextData.shell) {
			wl_shell_destroy(contextData.shell);
		}
		writeToLog(hAppLog, "Destroyed shell.");

		if (contextData.compositor) {
			wl_compositor_destroy(contextData.compositor);
		}
		writeToLog(hAppLog, "Destroyed compositor.");

		wl_display_flush(contextData.display);
		wl_display_disconnect(contextData.display);
		writeToLog(hAppLog, "Flushed display and disconnect.");
#else
		// destroy window and close display
		XDestroyWindow(x_display, win);
		XCloseDisplay(x_display);
		writeToLog(hAppLog, "Destroyed window and closed display.");
#endif
	} // isNoRender

CRAP_5:
	// dispose and free all the buffers, structs, callocs and etc
	// print end time
	strNow = (char *) calloc(20, sizeof(char));
	time(&now);
	timeInfo = localtime(&now);
	strftime(strNow, 20, "%Y-%m-%d %H:%M:%S", timeInfo);
	writeToLog(hAppLog, "stop_time: %s\n", strNow);
	free(strNow);

	writeToLog(hAppLog, "---bye---");
	fclose(hAppLog);

	free(config);
	return 0;
}
