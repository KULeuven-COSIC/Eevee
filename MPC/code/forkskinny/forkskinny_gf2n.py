from . import ForkSkinnyBase, SkinnyBase
from Compiler.types import sgf2n, cgf2n, Matrix, regint, vectorize
from Compiler.library import get_program, start_timer, stop_timer

import os

if "SKINNY_TARGET_FIELD" in os.environ:
    USE_F128_EMBEDDING = os.environ["SKINNY_TARGET_FIELD"] == 'f128'
else:
    USE_F128_EMBEDDING = False
if USE_F128_EMBEDDING:
    print('Generating (Fork)SKINNY implementation for GF(2^128) (USE_GF2N_LONG=1)')
else:
    print('Generating (Fork)SKINNY implementation for GF(2^40) (USE_GF2N_LONG=0)')

class VectorConstant:
    def __init__(self, f):
        self.v = dict()
        self.f = f
    def get_type(self, n):
        tape = get_program().curr_tape
        if (n,tape.name) not in self.v:
            self.v[(n,tape.name)] = self.f(n)
        return self.v[(n,tape.name)]
if not USE_F128_EMBEDDING:
    CEMBED_POWERS4 = [1 << 5, 1 << 10, 1 << 15, 1 << 20, 1 << 30, 1 << 35]
    CEMBED_POWERS8 = [1 << 5, 1 << 10, 1 << 15, 1 << 20, 1 << 25, 1 << 30, 1 << 35]
    EMBED_POWERS4 = VectorConstant(lambda n: [cgf2n(p, size=n) for p in CEMBED_POWERS4])
    EMBED_POWERS8 = VectorConstant(lambda n: [cgf2n(p, size=n) for p in CEMBED_POWERS8])

#RC2_4_EMBEDDED = embed4(0x2)
RC2_4_EMBEDDED = VectorConstant(lambda n: cgf2n(cembed4(0x2), size=n))
RC2_8_EMBEDDED = VectorConstant(lambda n: cgf2n(cembed8(0x2), size=n))

#ROUND_CONSTANTS4_EMBEDDED = [(embed4(rc & 0xf), embed4((rc >> 4) & 0x7), RC2_4_EMBEDDED) for rc in ROUND_CONSTANTS]
ROUND_CONSTANTS4_EMBEDDED = VectorConstant(lambda n: [(cgf2n(cembed4(rc & 0xf), size=n), cgf2n(cembed4((rc >> 4) & 0x7), size=n), RC2_4_EMBEDDED.get_type(n)) for rc in ForkSkinnyBase.ROUND_CONSTANTS])

#RC2_8_EMBEDDED = embed8(0x2)
RC2_8_EMBEDDED = VectorConstant(lambda n: cgf2n(cembed8(0x2), size=n))

#ROUND_CONSTANTS8_EMBEDDED = [(embed8(rc & 0xf), embed8((rc >> 4) & 0x3), RC2_8_EMBEDDED) for rc in ROUND_CONSTANTS]
ROUND_CONSTANTS8_EMBEDDED = VectorConstant(lambda n: [(cgf2n(cembed8(rc & 0xf), size=n), cgf2n(cembed8((rc >> 4) & 0x7), size=n), RC2_8_EMBEDDED.get_type(n)) for rc in ForkSkinnyBase.ROUND_CONSTANTS])

#BC4 = [0x1, 0x2, 0x4, 0x9, 0x3, 0x6, 0xd, 0xa, 0x5, 0xb, 0x7, 0xf, 0xe, 0xc, 0x8, 0x1]
BC4 = VectorConstant(lambda n: [cgf2n(cembed4(bc), size=n) for bc in ForkSkinnyBase.BC4])
#BC8 = [0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x41, 0x82, 0x05, 0x0a, 0x14, 0x28, 0x51, 0xa2, 0x44, 0x88]
BC8 = VectorConstant(lambda n: [cgf2n(cembed8(bc), size=n) for bc in ForkSkinnyBase.BC8])

