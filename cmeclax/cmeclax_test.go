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

package cmeclax

import (
	"bytes"
	"io"
	"os"
	"testing"
)

func TestAccumulate(t *testing.T) {
	f, err := os.Open("20480.test")
	if err != nil {
		t.Fatalf("Could not open file 20480.test, %s", err)
	}
	var buf bytes.Buffer
	io.Copy(&buf, f)
	f.Close()
	acc := Accumulate(buf.Bytes())

	if acc.String() != "ff0391a13788fe959469ec70df488e5a269bf54fad4de9614e6ae30196b5110e" {
		t.Errorf("Unexpected summary hash: %s\n", acc)
	}
}
