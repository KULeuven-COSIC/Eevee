from Compiler.GC.types import cbits, sbitvec
from Compiler.types import regint, program
from Compiler.types import MultiArray, Array, cgf2n
from Compiler.util import is_constant

class BitsVectorConstants:
    def __init__(self, values, cellsize, dtype, n):
        self.values = values
        self.array = MultiArray([len(values), cellsize], dtype.get_type(n))
        # populate array
        ZERO = dtype.get_type(n)(0)
        ONE = dtype.get_type(n)(2**n-1)
        for i,v in enumerate(self.values):
            for j in range(cellsize):
                self.array[i][j] = ONE if ((v >> j) & 0x1) > 0 else ZERO
    
    def get_bit(self, index, bitindex):
        return self.array[index][bitindex]

class BitsArray:
    def __init__(self, length, dtype):
        self.length = length
        self.dtype = dtype
        self.mem_size = self.dtype.mem_size()
        self.address = dtype.malloc(self.length, creator_tape=program.curr_tape)
        self.freed = False
    def _load(self, addr):
        return self.dtype.load_mem(addr)
    def load_mem(self, i=None):
        assert(not self.freed)
        if i is None or isinstance(i, tuple) or isinstance(i, list):
            if i is None:
                start = 0
                n = self.length
            else:
                start, n = i
            addr = self.address + self.mem_size * start
            l = []
            for i in range(n):
                l.append(self._load(addr))
                if i+1 < n:
                    addr += self.mem_size
            return l
        else:
            return self._load(self.address + i*self.mem_size)
    def _store(self, s, addr):
        assert(self.dtype == type(s) and self.dtype.n == s.n), f'{self.dtype} == {type(s)} and {self.dtype.n} == {s.n}'
        s.store_in_mem(addr)
    def store_in_mem(self, state, i=None):
        assert(not self.freed)
        if i is None or isinstance(i, tuple) or isinstance(i, list):
            if i is None:
                assert(isinstance(state, list))
                start = 0
                n = self.length
            else:
                start, n = i
            assert(len(state) == n), f'Expected list of size {n}, found {len(state)}'
            addr = self.address + self.mem_size * start
            for i in range(n):
                self._store(state[i], addr)
                if i+1 < n:
                    addr += self.mem_size
        else:
            self._store(state, self.address + i*self.mem_size)
    def free(self):
        assert(not self.freed)
        program.free(self.address, self.dtype.reg_type)

def tag_check(public_tag, secret_tag, bit_decompose):
    assert len(public_tag) == len(secret_tag)
    #assert isinstance(public_tag, cgf2n)
    #assert isinstance(secret_tag, sgf2n)
    delta_bits = [bit_decompose(secret_tag_i - public_tag_i) for secret_tag_i,public_tag_i in zip(secret_tag, public_tag)]
    one = cgf2n(1, size=len(public_tag[0]))
    # flatten
    delta_bits = [b + one for delta in delta_bits for b in delta]
    # multiply delta_bits in tree
    while len(delta_bits) > 1:
        delta_bits = [delta_bits[i] * delta_bits[i+1] for i in range(0, len(delta_bits)-1, 2)] + delta_bits[len(delta_bits) - (len(delta_bits) % 2) : len(delta_bits)]
    return delta_bits[0]
