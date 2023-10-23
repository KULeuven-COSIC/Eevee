import math
from Compiler.library import print_ln

class SkinnyCipherBase:
    ROUND_KEY_PERMUTATION = [9, 15, 8, 13, 10, 14, 12, 11, 0, 1, 2, 3, 4, 5, 6, 7]
    SHIFT_ROWS_PERMUTATION = [0, 1, 2, 3, 7, 4, 5, 6, 10, 11, 8, 9, 13, 14, 15, 12]
    SHIFT_ROWS_PERMUTATION_INV = [0, 1, 2, 3, 5, 6, 7, 4, 10, 11, 8, 9, 15, 12, 13, 14]
    MIX_COLUMNS_MATRIX = [1, 0, 1, 1, 1, 0, 0, 0, 0, 1, 1, 0, 1, 0, 1, 0]
    MIX_COLUMNS_MATRIX_INV = [0, 1, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 1, 0, 0, 1]

    def __init__(self, cellsize, double=False):
        assert cellsize in [4,8]
        self.cellsize = cellsize
        self.double = double

    def _xor_cell(self, a, b):
        raise NotImplemented

    def sub_cells(self, state, sbox):
        assert(len(state) == 16)
        for i in range(len(state)):
            state[i] = sbox(state[i])
        return state

    def add_round_constants(self, state, r, has_tweak):
        raise NotImplemented

    def s4_sbox(self, cell):
        raise NotImplemented
    def s8_sbox(self, cell):
        raise NotImplemented
    def s4_sbox_inv(self, cell):
        raise NotImplemented
    def s8_sbox_inv(self, cell):
        raise NotImplemented

    def add_round_key(self, state, tk):
        assert(len(state) == 16)
        # xor the first two rows
        for i in range(8):
            for j in range(len(tk)):
                state[i] = self._xor_cell(state[i], tk[j][i])
        return state

    def shift_rows(self, state, permutation):
        assert(len(state) == 16)
        new_state = [state[permutation[i]] for i in range(len(state))]
        return new_state

    def mix_columns(self, state, matrix):
        new_state = [None] * 16
        for i in range(4):
            for j in range(4):
                to_xor = [state[4*k+j] for k in range(4) if matrix[4*i+k] > 0]
                if len(to_xor) == 1:
                    new_state[4*i+j] = to_xor[0]
                elif len(to_xor) == 2:
                    new_state[4*i+j] = self._xor_cell(to_xor[0], to_xor[1])
                elif len(to_xor) == 3:
                    new_state[4*i+j] = self._xor_cell(to_xor[0], self._xor_cell(to_xor[1], to_xor[2]))
                else:
                    raise NotImplemented
        return new_state

    def update_round_key(self, tk):
        new_tk = []
        for ti in tk:
            new_tk.append([ti[self.ROUND_KEY_PERMUTATION[i]] for i in range(len(ti))])
        if len(new_tk) >= 2:
            # apply LFSR2 to the first two rows of TK2
            for i in range(8):
                if self.cellsize == 4:
                    x = new_tk[1][i]
                    if self.double:
                        new_tk[1][i] = [x[3] + x[2], x[0], x[1], x[2], x[7] + x[6], x[4], x[5], x[6]]
                    else:
                        new_tk[1][i] = [x[3] + x[2], x[0], x[1], x[2]]
                elif self.cellsize == 8:
                    assert not self.double, "not implemented"
                    x = new_tk[1][i]
                    new_tk[1][i] = [x[7] + x[5], x[0], x[1], x[2], x[3], x[4], x[5], x[6]]
                else:
                    raise NotImplemented

        if len(new_tk) >= 3:
            # apply LFSR3 to the first two rows of TK3
            for i in range(8):
                if self.cellsize == 4:
                    x = new_tk[2][i]
                    if self.double:
                        new_tk[2][i] = [x[1], x[2], x[3], x[0] + x[3], x[5], x[6], x[7], x[4] + x[7]]
                    else:
                        new_tk[2][i] = [x[1], x[2], x[3], x[0] + x[3]]
                elif self.cellsize == 8:
                    assert not self.double, "not implemented"
                    x = new_tk[2][i]
                    new_tk[2][i] = [x[1], x[2], x[3], x[4], x[5], x[6], x[7], x[0] + x[6]]
                else:
                    raise NotImplemented
        return new_tk

    def skinny_expand_key(self, key, tweak, rounds):
        '''
        Computes and returns the key schedule of (Fork)SKINNY
        key:        list of key bits encoded in groups of cellsize bits with least significant bit first
        tweak:      list of tweak bits encoded in groups of cellsize bits with least significant bit first

        Returns a list of round keys
        '''
        assert not self.double
        bits = key + tweak
        tk = []
        blocksize = 16 * self.cellsize
        n_blocks = int(math.ceil(len(bits)/blocksize))
        assert(len(bits) == blocksize*n_blocks), 'The implementation currently does not support incomplete tweakeys'
        for i in range(n_blocks):
            tki = [[bits[i*blocksize + j * self.cellsize + k] for k in range(self.cellsize)] for j in range(16)]
            tk.append(tki)

        schedule = []
        for r in range(rounds):
            schedule.append([[self._embed_cell(cell) for cell in tki] for tki in tk])
            tk = self.update_round_key(tk)
        return schedule

    def skinny_expand_key_double(self, key1, tweak1, key2, tweak2, rounds):
        '''
        Computes and returns the key schedule of (Fork)SKINNY (two at the same time)
        key1:        list of key bits encoded in groups of cellsize bits with least significant bit first
        tweak1:      list of tweak bits encoded in groups of cellsize bits with least significant bit first
        key2:        list of key bits encoded in groups of cellsize bits with least significant bit first
        tweak2:      list of tweak bits encoded in groups of cellsize bits with least significant bit first

        Returns a list of round keys
        '''
        assert self.double
        assert len(key1) == len(key2)
        assert len(tweak1) == len(tweak2)
        bits1 = key1 + tweak1
        bits2 = key2 + tweak2
        tk = []
        blocksize = 16 * self.cellsize
        n_blocks = int(math.ceil(len(bits1)/blocksize))
        assert(len(bits1) == blocksize*n_blocks), 'The implementation currently does not support incomplete tweakeys'
        for i in range(n_blocks):
            tki = [ \
            bits1[i*blocksize + j * self.cellsize: i*blocksize + j * self.cellsize + self.cellsize] \
            + bits2[i*blocksize + j * self.cellsize: i*blocksize + j * self.cellsize + self.cellsize] \
            for j in range(16)]
            tk.append(tki)

        schedule = []
        for r in range(rounds):
            schedule.append([[self._embed_cell(cell) for cell in tki] for tki in tk])
            tk = self.update_round_key(tk)
        return schedule

    def skinny_round_enc(self, state, tweakey, r, sbox, has_tweak):
        state = self.sub_cells(state, sbox)
        state = self.add_round_constants(state, r, has_tweak)
        state = self.add_round_key(state, tweakey)
        state = self.shift_rows(state, self.SHIFT_ROWS_PERMUTATION)
        state = self.mix_columns(state, self.MIX_COLUMNS_MATRIX)
        return state

    def skinny_round_dec(self, state, tweakey, r, sbox, has_tweak):
        state = self.mix_columns(state, self.MIX_COLUMNS_MATRIX_INV)
        state = self.shift_rows(state, self.SHIFT_ROWS_PERMUTATION_INV)
        state = self.add_round_key(state, tweakey)
        state = self.add_round_constants(state, r, has_tweak)
        state = self.sub_cells(state, sbox)
        return state


