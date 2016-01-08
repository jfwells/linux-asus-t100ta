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

#ifndef STR_STRUCT_H_
#define STR_STRUCT_H_

#include <stdbool.h>

typedef struct STR_S {
	char *str;
	int length;

    void (*set) (struct STR_S *, const char *, ...);
    void (*toUpper) (struct STR_S *);
    void (*toLower) (struct STR_S *);
    bool (*isEquals) (struct STR_S *, struct STR_S *);
    bool (*isEqualsIgnoreCase) (struct STR_S *, struct STR_S *);
    void (*leftTrim) (struct STR_S *);
    void (*rightTrim) (struct STR_S *);
    void (*trim) (struct STR_S *);
    struct STR_S * (*sub) (struct STR_S *, int, int);
    void (*append) (struct STR_S *, struct STR_S *);
    bool (*has) (struct STR_S *, const char *);
    int (*find) (struct STR_S *, const char *, int **);
    void (*replace) (struct STR_S *, const char *, const char *);
} Str;

Str *Str_new();
Str *Str_newWith(const char *, ...);
void Str_dispose(Str *);


#endif /* STR_STRUCT_H_ */