# two-step CRV decomposition polynomials for inverse 4-bit Sbox
P00 = VectorConstant(lambda n: [cgf2n(x, size=n) for x in [0x1, 0x800108001, 0x0, 0x40008421, 0x840108401]])
Q00 = VectorConstant(lambda n: [cgf2n(x, size=n) for x in [0x40000401, 0x800100020, 0x840100420, 0x800108001, 0x1]])
P01 = VectorConstant(lambda n: [cgf2n(x, size=n) for x in [0x8020, 0x840108400, 0x800100020, 0x0, 0x0]])
P10 = VectorConstant(lambda n: [cgf2n(x, size=n) for x in [0x840100421, 0x1, 0x840100420, 0x40000401, 0x800100020]])
Q10 = VectorConstant(lambda n: [cgf2n(x, size=n) for x in [0x840108401, 0x8020, 0x40008420, 0x840108400, 0x800100021]])
P11 = VectorConstant(lambda n: [cgf2n(x, size=n) for x in [0x800108000, 0x840100420, 0x800100021, 0x0, 0x0]])

U00 = VectorConstant(lambda n: [cgf2n(x, size=n) for x in [0x840100421, 0x40008421, 0x40008421, 0x840108401, 0x40008421]])
V00 = VectorConstant(lambda n: [cgf2n(x, size=n) for x in [0x800108000, 0x800100021, 0x800100020, 0x800108001, 0x800108000]])
U01 = VectorConstant(lambda n: [cgf2n(x, size=n) for x in [0x840108400, 0x40008421, 0x40008420, 0x840108401, 0x40008421]])


SKINNY_ROUND_CONSTANTS4_EMBEDDED = VectorConstant(lambda n: [(cgf2n(cembed4(rc & 0xf), size=n), cgf2n(cembed4((rc >> 4) & 0x3), size=n), RC2_4_EMBEDDED.get_type(n)) for rc in SkinnyBase.ROUND_CONSTANTS])
SKINNY_ROUND_CONSTANTS8_EMBEDDED = VectorConstant(lambda n: [(cgf2n(cembed8(rc & 0xf), size=n), cgf2n(cembed8((rc >> 4) & 0x3), size=n), RC2_8_EMBEDDED.get_type(n)) for rc in SkinnyBase.ROUND_CONSTANTS])

