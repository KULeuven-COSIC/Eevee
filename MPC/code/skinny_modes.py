from Programs.Source.testutils import import_path
from Programs.Source.utils import tag_check
import Programs.Source.skinny_gf2n as skinny

from Compiler.types import cgf2n, sgf2n, cint

def conv1_8(x):
    l =  [[int(c, 16) & 0x1, (int(c, 16) >> 1) & 0x1, (int(c, 16) >> 2) & 0x1, (int(c, 16) >> 3) & 0x1] for c in x]
    r = []
    for i in range(0,len(l), 2):
        r.append(l[i+1])
        r.append(l[i])
    # flatten
    return [b for cell in r for b in cell]

def digest(blocks, nonce):
    sigma = cint(0)
    for block in [nonce] + blocks:
        bits = [cint(b) for cell in block for b in skinny.bit_decompose8(cell)]
        element = sum((2**i * bits for i,bits in enumerate(bits)))
        sigma += element
    hash = sigma.digest(16)
    bits = hash.bit_decompose()
    block = [skinny.skinny_embed8([cgf2n(b) for b in bits[8*i:8*i+8]]) for i in range(16)]
    return block

def pack(blocks, base_type):
    n = len(blocks)
    if n == 1:
        return blocks[0]
    return [base_type([blocks[j][i] for j in range(n)], size=n) for i in range(len(blocks[0]))]

def unpack(block):
    n = len(block[0])
    blocks = [[block[j][i] for j in range(len(block))] for i in range(n)]
    return blocks

def ctr(n_blocks, key, nonce):
    assert n_blocks < 2**16-1
    tweakeys = []
    zero = cgf2n(0)
    one = cgf2n(1)
    for ctr in range(n_blocks):
        tweakey = key + [one]
        tweakey += [zero] * 111
        input_block = list(nonce)
        for i in range(16):
            tweakey.append(one if (ctr >> i) & 0x1 > 0 else zero)
        tweakeys.append(tweakey)
    tweakeys = pack(tweakeys, sgf2n)
    schedule = skinny.expand_key(tweakeys[:128], tweakeys[128:], skinny.SKINNY_128_256)
    block = [cgf2n([cell] * n_blocks, size=n_blocks) for cell in nonce]
    block += [cgf2n([zero] * n_blocks, size=n_blocks)] * 10
    keystream_blocks = skinny.skinny_128_256_enc(block, schedule)
    # unpack keystream blocks
    return unpack(keystream_blocks)

def ppmac(blocks, key, nonce):
    nonce_bits = [b for cell in nonce for b in skinny.bit_decompose8(cell)]
    tweakeys = []
    one = sgf2n(1)
    for ctr in range(1,len(blocks)+1):
        tweakey = list(key)
        tweakey += nonce_bits
        tweakey += one
        for i in range(79):
            tweakey.append(sgf2n((ctr >> i) & 0x1))
        tweakeys.append(tweakey)
    tweakeys = [sgf2n([tweakeys[i][j] for i in range(len(blocks))], size=len(blocks)) for j in range(256)]
    block = pack(blocks, blocks[0][0].__class__)
    schedule = skinny.expand_key(tweakeys[:128], tweakeys[128:], skinny.SKINNY_128_256)
    sigmas = skinny.skinny_128_256_enc(block, schedule)
    sigma = [sum(cell) for cell in sigmas]
    schedule = skinny.expand_key(key, nonce_bits + [sgf2n(1)] * 80, skinny.SKINNY_128_256)
    return skinny.skinny_128_256_enc(sigma, schedule)

def enc_pmac_skinny128_256(message, key, nonce):
    assert len(message) % 16 == 0
    assert len(nonce) == 6, f'{len(nonce)}: {nonce}'

    keystream = ctr(len(message) // 16, key, nonce)
    # flatten
    keystream = [cell for block in keystream for cell in block]
    # encrypt
    ciphertext = [(m + k).reveal() for m,k in zip(message, keystream)]
    # group into blocks
    ciphertext_blocks = [ciphertext[16*i:16*i+16] for i in range(len(message) // 16)]
    tag = ppmac(ciphertext_blocks, key, nonce)
    return ciphertext, [cell.reveal() for cell in tag]

def dec_pmac_skinny128_256(ciphertext, tag, key, nonce):
    assert len(ciphertext) % 16 == 0
    assert len(nonce) == 6, f'{len(nonce)}: {nonce}'

    keystream = ctr(len(ciphertext) // 16, key, nonce)
    # flatten
    keystream = [cell for block in keystream for cell in block]
    # decrypt
    message = [(m + k).reveal() for m,k in zip(ciphertext, keystream)]
    # group into blocks
    ciphertext_blocks = [ciphertext[16*i:16*i+16] for i in range(len(message) // 16)]
    computed_tag = ppmac(ciphertext_blocks, key, nonce)
    check = tag_check(tag, computed_tag, skinny.skinny_bit_decompose8)
    return check.reveal(), message

def enc_htmac_skinny128_256(message, key, nonce):
    assert len(message) % 16 == 0
    assert len(nonce) == 6, f'{len(nonce)}: {nonce}'
    
    keystream = ctr(len(message) // 16, key, nonce)
    # flatten
    keystream = [cell for block in keystream for cell in block]
    # encrypt
    ciphertext = [(m + k).reveal() for m,k in zip(message, keystream)]
    # group into blocks
    ciphertext_blocks = [ciphertext[16*i:16*i+16] for i in range(len(message) // 16)]
    h = digest(ciphertext_blocks, nonce)
    schedule = skinny.expand_key(key, [cgf2n(0)] * 128, skinny.SKINNY_128_256)
    tag = skinny.skinny_128_256_enc(h, schedule)
    return ciphertext, [cell.reveal() for cell in tag]

def dec_htmac_skinny128_256(ciphertext, tag, key, nonce):
    assert len(ciphertext) % 16 == 0
    assert len(nonce) == 6, f'{len(nonce)}: {nonce}'
    
    keystream = ctr(len(ciphertext) // 16, key, nonce)
    # flatten
    keystream = [cell for block in keystream for cell in block]
    # decrypt
    message = [(c - k) for c,k in zip(ciphertext, keystream)]
    # group into blocks
    ciphertext_blocks = [ciphertext[16*i:16*i+16] for i in range(len(message) // 16)]
    h = digest(ciphertext_blocks, nonce)
    schedule = skinny.expand_key(key, [cgf2n(1)] * 128, skinny.SKINNY_128_256)
    computed_tag = skinny.skinny_128_256_enc(h, schedule)
    check = tag_check(tag, computed_tag, skinny.skinny_bit_decompose8)
    return check.reveal(), message