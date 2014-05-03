// Copyright 2013, Michiel Buddingh, All rights reserved.  Use of this
// code is governed by version 2.0 or later of the Apache License,
// available at http://www.apache.org/licenses/LICENSE-2.0

// Package nilsimsa implements the nilsimsa fuzzy hash by cmeclax.
//
// About Nilsimsa
//
// In summary, nilsimsa is a trigram frequency table, with a bit depth
// of 1 bit.  Table positions are zero if the frequency of a specific
// hash value is lower than average, and 1 if it is higher than
// average.
//
// Nilsimsa codes of two texts can be compared; similar texts will
// have very similar frequency distributions.
package nilsimsa

import (
	"errors"
	"fmt"
	"runtime"
	"strconv"
)

const (
	segmentMinimum = 5
	segmentSize    = 4096 // seems to be optimal om amd64 Core i3
	segmentLimit   = 536870912
)

func init() {
	filltran(&tran)
	fillpopcount(&popcount)
}

var tran [256]byte
var popcount [256]byte

// filltran initializes the 256-byte permutation
// table
func filltran(buf *[256]byte) {
	j := 0
	for i := 0; i < 256; i++ {
		j = (j*53 + 1) & 255
		j = j + j
		if j > 255 {
			j -= 255
		}
		switch i {
		case 98:
			j += 4
		case 236:
			j += 37
		case 248:
			j += 12
		case 255:
			j += 23
		default:
			j += 0
		}
		j &= 255
		buf[i] = byte(j)
	}
	return
}

// popcount initializes a population count lookup table.
func fillpopcount(buf *[256]byte) {
	for i := 0; i < len(buf); i++ {
		count := 0
		dup := i
		for dup != 0 {
			count++
			dup &= (dup - 1)
		}
		buf[i] = byte(count)
	}
}

// tran3 produces a pseudo-random byte value based on four byte
// parameters
func tran3(a, b, c, n byte) byte {
	return (((tran[((a)+(n))&255] ^ tran[(b)]*((n)+(n)+1)) + tran[(c)^tran[n]]) & 255)
}

// Code is a 256-bit nilsimsa code.  It is a type alias for [32]byte.
type Code [32]byte

var (
	// ErrScanIncorrectLength is returned by Code.Scan when an
	// attempt is made to scan a go code that contains less or
	// more than 64 hex characters.
	ErrScanIncorrectLength = errors.New("nilsimsa Code.Scan: incorrect length of hex string")
)

// Scan implements the fmt.Scanner interface
func (c *Code) Scan(state fmt.ScanState, verb rune) error {
	var accept func(rune) bool

	switch verb {
	case 'x', 'X':
		accept = func(r rune) bool {
			return ('0' <= r && r <= '9') ||
				('a' <= r && r <= 'f') ||
				('A' <= r && r <= 'F')
		}
	default:
		return errors.New("nilsimsa Code.Scan: only hexadecimal codes are supported")
	}

	byteRange, err := state.Token(false, accept)
	if err != nil {
		return err
	}

	if len(byteRange) != 64 {
		return ErrScanIncorrectLength
	}

	for i := 0; i < 32; i++ {
		bits, err := strconv.ParseUint(string(byteRange[i*2:(i+1)*2]), 16, 8)
		if err != nil {
			return err
		}

		c[(31 - i)] = uint8(bits)
	}

	return nil
}

// String returns a 64-character hexadecimal representation of the
// nilsimsa code.
func (code Code) String() string {
	var result string

	for i := 0; i < len(code); i++ {
		result += fmt.Sprintf("%02x", code[len(code)-(1+i)])
	}
	return result
}

// Distance is the nilsimsa distance, ranging from -127 (as different
// as two codes can be) to 128 (identical codes)
func (a Code) Distance(b Code) int {
	return 128 - hammingDistance([32]byte(a), [32]byte(b))
}

// hammingDistance is the Metric Hamming distance between two 32-byte slices.
func hammingDistance(a, b [32]byte) (count int) {
	for i := 0; i < 32; i++ {
		difference := a[i] ^ b[i]

		count += int(popcount[difference])
	}
	return
}

// Writer computes frequency tables for the data written to it, and returns
// a Code for this data.
type Writer struct {
	count   uint64
	buckets [256]int64
	tail    []byte
}

// Reset returns a nilsimsa.Writer to its zero state.
func (n *Writer) Reset() {
	n.count = 0
	for i := range n.buckets {
		n.buckets[i] = 0
	}
	n.tail = n.tail[0:0]
}