if USE_F128_EMBEDDING:
    import functools
    import operator

    def embed4(n):
        '''
        Computes embedding F_{2^4} -> F_{2^128} via ?
        F_{2^4} = GF(2)[X]/X^4 + X^3 + X^2 + X + 1
        F_{2^128} = GF(2)[Y]/Y^128 + Y^7 + Y^2 + Y + 1

        Not implemented
        '''
        assert len(n) == 4
        b0, b1, b2, b3 = n
        assert [b0.size] * 3 == [b1.size, b2.size, b3.size]
        raise NotImplemented

    def cembed4(n):
        assert isinstance(n, int)
        b = [(n >> i) & 0x1 for i in range(4)]
        raise NotImplemented

    forward_embedding_matrix = [
        [1,1,0,1,1,1,0,0],
        [0,0,1,1,1,0,0,0],
        [0,0,0,1,0,0,0,1],
        [0,0,1,1,0,0,1,1],
        [0,1,1,0,1,0,0,0],
        [0,0,0,1,0,0,1,1],
        [0,0,0,1,0,0,0,0],
        [0,0,1,0,1,0,1,1],
        [0,0,1,0,0,0,0,1],
        [0,1,1,1,0,0,1,1],
        [0,0,0,0,0,0,1,0],
        [0,1,0,0,1,0,0,0],
        [0,1,1,0,0,1,0,1],
        [0,1,1,1,0,1,0,1],
        [0,1,0,0,1,1,1,1],
        [0,0,1,1,1,0,0,1],
        [0,1,0,1,1,1,1,0],
        [0,0,1,0,1,1,0,0],
        [0,1,0,1,1,1,0,1],
        [0,1,1,1,0,1,1,0],
        [0,1,1,1,1,1,1,1],
        [0,0,0,0,1,0,1,1],
        [0,1,1,1,1,0,0,1],
        [0,1,0,1,0,1,0,0],
        [0,0,1,0,1,0,0,1],
        [0,1,1,0,0,1,1,0],
        [0,1,0,0,0,1,1,0],
        [0,0,1,1,0,1,0,1],
        [0,0,0,0,1,1,1,1],
        [0,1,0,1,0,1,1,1],
        [0,0,0,1,0,0,0,1],
        [0,0,0,1,1,0,0,1],
        [0,1,0,0,1,0,0,1],
        [0,0,0,0,1,0,1,0],
        [0,0,1,1,1,1,1,0],
        [0,1,0,0,0,1,1,0],
        [0,0,0,1,1,0,0,1],
        [0,0,1,0,0,1,1,1],
        [0,0,1,0,1,0,0,0],
        [0,0,0,1,1,0,1,1],
        [0,0,1,0,0,1,0,1],
        [0,0,1,1,0,1,1,0],
        [0,0,0,1,1,0,0,1],
        [0,1,0,0,0,1,0,1],
        [0,0,1,0,1,0,0,0],
        [0,0,0,0,0,1,0,1],
        [0,1,1,0,0,1,0,0],
        [0,1,0,1,1,0,0,0],
        [0,0,1,1,0,1,1,1],
        [0,0,1,0,1,1,1,0],
        [0,0,1,1,1,0,1,0],
        [0,0,1,1,1,1,1,0],
        [0,0,1,1,0,1,0,0],
        [0,1,1,1,1,1,0,1],
        [0,1,0,1,1,0,1,0],
        [0,1,0,1,0,0,1,1],
        [0,1,1,0,0,0,1,1],
        [0,1,1,1,0,0,1,0],
        [0,0,0,0,1,0,1,0],
        [0,0,0,1,1,1,1,0],
        [0,0,0,0,1,1,1,0],
        [0,0,0,1,0,1,1,0],
        [0,0,1,1,0,0,0,1],
        [0,0,0,0,0,0,1,0],
        [0,1,0,0,1,0,0,1],
        [0,1,0,1,0,0,1,1],
        [0,0,1,1,0,1,1,1],
        [0,0,1,1,1,1,0,1],
        [0,0,0,0,0,1,0,1],
        [0,0,0,1,0,1,0,0],
        [0,1,1,1,0,0,1,0],
        [0,1,1,0,1,1,1,1],
        [0,1,1,1,0,1,0,1],
        [0,0,1,0,1,1,1,0],
        [0,1,0,1,0,0,0,0],
        [0,1,1,1,1,1,0,1],
        [0,1,1,1,0,0,1,1],
        [0,0,0,1,0,0,0,0],
        [0,1,1,0,0,1,1,0],
        [0,1,0,1,0,0,1,0],
        [0,0,1,0,0,0,1,1],
        [0,1,1,1,0,0,0,0],
        [0,0,0,0,1,1,0,0],
        [0,0,0,1,1,1,0,1],
        [0,0,1,0,0,0,1,1],
        [0,0,0,0,1,0,0,1],
        [0,0,0,1,0,1,1,1],
        [0,0,0,0,0,0,0,1],
        [0,1,1,1,0,1,1,1],
        [0,1,1,0,1,0,0,0],
        [0,1,1,0,0,0,1,0],
        [0,1,1,0,1,0,1,0],
        [0,0,1,1,0,0,1,0],
        [0,1,0,1,0,1,1,0],
        [0,1,1,1,0,0,1,0],
        [0,0,1,0,1,1,0,1],
        [0,1,0,0,1,0,1,1],
        [0,0,0,1,1,1,1,0],
        [0,0,1,0,1,0,1,0],
        [0,0,1,1,0,1,1,0],
        [0,1,1,0,0,0,0,0],
        [0,1,0,0,1,0,0,1],
        [0,0,1,1,0,1,1,1],
        [0,1,1,1,0,0,0,1],
        [0,0,0,0,1,1,0,0],
        [0,0,0,1,0,0,0,0],
        [0,1,0,1,0,0,0,1],
        [0,0,0,0,0,0,0,0],
        [0,1,1,1,1,0,0,0],
        [0,0,1,0,0,1,1,0],
        [0,0,0,1,0,1,1,1],
        [0,0,0,1,0,1,0,1],
        [0,0,0,1,0,0,1,0],
        [0,1,0,0,0,1,0,0],
        [0,0,1,1,1,0,1,0],
        [0,1,0,1,1,0,1,1],
        [0,1,0,0,0,1,0,1],
        [0,0,1,1,1,0,1,0],
        [0,0,0,0,1,0,0,1],
        [0,1,0,0,1,1,1,0],
        [0,0,1,1,0,0,1,1],
        [0,0,1,1,1,0,0,0],
        [0,0,1,1,0,1,1,1],
        [0,0,0,0,1,0,0,1],
        [0,1,0,1,1,1,1,0],
        [0,0,0,1,0,1,1,1],
        [0,0,0,1,1,1,0,1],
        [0,0,1,1,1,0,0,0],
    ]

    def embed8(n):
        '''
        Computes embedding F_{2^8} -> F_{2^128} via y^124 + y^119 + y^116 + y^115
         + y^113 + y^108 + y^106 + y^103 + y^101 + y^100 + y^96 + y^94 + y^93
         + y^91 + y^90 + y^89 + y^88 + y^81 + y^79 + y^78 + y^76 + y^75 + y^74
         + y^72 + y^71 + y^70 + y^65 + y^64 + y^57 + y^56 + y^55 + y^54 + y^53
         + y^47 + y^46 + y^43 + y^35 + y^32 + y^29 + y^26 + y^25 + y^23 + y^22
         + y^20 + y^19 + y^18 + y^16 + y^14 + y^13 + y^12 + y^11 + y^9 + y^4 + 1
        F_{2^8} = GF(2)[X]/X^8 + X^7 + X^6 + X^4 + X^2 + X + 1
        F_{2^128} = GF(2)[Y]/Y^128 + Y^7 + Y^2 + Y + 1
        '''
        if isinstance(n, int):
            b0, b1, b2, b3, b4, b5, b6, b7 = [cgf2n((n >> i) & 0x1) for i in range(8)]
        else:
            b0, b1, b2, b3, b4, b5, b6, b7 = n
        assert [b0.size] * 7 == [b1.size, b2.size, b3.size, b4.size, b5.size, b6.size, b7.size]
        b = [b0, b1, b2, b3, b4, b5, b6, b7]
        a = 0
        for i in range(128):
            a += 2**i * sum(( bj for j,bj in enumerate(b) if forward_embedding_matrix[i][j] == 1))
        return a

    def cembed8(n):
        assert isinstance(n, int)
        b = [(n >> i) & 0x1 for i in range(8)]
        a = 0
        for i in range(128):
            a += 2**i * functools.reduce(operator.xor, (bj for j,bj in enumerate(b) if forward_embedding_matrix[i][j] == 1), 0)
        return a

    def bit_decompose4(x):
        '''
        Computes the inverse embedding F_{2^4} -> F_{2^128} via ?
        F_{2^4} = GF(2)[X]/X^4 + X^3 + X^2 + X + 1
        F_{2^40} = GF(2)[Y]/Y^128 + Y^7 + Y^2 + Y + 1
        not implemented
        '''
        raise NotImplemented

    @vectorize
    def bit_decompose_embedding(x, wanted_positions):
        if isinstance(x, cgf2n):
            max_pos = max(wanted_positions)
            bits = x.bit_decompose(max_pos+1)
            return [bits[pos] for pos in wanted_positions]

        random_bits = [x.get_random_bit() \
                           for i in range(len(wanted_positions))]
        one = cgf2n(1)
        masked = sum([b * (one << wanted_positions[i]) for i,b in enumerate(random_bits)], x).reveal()
        return [x.clear_type((masked >> wanted_positions[i]) & one) + r for i,r in enumerate(random_bits)]

    def bit_decompose8(x):
        '''
        Computes the inverse embedding F_{2^8} -> F_{2^128} via y^124 + y^119 + y^116 + y^115
         + y^113 + y^108 + y^106 + y^103 + y^101 + y^100 + y^96 + y^94 + y^93
         + y^91 + y^90 + y^89 + y^88 + y^81 + y^79 + y^78 + y^76 + y^75 + y^74
         + y^72 + y^71 + y^70 + y^65 + y^64 + y^57 + y^56 + y^55 + y^54 + y^53
         + y^47 + y^46 + y^43 + y^35 + y^32 + y^29 + y^26 + y^25 + y^23 + y^22
         + y^20 + y^19 + y^18 + y^16 + y^14 + y^13 + y^12 + y^11 + y^9 + y^4 + 1
        F_{2^8} = GF(2)[X]/X^8 + X^7 + X^6 + X^4 + X^2 + X + 1
        F_{2^128} = GF(2)[Y]/Y^128 + Y^7 + Y^2 + Y + 1
        '''
        b = bit_decompose_embedding(x, [0,1,2,4,5,6,8,12])
        #                               0 1 2 3 4 5 6 7
        a3 = b[5]
        a7 = a3 + b[2]
        a6 = a3 + a7 + b[4]
        a2 = b[6] + a7
        a4 = a2 + a3 + b[1]
        a1 = b[3] + a2 + a4
        a5 = a1 + a2 + b[7] + a7
        a0 = b[0] + a1 + a3 + a4 + a5
        return [a0, a1, a2, a3, a4, a5, a6, a7]
