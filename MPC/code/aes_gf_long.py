from copy import copy
from Compiler.types import VectorArray, cgf2n, sgf2n

"""
This AES-GCM implementation is adapted to work when gf2n_long is used, ie GF(2^128).
The embeddings are different.
We embed the AES field into the GHASH field (x^128 + x^7 + x^2 + x + 1) via
y^123 + y^122 + y^120 + y^119 + y^118 + y^115 +y^113 + y^112 + y^109 + y^108 + y^106
+ y^105 + y^102 + y^98 + y^97 + y^94 + y^87 + y^85 + y^81 + y^77 + y^73 + y^71 + y^70
+ y^69 + y^68 + y^67 + y^66 + y^65 + y^62 + y^61 + y^59 + y^57 + y^56 + y^55 + y^49
+ y^48 + y^45 + y^44 + y^38 + y^35 + y^29 + y^28 + y^27 + y^26 + y^21 + y^19 + y^18
+ y^17 + y^15 + y^12 + y^11 + y^6 + y^3 + 1
"""

rcon_raw = [
        0x8d, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36, 0x6c, 0xd8, 0xab, 0x4d, 0x9a,
        0x2f, 0x5e, 0xbc, 0x63, 0xc6, 0x97, 0x35, 0x6a, 0xd4, 0xb3, 0x7d, 0xfa, 0xef, 0xc5, 0x91, 0x39,
        0x72, 0xe4, 0xd3, 0xbd, 0x61, 0xc2, 0x9f, 0x25, 0x4a, 0x94, 0x33, 0x66, 0xcc, 0x83, 0x1d, 0x3a,
        0x74, 0xe8, 0xcb, 0x8d, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36, 0x6c, 0xd8,
        0xab, 0x4d, 0x9a, 0x2f, 0x5e, 0xbc, 0x63, 0xc6, 0x97, 0x35, 0x6a, 0xd4, 0xb3, 0x7d, 0xfa, 0xef,
        0xc5, 0x91, 0x39, 0x72, 0xe4, 0xd3, 0xbd, 0x61, 0xc2, 0x9f, 0x25, 0x4a, 0x94, 0x33, 0x66, 0xcc,
        0x83, 0x1d, 0x3a, 0x74, 0xe8, 0xcb, 0x8d, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b,
        0x36, 0x6c, 0xd8, 0xab, 0x4d, 0x9a, 0x2f, 0x5e, 0xbc, 0x63, 0xc6, 0x97, 0x35, 0x6a, 0xd4, 0xb3,
        0x7d, 0xfa, 0xef, 0xc5, 0x91, 0x39, 0x72, 0xe4, 0xd3, 0xbd, 0x61, 0xc2, 0x9f, 0x25, 0x4a, 0x94,
        0x33, 0x66, 0xcc, 0x83, 0x1d, 0x3a, 0x74, 0xe8, 0xcb, 0x8d, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20,
        0x40, 0x80, 0x1b, 0x36, 0x6c, 0xd8, 0xab, 0x4d, 0x9a, 0x2f, 0x5e, 0xbc, 0x63, 0xc6, 0x97, 0x35,
        0x6a, 0xd4, 0xb3, 0x7d, 0xfa, 0xef, 0xc5, 0x91, 0x39, 0x72, 0xe4, 0xd3, 0xbd, 0x61, 0xc2, 0x9f,
        0x25, 0x4a, 0x94, 0x33, 0x66, 0xcc, 0x83, 0x1d, 0x3a, 0x74, 0xe8, 0xcb, 0x8d, 0x01, 0x02, 0x04,
        0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36, 0x6c, 0xd8, 0xab, 0x4d, 0x9a, 0x2f, 0x5e, 0xbc, 0x63,
        0xc6, 0x97, 0x35, 0x6a, 0xd4, 0xb3, 0x7d, 0xfa, 0xef, 0xc5, 0x91, 0x39, 0x72, 0xe4, 0xd3, 0xbd,
        0x61, 0xc2, 0x9f, 0x25, 0x4a, 0x94, 0x33, 0x66, 0xcc, 0x83, 0x1d, 0x3a, 0x74, 0xe8, 0xcb
]

