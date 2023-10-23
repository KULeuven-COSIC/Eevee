from Programs.Source.forkskinny import SkinnyGF2n, embed4, cembed4, embed8, cembed8, bit_decompose4, bit_decompose8
SKINNY_128_256 = SkinnyGF2n.SKINNY_128_256

def skinny_cembed4(x):
    return cembed4(x)
def skinny_embed4(x):
    return embed4(x)
def skinny_cembed8(x):
    return cembed8(x)
def skinny_embed8(x):
    return embed8(x)
def skinny_bit_decompose4(x):
    return bit_decompose4(x)
def skinny_bit_decompose8(x):
    return bit_decompose8(x)

def skinny_128_256_enc(message, tweakey_schedule):
    '''
    Computes SKINNY-128-256 forward direction.
    message: message block with dimension 16; each cell is embedded via skinny_embed8
    tweakey_schedule: list of round keys obtained from expand_key

    Returns
        ciphertext with dimension 16; each cell is embedded via skinny_embed8
    '''
    assert(len(message) == 16)
    assert all((message[i].size == message[0].size for i in range(16)))
    return SkinnyGF2n(SkinnyGF2n.SKINNY_128_256, vector_size=message[0].size).skinny_enc(message, tweakey_schedule)

def skinny_128_256_dec(ciphertext, tweakey_schedule):
    '''
    Computes SKINNY-128-256 inverse direction.
    ciphertext: ciphertext block with dimension 16; each cell is embedded via skinny_embed8
    tweakey_schedule: list of round keys obtained from expand_key

    Returns
        message with dimension 16; each cell is embedded via skinny_embed8
    '''
    assert(len(ciphertext) == 16), f'{len(ciphertext)}: {ciphertext}'
    assert all((ciphertext[i].size == ciphertext[0].size for i in range(16)))
    return SkinnyGF2n(SkinnyGF2n.SKINNY_128_256, vector_size=ciphertext[0].size).skinny_dec(ciphertext, tweakey_schedule)

def expand_key(key, tweak, variant):
    '''
    Computes and returns the key schedule of SKINNY
    key:        list of key bits encoded in groups of cellsize bits with least significant bit first
    tweak:      list of tweak bits encoded in groups of cellsize bits with least significant bit first
    cellsize:   4 or 8
    variant:    SKINNY_128_256

    Returns a list of round keys
    '''
    if variant == SkinnyGF2n.SKINNY_128_256:
        assert len(key) + len(tweak) == 256
    else:
        raise NotImplemented
    fk = SkinnyGF2n(variant, vector_size=key[0].size)
    return fk.expand_key(key, tweak)