else:
    def embed4(n):
        '''
        Computes embedding F_{2^4} -> F_{2^40} via Y^15 + Y^5
        F_{2^4} = GF(2)[X]/X^4 + X^3 + X^2 + X + 1
        F_{2^40} = GF(2)[Y]/Y^40 + Y^20 + Y^15 + Y^10 + 1
        This embedding requires 1 addition
        '''
        assert len(n) == 4
        b0, b1, b2, b3 = n
        assert [b0.size] * 3 == [b1.size, b2.size, b3.size]
        return b0 + sum((b * x for b,x in zip([b1 + b3, b2, b1, b3, b2, b3], EMBED_POWERS4.get_type(b0.size))))

    def cembed4(n):
        assert isinstance(n, int)
        b = [(n >> i) & 0x1 for i in range(4)]
        return b[0] + sum((b * x for b,x in zip([b[1] ^ b[3], b[2], b[1], b[3], b[2], b[3]], CEMBED_POWERS4)))

    def embed8(n):
        '''
        Computes embedding F_{2^8} -> F_{2^40} via Y^25 + Y^10
        F_{2^8} = GF(2)[X]/X^8 + X^7 + X^6 + X^4 + X^2 + X + 1
        F_{2^40} = GF(2)[Y]/Y^40 + Y^20 + Y^15 + Y^10 + 1
        This embedding requires 12 additions
        '''
        if isinstance(n, int):
            b0, b1, b2, b3, b4, b5, b6, b7 = [cgf2n((n >> i) & 0x1) for i in range(8)]
        else:
            b0, b1, b2, b3, b4, b5, b6, b7 = n
        assert [b0.size] * 7 == [b1.size, b2.size, b3.size, b4.size, b5.size, b6.size, b7.size]
        b56 = b5 + b6
        b456 = b4 + b56
        b35 = b3 + b5
        b12 = b1 + b2
        b126 = b12 + b6
        b124 = b12 + b4
        b27 = b2 + b7
        b34 = b3 + b4
        b03467 = b0 + b34 + b6 + b7
        b347 = b34 + b7
        return b03467 + sum((b * x for b,x in zip([b56, b126, b456, b35, b124, b27, b347], EMBED_POWERS8.get_type(b0.size))))

    def cembed8(n):
        assert isinstance(n, int)
        b = [(n >> i) & 0x1 for i in range(8)]
        b56 = b[5] ^ b[6]
        b456 = b[4] ^ b56
        b35 = b[3] ^ b[5]
        b12 = b[1] ^ b[2]
        b126 = b12 ^ b[6]
        b124 = b12 ^ b[4]
        b27 = b[2] ^ b[7]
        b34 = b[3] ^ b[4]
        b03467 = b[0] ^ b34 ^ b[6] ^ b[7]
        b347 = b34 ^ b[7]
        return b03467 + sum((b * x for b,x in zip([b56, b126, b456, b35, b124, b27, b347], CEMBED_POWERS8)))

    def bit_decompose4(x):
        '''
        Computes the inverse embedding F_{2^4} -> F_{2^40} via Y^15 + Y^5
        F_{2^4} = GF(2)[X]/X^4 + X^3 + X^2 + X + 1
        F_{2^40} = GF(2)[Y]/Y^40 + Y^20 + Y^15 + Y^10 + 1
        This inverse embedding requires 1 addition
        '''
        b0, b5, b10, b15 = x.bit_decompose(bit_length=20, step=5)
        return [b0, b15, b10, b5 + b15]

    def bit_decompose8(x):
        '''
        Computes the inverse embedding F_{2^8} -> F_{2^40} via Y^25 + Y^10
        F_{2^8} = GF(2)[X]/X^8 + X^7 + X^6 + X^4 + X^2 + X + 1
        F_{2^40} = GF(2)[Y]/Y^40 + Y^20 + Y^15 + Y^10 + 1
        This inverse embedding requires 27 additions
        '''
        b0, b1, b2, b3, b4, b5, b6, b7 = x.bit_decompose(bit_length=40, step=5)
        a0 = b0 + b1 + b2 + b3 + b5 + b7
        a1 = b2 + b3 + b4 + b6 + b7
        a2 = b1 + b2 + b4 + b5+ b6 + b7
        a3 = b2 + b3 + b4 + b5
        a4 = b1 + b3
        a5 = b2 + b3 + b5
        a6 = b1 + b2 + b3 + b5
        a7 = b1 + b2 + b4 + b5 + b7
        return [a0, a1, a2, a3, a4, a5, a6, a7]

