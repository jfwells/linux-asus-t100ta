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

#include "str_struct.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <regex.h>

#define STR_BUFSIZE 80

static int compileRegex(regex_t *_regex, const char *_pattern) {
    if (0 != regcomp(_regex, _pattern, REG_EXTENDED|REG_NEWLINE)) {
        return -1;
    }
    return 0;
}

static int matchRegex(regex_t *_regex, Str *self, bool _isReturnMatches, regmatch_t **_matches) {
    regmatch_t pm;
    regmatch_t *matches = NULL;
    regoff_t lastMatch = 0;
    int count = 0, offset = 0;
    do {
        int noMatch = regexec(_regex, self->str+lastMatch, 1, &pm, 0);
        if (noMatch == REG_NOMATCH) {
            break;
        }

        lastMatch += pm.rm_so + 1;
        count++;

        if (matches == NULL) {
            matches = (regmatch_t *) calloc(1, sizeof(regmatch_t));
        } else {
            matches = (regmatch_t *) realloc(matches, count * sizeof(regmatch_t));
        }

        matches[count-1] = pm;
        matches[count-1].rm_so += offset;
        matches[count-1].rm_eo += offset;

        offset += pm.rm_so + 1;
    } while(1);

    if (count > 0 && _isReturnMatches) {
        if (*_matches != NULL) {
            free(*_matches);
            *_matches = (regmatch_t *) calloc(count, sizeof(regmatch_t));

            int i;
            for (i=0; i < count; i++) {
                (*_matches)[i].rm_so = matches[i].rm_so;
                (*_matches)[i].rm_eo = matches[i].rm_eo;
            }
        }

        free(matches);
        matches = NULL;
    }

    return count;
}

static void vset(Str *self, const char *_format, va_list _args) {
	// initialize local buffer with default size
	char *buffer = (char *) calloc(STR_BUFSIZE, sizeof(char));

	// backup the args
	va_list backupArgs;

	va_copy(backupArgs, _args);

	// enlarge buffer if required
	int need = vsnprintf(buffer, STR_BUFSIZE, _format, _args);
	if ((need+1) > STR_BUFSIZE) {
		// enlarge
		buffer = (char *) calloc((need+1), sizeof(char));
		need = vsnprintf(buffer, (need+1), _format, backupArgs);
	}

	// initialize self->str and self->length
	self->str = (char *) calloc((need+1), sizeof(char));
	strncpy(self->str, buffer, (need+1));

	self->length = need;

	// clean up
	free(buffer);
}

static void set(Str *self, const char *_format, ...) {
    va_list args;
    va_start(args, _format);
    vset(self, _format, args);
    va_end(args);
}

static void toUpper(Str *self) {
    if (self->str == NULL) {
        return;
    }

    int i;
    for (i=0; i < self->length; i++) {
        self->str[i] = toupper(self->str[i]);
    }
}

static void toLower(Str *self) {
    if (self->str == NULL) {
        return;
    }

    int i;
    for (i=0; i < self->length; i++) {
        self->str[i] = tolower(self->str[i]);
    }
}

static bool isEquals(Str *self, Str *_other) {
    if (self->str == NULL && _other->str == NULL) {
        return true;
    }

    if (self->length != _other->length) {
        return false;
    }

    if (strncmp(self->str, _other->str, self->length) == 0) {
        return true;
    }

    return false;
}

static bool isEqualsIgnoreCase(Str *self, Str *_other) {
    if (self->str == NULL && _other->str == NULL) {
        return true;
    }

    if (self->length != _other->length) {
        return false;
    }

    Str *tempSelf = self;
    tempSelf->toUpper(tempSelf);
    Str *tempOther = _other;
    tempOther->toUpper(tempOther);

    return isEquals(tempSelf, tempOther);
}

static void leftTrim(Str *self) {
    if (self->str == NULL) {
        return;
    }

    int i, start = 0;
    for (i=0; i < self->length; i++) {
        if (self->str[i] == ' ' || self->str[i] == '\t') {
            continue;
        }

        start = i;
        break;
    }

    if (start == 0) {
        return;
    }

    int size = self->length - start;
    char *temp = (char *) calloc((size+1), sizeof(char));
    for (i=start; i < self->length; i++) {
        temp[i-start] = self->str[i];
    }

    free(self->str);
    self->str = NULL;
    self->str = (char *) calloc((size+1), sizeof(char));
    strncpy(self->str, temp, size);
    self->length = size;
    free(temp);
}

