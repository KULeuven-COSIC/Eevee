import matplotlib.pyplot as plt
import re
import numpy as np
import os
import pandas as pd

plt.rcParams.update({'font.size': 22})

primitives = {
    'Umbreon-Forkskinny-64-192': (r'''umbreon_fk_64_192_m(\d+)_cycles''', 0, 'dashed'),
    'Umbreon-Forkskinny-128-256': (r'''umbreon_fk_128_256_m(\d+)_cycles''', 0, 'solid'),
    'HtMAC-Skinny-128-256': (r'''htmac_skinny_128_256_m(\d+)_cycles''', 1, 'solid'),
    'PMAC-Skinny-128-256': (r'''pmac_skinny_128_256_m(\d+)_cycles''', 2, 'solid'),
    'Jolteon-Forkskinny-64-192': (r'''jolteon_fk_64_192_m(\d+)_cycles''', 3, 'dashed'),
    'Jolteon-Forkskinny-128-256': (r'''jolteon_fk_128_256_m(\d+)_cycles''', 3, 'solid'),
    'Espeon-Forkskinny-128-256': (r'''espeon_fk_128_256_m(\d+)_cycles''', 4, 'dashed'),
    'Espeon-Forkskinny-128-384': (r'''espeon_fk_128_384_m(\d+)_cycles''', 4, 'solid'),
    'HtMAC-MiMC-128': (r'''results/htmac_mimc_128_m(\d+)_cycles''', 5, 'solid'),
    'pPMAC-MiMC-128': (r'''results/ppmac_mimc_128_m(\d+)_cycles''', 6, 'solid'),
    'AES-GCM-128': (r'''aes_gcm_128_m(\d+)_cycles''', 7, 'dashed'),
    'AES-GCM-SIV-128': (r'''aes_gcm_siv_128_m(\d+)_cycles''', 6, 'dashed'),
    'Jolteon-AES-128': (r'''jolteon_aes_128_m(\d+)_cycles''', 3, 'dashed'),
    'Espeon-AES-128': (r'''espeon_aes_128_m(\d+)_cycles''', 4, 'dashed'),
}

def read_data(path):
    with open(path, 'r') as fp:
        lines = fp.readlines()
        return np.array([int(x) for x in lines])

def read_data_for_primitive(name, exp, files, path_to):
    matching = []
    for f in files:
        m = re.match(exp, f)
        if m is not None:
            matching.append((f, int(m.group(1))))
    raw_cycles = []
    avg_cycles_per_byte = []

    rows = []
    for path, mlen in matching:
        cycles = read_data(f'{path_to}/{path}')
        raw_cycles.append((mlen, cycles))
        avg_cycles_per_byte.append((mlen, np.average(cycles / float(mlen))))

        rows.append(pd.DataFrame.from_dict({
            'primitive': name,
            'messagelength': [mlen],
            'avgcycles': np.average(cycles),
            'avgcyclespermessagebyte': np.average(cycles / float(mlen)),
        }))
    # sort by mlen
    raw_cycles.sort(key=lambda t: t[0])
    avg_cycles_per_byte.sort(key=lambda t: t[0])
    return raw_cycles, avg_cycles_per_byte, rows

def split2(l):
    l1 = []
    l2 = []
    for x,y in l:
        l1.append(x)
        l2.append(y)
    return l1,l2


d = 'results'
data = {}
paths = os.listdir(d)

df = pd.DataFrame(data=None, columns=['primitive', 'messagelength', 'avgcycles', 'avgcyclespermessagebyte'])

for name,(exp,_,_) in primitives.items():
    raw, avg, rows = read_data_for_primitive(name, exp, paths, d)
    data[name] = (raw, avg)
    print(rows)
    df = pd.concat([df] + rows)

df.to_csv('results.csv')

colors = ['tab:blue', 'tab:orange', 'tab:green', 'tab:red', 'tab:purple', 'tab:brown', 'tab:pink', 'tab:gray', 'tab:olive']

fig = plt.figure()
ax0 = fig.subplots(1,1)
ax0.set_title('Average cycles')
ax0.set_xlabel('Message size in byte')
ax0.set_ylabel('Average cycles')
ax0.set_yscale('log')
for name, (_,color,line) in primitives.items():
    avg = data[name][1]
    mlens, raw = split2(data[name][0])
    raw_avg = [np.average(x) for x in raw]
    mlens, cpb = split2(avg)
    ax0.plot(mlens, raw_avg, label=name, marker='x', color=colors[color], linestyle=line, linewidth=3, markersize=12)
ax0.legend()
plt.show()

#fig = plt.figure()
#ax0 = fig.subplots(1,1)
#mleni=0
#ax0.set_ylabel('Cycles')
#for name in primitives.keys():
#    raw = data[name][0]
#    mlens, raw = split2(raw)
#    ax0.set_xlabel(f'Testvector (mlen={mlens[mleni]})')
#    ax0.scatter(range(len(raw[mleni])), raw[mleni], label=name, marker='.')
#ax0.legend()
#plt.show()
