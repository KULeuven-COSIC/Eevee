from Programs.Source.forkskinny import ForkSkinnyGF2n, embed4, cembed4, embed8, cembed8, bit_decompose4, bit_decompose8
FORKSKINNY_64_192 = ForkSkinnyGF2n.FORKSKINNY_64_192
FORKSKINNY_128_256 = ForkSkinnyGF2n.FORKSKINNY_128_256

def forkskinny_cembed4(x):
    return cembed4(x)
def forkskinny_embed4(x):
    return embed4(x)
def forkskinny_cembed8(x):
    return cembed8(x)
def forkskinny_embed8(x):
    return embed8(x)
def forkskinny_bit_decompose4(x):
    return bit_decompose4(x)
def forkskinny_bit_decompose8(x):
    return bit_decompose8(x)

def forkskinny_64_192_enc(message, tweakey_schedule, s):
    '''
    Computes ForkSKINNY-64-192 forward direction.
    message: message block with dimension 16; each cell is embedded via forkskinny_embed4
    tweakey_schedule: list of round keys obtained from expand_key
    s: Forkcipher mode
        '0' computes only the left branch
        '1' computes only the right branch
        'b' computes both branches

    Returns
        if s='0' left ciphertext with dimension 16; each cell is embedded via forkskinny_embed4
        if s='1' right ciphertext with dimension 16; each cell is embedded via forkskinny_embed4
        if s='b' tuple of left and right ciphertext
    '''
    assert(len(message) == 16)
    assert all((message[i].size == message[0].size for i in range(16)))
    return ForkSkinnyGF2n(ForkSkinnyGF2n.FORKSKINNY_64_192, vector_size=message[0].size).forkskinny_enc(message, tweakey_schedule, s)

def forkskinny_64_192_dec(ciphertext, tweakey_schedule, s, b):
    '''
    Computes ForkSKINNY-64-192 inverse direction.
    ciphertext: ciphertext block with dimension 16x4; each cell is embedded via forkskinny_embed4
    tweakey_schedule: list of round keys obtained from expand_key
    s: Inverse forkcipher mode
        'i' computes the inverse
        'o' computes the forward complementary branch, i.e. branch 1-b
        'b' computes the inverse and the forward complementary branch
    b: Inverse forkcipher input branch
        '0' ciphertext is the output of the left branch
        '1' ciphertext is the output of the right branch

    Returns
        if s='i' the message block with dimension 16; each cell is embedded via forkskinny_embed4
        if s='o' the ciphertext block of the left branch (if b='1') resp. the right branch (if b='0') with dimension 16; each cell is embedded via forkskinny_embed4
        if s='b' tuple of message block and the other ciphertext block
    '''
    assert(len(ciphertext) == 16)
    assert all((ciphertext[i].size == ciphertext[0].size for i in range(16)))
    return ForkSkinnyGF2n(ForkSkinnyGF2n.FORKSKINNY_64_192, vector_size=ciphertext[0].size).forkskinny_dec(ciphertext, tweakey_schedule, s, b)

def forkskinny_128_256_enc(message, tweakey_schedule, s):
    '''
    Computes ForkSKINNY-128-256 forward direction.
    message: message block with dimension 16; each cell is embedded via forkskinny_embed8
    tweakey_schedule: list of round keys obtained from expand_key
    s: Forkcipher mode
        '0' computes only the left branch
        '1' computes only the right branch
        'b' computes both branches

    Returns
        if s='0' left ciphertext with dimension 16; each cell is embedded via forkskinny_embed8
        if s='1' right ciphertext with dimension 16; each cell is embedded via forkskinny_embed8
        if s='b' tuple of left and right ciphertext
    '''
    assert(len(message) == 16)
    assert all((message[i].size == message[0].size for i in range(16)))
    return ForkSkinnyGF2n(ForkSkinnyGF2n.FORKSKINNY_128_256, vector_size=message[0].size).forkskinny_enc(message, tweakey_schedule, s)

def forkskinny_128_256_dec(ciphertext, tweakey_schedule, s, b):
    '''
    Computes ForkSKINNY-128-256 inverse direction.
    ciphertext: ciphertext block with dimension 16; each cell is embedded via forkskinny_embed8
    tweakey_schedule: list of round keys obtained from expand_key
    s: Inverse forkcipher mode
        'i' computes the inverse
        'o' computes the forward complementary branch, i.e. branch 1-b
        'b' computes the inverse and the forward complementary branch
    b: Inverse forkcipher input branch
        '0' ciphertext is the input to the left branch
        '1' ciphertext is the input to the right branch

    Returns
        if s='i' the message block with dimension 16; each cell is embedded via forkskinny_embed8
        if s='o' the ciphertext block of the left branch (if b='1') resp. the right branch (if b='0') with dimension 16; each cell is embedded via forkskinny_embed8
        if s='b' tuple of message block and the other ciphertext block
    '''
    assert(len(ciphertext) == 16)
    assert all((ciphertext[i].size == ciphertext[0].size for i in range(16)))
    return ForkSkinnyGF2n(ForkSkinnyGF2n.FORKSKINNY_128_256, vector_size=ciphertext[0].size).forkskinny_dec(ciphertext, tweakey_schedule, s, b)

def expand_key(key, tweak, cellsize, variant):
    '''
    Computes and returns the key schedule of ForkSKINNY
    key:        list of key bits encoded in groups of cellsize bits with least significant bit first
    tweak:      list of tweak bits encoded in groups of cellsize bits with least significant bit first
    cellsize:   4 or 8
    variant:    FORKSKINNY_64_192 or FORKSKINNY_128_256, must match with cellsize=4 for FORKSKINNY_64_192, cellsize=8 for FORKSKINNY_128_256
    
    Returns a list of round keys
    '''
    if variant == ForkSkinnyGF2n.FORKSKINNY_64_192:
        assert len(key) + len(tweak) == 192
    elif variant == ForkSkinnyGF2n.FORKSKINNY_128_256:
        assert len(key) + len(tweak) == 256
    else:
        raise NotImplemented
    fk = ForkSkinnyGF2n(variant, vector_size=key[0].size)
    return fk.expand_key(key, tweak)
