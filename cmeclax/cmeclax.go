// This file is part of nilsimsa/cmeclax, a Go package.
//
// nilsimsa/cmeclax is free software: you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.
//
// nilsimsa/cmeclax is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with nilsimsa/cmeclax.  If not, see
// <http://www.gnu.org/licenses/>

// Package cmeclax is a simple interface to the 0.2.4 C
// implementation of nilsimsa by cmeclax.
package cmeclax

// #define VERSION "0.2.4"
// #cgo LDFLAGS: -lpopt -lm
// #include "main.h"
// #include <stdio.h>
// #include <stdlib.h>
import "C"
import "unsafe"

func init() {
	C.filltran()
}

// Klani is a nilsisma frequency table
type Klani struct {
	nsrecord C.struct_nsrecord
}

const (
	codeLength = 32
)

var (
	readPermission = &([]C.char{'r', 0})[0]
)

// Buckets returns a copy of the raw nilsimsa frequency table
// of a Klani.
func (k Klani) Buckets() (buckets []int) {
	buckets = make([]int, 256)
	for i := 0; i < 256; i++ {
		buckets[i] = int(k.nsrecord.acc[i])
	}
	return
}

// Accumulate takes a byte slice and returns a nilsimsa frequency
// table.
func Accumulate(buf []byte) Klani {
	var terkarbi Klani

	file := C.fmemopen(unsafe.Pointer(&buf[0]), C.size_t(len(buf)), readPermission)
	defer C.fclose(file)

	C.accfile(file, &(terkarbi.nsrecord), C.int(0))
	return terkarbi
}

// String returns a 64-character hexadecimal representation of a
// nilsimsa code.
func (k Klani) String() string {
	C.makecode(&k.nsrecord)
	charBuffer := &(make([]C.char, (2*codeLength)+1))[0]
	C.codetostr(&k.nsrecord, charBuffer)
	return C.GoString(charBuffer)
}