forward_embedding_matrix = [
    [1,1,1,0,1,0,1,0],
    [0,0,0,0,1,0,0,1],
    [0,0,1,1,1,0,1,1],
    [0,1,1,1,1,1,1,1],
    [0,0,0,0,0,1,0,1],
    [0,0,1,1,1,0,0,1],
    [0,1,1,0,0,0,1,1],
    [0,0,1,1,0,0,0,0],
    [0,0,0,1,1,1,1,0],
    [0,0,0,1,0,0,0,0],
    [0,0,0,0,0,0,1,0],
    [0,1,0,0,0,0,1,1],
    [0,1,1,1,0,0,0,0],
    [0,0,0,1,0,0,1,1],
    [0,0,0,1,1,0,0,0],
    [0,1,0,1,0,0,0,1],
    [0,0,1,0,0,0,1,1],
    [0,1,1,0,1,0,1,1],
    [0,1,1,1,1,0,0,1],
    [0,1,0,0,1,0,0,1],
    [0,0,1,1,1,1,0,1],
    [0,1,1,1,0,1,1,0],
    [0,0,1,1,1,1,1,0],
    [0,0,0,0,1,1,0,1],
    [0,0,1,1,0,0,1,0],
    [0,0,1,0,1,0,1,0],
    [0,1,1,0,1,1,0,0],
    [0,1,1,1,1,1,0,0],
    [0,1,1,1,0,1,1,1],
    [0,1,0,1,0,1,1,1],
    [0,0,1,1,1,0,1,1],
    [0,0,0,1,0,1,1,1],
    [0,0,0,1,1,0,1,1],
    [0,0,1,0,1,1,1,0],
    [0,0,0,0,1,0,1,0],
    [0,1,1,0,1,1,0,0],
    [0,0,0,1,0,1,1,1],
    [0,0,0,1,1,1,0,1],
    [0,1,1,0,1,0,1,0],
    [0,0,0,1,0,1,0,1],
    [0,0,0,1,1,1,1,1],
    [0,0,1,0,0,1,1,0],
    [0,0,0,1,0,1,1,1],
    [0,0,1,1,0,1,1,0],
    [0,1,1,0,1,0,1,0],
    [0,1,0,1,1,0,0,1],
    [0,0,1,0,1,0,0,0],
    [0,0,1,0,0,0,0,0],
    [0,1,1,1,1,1,1,0],
    [0,1,1,0,1,0,0,1],
    [0,0,0,0,1,0,1,1],
    [0,0,0,0,1,0,1,0],
    [0,0,1,0,0,1,0,0],
    [0,0,1,1,1,1,1,1],
    [0,0,1,0,0,0,1,0],
    [0,1,0,1,0,1,1,0],
    [0,1,1,1,0,0,1,1],
    [0,1,0,0,1,0,0,0],
    [0,0,1,0,1,1,1,0],
    [0,1,0,0,1,1,0,0],
    [0,0,1,0,1,1,1,1],
    [0,1,1,0,0,0,0,0],
    [0,1,1,1,1,1,0,1],
    [0,0,0,0,0,0,1,0],
    [0,0,0,1,1,0,1,1],
    [0,1,0,1,0,1,1,0],
    [0,1,1,1,1,1,1,0],
    [0,1,0,1,0,0,0,0],
    [0,1,0,1,1,0,0,1],
    [0,1,1,0,0,0,1,0],
    [0,1,0,0,1,0,0,0],
    [0,1,0,1,1,1,1,0],
    [0,0,0,1,0,0,1,1],
    [0,1,1,0,1,0,0,1],
    [0,0,0,0,1,1,0,0],
    [0,0,1,1,1,1,1,1],
    [0,0,0,1,0,0,0,0],
    [0,1,1,0,0,0,1,1],
    [0,0,1,0,1,0,1,0],
    [0,0,0,0,1,1,1,0],
    [0,0,0,1,1,1,0,0],
    [0,1,0,0,1,0,1,0],
    [0,0,1,0,1,1,0,1],
    [0,0,0,1,0,1,1,0],
    [0,0,0,1,1,1,0,0],
    [0,1,1,1,0,1,0,0],
    [0,0,1,1,1,0,0,0],
    [0,1,0,1,1,0,0,0],
    [0,0,0,1,0,0,0,1],
    [0,0,0,0,0,1,0,1],
    [0,0,1,0,1,0,1,1],
    [0,0,0,0,0,1,1,1],
    [0,0,1,0,0,1,1,1],
    [0,0,0,0,1,1,1,1],
    [0,1,0,0,1,0,0,0],
    [0,0,1,1,0,0,1,1],
    [0,0,0,1,1,0,0,1],
    [0,1,0,0,1,1,0,0],
    [0,1,1,0,1,0,0,0],
    [0,0,1,0,0,1,1,0],
    [0,0,1,0,1,0,0,1],
    [0,0,0,1,1,0,1,1],
    [0,1,1,1,1,1,1,0],
    [0,0,0,1,0,0,1,0],
    [0,0,1,0,1,1,0,1],
    [0,1,1,0,0,0,1,1],
    [0,1,0,1,0,1,0,0],
    [0,0,0,0,0,0,0,0],
    [0,1,1,0,0,1,1,0],
    [0,1,0,0,0,1,0,1],
    [0,0,1,1,1,0,0,0],
    [0,0,1,1,1,0,1,0],
    [0,1,1,0,0,0,0,1],
    [0,1,1,0,1,1,1,0],
    [0,0,0,0,1,0,1,1],
    [0,1,1,1,1,0,1,0],
    [0,0,1,1,0,1,1,0],
    [0,0,0,0,1,0,1,1],
    [0,1,1,1,0,1,0,0],
    [0,1,0,0,0,0,0,0],
    [0,1,1,1,1,1,1,1],
    [0,0,0,0,1,0,0,1],
    [0,1,1,1,1,1,1,0],
    [0,1,1,1,0,1,0,0],
    [0,0,1,0,0,0,1,1],
    [0,0,1,1,1,0,0,0],
    [0,0,0,1,0,1,1,0],
    [0,0,0,0,1,0,0,1]
]

