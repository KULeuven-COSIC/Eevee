from Compiler.types import cgf2n
from Compiler.library import print_ln
import math

from Programs.Source.aes_gf_long import Aes128

class AesXEX:
    def __init__(self, key1, key2, long_tweak=False):
        assert len(key1) == 16
        self.nparallel = key1[0].size
        assert all(k.size == self.nparallel for k in key1)
        assert len(key2) == 16
        assert all(k.size == self.nparallel for k in key2)
        self.aes = Aes128(self.nparallel)
        key1 = [self.aes.ApplyEmbedding(k) for k in key1]
        self.key_schedule = self.aes.expandAESKey(key1)
        self.bc_forward = self.aes.encrypt_without_key_schedule_no_emb(self.key_schedule)
        self.bc_backward = self.aes.decrypt_without_key_schedule_no_emb(self.key_schedule)
        self.byte2gf_constants = [cgf2n(2**(8*i), size=self.nparallel) for i in range(16)]
        self.key2 = self._into_gf128(key2)
        if long_tweak:
            self.key2_sq = self.key2 * self.key2
            self.key2_cub = self.key2_sq * self.key2
    
    def _into_gf128(self, bytes):
        return sum(c * b for c, b in zip(self.byte2gf_constants, bytes))
    
    def _from_gf128(self, gf):
        bits = gf.bit_decompose(128)
        return [self.aes.ApplyBDEmbedding(bits[8*i:8*i+8]) for i in range(16)]
    
    def forward(self, message, tweak):
        assert message.size == self.nparallel
        if isinstance(tweak, (tuple,list)):
            assert all(isinstance(t, int) or t.size == self.nparallel for t in tweak)
            t1,t2,t3 = tweak
            tmp = t1 * self.key2 + t2 * self.key2_sq + t3 * self.key2_cub
        else:
            assert tweak.size == self.nparallel
            tmp = tweak * self.key2
        in_gf = message + tmp
        ciphertext = self.aes.state_collapse(self.bc_forward(self._from_gf128(in_gf)))
        c_gf = self._into_gf128(ciphertext)
        return c_gf + tmp
    
    def backward(self, ciphertext, tweak):
        assert ciphertext.size == self.nparallel
        if isinstance(tweak, (tuple,list)):
            assert all(isinstance(t, int) or t.size == self.nparallel for t in tweak)
            t1,t2,t3 = tweak
            tmp = t1 * self.key2 + t2 * self.key2_sq + t3 * self.key2_cub
        else:
            assert tweak.size == self.nparallel
            tmp = tweak * self.key2
        c = ciphertext + tmp
        mt = self.aes.state_collapse(self.bc_backward(self._from_gf128(c)))
        mt = self._into_gf128(mt)
        return mt + tmp

class TrivialForkAes64:
    """This is NOT ForkAES (ForkAES is broken)
    This is the trivial instantiation of a forkcipher from a tweakable block cipher.
    AES was made tweakable using the XEX-construction
    """
    def __init__(self, nparallel, key1, key2, long_tweak=False):
        self.nparallel = nparallel
        self.tbc = AesXEX(key1, key2, long_tweak)
    
    def gen_preproc(self, ncalls_forward_left, ncalls_forward_both, ncalls_backward_left, ncalls_invert):
        pass # no pre-processing needed
        
    def forward(self, message, tweak, mode):
        assert mode in ['l','b']
        assert message.size == self.nparallel
        if isinstance(tweak, (tuple,list)):
            assert all(isinstance(t, int) or t.size == self.nparallel for t in tweak)
            tweak_left = tweak[:-1] + (2 * tweak[-1],)
        else:
            assert tweak.size == self.nparallel
            tweak_left = 2*tweak
        left = self.tbc.forward(message, tweak_left)
        if mode == 'b':
            if isinstance(tweak, (tuple,list)):
                tweak_right = tweak[:-1] + (2*tweak[-1]+1,)
            else:
                tweak_right = 2*tweak + 1
            right = self.tbc.forward(message, tweak_right)
            return left, right
        else:
            return left
    
    def backward(self, ciphertext_left, tweak, mode):
        assert mode in ['i','b']
        assert ciphertext_left.size == self.nparallel
        if isinstance(tweak, (tuple,list)):
            assert all(isinstance(t, int) or t.size == self.nparallel for t in tweak)
            tweak_left = tweak[:-1] + (2 * tweak[-1],)
        else:
            assert tweak.size == self.nparallel
            tweak_left = 2*tweak
        message = self.tbc.backward(ciphertext_left, tweak_left)
        if mode == 'b':
            if isinstance(tweak, (tuple,list)):
                tweak_right = tweak[:-1] + (2*tweak[-1]+1,)
            else:
                tweak_right = 2*tweak + 1
            right = self.tbc.forward(message, tweak_right)
            return message, right
        else:
            return message


