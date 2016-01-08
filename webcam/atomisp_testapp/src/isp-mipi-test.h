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

#ifndef ISP_MIPI_TEST_H_
#define ISP_MIPI_TEST_H_

#include "utilities.h"
#include "str_struct.h"
#include "video.h"

#include <stdio.h>
#include <stdbool.h>
#include <linux/input.h>

#ifdef WAYLAND
#include <wayland-client.h>
#include <wayland-egl.h>
#else
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#endif

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

#define HMI_W 0.95f
#define HMI_H 0.95f
#define HMI_Z 0.0f
#define DO_ORTH_MATRIX(V,M) makeOrthMatrix(-V, V, -V, V, -V, V, M);
#define DO_MATRIX(matrix,row,col)  matrix[(col<<2)+row]

typedef struct AppConfig {
	Str *appCommand;
	Str *device;
	Str *mipiMain;
	Str *mipiViewFinder;
	int mipiPort;
#ifdef COLOR_CONVERSION
	PixelFormat_t inpixelFormat;
#endif
	PixelFormat_t pixelFormat;
	int width;
	int height;
	int maxFrameCount;
	int requestedBufferCount;
	int unsafeRepeatCount;
	bool isInterlaced;
	bool isQuiet;
	bool isUseDMABuf;
	bool isNoRender;
} AppConfig_t;

#ifdef WAYLAND
typedef struct ContextData {
	struct wl_display 		*display;
	struct wl_registry 		*registry;
	struct wl_compositor 	*compositor;
	struct wl_shell 		*shell;
	struct wl_egl_window 	*native;
	struct wl_callback 		*callback;
	struct wl_surface 		*surface;
	struct wl_shell_surface *shell_surface;
	struct wl_seat 			*seat;
	struct wl_pointer 		*pointer;
	uint32_t mask;
	int configured;
} ContextData;
#endif

GLfloat hmi_vtx[] = {
	-HMI_W,  HMI_H,  HMI_Z,
	-HMI_W, -HMI_H,  HMI_Z,
	HMI_W,  HMI_H,  HMI_Z,
	HMI_W, -HMI_H,  HMI_Z,
};

GLfloat hmi_tex[] = {
	0.0f, 0.0f,
	0.0f, 1.0f,
	1.0f, 0.0f,
	1.0f, 1.0f,
};

GLubyte hmi_ind[] = {
	0, 1, 3, 0, 3, 2,
};

EGLint eglConfigAttribRGB888[] = {
	EGL_RED_SIZE, 8,
    EGL_GREEN_SIZE, 8,
    EGL_BLUE_SIZE, 8,
    EGL_DEPTH_SIZE, 8,
    EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
    EGL_NONE
};

EGLint eglConfigAttribRGB565[] = {
	EGL_RED_SIZE, 5,
	EGL_GREEN_SIZE, 6,
	EGL_BLUE_SIZE, 5,
	EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
	EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
	EGL_NONE
};

EGLint eglContextAttrib[] = {
	EGL_CONTEXT_CLIENT_VERSION, 0x2,
	EGL_NONE
};

#endif /* ISP_MIPI_TEST_H_ */
