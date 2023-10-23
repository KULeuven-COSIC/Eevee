from Compiler.types import *
from Compiler.library import print_ln

from .jolteon import JolteonBase

class JolteonGF2n(JolteonBase):
    def __init__(self, fk, blocksize, tweaksize, fk_params, fk_enc, fk_dec):
        zero = sgf2n(0, size=1)
        one = sgf2n(1, size=1)
        if blocksize == 64:
            pad_one = sgf2n(fk.cembed4(0x8), size=1)
            pad_zero = zero
            cellsize = 4
        elif blocksize == 128:
            pad_one = sgf2n(fk.cembed8(0x80), size=1)
            pad_zero = zero
            cellsize = 8
        else:
            raise NotImplemented
        super().__init__(fk, blocksize, tweaksize, cellsize, fk_params, fk_enc, fk_dec, zero, one, pad_one=pad_one, pad_zero=pad_zero)

    def _convert_public(self, data):
        # input data is already embedded
        return [sgf2n.conv(cell) for cell in data]

    def _pack_cell(self, cells):
        assert(all((cell.size == 1 for cell in cells)))
        return sgf2n(cells, size=len(cells))

    def _pack_bit(self, bits):
        return self._pack_cell(bits)

    def _vector_size(self, x):
        return x.size

    def _vector_size_bit(self, b):
        return self._vector_size(b)

    def _zero_cell(self):
        return self.ZERO

    def _print_block(self, block, i=-1, msg=''):
        if isinstance(block[0], list):
            # bits
            block = [b for cell in block for b in cell]
        if i >= 0:
            block = [cell.get_all()[i] for cell in block]
        cells = [cell.reveal() for cell in block]
        print_ln(msg + ('%s ' * len(cells)), *cells)

    def _unpack_cell(self, packed_cell):
        n = packed_cell.size
        unpacked = [packed_cell[i] for i in range(n)]
        return unpacked

    def _xor_cells(self, cells):
        assert all((cell.size == cells[0].size for cell in cells)), ", ".join((str(cell.size) for cell in cells))
        return sum(cells)

    def _check_tag_eq(self, a, b):
        # flatten a and b
        if self.cellsize == 4:
            decompose = self.fk.forkskinny_bit_decompose4
        elif self.cellsize == 8:
            decompose = self.fk.forkskinny_bit_decompose8
        else:
            raise NotImplemented
        a = [x for c in a for x in decompose(c)]
        b = [x for c in b for x in decompose(c)]
        assert(len(a) == len(b))
        # check AND NOT(a XOR b)
        l = [(ai + bi + self.ONE) for ai,bi in zip(a,b)]
        while len(l) > 1:
            l = [l[i] * l[i+1] for i in range(0,len(l)-1,2)] + l[len(l)-(len(l) % 2) : len(l)]
        return l[0]

    def _get_bitlen(self, d):
        # inputs are already embedded in cells
        return len(d) * self.cellsize