class ForkSkinnyBase(SkinnyCipherBase):
    FORKSKINNY_64_192 = (17,23,23)
    FORKSKINNY_128_256 = (21,27,27)
    FORKSKINNY_128_384 = (25,31,31)
    ROUND_CONSTANTS = [
        0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7E, 0x7D, 0x7B, 0x77, 0x6F, 0x5F, 0x3E, 0x7C, 0x79, 0x73,
        0x67, 0x4F, 0x1E, 0x3D, 0x7A, 0x75, 0x6B, 0x57, 0x2E, 0x5C, 0x38, 0x70, 0x61, 0x43, 0x06, 0x0D,
        0x1B, 0x37, 0x6E, 0x5D, 0x3A, 0x74, 0x69, 0x53, 0x26, 0x4C, 0x18, 0x31, 0x62, 0x45, 0x0A, 0x15,
        0x2B, 0x56, 0x2C, 0x58, 0x30, 0x60, 0x41, 0x02, 0x05, 0x0B, 0x17, 0x2F, 0x5E, 0x3C, 0x78, 0x71,
        0x63, 0x47, 0x0E, 0x1D, 0x3B, 0x76, 0x6D, 0x5B, 0x36, 0x6C, 0x59, 0x32, 0x64, 0x49, 0x12, 0x25,
        0x4A, 0x14, 0x29, 0x52, 0x24, 0x48, 0x10
    ]
    assert(len(ROUND_CONSTANTS) == 87)
    BC4 = [0x1, 0x2, 0x4, 0x9, 0x3, 0x6, 0xd, 0xa, 0x5, 0xb, 0x7, 0xf, 0xe, 0xc, 0x8, 0x1]
    BC8 = [0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x41, 0x82, 0x05, 0x0a, 0x14, 0x28, 0x51, 0xa2, 0x44, 0x88]

    def __init__(self, cellsize, rounds, vector_size, double=False):
        super().__init__(cellsize, double)
        self.rounds = rounds
        self.vector_size = vector_size

    def _embed_cell(self, cell):
        raise NotImplemented

    def add_branch_constant(self, state):
        raise NotImplemented

    def deep_copy(self, block):
        if isinstance(block,list):
            return [self.deep_copy(x) for x in block]
        else:
            return block

    def print_block(self, block):
        print_ln('%s ' * len(block), *[cell.reveal() for cell in block])

    def expand_key(self, key, tweak):
        r_init, r0, r1 = self.rounds
        return self.skinny_expand_key(key, tweak, r_init + r0 + r1)

    def expand_key_double(self, key1, tweak1, key2, tweak2):
        r_init, r0, r1 = self.rounds
        return self.skinny_expand_key_double(key1, tweak1, key2, tweak2, r_init + r0 + r1)

    def forkskinny_enc(self, state, tweakey_schedule, s):
        assert not self.double, "not implemented"
        r_init, r_0, r_1 = self.rounds
        has_tweak = True
        assert(len(tweakey_schedule) == r_init + r_0 + r_1)
        if self.cellsize == 4:
            sbox = self.s4_sbox
        elif self.cellsize == 8:
            sbox = self.s8_sbox
        else:
            raise NotImplemented
        state = self.deep_copy(state)
        tweakey_schedule = self.deep_copy(tweakey_schedule)
        for r in range(r_init):
            state = self.skinny_round_enc(state, tweakey_schedule[r], r, sbox, has_tweak)
        if s == '0' or s == 'b':
            c0 = self.add_branch_constant(state)
            for r in range(r_init + r_1, r_init + r_0 + r_1):
                c0 = self.skinny_round_enc(c0, tweakey_schedule[r], r, sbox, has_tweak)
        if s == '1' or s == 'b':
            c1 = state
            for r in range(r_init, r_init + r_1):
                c1 = self.skinny_round_enc(c1, tweakey_schedule[r], r, sbox, has_tweak)
        if s == '0':
            return c0
        elif s == '1':
            return c1
        elif s == 'b':
            return c0, c1
        else:
            raise NotImplemented

    def forkskinny_dec(self, state, tweakey_schedule, s, b):
        assert not self.double, "not implemented"
        r_init, r_0, r_1 = self.rounds
        has_tweak = True
        assert(len(tweakey_schedule) == r_init + r_0 + r_1)
        if self.cellsize == 4:
            sbox = self.s4_sbox_inv
        elif self.cellsize == 8:
            sbox = self.s8_sbox_inv
        else:
            raise NotImplemented
        state = self.deep_copy(state)
        tweakey_schedule = self.deep_copy(tweakey_schedule)
        if b == '0':
            for r in reversed(range(r_init + r_1, r_init + r_0 + r_1)):
                state = self.skinny_round_dec(state, tweakey_schedule[r], r, sbox, has_tweak)
            state = self.add_branch_constant(state)
        elif b == '1':
            for r in reversed(range(r_init, r_init + r_1)):
                state = self.skinny_round_dec(state, tweakey_schedule[r], r, sbox, has_tweak)
        else:
            raise NotImplemented
        if s == 'i' or s == 'b':
            message = self.deep_copy(state)
            for r in reversed(range(r_init)):
                message = self.skinny_round_dec(message, tweakey_schedule[r], r, sbox, has_tweak)
        if s == 'o' or s == 'b':
            round_range = range(r_init, r_init + r_1) if b == '0' else range(r_init + r_1, r_init + r_0 + r_1)
            if b == '1':
                ciphertext =self.add_branch_constant(self.deep_copy(state))
            else:
                ciphertext = state
            if self.cellsize == 4:
                sbox = self.s4_sbox
            elif self.cellsize == 8:
                sbox = self.s8_sbox
            else:
                raise NotImplemented
            for r in round_range:
                ciphertext = self.skinny_round_enc(ciphertext, tweakey_schedule[r], r, sbox, has_tweak)
        if s == 'i':
            return message
        elif s == 'o':
            return ciphertext
        elif s == 'b':
            return message, ciphertext
        else:
            raise NotImplemented