static void rightTrim(Str *self) {
    if (self->str == NULL) {
        return;
    }

    int i, end = 0;
    for (i=self->length-1; i >= 0; i--) {
        if (self->str[i] == ' ' || self->str[i] == '\t') {
            continue;
        }

        end = i;
        break;
    }

    if (end == 0) {
        return;
    }

    int size = end + 1;
    char *temp = (char *) calloc((size+1), sizeof(char));
    for (i=0; i <= end; i++) {
        temp[i] = self->str[i];
    }
    temp[i+1] = '\0';

    free(self->str);
    self->str = NULL;
    self->str = (char *) calloc((size+1), sizeof(char));
    strncpy(self->str, temp, size);
    self->length = size;
    free(temp);
}

static void trim(Str *self) {
    leftTrim(self);
    rightTrim(self);
}

static Str *sub(Str *self, int _beginIndex, int _length) {
    if (self->length <= 1 || self->str[0] == '\0') {
        return NULL;
    }

    if (_beginIndex < 0) {
        _beginIndex = self->length + _beginIndex;
    }

    if (_length <= 0 || (_beginIndex + _length - 1) > self->length) {
        _length = self->length;
    }

    int i, size = (_length - _beginIndex + 1) + 1;
    char *temp = (char *) calloc(size, sizeof(char));
    for (i=_beginIndex; i < (_beginIndex + _length); i++) {
        if (self->str[i] == '\0') {
            break;
        }
        temp[i-_beginIndex] = self->str[i];
    }
    temp[i] = '\0';

    Str *out = Str_newWith("%s", temp);
    free(temp);
    return out;
}

static void append(Str *self, Str *_other) {
    if (_other == NULL || _other->length < 1) {
        return;
    }

    int size = self->length + _other->length + 1;
    self->str = (char *) realloc(self->str, size + sizeof(char));
    strncat(self->str, _other->str, size);
    self->length = size - 1;

}

static bool has(Str *self, const char *_pattern) {
    regex_t regex;
    bool isFound = false;
    if (compileRegex(&regex, _pattern) >= 0) {
        if (matchRegex(&regex, self, false, NULL) > 0) {
            isFound = true;
        }
    }
    regfree(&regex);
    return isFound;
}

static int find(Str *self, const char *_pattern, int **_indices) {
    regex_t regex;
    regmatch_t *matches = (regmatch_t *) calloc(1, sizeof(regmatch_t));
    int count = 0;

    if (*_indices == NULL) {
        *_indices = (int *) calloc(1, sizeof(int));
    }

    if (compileRegex(&regex, _pattern) >= 0) {
        count = matchRegex(&regex, self, true, &matches);

        if (count > 0) {
            free(*_indices);
            *_indices = (int *) calloc(count, sizeof(int));

            int i;
            for (i=0; i < count; i++) {
                (*_indices)[i] = matches[i].rm_so;
            }

            free(matches);
        }
    }
    regfree(&regex);

    return count;
}

static void replace(Str *self, const char *_pattern, const char *_replacement) {
	// TODO: Need to implement this ASAP.
}

static void Str_init(Str *self) {
    self->str = (char *) calloc(1, sizeof(char));
	self->str[0] = '\0';
	self->length = 1;

    self->set = set;
    self->toUpper = toUpper;
    self->toLower = toLower;
    self->isEquals = isEquals;
    self->isEqualsIgnoreCase = isEqualsIgnoreCase;
    self->leftTrim = leftTrim;
    self->rightTrim = rightTrim;
    self->trim = trim;
    self->sub = sub;
    self->append = append;
    self->has = has;
    self->find = find;
    self->replace = replace;
}

Str *Str_new() {
    Str *str = (Str *) calloc(1, sizeof(Str));
    Str_init(str);
    return str;
}

Str *Str_newWith(const char *_format, ...) {
	// initialize the string object
	Str *str = (Str *) calloc(1, sizeof(Str));
	Str_init(str);

	// set the object with entered format and string
	va_list args;
	va_start(args, _format);
    vset(str, _format, args);
    va_end(args);

    return str;
}

void Str_dispose(Str *_str) {
    if (_str == NULL) {
        return;
    }

    if (_str->str == NULL) {
    	return;
    }

    free(_str->str);
    _str->str = NULL;
    _str->length = 0;

    free(_str);
    _str = NULL;
}