def square4(x):
    """
    Computes the square of the decomposed field element in F_{2^4}
    F_{2^4} = GF(2)[X]/X^4 + X^3 + X^2 + X + 1
    Squaring is linear
    """
    b0,b1,b2,b3 = x
    return [b0+b2, b2+b3, b1+b2, b2]

def compute_polynomial(coeffs, xpowers):
    assert len(coeffs) == len(xpowers) + 1
    return coeffs[0] + sum([c * x for c, x in zip(coeffs[1:], xpowers)])

def _s4_sbox(cell, ONE):
    b0, b1, b2, b3 = bit_decompose4(cell)
    not_b2 = b2 + ONE
    not_b1 = b1 + ONE
    b3_ = b0 + ((b3 + ONE) * not_b2)
    b2_ = b3 + (not_b2 * not_b1)
    not_b3_ = b3_ + ONE
    b1_ = b2 + (not_b1 * not_b3_)
    b0_ = b1 + (not_b3_ * (b2_ + ONE))
    return embed4([b0_, b1_, b2_, b3_])

def _s8_sbox(cell, ONE):
    b0,b1,b2,b3,b4,b5,b6,b7 = bit_decompose8(cell)
    s6 = b4 + ((b7 + ONE) * (b6 + ONE))
    not_b3 = b3 + ONE
    not_b2 = b2 + ONE
    s5 = b0 + (not_b3 * not_b2)
    s2 = b6 + (not_b2 * (b1 + ONE))
    not_s5 = s5 + ONE
    not_s6 = s6 + ONE
    s3 = b1 + (not_s5 * not_b3)
    s7 = b5 + (not_s6 * not_s5)
    not_s7 = s7 + ONE
    s4 = b3 + (not_s7 * not_s6)
    s1 = b7 + (not_s7 * (s2 + ONE))
    s0 = b2 + ((s3 + ONE) * (s1 + ONE))
    return embed8([s0,s1,s2,s3,s4,s5,s6,s7])

