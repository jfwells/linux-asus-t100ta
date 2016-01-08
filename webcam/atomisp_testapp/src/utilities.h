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

#ifndef UTILITIES_H_
#define UTILITIES_H_

typedef enum PIXEL_FORMAT {
	YVYU,
	YUYV,
	UYVY,
	VYUY,
	YV16,
	NV12,
	RGBP,	/* RGB565 */
	RGB3,  	/* RGB888 */
	BA10	/* SGRBG10 */
} PixelFormat_t;

int strWithFormat(char**, const char*, ...);

#endif /* UTILITIES_H_ */