_embedded_powers = [
    [0x1, 0x3d5bd35c94646a247573da4a5f7710ed, 0xa72ec17764d7ced55e2f716f4ede412f, 0x553e92e8bc0ae9a795ed1f57f3632d4d, 0xc7bd33d0a58cf5b4740d6c968b842acb, 0x486fb01f93aa169afd8ee716e990cf14, 0xbee4e4dc44629cf627b537f28935c282, 0x549810e11a88dea5252b49277b1b82b4],
    [0x1, 0xa72ec17764d7ced55e2f716f4ede412f, 0xc7bd33d0a58cf5b4740d6c968b842acb, 0xbee4e4dc44629cf627b537f28935c282, 0xafd872648de276379493a98b2790176a, 0x49b075c0f15ad1e11f9bedcdd1861f4, 0x7492e14aa14c4bbc383b6b2c3e9f7001, 0xfb406285976aa892b1b8e0ac5c8b95df],
    [0x1, 0xc7bd33d0a58cf5b4740d6c968b842acb, 0xafd872648de276379493a98b2790176a, 0x7492e14aa14c4bbc383b6b2c3e9f7001, 0xb61257cfad572414ed09ef16e07b94c6, 0x950311a4fb78fe07a7a8e94e136f9bc, 0xe6114072b8ca57afd9db18ed46787787, 0x4d52354a3a3d8c865cb10fbabcf00118],
    [0x1, 0xafd872648de276379493a98b2790176a, 0xb61257cfad572414ed09ef16e07b94c6, 0xe6114072b8ca57afd9db18ed46787787, 0x53d8555a9979a1ca13fe8ac5560ce0d, 0x340be246dbd3e5c40f0954debe41e950, 0xf72dd6ca714abd6e6afd8694e8dda26f, 0x486fb01f93aa169afd8ee716e990cf14],
    [0x1, 0xb61257cfad572414ed09ef16e07b94c6, 0x53d8555a9979a1ca13fe8ac5560ce0d, 0xf72dd6ca714abd6e6afd8694e8dda26f, 0x4cf4b7439cbfbb84ec7759ca3488aee1, 0x93252331bf042b11512625b1f09fa87e, 0x35ad604f7d51d2c6bfcf02ae363946a8, 0x49b075c0f15ad1e11f9bedcdd1861f4],
    [0x1, 0x53d8555a9979a1ca13fe8ac5560ce0d, 0x4cf4b7439cbfbb84ec7759ca3488aee1, 0x35ad604f7d51d2c6bfcf02ae363946a8, 0xdcb364640a222fe6b8330483c2e9849, 0x549810e11a88dea5252b49277b1b82b4, 0xd681a5686c0c1f75c72bf2ef2521ff22, 0x950311a4fb78fe07a7a8e94e136f9bc],
    [0x1, 0x4cf4b7439cbfbb84ec7759ca3488aee1, 0xdcb364640a222fe6b8330483c2e9849, 0xd681a5686c0c1f75c72bf2ef2521ff22, 0x3d5bd35c94646a247573da4a5f7710ed, 0xfb406285976aa892b1b8e0ac5c8b95df, 0x6d58c4e181f9199f41a12db1f974f3ac, 0x340be246dbd3e5c40f0954debe41e950],
]

