from Programs.Source.eevee import EeveeGF2n
from Programs.Source.eevee.jolteon_gf2n import JolteonGF2n
from Programs.Source.utils import tag_check

from Compiler.types import sgf2n

def umbreon_dec_forkskinny_64_192_n48(fk, associated_data, ciphertext, tag, key, nonce):
    assert(len(key) == 128)
    assert len(nonce) == 6*8
    assert len(tag) == 16
    eevee = EeveeGF2n(fk, blocksize=64, tweaksize=64, fk_params=fk.FORKSKINNY_64_192, fk_enc=fk.forkskinny_64_192_enc, fk_dec=fk.forkskinny_64_192_dec)
    open_tag, message = eevee.umbreon_dec(associated_data, ciphertext, tag, key, nonce)
    # flatten message
    return open_tag, [b for cell in message for b in cell]

def umbreon_dec_forkskinny_128_256_n112(fk, associated_data, ciphertext, tag, key, nonce):
    assert(len(key) == 128)
    assert len(nonce) == 14*8
    assert len(tag) == 16
    eevee = EeveeGF2n(fk, blocksize=128, tweaksize=128, fk_params=fk.FORKSKINNY_128_256, fk_enc=fk.forkskinny_128_256_enc, fk_dec=fk.forkskinny_128_256_dec)
    open_tag, message = eevee.umbreon_dec(associated_data, ciphertext, tag, key, nonce)
    # flatten message
    return open_tag, [b for cell in message for b in cell]

def jolteon_dec_forkskinny_64_192_n48(fk, associated_data, ciphertext, tag, key, nonce):
    assert(len(key) == 128)
    assert len(nonce) == 6*8
    assert len(tag) == 16
    eevee = JolteonGF2n(fk, blocksize=64, tweaksize=64, fk_params=fk.FORKSKINNY_64_192, fk_enc=fk.forkskinny_64_192_enc, fk_dec=fk.forkskinny_64_192_dec)
    open_tag, message = eevee.jolteon_dec(associated_data, ciphertext, tag, key, nonce)
    # flatten message
    return open_tag, [b for cell in message for b in cell]

def jolteon_dec_forkskinny_128_256_n112(fk, associated_data, ciphertext, tag, key, nonce):
    assert(len(key) == 128)
    assert len(nonce) == 14*8
    assert len(tag) == 16
    eevee = JolteonGF2n(fk, blocksize=128, tweaksize=128, fk_params=fk.FORKSKINNY_128_256, fk_enc=fk.forkskinny_128_256_enc, fk_dec=fk.forkskinny_128_256_dec)
    open_tag, message = eevee.jolteon_dec(associated_data, ciphertext, tag, key, nonce)
    # flatten message
    return open_tag, [b for cell in message for b in cell]

def into_block(fk, bits):
    assert len(bits) == 128
    return [fk.embed8(bits[8*i:8*i+8]) for i in range(16)]

def block_xor(a, b):
    assert len(a) == 16
    assert len(b) == 16
    return [ai + bi for ai,bi in zip(a,b)]

def espeon_forkskinny_128_384(fk, key, nonce, ciphertext, tag):
    ZERO = sgf2n(fk.cembed8(0),size=key[0].size)
    ONE = sgf2n(fk.cembed8(1),size=key[0].size)

    espeon_fk = fk.ForkSkinnyGF2n(fk.ForkSkinnyGF2n.FORKSKINNY_128_384, vector_size=key[0].size, thread_fork=False)
    t_a = [ZERO] * 16

    # split ciphertext into blocks
    blocks = [into_block(fk, ciphertext[128*i:128*i+128]) for i in range(len(ciphertext)//128)]
    m = len(blocks)
    pad_c = ONE
    res_c = len(ciphertext) % 128
    if res_c != 0:
        padded_block = list(ciphertext[len(ciphertext) - (len(ciphertext)%128):])
        padded_block.append(ONE)
        padded_block += [ZERO] * (128 - len(padded_block))
        blocks.append(into_block(fk, padded_block))
        pad_c = ZERO
        res_c //= 8
    else:
        res_c = 16
        m -= 1

    lastlast_c = [ZERO] * 128
    last_c = nonce + [ZERO] * (128 - len(nonce))
    message = []

    for i in range(m):
        tweak = last_c + lastlast_c[:126] + [ZERO, ONE]
        #counter = encode_counter(i+2, 15)
        #tweak = nonce + counter[8:15] + [ONE] + counter[0:8]
        assert len(tweak) == 256

        tks = espeon_fk.expand_key(key, tweak)
        m_i = espeon_fk.forkskinny_dec(blocks[i], tks, 'i', '0')
        if i == 0:
            m_i = block_xor(m_i, t_a)
        t_a = block_xor(t_a, m_i)
        lastlast_c = last_c
        last_c = ciphertext[128*i:128*i + 128]
        message.append(m_i)

    tweak = last_c + lastlast_c[:126] + [ONE, pad_c]
    assert len(tweak) == 256

    tks = espeon_fk.expand_key(key, tweak)
    m_star, c_star_prime = espeon_fk.forkskinny_dec(tag, tks, 'b', '0')
    m_star = block_xor(m_star, t_a)

    message.append(m_star[:res_c])

    # tag check
    ciphertext_match = [m_stari + c_star_primei for m_stari, c_star_primei in zip(m_star[:res_c], c_star_prime[:res_c])]
    padding_match = m_star[res_c:]
    assert len(ciphertext_match) + len(padding_match) == 16
    check = tag_check(ciphertext_match + padding_match, blocks[-1], fk.bit_decompose8)

    return check.reveal(), [cell for block in message for cell in block]