def _s4_sbox_inv(cell, vector_size):
    cellbits = bit_decompose4(cell)
    cellbits2 = square4(cellbits)
    cellbits4 = square4(cellbits2)
    cellbits8 = square4(cellbits4)

    cell2 = embed4(cellbits2)
    cell4 = embed4(cellbits4)
    cell8 = embed4(cellbits8)

    powers = [cell, cell2, cell4, cell8]
    p00 = compute_polynomial(P00.get_type(vector_size), powers)
    q00 = compute_polynomial(Q00.get_type(vector_size), powers)
    p01 = compute_polynomial(P01.get_type(vector_size), powers)
    p10 = compute_polynomial(P10.get_type(vector_size), powers)
    q10 = compute_polynomial(Q10.get_type(vector_size), powers)
    p11 = compute_polynomial(P11.get_type(vector_size), powers)
    u00 = compute_polynomial(U00.get_type(vector_size), powers)
    v00 = compute_polynomial(V00.get_type(vector_size), powers)
    u01 = compute_polynomial(U01.get_type(vector_size), powers)

    p0 = p00 * q00 + p01
    p1 = p10 * q10 + p11
    q0 = u00 * v00 + u01
    res = p0 * q0 + p1
    return res

def _s8_sbox_inv(cell, ONE):
    b0,b1,b2,b3,b4,b5,b6,b7 = bit_decompose8(cell)
    res = [None] * 8
    not_b5 = b5 + ONE
    not_b6 = b6 + ONE
    not_b7 = b7 + ONE
    s2 = b0 + ((b1 + ONE) * (b3 + ONE))
    s3 = b4 + (not_b6 * not_b7)
    s5 = b7 + (not_b5 * not_b6)
    s7 = b1 + ((b2 + ONE) * not_b7)
    not_s2 = s2 + ONE
    not_s3 = s3 + ONE
    s0 = b5 + (not_s2 * not_s3)
    s1 = b3 + (not_s3 * not_b5)
    s6 = b2 + ((s1 + ONE) * not_s2)
    s4 = b6 + ((s6 + ONE) * (s7 + ONE))
    e = embed8([s0,s1,s2,s3,s4,s5,s6,s7])
    return e