class SpdzBox(object):
    def init_matrices(self):
        self.matrix_inv = [
            [0,0,1,0,0,1,0,1],
            [1,0,0,1,0,0,1,0],
            [0,1,0,0,1,0,0,1],
            [1,0,1,0,0,1,0,0],
            [0,1,0,1,0,0,1,0],
            [0,0,1,0,1,0,0,1],
            [1,0,0,1,0,1,0,0],
            [0,1,0,0,1,0,1,0]
        ]
        to_add = [1,0,1,0,0,0,0,0]
        self.addition_inv = [cgf2n(_,size=self.aes.nparallel) for _ in to_add]
        self.forward_matrix = [
            [1,0,0,0,1,1,1,1],
            [1,1,0,0,0,1,1,1],
            [1,1,1,0,0,0,1,1],
            [1,1,1,1,0,0,0,1],
            [1,1,1,1,1,0,0,0],
            [0,1,1,1,1,1,0,0],
            [0,0,1,1,1,1,1,0],
            [0,0,0,1,1,1,1,1]
        ]
        forward_add = [1,1,0,0,0,1,1,0]
        self.forward_add = VectorArray(len(forward_add), cgf2n, self.aes.nparallel)
        for i,x in enumerate(forward_add):
            self.forward_add[i] = cgf2n(x, size=self.aes.nparallel)
        self.K01 = VectorArray(8, cgf2n, self.aes.nparallel)
        for idx in range(8):
            self.K01[idx] = self.aes.ApplyBDEmbedding([0,1]) ** idx

    def __init__(self, aes):
        self.aes = aes
        
        #constants = [
        #    0x63, 0x8F, 0xB5, 0x01, 0xF4, 0x25, 0xF9, 0x09, 0x05
        #]
        #self.powers = [
        #    0, 127, 191, 223, 239, 247, 251, 253, 254
        #]
        #self.constants = [self.aes.ApplyEmbedding(cgf2n(_,size=self.aes.nparallel)) for _ in constants]
        self.init_matrices()

    def forward_bit_sbox(self, emb_byte):
        emb_byte_inverse = self.aes.inverseMod(emb_byte)
        unembedded_x = self.aes.InverseBDEmbedding(emb_byte_inverse)

        linear_transform = list()
        for row in self.forward_matrix:
            result = cgf2n(0, size=self.aes.nparallel)
            for idx in range(len(row)):
                result = result + unembedded_x[idx] * row[idx]
            linear_transform.append(result)

        #do the sum(linear_transform + additive_layer)
        summation_bd = [0 for _ in range(8)]
        for idx in range(8):
            summation_bd[idx] = linear_transform[idx] + self.forward_add[idx]

        #Now raise this to power of 254
        result = cgf2n(0,size=self.aes.nparallel)
        for idx in range(8):
            result += self.aes.ApplyBDEmbedding([summation_bd[idx]]) * self.K01[idx];
        return result

    def apply_sbox(self, what):
        #applying with the multiplicative chain
        return self.forward_bit_sbox(what)
    
    def backward_bit_sbox(self, emb_byte):
        unembedded_x = self.aes.InverseBDEmbedding(emb_byte)
        # invert additive layer
        unembedded_x = [x - c for x,c in zip(unembedded_x, self.forward_add)]
        # invert linear transform
        linear_transform = list()
        for row in self.matrix_inv:
            result = cgf2n(0, size=self.aes.nparallel)
            for idx in range(len(row)):
                result = result + unembedded_x[idx] * row[idx]
            linear_transform.append(result)
        return self.aes.inverseMod(linear_transform) #self.aes.inverseMod(self.aes.embed_helper(linear_transform))

