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

#include "isp-mipi-way.h"

#include <stdio.h>
#include <stdlib.h>
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
#include "log.h"
#include "video.h"
#include "shader.h"

#define APP_NAME "isp-fifo-way"
#define APP_VERSION "0.0.0"

#define MAX_FRAME_COUNT 500
#define OV5640_1_MAIN "/dev/video4"
#define OV5640_1_VF "/dev/video6"
#define OV5640_2_MAIN "/dev/video8" // ???
#define OV5640_2_VF "/dev/video10" // ???

#define SIZE_OF_SHADER_LOG 1000

/**
 * Globals begin
 */

// common variables
Str *fifoFile = NULL;
ContextData contextData = {0};
Video *mipi;
bool gIsForever = true;
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
	_config->width = 640;
	_config->height = 480;
	_config->pixelFormat = YV16;
	_config->maxFrameCount = MAX_FRAME_COUNT;
}

int parseArguments(int argc, char *argv[], AppConfig_t *_config) {
	_config->appCommand->set(_config->appCommand, argv[0]);

	bool didProcessedOptions = false;

	static const char *options = "d:c:w:h:f:n:?";

	int c;
	while ((c = getopt(argc, argv, options)) != -1) {
		didProcessedOptions = true;
		switch (c) {
		case 'd':
			_config->device->set(_config->device, optarg);
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
			} else {
				fprintf(stderr, "\n\n%s : Unrecognized colorformat for Atom ISP.\n\n", colorFormat->str);
				fflush(stderr);
				return 0;
			}

			Str_dispose(colorFormat);
			break;
		}
		case 'w':
			_config->width = atoi(optarg);
			break;
		case 'h':
			_config->height = atoi(optarg);
			break;
		case 'f':
		{
			fifoFile = Str_newWith(optarg);
			fifoFile->trim(fifoFile);
			if (fifoFile->length <= 1) {
				fprintf(stderr, "\n\nInvalid file source specified.\n\n");
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

void logConfig(Log *_logger, AppConfig_t *_config) {
	if (_logger == NULL || _config == NULL) {
		return;
	}

	// turn on emit
	_logger->isEmit = true;

	// log and emit
	Str *message = Str_newWithFormat("config.device: %s", _config->device->str);
	_logger->writeLine(_logger, message);

	message->setWithFormat(message, "config.width: %d", _config->width);
	_logger->writeLine(_logger, message);

	message->setWithFormat(message, "config.height: %d", _config->height);
	_logger->writeLine(_logger, message);

	switch (_config->pixelFormat) {
	case YV16:
		message->set(message, "config.pixelFormat: YV16");
		break;
	case RGBP:
		message->set(message, "config.pixelFormat: RGB565 / RGBP");
		break;
	case RGB3:
		message->set(message, "config.pixelFormat: RGB888 / RGB3");
		break;
	default:
		message->set(message, "config.pixelFormat: invalid");
		break;
	}
	_logger->writeLine(_logger, message);

	if (NULL == fifoFile || fifoFile->length <= 1) {
		message->set(message, "fifoFile: Not Provided");
	} else {
		message->setWithFormat(message, "fifoFile: %s", fifoFile->str);
	}
	_logger->writeLine(_logger, message);

	// turn off emit
	_logger->isEmit = false;
}

void logBye(Log *_logger) {
	_logger->writeLine(_logger, Str_newWith("---bye---"));
}

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
void finishApp(int _signal) {
	gIsForever = false;
}

int main(int argc, char *argv[]) {
	// 0. prep all the app for test
    signal(SIGABRT, &finishApp);
    signal(SIGTERM, &finishApp);
    signal(SIGINT, &finishApp);

    Log *logger = Log_newWith(NULL);
    logger->writeLine(logger, Str_newWith("---hey---"));

	AppConfig_t *config;

	config = (AppConfig_t *) calloc(1, sizeof(AppConfig_t));
	initConfigWithDefaults(config);

	if (parseArguments(argc, argv, config) <= 0) {
		char *help = "\n  -d <device> \n  -c <color_format> \n  -w <width> \n  -h <height> \n  -f <raw_image_file_source> \n  -n <max_frame_count>";
		fprintf(stdout, "%s %s\n\n", config->appCommand->str, help);
		fflush(stdout);

		// just log that the app stops because of invalid parameters
		logConfig(logger, config);
		logger->writeLine(logger, Str_newWith("Invalid parameters or no parameters given."));

		goto CRAP_5;
		return 0;
	}

	// log the config for debugging purposes
	logConfig(logger, config);

	// store width and height for used by eglCreateSurfaceWindow
	g_VideoWidth = config->width;
	g_VideoHeight = config->height;
	g_PixelFormat = config->pixelFormat;

	// 1. init video
	mipi = Video_newWith(config->device->str,
				         config->width, config->height,
				         config->pixelFormat, config->mipiPort);
	mipi->setLoggerWith(mipi, logger);

	// set FIFO active
	mipi->isFIFO = true;
	mipi->rawFile = fifoFile->str;

	// init MIPI with FIFO
	int ret = mipi->initDevice(mipi);

	if (ret != 1) {
		logger->writeLineWithFormat(logger, "%s", mipi->error);
		fprintf(stdout, "\n\n%s\n\n", mipi->error);
		fflush(stdout);
		Video_dispose(mipi);
		goto CRAP_5;
		return 0;
	}

	// 2. init wayland
	logger->writeLine(logger, Str_newWith("Initializing Wayland..."));
	memset(&contextData, 0, sizeof(ContextData));
	contextData.display = wl_display_connect(NULL);
	contextData.registry = wl_display_get_registry(contextData.display);
	wl_registry_add_listener(contextData.registry, &registryListener, &contextData);
	wl_display_get_fd(contextData.display);
	wl_display_dispatch(contextData.display);
	logger->writeLine(logger, Str_newWith("Initializing Wayland... done"));

	// 3. init EGL and shaders
	logger->writeLine(logger, Str_newWith("Starting EGL..."));
	// start the EGL
	EGLint numEglConfigs;
	EGLConfig *matchingEglConfigs;
	EGLConfig eglConfig;

	eglDisplay = eglGetDisplay((NativeDisplayType) contextData.display);
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
			logger->writeLine(logger, Str_newWith("EGL: No RGB565 EGLConfig found. Fell back to RGB888."));
		}

		free(matchingEglConfigs);
	} else {
		eglChooseConfig(eglDisplay, eglConfigAttribRGB888, &eglConfig, 1, &numEglConfigs);
	}

	eglBindAPI(EGL_OPENGL_ES_API);
	eglContext0 = eglCreateContext(eglDisplay, eglConfig, 0, eglContextAttrib);

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

	eglMakeCurrent(eglDisplay, eglSurface0, eglSurface0, eglContext0);
	eglSwapInterval(eglDisplay, 0);

	logger->writeLine(logger, Str_newWith("Starting EGL... done"));

	// init scene
	logger->writeLine(logger, Str_newWith("Initializing scene..."));
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
	logger->writeLine(logger, Str_newWith("Initializing scene... done"));

	// load the shader for use by EGL
	logger->writeLine(logger, Str_newWith("Initializing Shaders..."));
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
	logger->writeLineWithFormat(logger, "Vertex Shader: %s", shader->vertexShaderFile->str);
	logger->writeLineWithFormat(logger, "Fragment Shader: %s", shader->fragmentShaderFile->str);

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
		logger->writeLine(logger, Str_newWithFormat("Error Compiling Fragment Shader %s: %s", shader->fragmentShaderFile->str, shaderLog));
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
		logger->writeLine(logger, Str_newWithFormat("Error Compiling Vertex Shader %s: %s", shader->vertexShaderFile->str, shaderLog));
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
		logger->writeLine(logger, Str_newWithFormat("Error Linking Shader: %s", shaderLog));
		goto CRAP_1;
		return 0;
	}

	// dispose shader object after use
	Shader_dispose(shader);
	logger->writeLine(logger, Str_newWith("Initializing Shaders... done"));

	logger->writeLine(logger, Str_newWith("Mapping Shaders variables..."));
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
	logger->writeLine(logger, Str_newWith("Mapping Shaders variables... done"));

	logger->writeLine(logger, Str_newWith("Aligning viewport..."));
	glUniform2f(g_u_matrix_sz, (float) config->width, (float) config->height);

	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	DO_ORTH_MATRIX(1.0f, g_Draw1ViewPort);
	logger->writeLine(logger, Str_newWith("Aligning viewport... done"));

	// 4. start streaming
	// initialize performance log
	char perfLogFile[256];
	sprintf(perfLogFile, "%s_isp-mipi_port-%d_%dx%d.fps",
						 logger->getCurrentTimestamp(), config->mipiPort,
						 config->width, config->height);
	Log *perfLogger = Log_newWith(perfLogFile);

	// prepare clocking variables
	bool isClocked = false;
	struct timeval clockIn, clockOut;
	long clockElapsed = 0;
	long long i = 0;

	logger->isEmit = false;
	logger->writeLine(logger, Str_newWith("Going into main loop..."));
	gettimeofday(&clockIn, NULL);

	if (mipi->startStream(mipi) <= 0) {
		logger->writeLineWithFormat(logger, "%s", mipi->error);
		fprintf(stdout, "\n\n%s\n\n", mipi->error);
		fflush(stdout);

		Str *temp = Str_newWith(mipi->error);
		if (temp->has(temp, "VIDIOC_STREAMON")) {
			Str_dispose(temp);
			goto CRAP_0;
		} else {
			Str_dispose(temp);
			Video_dispose(mipi);
			goto CRAP_1;
		}
		return 0;
	}
	logger->writeLine(logger, Str_newWith("MIPI Started Stream."));

	while(gIsForever) {
		++i;

		mipi->autoDequeue(mipi);
		waylandRun();
		gettimeofday(&clockOut, NULL);

		// do performance calculations
		if (config->maxFrameCount > 0) {
			// not infinity
			if (config->maxFrameCount <= i) {
				finishApp(0);
			}
		}

		if (!isClocked) {
			clockElapsed = ((clockOut.tv_sec - clockIn.tv_sec)*1000000L) + (clockOut.tv_usec - clockIn.tv_usec);

			// log time to get frame shown
			Str *msg = Str_newWithFormat("Clocked: %ld", clockElapsed);
			logger->writeLine(logger, msg);
			logger->emit(msg);
			Str_dispose(msg);
			isClocked = true;
		}
	}
	logger->writeLine(logger, Str_newWith("Gone out of main loop..."));
	Log_dispose(perfLogger);

CRAP_0:
	// 5. stop streaming
	mipi->stopStream(mipi);
	logger->writeLine(logger, Str_newWith("MIPI Stopped Stream."));

	Video_dispose(mipi);
	logger->writeLine(logger, Str_newWith("Freed video self."));
	logger->writeLine(logger, Str_newWith("MIPI object disposed"));

CRAP_1:
	// reset logger to emit
	logger->isEmit = true;

	// 6. close window
	// destroy surface
	wl_egl_window_destroy(contextData.native);
	wl_shell_surface_destroy(contextData.shell_surface);
	wl_surface_destroy(contextData.surface);
	if (contextData.callback) {
		wl_callback_destroy(contextData.callback);
	}
	logger->writeLine(logger, Str_newWith("Destroyed surface."));

//CRAP_3:
	// stop EGL
	eglMakeCurrent(eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	eglDestroySurface(eglDisplay, eglSurface0);
	eglDestroyContext(eglDisplay, eglContext0);
	eglTerminate(eglDisplay);
	eglReleaseThread();

	logger->writeLine(logger, Str_newWith("Stopped EGL."));

//CRAP_4:
	// destroy wayland
	if (contextData.shell) {
		wl_shell_destroy(contextData.shell);
	}
	logger->writeLine(logger, Str_newWith("Destroyed shell."));

	if (contextData.compositor) {
		wl_compositor_destroy(contextData.compositor);
	}
	logger->writeLine(logger, Str_newWith("Destroyed compositor."));

	wl_display_flush(contextData.display);
	wl_display_disconnect(contextData.display);
	logger->writeLine(logger, Str_newWith("Flushed display and disconnect."));

CRAP_5:
	// dispose and free all the buffers, structs, callocs and etc
	logBye(logger);
	Log_dispose(logger);
	free(config);
	return 0;
}
