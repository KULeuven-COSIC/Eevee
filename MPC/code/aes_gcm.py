from copy import copy
import sys

from Programs.Source.utils import tag_check
from Programs.Source.aes_gf_long import Aes128

from Compiler.types import program, cgf2n, regint

program.bit_length = 128
#BENCHMARK=False

#N = int(sys.argv[2])
#nparallel = int(sys.argv[3])

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



def single_encryption():
    key = [sgf2n(x) for x in conv(test_key)]
    # key[0] = box.apply_sbox(key[0])
    # PreprocInverseEmbedding(key[0])
    message = [sgf2n(x) for x in conv(test_message)]

    cipher = Aes128(nparallel)
    key = [cipher.ApplyEmbedding(_) for _ in key]
    expanded_key = cipher.expandAESKey(key)

    AES = cipher.encrypt_without_key_schedule(expanded_key)

    ciphertext = AES(message)

    for block in ciphertext:
        print_ln('%s', block.reveal())

single_encryption()

"""

def conv(x):
    return [int(x[i : i + 2], 16) for i in range(0, len(x), 2)]

# def print_embedded_buf(buf, msg=''):
#     buf = [InverseEmbedding(b).reveal() for b in buf]
#     s = ' '.join(['%s'] * len(buf))
#     print_ln(f'{msg}{s}', *buf)
#
# def print_buf(buf, msg=''):
#     buf = [b.reveal() for b in buf]
#     s = ' '.join(['%s'] * len(buf))
#     print_ln(f'{msg}{s}', *buf)

def derive_keys(key, nonce):
    assert len(nonce) == 12
    key = [ApplyEmbedding(_) for _ in key]
    # print_embedded_buf(key, 'Derive Key: ')
    expanded_key = expandAESKey(key)
    AES = encrypt_without_key_schedule_no_emb(expanded_key)

    block1 = [ApplyEmbedding(cgf2n(0, size=nparallel))] * 4 + nonce

    auth_key_part1 = AES(block1)[:8]

    block2 = copy(block1)
    block2[0] = ApplyEmbedding(cgf2n(1, size=nparallel))
    auth_key_part2 = AES(block2)[:8]

    block3 = copy(block1)
    block3[0] = ApplyEmbedding(cgf2n(0x2, size=nparallel))
    enc_key_part1 = AES(block3)[:8]

    block4 = copy(block1)
    block4[0] = ApplyEmbedding(cgf2n(3, size=nparallel))
    enc_key_part2 = AES(block4)[:8]
    return enc_key_part1 + enc_key_part2, auth_key_part1 + auth_key_part2

def flatten(l):
    return [lii for li in l for lii in li]
def into_gf2_128_bits(bits):
    assert len(bits) == 128
    bytes = [bits[8*i:8*i+8][::-1] for i in range(16)]
    return sum((2**i * b for i,b in enumerate(flatten(bytes))))
def into_gf2_128(cipher, block):
    """
    The correct GHASH encoding from bitstring to field element is
    b_0, ...., b_127 (bitstring)
    to
    b_7 + b_6 X + b_5 X^2 + ... b_0 X^7 + b_15 X^8 + ...
    i.e. a byte is encoded big endian into coefficients
    """
    assert len(block) <= 16
    if len(block) != 16:
        block = copy(block)
        while len(block) < 16:
            block.append(cgf2n(0, size=cipher.nparallel))
    bits = flatten([cell.bit_decompose(8)[::-1] for cell in block])
    return sum((2**i * b for i,b in enumerate(bits)))

def from_gf2_128(x):
    bits = x.bit_decompose(128)
    bytes = []
    for i in range(16):
        n = 0
        for j in range(8):
            n += 2**(7-j) * bits[8*i+j]
        bytes.append(n)
    return bytes

# def print_f128(x, msg=''):
#     bytes = from_gf2_128(x)
#     print_buf(bytes, msg)

def aes_ctr_dec(cipher, AES, counter, ciphertext):
    # key is embedded, counter and ciphertext is not

    # counter is big endian (bytewise)
    counter_bytes = [t.bit_decompose(8) for t in counter[12:]]
    counter_bytes = [sum((2**i * b for i,b in enumerate(bits))) for bits in counter_bytes]
    counter_block_res = cipher.SecretArrayEmbedd(counter[:12])

    # set MSbit of counter_block[15] to 1
    # tag_bits = tag[15].bit_decompose(7) + [1]
    # counter_block_res.append(ApplyBDEmbedding(tag_bits))

    n_blocks = len(ciphertext) // 16
    if len(ciphertext) % 16 != 0:
        n_blocks += 1
    ctr = regint(sum(2**(3-i) * b  for i,b in enumerate(counter_bytes)))
    # print_ln('Parsed ctr = %s', ctr)
    message = []
    m_ctr = 0

    for i in range(n_blocks):
        # set ctr
        ctr_bits = ((ctr + i) % 2**32).bit_decompose(32)
        ctr_cells = [sum([cgf2n(2**i, size=cipher.nparallel) * b for i,b in enumerate(ctr_bits[8*(3-j):8*(3-j)+8])]) for j in range(32//8)]
        counter_block =  list(counter_block_res) + cipher.SecretArrayEmbedd(ctr_cells)
        # print_embedded_buf(counter_block, "Counter input")
        mask = AES(counter_block)
        # print_embedded_buf(mask, "Counter output")
        for i in range(min(16,len(ciphertext)-m_ctr)):
            message.append(ciphertext[m_ctr] + cipher.InverseEmbedding(mask[i]))
            m_ctr += 1
    return message

def be_uint64(x):
    bits = [1 if ((x >> i) & 0x1) > 0 else 0 for i in range(64)]
    bytes = [bits[8*(7-i):8*(7-i)+8] for i in range(8)]
    return flatten(bytes)

def ghash(cipher, plaintext, key):
    key = into_gf2_128(cipher, cipher.state_collapse(key))
    # print_f128(key, 'F128 key: ')
    bytelen = len(plaintext)
    tag = cgf2n(0, size=cipher.nparallel)
    n_blocks = len(plaintext) // 16
    for i in range(n_blocks):
        x = into_gf2_128(cipher, plaintext[16*i:16*i+16])
        # print_f128(x, 'GHASH input: ')
        tag = (tag + x) * key
        # print_f128(tag, 'GHASH output: ')
    if len(plaintext) % 16 != 0:
        # remaining block
        x = into_gf2_128(cipher, plaintext[16*n_blocks:])
        # print_f128(x, 'GHASH input: ')
        tag = (tag + x) * key
        # print_f128(tag, 'GHASH output: ')

    # length block
    length_block_bits = be_uint64(0) + be_uint64(len(plaintext) * 8)
    length_block = cgf2n(into_gf2_128_bits(length_block_bits), size=cipher.nparallel)
    # print_f128(length_block, 'Length block: ')
    tag = (tag + length_block) * key
    # print_f128(tag, 'GHASH output: ')

    # decompose into bytes
    tag_bytes = from_gf2_128(tag)
    return tag_bytes

def aes_gcm_128_decrypt(ciphertext, tag, key, nonce):
    assert len(tag) == 16
    nparallel = tag[0].size
    assert all(c.size == nparallel for c in ciphertext)
    assert all(t.size == nparallel for t in tag)
    assert len(key) == 16
    assert all(k.size == nparallel for k in key)
    assert len(nonce) == 12
    assert all(n.size == nparallel for n in nonce)
    cipher = Aes128(nparallel)
    key = cipher.SecretArrayEmbedd(key)
    # print_embedded_buf(key, "K: ")
    expanded_key = cipher.expandAESKey(key)
    AES = cipher.encrypt_without_key_schedule_no_emb(expanded_key)

    message_authentication_key = AES(cipher.SecretArrayEmbedd([cgf2n(0, size=nparallel)] * 16))
    # print_embedded_buf(message_authentication_key, 'H: ')


    ctr_init = nonce + [cgf2n(0, size=nparallel), cgf2n(0, size=nparallel), cgf2n(0, size=nparallel), cgf2n(0x2, size=nparallel)]

    plaintext = aes_ctr_dec(cipher, AES, ctr_init, ciphertext)

    s = ghash(cipher, ciphertext, message_authentication_key)

    # encrypt S to get final tag
    tag_mask = AES(cipher.SecretArrayEmbedd(nonce + [cgf2n(0, size=nparallel), cgf2n(0, size=nparallel), cgf2n(0, size=nparallel), cgf2n(0x1, size=nparallel)]))
    computed_tag = []
    for i,m in enumerate(cipher.state_collapse(tag_mask)):
        computed_tag.append(s[i] + m)
    check = tag_check(tag, computed_tag, bit_decompose=lambda x: x.bit_decompose(8))
    return check.reveal(), plaintext

"""
if BENCHMARK:
    import random
    import math
    random.seed(N)

    def random_bytes(n, type):
        return [type(random.randint(0,255), size=nparallel) for i in range(n)]

    # in the benchmark, we don't care about supplying correct ciphertext,tag pairs
    key = random_bytes(16, sgf2n)
    nonce = random_bytes(12, cgf2n)
    ciphertext = random_bytes(N, cgf2n)
    tag = random_bytes(16, cgf2n)
    start_timer(1)
    check, message = aes_gcm_128_decrypt(ciphertext, tag, key, nonce)
    stop_timer(1)
    print_ln('Tag: %s', check)
    print_ln("Message")
    print_ln('%s' * N, *[m.reveal() for m in message])
else:
    key = [sgf2n(x, size=nparallel) for x in conv("320db73158a35a255d051758e95ed4ab")]
    nonce = [sgf2n(x, size=nparallel) for x in conv("b2cdc69bb454110e82744121")]
    message = conv("67c6697351ff4aec29cdbaabf2fbe3467cc254f81be8e78d765a2e63339fc99a66")
    ciphertext = [cgf2n(x, size=nparallel) for x in conv("9d6490765381e7f2241218de5caeae3e7af6f4ee93f7ae3562051e5f088a43747f")]
    tag = [cgf2n(x, size=nparallel) for x in conv("ffa633b9d065d916bdee1d0ae8d3cd9e")]

    check, dec_mes = aes_gcm_128_decrypt(ciphertext, tag, key, nonce)
    print_ln('Check tag = %s', check)
    for byte in dec_mes:
        print_ln("%s", byte.reveal())
"""