class SkinnyBase(SkinnyCipherBase):
    ROUND_CONSTANTS = [
        0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3E, 0x3D, 0x3B, 0x37, 0x2F, 0x1E, 0x3C, 0x39, 0x33, 0x27, 0x0E,
        0x1D, 0x3A, 0x35, 0x2B, 0x16, 0x2C, 0x18, 0x30, 0x21, 0x02, 0x05, 0x0B, 0x17, 0x2E, 0x1C, 0x38,
        0x31, 0x23, 0x06, 0x0D, 0x1B, 0x36, 0x2D, 0x1A, 0x34, 0x29, 0x12, 0x24, 0x08, 0x11, 0x22, 0x04,
        0x09, 0x13, 0x26, 0x0C, 0x19, 0x32, 0x25, 0x0A
    ]
    SKINNY_128_128 = (8, 40, 1)
    SKINNY_128_256 = (8, 48, 2)
    def __init__(self, variant, double=False):
        cellsize, rounds, tk = variant
        super().__init__(cellsize)
        self.rounds = rounds
        self.tk = tk

    def expand_key(self, key, tweak):
        return self.skinny_expand_key(key, tweak, self.rounds)

    def skinny_enc(self, state, tweakey_schedule):
        assert len(tweakey_schedule) == self.rounds, f'{len(tweakey_schedule)} != {self.rounds}'
        assert all((len(tk) == self.tk for tk in tweakey_schedule))
        if self.cellsize == 4:
            sbox = self.s4_sbox
        elif self.cellsize == 8:
            sbox = self.s8_sbox
        else:
            raise NotImplemented
        for r in range(self.rounds):
            state = self.skinny_round_enc(state, tweakey_schedule[r], r, sbox, False)
        return state

    def skinny_dec(self, state, tweakey_schedule):
        assert len(tweakey_schedule) == self.rounds, f'{len(tweakey_schedule)} != {self.rounds}'
        assert all((len(tk) == self.tk for tk in tweakey_schedule))
        if self.cellsize == 4:
            sbox = self.s4_sbox_inv
        elif self.cellsize == 8:
            sbox = self.s8_sbox_inv
        else:
            raise NotImplemented
        for r in reversed(range(self.rounds)):
            state = self.skinny_round_dec(state, tweakey_schedule[r], r, sbox, False)
        return state

# expose
from .forkskinny_gf2n import ForkSkinny as ForkSkinnyGF2n, Skinny as SkinnyGF2n, cembed4, cembed8, embed4, embed8, bit_decompose4, bit_decompose8