def tag_check(tag1, tag2):
    assert tag1.size == tag2.size
    num_random = type(tag1).get_random_triple(size=tag1.size)[0]
    zero_checker = num_random * (tag1 - tag2)
    b = zero_checker.reveal() == 0
    return b

def _sum(a,b):
    assert len(a) == len(b)
    return [ai + bi  for ai, bi in zip(a,b)]

def jolteon_aes_enc(fk, nonce, message, into_gf):
    """
    nonce size 104 bit
    """
    assert len(nonce) == 13
    nparallel = nonce[0].size
    assert all(n.size == nparallel for n in nonce)
    assert all(m.size == nparallel for m in message)
    
    assert len(message) / 16 < (2**15-1)
    
    # chop message into blocks of 16 elements
    n_blocks = int(math.ceil(len(message) / 16))
    
    message_blocks = [list(message[16*i:16*(i+1)]) for i in range(n_blocks-1)]
    pad = [cgf2n(0, size=nparallel)] * (16*n_blocks - len(message))
    message_blocks.append(message[16*(n_blocks-1):] + pad)
    
    clear_type = cgf2n
    
    t_a = cgf2n(0x0, size=nparallel)
    cnt = 2
    upper_nonce = into_gf([0,0] + nonce)
    ciphertext = []
    for m in message_blocks[:-1]:
        m = into_gf(m)
        t_a += m
        tweak = upper_nonce + cgf2n(cnt + 2**15, size=nparallel)
        print('Forward')
        c = fk.forward(m, tweak, 'l')
        t_a += c
        ciphertext.append(c)
        cnt += 1
    
    print('Forkcall')
    # forkcipher call
    m = t_a + into_gf(message_blocks[-1])
    tweak = upper_nonce
    tag, c_star = fk.forward(m, tweak, 'b')
    ciphertext.append(c_star + m)
    
    return ciphertext, tag


def jolteon_forkaes64_enc(key1, key2, nonce, message):
    assert len(key1) == 16
    assert len(key2) == 16
    nparallel = key1[0].size
    assert len(nonce) == 13
    assert all(k.size == nparallel for k in key1)
    assert all(k.size == nparallel for k in key2)
    assert all(n.size == nparallel for n in nonce)
    assert all(m.size == nparallel for m in message)
    
    fk = TrivialForkAes64(nparallel, key1, key2)
    print(fk.nparallel)
    return jolteon_aes_enc(fk, nonce, message, fk.tbc._into_gf128)

def jolteon_aes_dec(fk, nonce, ciphertext, tag, into_gf):
    """
    nonce size 104 bit
    """
    assert len(nonce) == 13
    nparallel = nonce[0].size
    assert all(n.size == nparallel for n in nonce)
    assert all(c.size == nparallel for c in ciphertext)
    
    assert len(ciphertext) / 16 < (2**15-1)
    
    assert tag.size == nparallel
    
    t_a = cgf2n(0, size=nparallel)
    cnt = 2
    upper_nonce = into_gf([0,0] + nonce)
    message = []
    for c in ciphertext[:-1]:
        t_a += c
        tweak = upper_nonce + cgf2n(cnt + 2**15, size=nparallel)
        m = fk.backward(c, tweak, 'i')
        message.append(m)
        t_a += m
        cnt += 1
    
    # forkcipher call
    tweak = upper_nonce
    
    tmp, c_bar = fk.backward(tag, tweak, 'b')
    message.append(tmp + t_a)
    c_bar += message[-1] + t_a
    
    # tag check
    check = tag_check(c_bar,ciphertext[-1])
    
    return check, message

def jolteon_forkaes64_dec(key1, key2, nonce, ciphertext, tag):
    assert len(key1) == 16
    assert len(key2) == 16
    nparallel = key1[0].size
    assert len(nonce) == 13
    assert all(k.size == nparallel for k in key1)
    assert all(k.size == nparallel for k in key2)
    assert all(n.size == nparallel for n in nonce)
    assert all(c.size == nparallel for c in ciphertext)
    assert tag.size == nparallel
    
    fk = TrivialForkAes64(nparallel, key1, key2)
    return jolteon_aes_dec(fk, nonce, ciphertext, tag, fk.tbc._into_gf128)