class ForkSkinny(ForkSkinnyBase):
    def __init__(self, variant, vector_size, thread_fork=True):
        if variant == ForkSkinnyBase.FORKSKINNY_64_192:
            cellsize = 4
        elif variant == ForkSkinnyBase.FORKSKINNY_128_256 or variant == ForkSkinnyBase.FORKSKINNY_128_384:
            cellsize = 8
        else:
            raise NotImplemented
        super().__init__(cellsize, variant, vector_size)
        if cellsize == 4:
            one = cembed4(0x1)
        elif cellsize == 8:
            one = cembed8(0x1)
        else:
            raise NotImplemented
        self.ONE = VectorConstant(lambda n: cgf2n(one, size=n))
        self.thread_fork = thread_fork

    def _embed_cell(self, cell):
        if self.cellsize == 4:
            return embed4(cell)
        elif self.cellsize == 8:
            return embed8(cell)
        else:
            raise NotImplemented

    def _xor_cell(self, a, b):
        return a+b

    def add_round_constants(self, state, r, has_tweak):
        assert(len(state) == 16)
        if self.cellsize == 4:
            round_constants = ROUND_CONSTANTS4_EMBEDDED.get_type(self.vector_size)
        elif self.cellsize == 8:
            round_constants = ROUND_CONSTANTS8_EMBEDDED.get_type(self.vector_size)
        else:
            raise NotImplemented
        c_0, c_1, c_2 = round_constants[r]
        state[0] += c_0
        if has_tweak:
            # s_2 xor 0x2 if tweak
            if self.cellsize == 4:
                state[2] += RC2_4_EMBEDDED.get_type(self.vector_size)
            elif self.cellsize == 8:
                state[2] += RC2_8_EMBEDDED.get_type(self.vector_size)
            else:
                raise NotImplemented
        state[4] += c_1
        state[8] += c_2
        return state

    def add_branch_constant(self, state):
        if self.cellsize == 4:
            bc = BC4.get_type(self.vector_size)
        elif self.cellsize == 8:
            bc = BC8.get_type(self.vector_size)
        else:
            raise NotImplemented
        assert len(state) == len(bc)
        return [s + c for s,c in zip(state, bc)]

    def s4_sbox(self, cell):
        ONE = self.ONE.get_type(self.vector_size)
        return _s4_sbox(cell, ONE)

    def s8_sbox(self, cell):
        ONE = self.ONE.get_type(self.vector_size)
        return _s8_sbox(cell, ONE)

    def s4_sbox_inv(self, cell):
        return _s4_sbox_inv(cell, self.vector_size)

    def s8_sbox_inv(self, cell):
        ONE = self.ONE.get_type(self.vector_size)
        return _s8_sbox_inv(cell, ONE)

    def message_leg(self, r_init, message_array, tweakey_schedule, tk_len, sbox, has_tweak):
        message = [message_array[i][:] for i in range(16)]
        tweakey_schedule = [[[tweakey_schedule[r*tk_len*16 + tki*16 + i] for i in range(16)] for tki in range(tk_len)] for r in range(r_init)]
        for r in reversed(range(r_init)):
            message = self.skinny_round_dec(message, tweakey_schedule[r], r, sbox, has_tweak)
        for i in range(16):
            message_array[regint(i)][:] = message[i]

    def forkskinny_dec(self, state, tweakey_schedule, s, b):
        assert s in ['i', 'o', 'b']
        r_init, r_0, r_1 = self.rounds
        has_tweak = True
        assert len(tweakey_schedule) == r_init + r_0 + r_1
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
            if s=='b' and self.thread_fork:
                message_array = sgf2n.Matrix(len(state), len(state[0]))
                for i in range(16):
                    message_array[i][:] = state[i]
                tk_len = len(tweakey_schedule[0])
                tweakey_schedule_array = sgf2n.Matrix(r_init * tk_len * 16, len(state[0]))
                for r in range(r_init):
                    for tki in range(tk_len):
                        for i in range(16):
                            tweakey_schedule_array[r*tk_len*16 + tki * 16 + i][:] = tweakey_schedule[r][tki][i]
                program = get_program()
                message_leg_tape = program.new_tape(self.message_leg, args=[r_init, message_array, tweakey_schedule_array, tk_len, sbox, has_tweak])
                message_leg_thread = program.run_tapes([message_leg_tape])[0]
            else:
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
            if self.thread_fork:
                program.join_tapes([message_leg_thread])
                message = [message_array[i][:] for i in range(16)]
            return message, ciphertext
        else:
            raise NotImplemented

    def forkskinny_invert_dec(self, state, tweakey_schedule, s, b):
        assert b == '0'
        assert s in ['i', 'o', 'b']
        r_init, r_0, r_1 = self.rounds
        has_tweak = True
        assert len(tweakey_schedule) == r_init + r_0 + r_1
        if self.cellsize == 4:
            sbox = self.s4_sbox_inv
        elif self.cellsize == 8:
            sbox = self.s8_sbox_inv
        else:
            raise NotImplemented
        state = self.deep_copy(state)
        tweakey_schedule = self.deep_copy(tweakey_schedule)
        if b == '0':
            # for r in reversed(range(r_init + r_1, r_init + r_0 + r_1)):
            for r in range(r_init):
                state = self.skinny_round_enc(state, tweakey_schedule[r], r, sbox, has_tweak)
            state = self.add_branch_constant(state)
        else:
            raise NotImplemented

        if s == 'i' or s == 'b':
            if s=='b' and self.thread_fork:
                assert False # TODO
                message_array = sgf2n.Matrix(len(state), len(state[0]))
                for i in range(16):
                    message_array[i][:] = state[i]
                tk_len = len(tweakey_schedule[0])
                tweakey_schedule_array = sgf2n.Matrix(r_init * tk_len * 16, len(state[0]))
                for r in range(r_init):
                    for tki in range(tk_len):
                        for i in range(16):
                            tweakey_schedule_array[r*tk_len*16 + tki * 16 + i][:] = tweakey_schedule[r][tki][i]
                program = get_program()
                message_leg_tape = program.new_tape(self.message_leg, args=[r_init, message_array, tweakey_schedule_array, tk_len, sbox, has_tweak])
                message_leg_thread = program.run_tapes([message_leg_tape])[0]
            else:
                message = self.deep_copy(state)
                for r in range(r_init, r_init + r_0):
                    message = self.skinny_round_enc(message, tweakey_schedule[r], r, sbox, has_tweak)
        if s == 'o' or s == 'b':
            round_range = reversed(range(r_init + r_0, r_init + r_0 + r_1)) #if b == '0' else range(r_init + r_1, r_init + r_0 + r_1)
            ciphertext = state
            if self.cellsize == 4:
                sbox = self.s4_sbox
            elif self.cellsize == 8:
                sbox = self.s8_sbox
            else:
                raise NotImplemented

            for r in round_range:
                ciphertext = self.skinny_round_dec(ciphertext, tweakey_schedule[r], r, sbox, has_tweak)
        if s == 'i':
            return message
        elif s == 'o':
            return ciphertext
        elif s == 'b':
            if self.thread_fork:
                program.join_tapes([message_leg_thread])
                message = [message_array[i][:] for i in range(16)]
            return message, ciphertext
        else:
            raise NotImplemented

