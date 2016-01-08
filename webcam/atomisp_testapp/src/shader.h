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

#ifndef SHADER_H_
#define SHADER_H_

#include "utilities.h"
#include "str_struct.h"

#include <stdbool.h>

typedef enum SHADER_TYPE {
	VERTEX, FRAGMENT
} ShaderType_t;

typedef struct SHADER_S {
	char *error;
	char *vertexShaderPath;
	char *fragmentShaderPath;

	struct STR_S *vertexShaderFile;
	struct STR_S *fragmentShaderFile;

	struct STR_S *vertexShader;
	struct STR_S *fragmentShader;

	int (*loadVertexShader) (struct SHADER_S *, const char*);
	int (*loadDefaultVertexShader) (struct SHADER_S *);
	int (*loadFragmentShader) (struct SHADER_S *, const char*);
	int (*loadDefaultFragmentShader) (struct SHADER_S *);
	int (*loadBuiltInFragmentShader) (struct SHADER_S *, PixelFormat_t);
} Shader;

Shader *Shader_new();
Shader *Shader_newWith(const char *, const char *);
void Shader_dispose(Shader *);

#endif /* SHADER_H_ */
