from Compiler.library import print_ln
from Compiler.GC.types import cbits, sbit, cbit
from Compiler.types import sgf2n, cgf2n
import importlib.util
import sys

def import_path(module_name, path):
    spec = importlib.util.spec_from_loader(
        module_name,
        importlib.machinery.SourceFileLoader(module_name, path)
    )
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    sys.modules[module_name] = module
    return module

def test_sbits_eq(a,b):
    assert(a.n == b.n)
    print_ln('t: %s == %s :t', a.reveal(), b.reveal())

def test_cbit_eq(a,b):
    assert(isinstance(a, cbit)), f'Expected type cbit for argument a ({a}), got {type(a)}'
    assert(isinstance(b, cbit)), f'Expected type cbit for argument b ({b}), got {type(b)}'
    print_ln('t: %s == %s :t', a, b)

def test_sgf2n_eq(a,b):
    assert a.size == b.size
    print_ln('t: %s == %s :t', a.reveal(), b.reveal())

def test_cgf2n_eq(a,b):
    assert(isinstance(a, cgf2n)), f'Expected type cgf2n for argument a ({a}), got {type(a)}'
    assert(isinstance(b, cgf2n)), f'Expected type cgf2n for argument b ({b}), got {type(b)}'
    assert a.size == b.size
    print_ln('t: %s == %s :t', a, b)

def test(a,b):
    bitlen = a.single_wire_n if a.single_wire_n > 1 else a.n
    n_wires = 1 if a.single_wire_n == 1 else a.n
    
    #if a.single_wire_n == 1 and a.n > 1:
    #    
    
    if a.single_wire_n > 1:
        a = a.reveal() # this returns a list
    else:
        a = [a.reveal()]
    if n_wires == 1:
        b = [b]
    assert len(b) == n_wires
    assert len(a) == n_wires
    print_ln('t: %s == %s :t', a, [cbits(x, n=bitlen) for x in b])

#def test2k(a,b):
#    assert isinstance(a, sgf2n)
#    assert isinstance(b, cgf2n)
#    print_ln('t: %s == %s :t', a.reveal(), b)

def _into_bits(s,n,dtype):
    assert len(s) %  (n/4) == 0
    assert n == 4 or n == 8
    r = []
    if n == 4:
        for c in s:
            c = int(c,16)
            r.append([dtype((c >> i) & 1) for i in range(n)])
    else:
        for i in range(0, len(s), 2):
            c0 = int(s[i], 16)
            c1 = int(s[i+1], 16)
            r.append([dtype((c1 >> i) & 1) for i in range(4)] + [dtype((c0 >> i) & 1) for i in range(4)])
    return r

def into_sbits(s, n):
    return _into_bits(s,n,sbit)

def into_cbits(s, n):
    return _into_bits(s,n,cbit)