class Skinny(SkinnyBase):
    def __init__(self, variant, vector_size):
        super().__init__(variant)
        if self.cellsize == 4:
            one = cembed4(0x1)
        elif self.cellsize == 8:
            one = cembed8(0x1)
        else:
            raise NotImplemented
        self.ONE = VectorConstant(lambda n: cgf2n(one, size=n))
        self.vector_size = vector_size

    def _embed_cell(self, cell):
        if self.cellsize == 4:
            return embed4(cell)
        elif self.cellsize == 8:
            return embed8(cell)
        else:
            raise NotImplemented

    def _xor_cell(self, a, b):
        return a+b

    def s4_sbox(self, cell):
        ONE = self.ONE.get_type(self.vector_size)
        return _s4_sbox(cell, ONE)

    def s8_sbox(self, cell):
        ONE = self.ONE.get_type(self.vector_size)
        return _s8_sbox(cell, ONE)

    def s4_sbox_inv(self, cell):
        return _s4_sbox_inv(cell, self.vector_size)

    def s8_sbox_inv(self, cell):
        ONE = self.ONE.get_type(self.vector_size)
        return _s8_sbox_inv(cell, ONE)

    def add_round_constants(self, state, r, has_tweak):
        assert(len(state) == 16)
        if self.cellsize == 4:
            round_constants = SKINNY_ROUND_CONSTANTS4_EMBEDDED.get_type(self.vector_size)
        elif self.cellsize == 8:
            round_constants = SKINNY_ROUND_CONSTANTS8_EMBEDDED.get_type(self.vector_size)
        else:
            raise NotImplemented
        c_0, c_1, c_2 = round_constants[r]
        state[0] += c_0
        state[4] += c_1
        state[8] += c_2
        return state
