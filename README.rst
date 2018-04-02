nilsimsa - locality-sensitive hash
==================================

This is a Go implementation of `the nilsimsa algorithm`_,
a locality-sensitive hash for spam detection.  It's probably only
of interest to software archaeologists.

Usage
~~~~~
The Go library implements the hash.Hash interface.  Note that using
a locality-sensitive hash in places that expect an ordinary hash function
may yield funky results.

Distribution
~~~~~~~~~~~~
A package ``cmeclax`` that wraps the original C library in Go is
included in this repository.  The Go wrapper, like the original C
library, is licensed under the GNU General Public License.  The
pure-Go implementation uses the Apache v2 License.

.. _`the nilsimsa algorithm`: https://en.wikipedia.org/wiki/Nilsimsa_Hash