class Aes128:
    
    def __init__(self, nparallel):
        self.nparallel = nparallel
        self.rcon = VectorArray(len(rcon_raw), cgf2n, nparallel)
        for idx in range(len(rcon_raw)):
            self.rcon[idx] = cgf2n(rcon_raw[idx],size=nparallel)

        self.powers2 = VectorArray(len(forward_embedding_matrix), cgf2n, nparallel)
        for idx in range(len(forward_embedding_matrix)):
            self.powers2[idx] = cgf2n(2,size=nparallel) ** idx
        
        # mixColumn takes a column and does stuff

        self.Kv = VectorArray(4, cgf2n, nparallel)
        self.Kv[1] = self.ApplyEmbedding(cgf2n(1,size=nparallel))
        self.Kv[2] = self.ApplyEmbedding(cgf2n(2,size=nparallel))
        self.Kv[3] = self.ApplyEmbedding(cgf2n(3,size=nparallel))
        self.Kv[4] = self.ApplyEmbedding(cgf2n(4,size=nparallel))
        
        self.InvMixColKv = [None] * 4
        self.InvMixColKv[0] = self.ApplyEmbedding(cgf2n(0xe,size=nparallel))
        self.InvMixColKv[1] = self.ApplyEmbedding(cgf2n(0xb,size=nparallel))
        self.InvMixColKv[2] = self.ApplyEmbedding(cgf2n(0xd,size=nparallel))
        self.InvMixColKv[3] = self.ApplyEmbedding(cgf2n(0x9,size=nparallel))
        
        self.enum_squarings = VectorArray(8 * len(_embedded_powers), cgf2n, nparallel)
        for i,_list in enumerate(_embedded_powers):
            for j,x in enumerate(_list):
                self.enum_squarings[len(_list) * i + j] = cgf2n(x, size=nparallel)
        
        self.box = SpdzBox(self)
    
    def ApplyEmbedding(self, x):
        in_bytes = x.bit_decompose(8)

        out_bytes = [cgf2n(0, size=self.nparallel) for _ in range(len(forward_embedding_matrix))]
        for i in range(len(forward_embedding_matrix)):
            out_bytes[i] = sum([b for j,b in enumerate(in_bytes) if forward_embedding_matrix[i][j] == 1])

        return sum(self.powers2[idx] * out_bytes[idx] for idx in range(len(forward_embedding_matrix)))
    
    def embed_helper(self, in_bytes):
        out_bytes = [None] * len(forward_embedding_matrix)
        for i in range(len(forward_embedding_matrix)):
            out_bytes[i] = sum([b for j,b in enumerate(in_bytes) if forward_embedding_matrix[i][j] == 1])
        return out_bytes
    
    def ApplyBDEmbedding(self, x):
        entire_sequence_bits = copy(x)

        while len(entire_sequence_bits) < 8:
            entire_sequence_bits.append(0)

        in_bytes = entire_sequence_bits
        out_bytes = self.embed_helper(in_bytes)

        return sum(self.powers2[idx] * out_bytes[idx] for idx in range(len(forward_embedding_matrix)))
    
    def bit_decompose_embedding(self, x, wanted_positions):
        if isinstance(x, cgf2n):
            max_pos = max(wanted_positions)
            bits = x.bit_decompose(max_pos+1)
            return [bits[pos] for pos in wanted_positions]

        random_bits = [x.get_random_bit(size=x.size) \
                           for i in range(len(wanted_positions))]
        one = cgf2n(1, size=self.nparallel)
        masked = sum([b * (one << wanted_positions[i]) for i,b in enumerate(random_bits)], x).reveal()
        return [((masked >> wanted_positions[i]) & one) + r for i,r in enumerate(random_bits)]
    
    def PreprocInverseEmbedding(self, x):
        in_bytes = self.bit_decompose_embedding(x, [0,1,3,4,7,9,10,13])

        out_bytes = [cgf2n(0, size=self.nparallel) for _ in range(8)]

        out_bytes[3] = in_bytes[5]
        out_bytes[6] = in_bytes[6]
        out_bytes[2] = in_bytes[4] + out_bytes[3]
        out_bytes[7] = in_bytes[7] + out_bytes[3] + out_bytes[6]
        out_bytes[4] = in_bytes[1] + out_bytes[7]
        out_bytes[5] = in_bytes[3] + out_bytes[7]
        out_bytes[1] = in_bytes[2] + sum(out_bytes[2:8])
        out_bytes[0] = in_bytes[0] + out_bytes[1] + out_bytes[2] + out_bytes[4] + out_bytes[6]

        return out_bytes
    
    def InverseEmbedding(self, x):
        out_bytes = self.PreprocInverseEmbedding(x)
        ret = cgf2n(0, size=self.nparallel)
        for idx in range(8):
            ret = ret + (cgf2n(2, size=self.nparallel) ** idx) * out_bytes[idx]
        return ret

    def InverseBDEmbedding(self, x):
        return self.PreprocInverseEmbedding(x)
    
    def expandAESKey(self, cipherKey, Nr = 10, Nb = 4, Nk = 4):
        #cipherkey should be in hex
        cipherKeySize = len(cipherKey)

        round_key = [sgf2n(0,size=self.nparallel)] * 176
        temp = [cgf2n(0,size=self.nparallel)] * 4

        for i in range(Nk):
            for j in range(4):
                round_key[4 * i + j] = cipherKey[4 * i + j]

        for i in range(Nk, Nb * (Nr + 1)):
            for j in range(4):
                temp[j] = round_key[(i-1) * 4 + j]
            if i % Nk == 0:
                #rotate the 4 bytes word to the left
                k = temp[0]
                temp[0] = temp[1]
                temp[1] = temp[2]
                temp[2] = temp[3]
                temp[3] = k

                #now substitute word
                temp[0] = self.box.apply_sbox(temp[0])
                temp[1] = self.box.apply_sbox(temp[1])
                temp[2] = self.box.apply_sbox(temp[2])
                temp[3] = self.box.apply_sbox(temp[3])

                temp[0] = temp[0] + self.ApplyEmbedding(self.rcon[int(i//Nk)])

            for j in range(4):
                round_key[4 * i + j] = round_key[4 * (i - Nk) + j] + temp[j]
        return round_key

        #Nr = 10 -> The number of rounds in AES Cipher.
        #Nb = 4 -> The number of columns of the AES state
        #Nk = 4 -> The number of words of a AES key

    def SecretArrayEmbedd(self, byte_array):
        return [self.ApplyEmbedding(_) for _ in byte_array]   

    def subBytes(self, state):
        for i in range(len(state)):
            state[i] = self.box.apply_sbox(state[i])
    
    def invSubBytes(self,state):
        for i in range(len(state)):
            state[i] = self.box.backward_bit_sbox(state[i])
            
    def addRoundKey(self, roundKey):
        def inner(state):
            for i in range(len(state)):
                state[i] = state[i] + roundKey[i]
        return inner

    def mixColumn(self, column):
        temp = copy(column)
        # no multiplication
        doubles = [self.Kv[2] * t for t in temp]
        column[0] = doubles[0] + (temp[1] + doubles[1]) + temp[2] + temp[3]
        column[1] = temp[0] + doubles[1] + (temp[2] + doubles[2]) + temp[3]
        column[2] = temp[0] + temp[1] + doubles[2] + (temp[3] + doubles[3])
        column[3] = (temp[0] + doubles[0]) + temp[1] + temp[2] + doubles[3]
    
    def mixColumns(self, state):
        for i in range(4):
            column = []
            for j in range(4):
                column.append(state[i*4+j])
            self.mixColumn(column)
            for j in range(4):
                state[i*4+j] = column[j]
    
    def invMixColumn(self, column):
        temp = copy(column)
        column[0] = self.InvMixColKv[0] * temp[0] + self.InvMixColKv[1] * temp[1] + self.InvMixColKv[2] * temp[2] + self.InvMixColKv[3] * temp[3]
        column[1] = self.InvMixColKv[3] * temp[0] + self.InvMixColKv[0] * temp[1] + self.InvMixColKv[1] * temp[2] + self.InvMixColKv[2] * temp[3]
        column[2] = self.InvMixColKv[2] * temp[0] + self.InvMixColKv[3] * temp[1] + self.InvMixColKv[0] * temp[2] + self.InvMixColKv[1] * temp[3]
        column[3] = self.InvMixColKv[1] * temp[0] + self.InvMixColKv[2] * temp[1] + self.InvMixColKv[3] * temp[2] + self.InvMixColKv[0] * temp[3]

    def invMixColumns(self,state):
        for i in range(4):
            column = []
            for j in range(4):
                column.append(state[i*4+j])
            self.invMixColumn(column)
            for j in range(4):
                state[i*4+j] = column[j]
    
    def rotate(self, word, n):
        return word[n:]+word[0:n]
    
    def shiftRows(self, state):
        for i in range(4):
            state[i::4] = self.rotate(state[i::4],i)
    
    def invShiftRows(self,state):
        for i in range(4):
            word = state[i::4]
            state[i::4] = word[4-i:] + word[0:4-i]
    
    def state_collapse(self, state):
        return [self.InverseEmbedding(_) for _ in state]
    
    def fancy_squaring(self, bd_val, exponent):
        #This is even more fancy; it performs directly on bit dec values
        #returns x ** (2 ** exp) from a bit decomposed value
        return sum(self.enum_squarings[(exponent-1) * 8 + idx] * bd_val[idx]
                for idx in range(len(bd_val)))
    
    def inverseMod(self, val):
        #embedded now!
        #returns x ** 254 using offline squaring
        #returns an embedded result
        
        if isinstance(val, (sgf2n,cgf2n)):
            bd_val = self.PreprocInverseEmbedding(val)
        else:
            assert isinstance(val, list)
            bd_val = val

        bd_squared = bd_val
        squared_index = 2

        mapper = [0] * 129
        for idx in range(1, 8):
            bd_squared = self.fancy_squaring(bd_val, idx)
            mapper[squared_index] = bd_squared
            squared_index *= 2

        enum_powers = [
            2, 4, 8, 16, 32, 64, 128
        ]

        inverted_product = \
            ((mapper[2] * mapper[4]) * (mapper[8] * mapper[16])) * ((mapper[32] * mapper[64]) * mapper[128])
        return inverted_product
    
    def aesRound(self, roundKey):
        def inner(state):
            self.subBytes(state)
            self.shiftRows(state)
            self.mixColumns(state)
            self.addRoundKey(roundKey)(state)
        return inner
    
    def invAesRound(self,roundKey):
        def inner(state):
            self.addRoundKey(roundKey)(state)
            self.invMixColumns(state)
            self.invShiftRows(state)
            self.invSubBytes(state)
        return inner
    
    # returns a 16-byte round key based on an expanded key and round number
    def createRoundKey(self, expandedKey, n):
        return expandedKey[(n*16):(n*16+16)]
    
    # wrapper function for 10 rounds of AES since we're using a 128-bit key
    def aesMain(self, expandedKey, numRounds=10):
        def inner(state):
            roundKey = self.createRoundKey(expandedKey, 0)
            self.addRoundKey(roundKey)(state)
            for i in range(1, numRounds):

                roundKey = self.createRoundKey(expandedKey, i)
                self.aesRound(roundKey)(state)

            roundKey = self.createRoundKey(expandedKey, numRounds)

            self.subBytes(state)
            self.shiftRows(state)
            self.addRoundKey(roundKey)(state)
        return inner
    
    # wrapper function for 10 rounds of AES since we're using a 128-bit key
    def invAesMain(self, expandedKey, numRounds=10):
        def inner(state):
            roundKey = self.createRoundKey(expandedKey, numRounds)
            self.addRoundKey(roundKey)(state)
            self.invShiftRows(state)
            self.invSubBytes(state)
            
            for i in list(range(1, numRounds))[::-1]:
                roundKey = self.createRoundKey(expandedKey, i)
                self.invAesRound(roundKey)(state)

            roundKey = self.createRoundKey(expandedKey, 0)
            self.addRoundKey(roundKey)(state)
        return inner
    
    def encrypt_without_key_schedule(self, expandedKey):
        def encrypt(plaintext):
            plaintext = self.SecretArrayEmbedd(plaintext)
            self.aesMain(expandedKey)(plaintext)
            return self.state_collapse(plaintext)
        return encrypt
    
    def encrypt_without_key_schedule_no_emb(self, expandedKey):
        def encrypt(plaintext):
            state = copy(plaintext)
            self.aesMain(expandedKey)(state)
            return state
        return encrypt
    
    def decrypt_without_key_schedule(self, expandedKey):
        def decrypt(ciphertext):
            ciphertext = self.SecretArrayEmbedd(ciphertext)
            self.invAesMain(expandedKey)(ciphertext)
            return self.state_collapse(ciphertext)
        return decrypt
    
    def decrypt_without_key_schedule_no_emb(self, expandedKey):
        def decrypt(ciphertext):
            ciphertext = copy(ciphertext)
            self.invAesMain(expandedKey)(ciphertext)
            return ciphertext
        return decrypt


"""
Test Vectors:

plaintext:
6bc1bee22e409f96e93d7e117393172a

key: 
2b7e151628aed2a6abf7158809cf4f3c

resulting cipher
3ad77bb40d7a3660a89ecaf32466ef97 

test_message = "6bc1bee22e409f96e93d7e117393172a"
test_key = "2b7e151628aed2a6abf7158809cf4f3c"

def conv(x):
    return [int(x[i : i + 2], 16) for i in range(0, len(x), 2)]

def single_encryption(nparallel = 1):
    from Compiler.library import print_ln
    key = [sgf2n(x, size=nparallel) for x in conv(test_key)]
    message = [sgf2n(x, size=nparallel) for x in conv(test_message)]

    cipher = Aes128(nparallel)

    key = [cipher.ApplyEmbedding(_) for _ in key]
    expanded_key = cipher.expandAESKey(key)

    AES = cipher.encrypt_without_key_schedule(expanded_key)

    ciphertext = AES(message)

    #for block in ciphertext:
    #    print_ln('%s', block.reveal())

    #invAES = cipher.decrypt_without_key_schedule(expanded_key)
    #decrypted_message = invAES(ciphertext)
    #for block in (decrypted_message):
    #    print_ln('%s', block.reveal())

single_encryption(1)
"""
