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

#include "log.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#define TIMESTAMP_SIZE 18

static const char *getCurrentTimestamp() {
	char *buffer = NULL;

	time_t rawTime;
	struct tm *timeInfo;

	time(&rawTime);
	timeInfo = localtime(&rawTime);

	// format: YYYY-MM-dd_hhmmss
	//         12456789012345678
	buffer = (char *) calloc(TIMESTAMP_SIZE+1, sizeof(char));
	strftime(buffer, TIMESTAMP_SIZE+1, "%Y%m%d_%H%M%S", timeInfo);

	return buffer;
}

static void emit(Str *_message) {
	Str *message = Str_newWith("%s", _message->str);
	fprintf(stdout, "%s", message->str);
	fflush(stdout);
}

static void emitWithFormat(const char *_format, ...) {
	char buffer[256];
	va_list args;
	va_start(args, _format);
	vsprintf(buffer, _format, args);
	va_end(args);

	emit(Str_newWith(buffer));
}

static void write(Log *self, Str *_message) {
	Str *message = Str_newWith("[%s] %s", getCurrentTimestamp(), _message->str);

	if (self->isEmit) {
		emit(message);
	}
    
    if (!self->isQuiet) {
	    FILE *handle = fopen(self->file, "a");
	    fprintf(handle, "%s", message->str);
	    fclose(handle);
    }
    
	Str_dispose(message);
}

static void writeWithFormat(Log *self, const char *_format, ...) {
	char buffer[256];
	va_list args;
	va_start(args, _format);
	vsprintf(buffer, _format, args);
	va_end(args);

	write(self, Str_newWith(buffer));
}

static void writeLine(Log *self, Str *_message) {
	_message->append(_message, Str_newWith("\n"));
	write(self, _message);
}

static void writeLineWithFormat(Log *self, const char *_format, ...) {
	char buffer[256];
	va_list args;
	va_start(args, _format);
	vsprintf(buffer, _format, args);
	va_end(args);

	write(self, Str_newWith("%s\n", buffer));
}

static void Log_init(Log *self, const char *_file) {
	if (_file == NULL || strlen(_file) <= 0) {
		Str *defaultFile = Str_newWith("%s_isp-mipi.log", getCurrentTimestamp());
		self->file = (char *) calloc(defaultFile->length, sizeof(char));
		strncpy(self->file, defaultFile->str, defaultFile->length);
		Str_dispose(defaultFile);
	} else {
		self->file = (char *) calloc(strlen(_file), sizeof(char));
		strncpy(self->file, _file, sizeof(_file)*strlen(_file));
	}

	self->isEmit = false;
	self->isQuiet = false;
	self->write = write;
	self->writeWithFormat = writeWithFormat;
	self->writeLine = writeLine;
	self->writeLineWithFormat = writeLineWithFormat;
	self->emit = emit;
	self->emitWithFormat = emitWithFormat;
	self->getCurrentTimestamp = getCurrentTimestamp;
}

Log *Log_newWith(const char *_file) {
	Log *log = (Log *) calloc(1, sizeof(Log));
	Log_init(log, _file);
	return log;
}

void Log_dispose(Log *_log) {
	free(_log);
}
