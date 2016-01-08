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

#ifndef LOG_H_
#define LOG_H_

#include <stdbool.h>
#include "str_struct.h"

typedef struct LOG_S {
	char *file;
	bool isEmit;
	bool isQuiet;

	void (*write) (struct LOG_S *, Str *);
	void (*writeWithFormat) (struct LOG_S *, const char *, ...);
	void (*writeLine) (struct LOG_S *, Str *);
	void (*writeLineWithFormat) (struct LOG_S *, const char *, ...);
	void (*emit) (Str *);
	void (*emitWithFormat) (const char *, ...);
	const char *(*getCurrentTimestamp) (void);
} Log;

Log *Log_newWith(const char *);
void Log_dispose(Log *);

#endif /* LOG_H_ */