// Size is the length of a nilsimsa code, in bytes.  The length is 256
// bits, or 32 bytes
func (n Writer) Size() int {
	return 256 / 8
}

// BlockSize returns the optimal block size to use for writing to a writer.
func (n Writer) BlockSize() int {
	return runtime.NumCPU() * segmentSize
}

// Sum takes a byte slice, adds it to the current nilsimsa state, and
// returns a 32-byte nilsimsa hash of the total data.
func (sim Writer) Sum(b []byte) []byte {
	c := sim
	c.Write(b)
	array := [32]byte(c.Code())
	return array[:]
}

// Code returns the nilsimsa code for the data written to the Writer
// thusfar.
func (sim Writer) Code() Code {
	var threshold int64
	if sim.count < 29 {
		threshold = 0
	} else {
		threshold = int64(((sim.count * 8) - 28) / 256)
	}

	var result [32]byte
	for i := 0; i < 256; i++ {
		if sim.buckets[i] > threshold {

			result[i>>3] += 1 << uint(i&7)
		}
	}
	return Code(result)
}

type buckets struct {
	count   uint32
	buckets [256]uint32
}

func (n *Writer) aggregate(pipe <-chan *buckets, done chan<- int) {
	for buckets := range pipe {
		n.count += uint64(buckets.count)
		for i := 0; i < 256; i++ {
			n.buckets[i] += int64(buckets.buckets[i])
		}
	}
	done <- 1
}

type slice struct {
	tail []byte
	rest []byte
}

func eater(slices <-chan slice, pipe chan<- *buckets, done chan<- int) {
	for slice := range slices {
		pipe <- block(slice.tail, slice.rest)
	}
	done <- 1
}

// Write takes a slice of bytes and passes them through the nilsimsa
// frequency table algorithm.
func (sim *Writer) Write(buf []byte) (n int, err error) {
	pipe := make(chan *buckets)
	done := make(chan int)
	length := len(buf)

	slices := make(chan slice, runtime.NumCPU())

	go sim.aggregate(pipe, done)

	for c := 0; c < runtime.NumCPU(); c++ {
		go eater(slices, pipe, done)
	}

	index := 0

	if len(sim.tail) > 0 {
		slices <- slice{
			sim.tail,
			buf[index:min(index+segmentSize, length)]}
		index += segmentSize
	}

	for {
		if index > length {
			break
		}

		slices <- slice{
			buf[max(0, index-segmentMinimum):index],
			buf[index:min(index+segmentSize, length)]}

		index += segmentSize
	}

	close(slices)

	tail := append(sim.tail, buf[max(0, length-segmentMinimum):]...)
	sim.tail = tail[max(0, len(tail)-segmentMinimum):]

	for c := 0; c < runtime.NumCPU(); c++ {
		<-done
	}

	close(pipe)

	<-done

	return length, nil
}

func block(tail, rest []byte) *buckets {
	b := new(buckets)
	var lookback [5]byte

	taillength := len(tail)

	for i, bb := range tail {
		lookback[taillength-(i+1)] = bb
	}

	for i := 0; i < len(rest); i++ {

		lookback[4] = lookback[3]
		lookback[3] = lookback[2]
		lookback[2] = lookback[1]
		lookback[1] = lookback[0]

		lookback[0] = rest[i]

		if i+taillength > 1 {
			b.buckets[tran3(lookback[0], lookback[1], lookback[2], 0)]++
		}
		if i+taillength > 2 {
			b.buckets[tran3(lookback[0], lookback[1], lookback[3], 1)]++
			b.buckets[tran3(lookback[0], lookback[2], lookback[3], 2)]++
		}
		if i+taillength > 3 {
			b.buckets[tran3(lookback[0], lookback[1], lookback[4], 3)]++
			b.buckets[tran3(lookback[0], lookback[2], lookback[4], 4)]++
			b.buckets[tran3(lookback[0], lookback[3], lookback[4], 5)]++
			b.buckets[tran3(lookback[4], lookback[1], lookback[0], 6)]++
			b.buckets[tran3(lookback[4], lookback[3], lookback[0], 7)]++
		}
	}
	b.count = uint32(len(rest))
	return b
}

func min(values ...int) (min int) {
	min = 2147483647
	for _, v := range values {
		if v < min {
			min = v
		}
	}
	return
}

func max(values ...int) (max int) {
	max = -2147483648
	for _, v := range values {
		if v > max {
			max = v
		}
	}
	return
}
