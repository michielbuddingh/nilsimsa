package nilsimsa

import (
	"bytes"
	"encoding/binary"
	"math/rand"
	"testing"
)

type randReader struct {
	rand *rand.Rand
	tail []byte
}

func newRandReader(seed int64) (r *randReader) {
	r = new(randReader)
	r.rand = rand.New(rand.NewSource(seed))
	return
}

func (r *randReader) Read(p []byte) (n int, err error) {
	i := 0
	if len(r.tail) > 0 {
		for i < len(p) && i < len(r.tail) {
			p[i] = r.tail[i]
			i++
		}
		r.tail = r.tail[i:]
	}
	for i+4 < len(p) {
		binary.BigEndian.PutUint32(p[i:], r.rand.Uint32())
		i += 4
	}
	if i < len(p) {
		var tail [4]byte
		binary.BigEndian.PutUint32(tail[:], r.rand.Uint32())
		j := i
		for i < len(p) {
			p[i] = tail[i-j]
			i++
		}
		r.tail = tail[i-j:]
	}
	return i, nil
}

func TestRandReader(t *testing.T) {
	expected := []byte{
		217, 70, 104, 150, 165, 37, 204, 191, 188, 251, 50, 224,
		80, 178, 100, 184, 14, 151, 174, 178, 247, 157, 93, 39,
		157, 44, 25, 189, 3, 218, 247, 85, 34, 52, 230, 106,
		131, 7, 107, 76, 10, 97, 203, 169, 90, 180, 219, 210,
		203, 59, 19, 34, 149, 138, 159, 129, 128, 182, 201, 57,
		74, 145, 48, 165, 122, 55, 213, 227, 109, 44, 221, 171,
		62, 145, 2, 131, 232, 131, 50, 163, 204, 33, 210, 9,
		27, 21, 43, 22, 67, 40, 98, 73, 142, 246, 58, 35,
		177, 141, 104, 76, 170, 14, 109, 84, 100, 145, 132, 249,
		62, 81, 144, 240, 117, 209, 44, 25, 165, 152, 31, 165,
	}

	for bufSize := 1; bufSize < 28; bufSize *= 3 {
		var results bytes.Buffer
		var source = newRandReader(12345)

		buf := make([]byte, bufSize)
		written := 0
		for written < len(expected) {
			length, _ := source.Read(buf)
			results.Write(buf[:length])
			written += length
		}

		bytes := results.Bytes()

		for i := 0; i < len(expected); i++ {
			if expected[i] != bytes[i] {
				t.Errorf("Expected %d, was %d at position %d",
					expected[i],
					bytes[i],
					i)
				break
			}
		}
	}
}
