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

#include "shader.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *vertexShadersPath = "./shaders/vertex/";
static const char *fragmentShadersPath = "./shaders/fragment";

static Str *getShaderFile(Shader *self, ShaderType_t _shaderType, const char *_shader) {
	Str *fullShaderPath = Str_new();

	switch (_shaderType) {
	case VERTEX:
		fullShaderPath->set(fullShaderPath, "%s%s.c", self->vertexShaderPath, _shader);
		break;
	case FRAGMENT:
#ifdef WAYLAND
		fullShaderPath->set(fullShaderPath, "%s/weston/%s.c", self->fragmentShaderPath, _shader);
#else
		fullShaderPath->set(fullShaderPath, "%s/x/%s.c", self->fragmentShaderPath, _shader);
#endif
		break;
	}

	return fullShaderPath;
}

static int loadVertexShader(Shader *self, const char *_shader) {
	if (_shader == NULL || strlen(_shader) <= 0) {
		sprintf(self->error, "Invalid Vertex Shader name.");
		return 0;
	}

	Str *shaderFullPath = getShaderFile(self, VERTEX, _shader);
	if (0 == shaderFullPath->length) {
		sprintf(self->error, "Invalid Vertex Shader file path of empty.");
		Str_dispose(shaderFullPath);
		return 0;
	}

	self->vertexShaderFile->set(self->vertexShaderFile, "%s", shaderFullPath->str);

	FILE *fp = fopen(shaderFullPath->str, "r");
	if (NULL == fp) {
		sprintf(self->error, "Cannot open file: %s", shaderFullPath->str);
		Str_dispose(shaderFullPath);
		return 0;
	}

	// get the size of file
	fseek(fp, 0, SEEK_END);
	int size = ftell(fp);
	rewind(fp);

	if (0 >= size) {
		sprintf(self->error, "File %s is zero in size", shaderFullPath->str);
		fclose(fp);
		Str_dispose(shaderFullPath);
		return 0;
	}

	char *temp = (char *) calloc(size+1, sizeof(char));

	// read
	size_t result = fread(temp, sizeof(char), size, fp);
	if (0 >= result) {
		sprintf(self->error, "Cannot read %s", shaderFullPath->str);
		free(temp);
		fclose(fp);
		Str_dispose(shaderFullPath);
		return 0;
	}

	// copy
	self->vertexShader->set(self->vertexShader, "%s", temp);

	free(temp);
	fclose(fp);
	Str_dispose(shaderFullPath);

	return 1; // all good
}

static int loadFragmentShader(Shader *self, const char *_shader) {
	if (_shader == NULL || strlen(_shader) <= 0) {
		sprintf(self->error, "Invalid Fragment Shader name.");
		return 0;
	}

	Str *shaderFullPath = getShaderFile(self, FRAGMENT, _shader);
	if (0 == shaderFullPath->length) {
		sprintf(self->error, "Invalid Fragment Shader file path of empty.");
		Str_dispose(shaderFullPath);
		return 0;
	}

	self->fragmentShaderFile->set(self->fragmentShaderFile, "%s", shaderFullPath->str);

	FILE *fp = fopen(shaderFullPath->str, "r");
	if (NULL == fp) {
		sprintf(self->error, "Cannot open file: %s", shaderFullPath->str);
		Str_dispose(shaderFullPath);
		return 0;
	}

	// get the size of file
	fseek(fp, 0, SEEK_END);
	int size = ftell(fp);
	rewind(fp);

	if (0 >= size) {
		sprintf(self->error, "File %s is zero in size", shaderFullPath->str);
		fclose(fp);
		Str_dispose(shaderFullPath);
		return 0;
	}

	char *temp = (char *) calloc(size+1, sizeof(char));

	// read
	size_t result = fread(temp, sizeof(char), size, fp);
	if (0 >= result) {
		sprintf(self->error, "Cannot read %s", shaderFullPath->str);
		free(temp);
		fclose(fp);
		Str_dispose(shaderFullPath);
		return 0;
	}

	// copy
	self->fragmentShader->set(self->fragmentShader, "%s", temp);

	free(temp);
	fclose(fp);
	Str_dispose(shaderFullPath);

	return 1; // all good
}

static int loadDefaultVertexShader(Shader *self) {
	return loadVertexShader(self, "default");
}

static int loadBuiltInFragmentShader(Shader *self, PixelFormat_t _format) {
	switch (_format) {
	case YV16:
		return loadFragmentShader(self, "yv16");
	case RGBP:
		return loadFragmentShader(self, "rgb_passthru");
	case NV12:
		return loadFragmentShader(self, "nv12");
	case UYVY:
		return loadFragmentShader(self, "uyvy");
	case VYUY:
		return loadFragmentShader(self, "vyuy");
	case YVYU:
		return loadFragmentShader(self, "yvyu");
	default:
		return loadFragmentShader(self, "yuyv");
	}
}

static int loadDefaultFragmentShader(Shader *self) {
	return loadBuiltInFragmentShader(self, YUYV);
}

static void Shader_init(Shader *self, const char *_vertexPath, const char *_fragPath) {
	if (strlen(_vertexPath) <= 0) {
		int size = strlen(vertexShadersPath)+1;
		self->vertexShaderPath = (char *) calloc(size, sizeof(char));
		strncpy(self->vertexShaderPath, vertexShadersPath, size);
	} else {
		int size = strlen(_vertexPath)+1;
		self->vertexShaderPath = (char *) calloc(size, sizeof(char));
		strncpy(self->vertexShaderPath, _vertexPath, size);
	}

	if (strlen(_fragPath) <= 0) {
		int size = strlen(fragmentShadersPath)+1;
		self->fragmentShaderPath = (char *) calloc(size, sizeof(char));
		strncpy(self->fragmentShaderPath, fragmentShadersPath, size);
	} else {
		int size = strlen(_fragPath)+1;
		self->fragmentShaderPath = (char *) calloc(size, sizeof(char));
		strncpy(self->fragmentShaderPath, _fragPath, size);
	}

	self->vertexShaderFile = Str_new();
	self->fragmentShaderFile = Str_new();

	self->vertexShader = Str_new();
	self->fragmentShader = Str_new();
	self->error = (char *) calloc(256, sizeof(char));

	// methods
	self->loadVertexShader = loadVertexShader;
	self->loadFragmentShader = loadFragmentShader;
	self->loadDefaultVertexShader = loadDefaultVertexShader;
	self->loadBuiltInFragmentShader = loadBuiltInFragmentShader;
	self->loadDefaultFragmentShader = loadDefaultFragmentShader;
}

Shader *Shader_new() {
	Shader *shader = (Shader *) calloc(1, sizeof(Shader));
	Shader_init(shader, "", "");
	return shader;
}

Shader *Shader_newWith(const char *_vertexPath, const char *_fragPath) {
	Shader *shader = (Shader *) calloc(1, sizeof(Shader));
	Shader_init(shader, _vertexPath, _fragPath);
	return shader;
}

void Shader_dispose(Shader *self) {
	Str_dispose(self->fragmentShader);
	Str_dispose(self->vertexShader);

	free(self->error);
	free(self);
}