def espeon_aes_enc(fk, nonce, message, into_gf):
    assert len(nonce) == 13
    nparallel = nonce[0].size
    assert all(n.size == nparallel for n in nonce)
    assert all(m.size == nparallel for m in message)
    
    assert len(message) / 16 < (2**15-1)
    
    # chop message into blocks of 16 elements
    n_blocks = int(math.ceil(len(message) / 16))
    
    message_blocks = [list(message[16*i:16*(i+1)]) for i in range(n_blocks-1)]
    pad = [cgf2n(0, size=nparallel)] * (16*n_blocks - len(message))
    message_blocks.append(message[16*(n_blocks-1):] + pad)
    
    ciphertext = [into_gf([0,0] + nonce)]
    running_tag = cgf2n(0, size=nparallel)
    
    for i,m in enumerate(message_blocks[:-1]):
        m = into_gf(m)
        running_tag += m
        if i == 0:
            tweak1 = ciphertext[0] + 4
            tweak2 = 0
            ciphertext.append(fk.forward(m, (tweak1, tweak2, 0), 'l'))
        else:
            tweak1 = ciphertext[i]
            tweak2 = ciphertext[i-1]
            ciphertext.append(fk.forward(m, (tweak1, tweak2, 1), 'l'))
    
    # forkcipher call
    tweak1 = ciphertext[-1]
    if len(message) > 16:
        tweak2 = ciphertext[-2]
    else:
        tweak2 = ciphertext[0] + 4
    tag, c_star = fk.forward(running_tag + message[-1], (tweak1, tweak2, 2), 'b')
    ciphertext.append(c_star + message[-1] )
    return list(ciphertext[1:]), tag

def espeon_forkaes64_enc(key1, key2, nonce, message):
    assert len(key1) == 16
    assert len(key2) == 16
    nparallel = key1[0].size
    assert len(nonce) == 13
    assert all(k.size == nparallel for k in key1)
    assert all(k.size == nparallel for k in key2)
    assert all(n.size == nparallel for n in nonce)
    assert all(m.size == nparallel for m in message)
    
    fk = TrivialForkAes64(nparallel, key1, key2, long_tweak=True)
    return espeon_aes_enc(fk, nonce, message, fk.tbc._into_gf128)

def espeon_aes_dec(fk, nonce, ciphertext, tag, into_gf):
    assert len(nonce) == 13
    nparallel = nonce[0].size
    assert tag.size == nparallel
    assert all(c.size == nparallel for c in ciphertext)
    
    message = []
    running_tag = cgf2n(0, size=nparallel)
    c0 = into_gf([0,0] + nonce)
    if len(ciphertext) > 1:
        # first call
        m = fk.backward(ciphertext[0], (c0+4, 0, 0), 'i')
        running_tag += m
        message.append(m)
    for i,c in enumerate(ciphertext[1:-1]):
        tweak1 = ciphertext[i]
        tweak2 = ciphertext[i-1] if i > 0 else c0
        m = fk.backward(ciphertext[i+1], (tweak1, tweak2, 1), 'i')
        running_tag += m
        message.append(m)
    
    #forkcipher call
    tweak1 = ciphertext[-2] if len(ciphertext) > 1 else c0
    if len(ciphertext) == 2:
        tweak2 = c0
    elif len(ciphertext) > 2:
        tweak2 = ciphertext[-3]
    else:
        tweak2 = c0 + 4
    m_star, c_bar = fk.backward(tag, (tweak1, tweak2, 2), 'b')
    m = m_star - running_tag
    c_bar += m
    message.append(m)
    
    # tag check
    return tag_check(c_bar, ciphertext[-1]), message

def espeon_forkaes64_dec(key1, key2, nonce, ciphertext, tag):
    assert len(key1) == 16
    assert len(key2) == 16
    nparallel = key1[0].size
    assert len(nonce) == 13
    assert all(k.size == nparallel for k in key1)
    assert all(k.size == nparallel for k in key2)
    assert all(n.size == nparallel for n in nonce)
    assert all(c.size == nparallel for c in ciphertext)
    assert tag.size == nparallel
    
    fk = TrivialForkAes64(nparallel, key1, key2, long_tweak=True)
    return espeon_aes_dec(fk, nonce, ciphertext, tag, fk.tbc._into_gf128)
