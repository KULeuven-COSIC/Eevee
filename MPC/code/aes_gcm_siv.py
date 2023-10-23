from Programs.Source.aes_gf_long import Aes128
from Programs.Source.utils import tag_check
from Compiler.types import sgf2n, cgf2n, regint
from copy import copy

"""
Note that this AES-GCM-SIV implementation is not strictly the standard as described in RFC 8452
since the polynomial authenticator used GHASH's field, not the POLYVAL field.
This implementation uses P(x) mod x^128 + x^7 + x^2 + x + 1
The standard uses P(x) mod x^128 + x^127 + x^126 + x^121 + 1

Appendix A in the RFC contains details about conversion and the relationship between the two fields.
For benchmarking purposes in MPC, the performance difference in the arithmetic implementation of the players
is insignificant.
"""

def derive_keys(cipher, key, nonce):
    assert len(nonce) == 12
    key = [cipher.ApplyEmbedding(_) for _ in key]
    # print_embedded_buf(key, 'Derive Key: ')
    expanded_key = cipher.expandAESKey(key)
    AES = cipher.encrypt_without_key_schedule_no_emb(expanded_key)

    block1 = [cipher.ApplyEmbedding(cgf2n(0, size=cipher.nparallel))] * 4 + nonce

    auth_key_part1 = AES(block1)[:8]

    block2 = copy(block1)
    block2[0] = cipher.ApplyEmbedding(cgf2n(1, size=cipher.nparallel))
    auth_key_part2 = AES(block2)[:8]

    block3 = copy(block1)
    block3[0] = cipher.ApplyEmbedding(cgf2n(0x2, size=cipher.nparallel))
    enc_key_part1 = AES(block3)[:8]

    block4 = copy(block1)
    block4[0] = cipher.ApplyEmbedding(cgf2n(3, size=cipher.nparallel))
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

def aes_ctr_dec(cipher, AES, counter, ciphertext):
    # key is embedded, counter and ciphertext is not

    counter_bits = flatten([t.bit_decompose(8) for t in counter[:4]])
    counter_block_res = cipher.SecretArrayEmbedd(counter[4:15])

    # set MSbit of counter_block[15] to 1
    tag_bits = counter[15].bit_decompose(7) + [1]
    counter_block_res.append(cipher.ApplyBDEmbedding(tag_bits))

    n_blocks = len(ciphertext) // 16
    if len(ciphertext) % 16 != 0:
        n_blocks += 1
    ctr = regint(sum(2**i * b  for i,b in enumerate(counter_bits)))
    message = []
    m_ctr = 0

    for i in range(n_blocks):
        # set ctr
        ctr_bits = ((ctr + i) % 2**32).bit_decompose(32)
        ctr_cells = [sum([cgf2n(2**i, size=cipher.nparallel) * b for i,b in enumerate(ctr_bits[8*j:8*j+8])]) for j in range(32//8)]
        counter_block = cipher.SecretArrayEmbedd(ctr_cells) + list(counter_block_res)
        mask = AES(counter_block)
        for i in range(min(16,len(ciphertext)-m_ctr)):
            message.append(ciphertext[m_ctr] + cipher.InverseEmbedding(mask[i]))
            m_ctr += 1
    return message

def le_uint64(x):
    bits = [1 if ((x >> i) & 0x1) > 0 else 0 for i in range(64)]
    return bits

def polyval(cipher, plaintext, key):
    key = into_gf2_128(cipher, cipher.state_collapse(key))
    bytelen = len(plaintext)
    tag = cgf2n(0, size=cipher.nparallel)
    n_blocks = len(plaintext) // 16
    for i in range(n_blocks):
        x = into_gf2_128(cipher, plaintext[16*i:16*i+16])
        tag = (tag + x) * key
    if len(plaintext) % 16 != 0:
        # remaining block
        x = into_gf2_128(cipher, plaintext[16*n_blocks:])
        tag = (tag + x) * key

    # length block
    length_block_bits = le_uint64(0) + le_uint64(len(plaintext) * 8)
    length_block = cgf2n(into_gf2_128_bits(length_block_bits), size=cipher.nparallel)
    tag = (tag + length_block) * key

    # decompose into bytes
    tag_bytes = cipher.SecretArrayEmbedd(from_gf2_128(tag))
    return tag_bytes

def aes_gcm_siv_128_decrypt(ciphertext, tag, key, nonce):
    assert len(tag) == 16
    nparallel = tag[0].size
    assert all(c.size == nparallel for c in ciphertext)
    assert all(t.size == nparallel for t in tag)
    assert len(key) == 16
    assert all(k.size == nparallel for k in key)
    assert len(nonce) == 12
    assert all(n.size == nparallel for n in nonce)
    
    cipher = Aes128(nparallel)
    nonce = cipher.SecretArrayEmbedd(nonce)
    message_encryption_key, message_authentication_key = derive_keys(cipher, key, nonce)


    expanded_key = cipher.expandAESKey(message_encryption_key)
    AES = cipher.encrypt_without_key_schedule_no_emb(expanded_key)
    plaintext = ciphertext
    plaintext = aes_ctr_dec(cipher, AES, tag, ciphertext)

    s = polyval(cipher, plaintext, message_authentication_key)

    # XOR nonce
    for i in range(12):
        s[i] += nonce[i]
    # clear MSbit of s[15]
    s_bits = cipher.InverseBDEmbedding(s[15])
    s_bits[7] = cgf2n(0, size=nparallel)
    s[15] = cipher.ApplyBDEmbedding(s_bits)

    # encrypt S to get final tag
    computed_tag = cipher.state_collapse(AES(s))
    check = tag_check(tag, computed_tag, bit_decompose=lambda x: x.bit_decompose(8))
    return check.reveal(), plaintext

"""
BENCHMARK=False
from Compiler.library import print_ln
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
    check, message = aes_gcm_siv_128_decrypt(ciphertext, tag, key, nonce)
    stop_timer(1)
    print_ln('Tag: %s', check)
    print_ln("Message")
    print_ln('%s' * N, *[m.reveal() for m in message])
else:
    def conv(x):
        return [int(x[i : i + 2], 16) for i in range(0, len(x), 2)]
    simd=1
    key = [sgf2n(x, size=simd) for x in conv("320db73158a35a255d051758e95ed4ab")]
    nonce = [sgf2n(x, size=simd) for x in conv("b2cdc69bb454110e82744121")]
    message = conv("67c6697351ff4aec29cdbaabf2fbe3467cc254f81be8e78d765a2e63339fc99a66")
    ciphertext = [cgf2n(x, size=simd) for x in conv("3eb593f99c5be3f55dbb960a2dfdf172ff4630a1ba21199a6980283e633eea5a65")]
    tag = [cgf2n(x, size=simd) for x in conv("6a0c431b55b5f7486742cba0ca7556c5")]

    check, dec_mes = aes_gcm_siv_128_decrypt(ciphertext, tag, key, nonce)
    print_ln('Check tag = %s', check)
    for byte in dec_mes:
        print_ln("%s", byte.reveal())
"""