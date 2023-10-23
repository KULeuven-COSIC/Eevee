import math
from Compiler.library import print_ln

PRINT_DEBUG=False

class EeveeBase:
    def __init__(self, fk, blocksize, tweaksize, cellsize, fk_params, fk_enc, fk_dec, zero, one, pad_one, pad_zero):
        self.fk = fk
        self.blocksize = blocksize
        self.tweaksize = tweaksize
        self.cellsize = cellsize
        self.fk_params = fk_params
        self.fk_enc = fk_enc
        self.fk_dec = fk_dec
        self.ZERO = zero
        self.ONE = one
        self.pad_one = pad_one
        self.pad_zero = pad_zero

    def _convert_public(self, data):
        raise NotImplemented

    def _pack_cell(self, cells):
        """
        cells: list of implementation-specific cell representations to be packed into a single cell vector
        Returns a single implementation-specific cell representation with vector dimension len(cells)
        """
        raise NotImplemented

    def _pack_bit(self, bits):
        """
        bits: list of implementation-specific bits to be packed into a single vector
        Returns a single implementation-specific bit with vector dimension len(bits)
        """
        raise NotImplemented

    def _vector_size(self, x):
        """
        x: implementation-specific cell representation
        Returns the vector size of the cell representation
        """
        raise NotImplemented

    def _vector_size_bit(self, b):
        """
        x: implementation-specific bit representation
        Returns the vector size of the bit representation
        """
        raise NotImplemented

    def _zero_cell(self):
        """
        Returns implementation-specific cell representation for a cell with 0
        """
        raise NotImplemented

    def _print_block(self, block, i=-1, msg=''):
        """
        Prints the block with message msg at vector index i
        """
        raise NotImplemented

    def _unpack_cell(self, packed_cell):
        """
        packed_cell: implementation-specific cell representation
        Returns a list of implementation-specific cell representation
        """
        raise NotImplemented

    def _xor_cells(self, cells):
        """
        cells: List of implementation-specific cell representation to xor together
        Returns implementation-specific cell representation holding the xor of cells
        """
        raise NotImplemented

    def _check_tag_eq(self, a, b):
        """
        a: tag a
        b: tag b
        Returns a implementation-specific bit representation holding 1 if a==b, 0 otherwise
        """
        raise NotImplemented

    def _get_bitlen(self, d):
        """
        d: list of implementation-specific public data input
        Returns number of bits of the data input
        """
        raise NotImplemented

    def _apply_padding(self, data):
        datalength = self.cellsize * len(data)
        n_blocks = int(math.ceil(datalength/self.blocksize))

        complete = datalength % self.blocksize == 0
        if not complete:
            # padd AD
            data.append(self.pad_one)
            remaining = len(data) % 16
            if remaining > 0:
                data += [self.pad_zero] * (16 - remaining)
        return n_blocks, complete, data

    def _pack_blocks(self, blocks):
        assert len(blocks) > 0
        assert len(blocks) % 16 == 0, f'{len(blocks)}'
        return [self._pack_cell(blocks[i:len(blocks):16]) for i in range(16)]

    def _pack_tweak(self, nonce, sep_bit, n_blocks, cnt_start, final_block_counter):
        assert isinstance(nonce, list)
        assert final_block_counter < 2**15
        assert n_blocks+2 < 2**15
        assert len(nonce) + 1 + 15 == self.tweaksize, f'{len(nonce)} + 16 == {self.tweaksize}'
        tweak = []
        # tweak: nonce||sep_bit||counter
        for noncebit in nonce:
            tweak.append(self._pack_bit([noncebit] * n_blocks))
        counter = []
        for i in range(15):
            counter_bits = [self.ONE if ((cnt >> i) & 0x1) > 0 else self.ZERO for cnt in range(cnt_start,n_blocks+1)]
            counter_bits.append(self.ONE if ((final_block_counter >> i) & 0x1) > 0 else self.ZERO)
            counter.append(self._pack_bit(counter_bits))
        if self.cellsize == 4:
            # we represent the counter bits cellwise reversed, like in 4bit-based architectures
            tweak += counter[12:15]
            # sep_bit is bit 15
            tweak.append(self._pack_bit([sep_bit] * n_blocks))
            tweak += counter[8:12]
            tweak += counter[4:8]
            tweak += counter[0:4]
        elif self.cellsize == 8:
            # we represent the counter bits cellwise reversed, like in 8bit-based architectures
            tweak += counter[8:15]
            # sep_bit is bit 15
            tweak.append(self._pack_bit([sep_bit] * n_blocks))
            tweak += counter[0:8]
        else:
            raise NotImplemented

        assert(len(tweak) == self.tweaksize), f'Expected {self.tweaksize}, got {len(tweak)}'
        return tweak

    def umbreon_dec(self, associated_data, ciphertext, tag, key, nonce):
        assert len(key) == 128
        assert len(nonce) + 16 == self.tweaksize
        noncesize = len(nonce)

        # convert to secret
        n_associated_data = self._get_bitlen(associated_data)
        associated_data = self._convert_public(associated_data)
        assert len(associated_data) * self.cellsize == n_associated_data

        n_ciphertext = self._get_bitlen(ciphertext)
        ciphertext = self._convert_public(ciphertext)
        assert len(ciphertext) * self.cellsize == n_ciphertext

        tag = self._convert_public(tag)
        assert len(tag) * self.cellsize == self.blocksize

        if len(associated_data) > 0:
            n_ad_blocks, complete_ad, associated_data = self._apply_padding(associated_data)
            ad_packed = self._pack_blocks(associated_data)
            assert all((self._vector_size(cell) == n_ad_blocks for cell in ad_packed))
        else:
            n_ad_blocks = 0

        n_c_blocks = int(math.ceil(n_ciphertext/self.blocksize))
        complete_c = n_ciphertext % self.blocksize == 0

        # compute T_A
        t_a = [self._zero_cell() for i in range(16)]
        assert all((self._vector_size(t_a[i]) == 1 for i in range(16)))

        # pack tweakey for AD
        if len(associated_data) > 0:
            # non-empty AD
            no_m = 0 if len(ciphertext) > 0 else 1
            final_ad_block_counter = no_m + 0 if complete_ad else 2
            ad_tweak = self._pack_tweak(nonce, self.ZERO, n_ad_blocks, 4, final_ad_block_counter)
            # pack key
            ad_key_packed = [self._pack_bit([keybit] * n_ad_blocks) for keybit in key]
            assert all(self._vector_size_bit(b) == n_ad_blocks for b in ad_tweak), f'n_ad_blocks={n_ad_blocks}, ad_tweak={ad_tweak}'
            if PRINT_DEBUG:
                for i in range(n_ad_blocks):
                    self._print_block(ad_packed, i, f'AD {i}: ')
                    self._print_block([ad_key_packed[j:j+self.cellsize] for j in range(0,128,self.cellsize)] + [ad_tweak[j:j+self.cellsize] for j in range(0,self.tweaksize,self.cellsize)], i, f'AD {i} Tweakey: ')
                print_ln('%s ' * self.tweaksize, *[x.reveal() for x in ad_tweak])
            ad_key_schedule = self.fk.expand_key(ad_key_packed, ad_tweak, self.cellsize, self.fk_params)
            ad_c0_packed = self.fk_enc(ad_packed, ad_key_schedule, '0')
            #try free key schedule
            try:
                ad_key_schedule.free()
            except AttributeError:
                pass
            if PRINT_DEBUG:
                for i in range(n_ad_blocks):
                    self._print_block(ad_c0_packed, i, f'C0 {i}: ')

            for i in range(16):
                t_a[i] = self._xor_cells([t_a[i]] + self._unpack_cell(ad_c0_packed[i]))

        if PRINT_DEBUG:
            self._print_block(t_a, -1, 'T_A: ')

        message = [[ None for i in range(16)] for j in range(n_c_blocks)]
        if n_c_blocks > 0:
            # pack ciphertext except the last block but include the tag instead
            ciphertext_packed = self._pack_blocks(ciphertext[:(n_c_blocks-1)*16] + tag)
            final_c_block_counter = 0 if complete_c else 1
            c_tweak = self._pack_tweak(nonce, self.ONE, n_c_blocks, 2, final_c_block_counter)
            # pack key
            c_key_packed = [self._pack_bit([keybit] * n_c_blocks) for keybit in key]
            if PRINT_DEBUG:
                for i in range(n_c_blocks):
                    self._print_block(ciphertext_packed, i, f'C Block {i}: ')
                    self._print_block([c_key_packed[j:j+self.cellsize] for j in range(0,128,self.cellsize)] + [c_tweak[j:j+self.cellsize] for j in range(0,self.tweaksize,self.cellsize)], i, f'C {i} Tweakey: ')
            c_key_schedule = self.fk.expand_key(c_key_packed, c_tweak, self.cellsize, self.fk_params)

            message_packed, m_c1_packed = self.fk_dec(ciphertext_packed, c_key_schedule, 'b', '0')
            #try free key schedule
            try:
                c_key_schedule.free()
            except AttributeError:
                pass
            if PRINT_DEBUG:
                for i in range(n_c_blocks):
                    self._print_block(message_packed, i, f'M Out {i}: ')
                    self._print_block(m_c1_packed, i, f'C1 Out {i}: ')


            last_block_length = len(ciphertext) % 16 if not complete_c else 16
            c_star = [None] * last_block_length
            for i in range(16):
                message_cells = self._unpack_cell(message_packed[i])
                c1_cells = self._unpack_cell(m_c1_packed[i])
                assert len(message_cells) == n_c_blocks
                assert len(c1_cells) == n_c_blocks
                for k in range(n_c_blocks):
                    if k == 0 and k < n_c_blocks-1:
                        cell = self._xor_cells([t_a[i], message_cells[k]])
                    elif k < n_c_blocks-1:
                        cell = self._xor_cells([c1_cells[k-1], message_cells[k]])
                    else:
                        cell = message_cells[k]
                    message[k][i] = cell
                t_a[i] = self._xor_cells([t_a[i]] + c1_cells[:-1])
                message[-1][i] = self._xor_cells([message[-1][i], t_a[i]])
                if i < last_block_length:
                    c_star[i] = self._xor_cells([message[-1][i], c1_cells[-1]])

            if PRINT_DEBUG:
                for i in range(n_c_blocks):
                    self._print_block(message[i], -1, f'M Block {i}: ')
        # check tag
        if n_c_blocks == 0:
            tag_correct = self._check_tag_eq(t_a, tag)
        elif complete_c:
            assert(last_block_length == 16)
            tag_correct = self._check_tag_eq(c_star, ciphertext[-last_block_length:])
        else:
            last_message = message[-1]
            padding = [self.pad_one] + [self.pad_zero] * (16-last_block_length-1)
            tag_correct = self._check_tag_eq(c_star, ciphertext[-last_block_length:])
            tag_correct = tag_correct * self._check_tag_eq(last_message[last_block_length:], padding)
        open_tag = tag_correct.reveal()
        if PRINT_DEBUG:
            print_ln('Correct tag: %s', open_tag)
        return open_tag, [message[k][i] for k in range(n_c_blocks) for i in range(16) if k < n_c_blocks - 1 or i < last_block_length]

from .eevee_gf2n import EeveeGF2n
