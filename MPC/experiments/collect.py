
import re
import pandas as pd
import matplotlib.pyplot as plt
from pathlib import Path

plt.rcParams.update({'font.size': 18})

def parse_line(path, exp):
    with open(path, 'r') as fp:
        for line in fp.readlines():
            m = exp.match(line)
            if m is not None:
                return m
    raise Exception(f"Not found in {path}")

def parse_n(path, exp):
    return int(parse_line(path, exp).group(1))

N_ITER_EXP = re.compile(r"""\s*N_ITER\s*=\s*(\d+)""")
PREP_EXP = re.compile(r"""Player: Kernel (\d+\.?\d*)s User (\d+\.?\d*)s""")
ONLINE_EXP = re.compile(r"""Time1 = (\d+\.?\d*) seconds \((\d+\.?\d*(e\+\d+)?) MB\)""")

def parse_prep(path):
    m = parse_line(path, PREP_EXP)
    return float(m.group(1)) + float(m.group(2))

def parse_online(path):
    m = parse_line(path, ONLINE_EXP)
    return float(m.group(1)), float(m.group(2))

def collect_eevee(path):
    n = parse_n(f'{path}/eevee_gf2n_benchmark.mpc', N_ITER_EXP)
    offline_time = parse_prep(f'{path}/run-prep0.log')
    online_time, online_data = parse_online(f'{path}/run-online0.log')
    combined_time, combined_data = parse_online(f'{path}/run-both0.log')

    return {
    'n': n,
    'offline_time': offline_time,
    'online_time': online_time,
    'combined_time': combined_time,
    'online_data': online_data,
    'combined_data': combined_data
    }

def collect_htmac(path):
    n = parse_n(f'{path}/htmac-skinny.mpc', N_ITER_EXP)
    offline_time = parse_prep(f'{path}/run-prep0.log')
    online_time, online_data = parse_online(f'{path}/run-online0.log')
    combined_time, combined_data = parse_online(f'{path}/run-both0.log')

    return {
    'n': n,
    'offline_time': offline_time,
    'online_time': online_time,
    'combined_time': combined_time,
    'online_data': online_data,
    'combined_data': combined_data
    }

def collect_jolteon(path):
    n = parse_n(f'{path}/jolteon_gf2n_benchmark.mpc', N_ITER_EXP)
    offline_time = parse_prep(f'{path}/run-prep0.log')
    online_time, online_data = parse_online(f'{path}/run-online0.log')
    combined_time, combined_data = parse_online(f'{path}/run-both0.log')

    return {
    'n': n,
    'offline_time': offline_time,
    'online_time': online_time,
    'combined_time': combined_time,
    'online_data': online_data,
    'combined_data': combined_data
    }
def collect_parallel(path):
    with open(f'{path}/iters', 'r') as fp:
        n = int(fp.read().strip())
    offline_time = parse_prep(f'{path}/run-prep0.log')
    online_time, online_data = parse_online(f'{path}/run-online0.log')
    combined_time, combined_data = parse_online(f'{path}/run-both0.log')

    return {
    'n': n,
    'offline_time': offline_time,
    'online_time': online_time,
    'combined_time': combined_time,
    'online_data': online_data,
    'combined_data': combined_data
    }

def prepare_data(data):
    df = pd.DataFrame(data=None, columns=['primitive', 'message length', 'n', 'offline_time', 'online_time', 'combined_time', 'online_data', 'combined_data'])
    for primitive,(collect_fun, filelist) in data.items():
        for mlen,path in filelist:
            d = collect_fun(path)
            d['primitive'] = primitive
            d['message length'] = mlen
            for k,v in d.items():
                d[k] = [v]
            d = pd.DataFrame.from_dict(d)
            df = pd.concat([df, d])

    # compute offline data
    df['offline_data'] = df['combined_data'] - df['online_data']

    # compute average
    def average(df, field):
        df[f'avg_{field}'] = df[field]/df['n']
    average(df, 'offline_time')
    average(df, 'online_time')
    average(df, 'combined_time')
    average(df, 'online_data')
    average(df, 'offline_data')
    average(df, 'combined_data')
    return df

data_2players = {
    'htmac-skinny-128-256': (collect_parallel, [(8, '21-htmac-skinny-128-256-parallel-m8'), (128, '23-parallel/htmac-skinny-parallel-128-100'), (500, '23-parallel/htmac-skinny-parallel-500-100')]),
    'eevee-fk-128-256': (collect_parallel, [(8, '23-parallel/eevee_parallel-8-1000'), (128, '23-parallel/eevee_parallel-128-100'), (500, '23-parallel/eevee_parallel-500-10')]),
    'jolteon-fk-128-256': (collect_parallel, [(8, '23-parallel/jolteon_parallel-8-1000'), (128, '23-parallel/jolteon_parallel-128-500'), (500, '23-parallel/jolteon_parallel-500-100')]),
    'eevee-fk-64-192': (collect_parallel, [(8, '31-eevee64-double/eevee64_parallel_double-8-1000'), (128, '31-eevee64-double/eevee64_parallel_double-128-500'), (500, '31-eevee64-double/eevee64_parallel_double-500-10')]),
    'jolteon-fk-64-192': (collect_parallel, [(8, '25-jolteon64-parallel/jolteon64_parallel-8-1000'), (128, '25-jolteon64-parallel/jolteon64_parallel-128-100'), (500, '25-jolteon64-parallel/jolteon64_parallel-500-10')]),
    'jolteon-fk-64-192': (collect_parallel, [(8, '25-jolteon64-parallel/jolteon64_parallel-8-1000'), (128, '30-jolteon64-double/jolteon64_parallel_double-128-500'), (500, '30-jolteon64-double/jolteon64_parallel_double-500-10')]),
    'espeon-fk-128-384': (collect_parallel, [(8, '36-espeon/espeon_parallel-8-500'), (128, '36-espeon/espeon_parallel-128-100'), (500, '36-espeon/espeon_parallel-500-10')]),
    'pmac-skinny-128-256': (collect_parallel, [(8, '40-pmac-skinny/pmac-skinny-parallel-8-1000'), (128, '40-pmac-skinny/pmac-skinny-parallel-128-500'), (500, '40-pmac-skinny/pmac-skinny-parallel-500-10')]),
}

data_3players = {
    'htmac-skinny-128-256': (collect_parallel, [(8, '26-3players/htmac-skinny-parallel-8-1000'), (128, '26-3players/htmac-skinny-parallel-128-500')]),
    'eevee-fk-128-256': (collect_parallel, [(8, '26-3players/eevee_parallel-8-1000'), (128, '26-3players/eevee_parallel-128-500')]),
    'jolteon-fk-128-256': (collect_parallel, [(8, '26-3players/jolteon_parallel-8-1000'), (128, '26-3players/jolteon_parallel-128-500')]),
    'eevee-fk-64-192': (collect_parallel, [(8, '33-eevee64-jolteon64-3players/eevee64_parallel_double-8-1000'), (128, '33-eevee64-jolteon64-3players/eevee64_parallel_double-128-500')]),
    'jolteon-fk-64-192': (collect_parallel, [(8, '33-eevee64-jolteon64-3players/jolteon64_parallel_double-8-1000'), (128, '33-eevee64-jolteon64-3players/jolteon64_parallel_double-128-500')]),
    'espeon-fk-128-384': (collect_parallel, [(8, '37-espeon-3players/espeon_parallel-8-500'), (128, '37-espeon-3players/espeon_parallel-128-100')]),
    'pmac-skinny-128-256': (collect_parallel, [(8, '41-pmac-skinny-3players/pmac-skinny-parallel-8-1000'), (128, '41-pmac-skinny-3players/pmac-skinny-parallel-128-500'), (500, '41-pmac-skinny-3players/pmac-skinny-parallel-500-10')]),
}

data_4players = {
    'htmac-skinny-128-256': (collect_parallel, [(8, '28-4players/htmac-skinny-parallel-8-1000'), (128, '28-4players/htmac-skinny-parallel-128-500')]),
    'eevee-fk-128-256': (collect_parallel, [(8, '28-4players/eevee_parallel-8-1000'), (128, '28-4players/eevee_parallel-128-500')]),
    'jolteon-fk-128-256': (collect_parallel, [(8, '28-4players/jolteon_parallel-8-1000'), (128, '28-4players/jolteon_parallel-128-500')]),
    'eevee-fk-64-192': (collect_parallel, [(8, '34-eevee64-jolteon64-4players/eevee64_parallel_double-8-1000'), (128, '34-eevee64-jolteon64-4players/eevee64_parallel_double-128-500')]),
    'jolteon-fk-64-192': (collect_parallel, [(8, '34-eevee64-jolteon64-4players/jolteon64_parallel_double-8-1000'), (128, '34-eevee64-jolteon64-4players/jolteon64_parallel_double-128-500')]),
    'espeon-fk-128-384': (collect_parallel, [(8, '38-espeon-4players/espeon_parallel-8-500'), (128, '38-espeon-4players/espeon_parallel-128-100')]),
    'pmac-skinny-128-256': (collect_parallel, [(8, '42-pmac-skinny-4players/pmac-skinny-parallel-8-1000'), (128, '42-pmac-skinny-4players/pmac-skinny-parallel-128-500'), (500, '42-pmac-skinny-4players/pmac-skinny-parallel-500-10')])
}

data_5players = {
    'htmac-skinny-128-256': (collect_parallel, [(8, '29-5players/htmac-skinny-parallel-8-1000'), (128, '29-5players/htmac-skinny-parallel-128-500')]),
    'eevee-fk-128-256': (collect_parallel, [(8, '29-5players/eevee_parallel-8-1000'), (128, '29-5players/eevee_parallel-128-500')]),
    'jolteon-fk-128-256': (collect_parallel, [(8, '29-5players/jolteon_parallel-8-1000'), (128, '29-5players/jolteon_parallel-128-500')]),
    'eevee-fk-64-192': (collect_parallel, [(8, '35-eevee64-jolteon64-5players/eevee64_parallel_double-8-1000'), (128, '35-eevee64-jolteon64-5players/eevee64_parallel_double-128-500')]),
    'jolteon-fk-64-192': (collect_parallel, [(8, '35-eevee64-jolteon64-5players/jolteon64_parallel_double-8-1000'), (128, '35-eevee64-jolteon64-5players/jolteon64_parallel_double-128-500')]),
    'espeon-fk-128-384': (collect_parallel, [(8, '39-espeon-5players/espeon_parallel-8-500'), (128, '39-espeon-5players/espeon_parallel-128-100')]),
    'pmac-skinny-128-256': (collect_parallel, [(8, '43-pmac-skinny-5players/pmac-skinny-parallel-8-1000'), (128, '43-pmac-skinny-5players/pmac-skinny-parallel-128-500'), (500, '43-pmac-skinny-5players/pmac-skinny-parallel-500-10')])
}

data_lat = {
    'eevee-fk-64-192': (collect_parallel, [(8, '32-all-lat/eevee64_parallel_double-8-1000'), (128, '32-all-lat/eevee64_parallel_double-128-500')]),
    'eevee-fk-128-256': (collect_parallel, [(8, '32-all-lat/eevee_parallel-8-1000'), (128, '32-all-lat/eevee_parallel-128-500')]),
    'htmac-skinny-128-256': (collect_parallel, [(8, '32-all-lat/htmac-skinny-parallel-8-1000'), (128, '32-all-lat/htmac-skinny-parallel-128-500')]),
    'jolteon-fk-64-192': (collect_parallel, [(8, '32-all-lat/jolteon64_parallel_double-8-1000'), (128, '32-all-lat/jolteon64_parallel_double-128-500')]),
    'jolteon-fk-128-256': (collect_parallel, [(8, '32-all-lat/jolteon_parallel-8-1000'), (128, '32-all-lat/jolteon_parallel-128-500')]),
    'pmac-skinny-128-256': (collect_parallel, [(8, '44-pmac-skinny-lat/pmac-skinny-parallel-8-1000'), (128, '44-pmac-skinny-lat/pmac-skinny-parallel-128-500')]),
    'espeon-fk-128-384': (collect_parallel, [(8, '45-espeon-lat/espeon_parallel-8-500'), (128, '46-espeon-lat/espeon_parallel-128-500')])
}

def prepare_data_lat(data_2players, data_lat):
    df0 = prepare_data(data_2players)
    df40 = prepare_data(data_lat)
    latdir = Path('lat')
    latdir.mkdir(parents=True, exist_ok=True)
    df0['latency'] = 0
    df40['latency'] = 40
    df = pd.concat([df0, df40])
    df.to_csv(latdir/'data_lat.csv')
    
    primitives = set(df['primitive'])
    print(primitives)
    message_lengths = set(df['message length'])
    for primitive in primitives:
        view = df[df['primitive'] == primitive]
        for mlen in message_lengths:
            view2 = view[view['message length'] == mlen]
            view2.to_csv(latdir / f'{primitive}-m{mlen}-lat.csv')

def plot_online_offline_2players(data):
    df = prepare_data(data)
    fig, ((ax0, ax1), (ax2, ax3)) = plt.subplots(2,2, sharex=True)

    ax0.set_title('Offline time')
    #ax0.set_xlabel('Message size (byte)')
    #ax0.set_yscale('log')
    ax0.set_ylabel('Time (s)')
    ax1.set_title('Offline data')
    #ax1.set_xlabel('Message size (byte)')
    ax1.set_ylabel('Comm. Data (MB)')
    ax2.set_title('Online time')
    #ax2.set_yscale('log')
    ax2.set_xlabel('Message size (byte)')
    ax2.set_ylabel('Time (s)')
    ax3.set_title('Online data')
    ax3.set_xlabel('Message size (byte)')
    ax3.set_ylabel('Comm. Data (MB)')
    
    #colors = ['tab:blue', 'tab:orange', 'tab:green', 'tab:red', 'tab:purple']
    style = {
        'eevee-fk-64-192': ('Eevee-Forkskinny-64-192', 'tab:blue', 'dashed'),
        'eevee-fk-128-256': ('Eevee-Forkskinny-128-256', 'tab:blue', 'solid'),
        'htmac-skinny-128-256': ('HtMAC-Skinny-128-256', 'tab:orange', 'solid'),
        'jolteon-fk-64-192': ('Jolteon-Forkskinny-64-192', 'tab:red', 'dashed'),
        'jolteon-fk-128-256': ('Jolteon-Forkskinny-128-256', 'tab:red', 'solid'),
        'espeon-fk-128-384': ('Espeon-Forkskinny-128-384', 'tab:purple', 'solid'),
        'pmac-skinny-128-256': ('PMAC-Skinny-128-256', 'tab:orange', 'dashed'),
        'multi-pmac-skinny-128': ('PMAC-then-MultiCTR-Skinny-128-256', 'black', 'dashed'),
        'aes-gcm-siv-128': ('AES-GCM-SIV-128', 'green', 'dashed'),
        'aes-gcm-128': ('AES-GCM-128', 'green', 'solid'),
        # 'skinny-gcm-siv-128': ('SKINNY-GCM-SIV-128', 'green', 'solid'),
    }
    for primitive in data.keys():
        label, color, linestyle = style[primitive]
        v = df[df['primitive'] == primitive]
        if primitive == 'eevee-fk-64-192' or primitive == 'eevee-fk-128-256':
            print(f'{primitive}: {v["avg_offline_time"]}')
        ax0.plot(v['message length'], v['avg_offline_time'], marker='x', label=label, color=color, linestyle=linestyle)
        ax1.plot(v['message length'], v['avg_offline_data'], marker='x', label=label, color=color, linestyle=linestyle)
        ax2.plot(v['message length'], v['avg_online_time'], marker='x', label=label, color=color, linestyle=linestyle)
        ax3.plot(v['message length'], v['avg_online_data'], marker='x', label=label, color=color, linestyle=linestyle)
        #ax4.plot(v['message length'], v['avg_combined_time'], marker='x', label=primitive)
        #ax5.plot(v['message length'], v['avg_combined_data'], marker='x', label=primitive)

    ax0.legend()
    d = Path('2players')
    d.mkdir(parents=True, exist_ok=True)
    df.to_csv(d / 'data.csv')
    plt.show()

def plot_nplayers(mlen, data2, data3, data4, data5):
    df2 = prepare_data(data2)
    df2['players'] = 2
    df3 = prepare_data(data3)
    df3['players'] = 3
    df4 = prepare_data(data4)
    df4['players'] = 4
    df5 = prepare_data(data5)
    df5['players'] = 5
    dfs = [df2, df3, df4, df5]
    fig, ((ax0, ax1), (ax2, ax3)) = plt.subplots(2,2, sharex=True)
    ax0.set_title('Offline time')
    #ax0.set_xlabel('Message size (byte)')
    #ax0.set_yscale('log')
    ax0.set_ylabel('Time (s)')
    ax1.set_title('Offline data')
    #ax1.set_xlabel('Message size (byte)')
    ax1.set_ylabel('Comm. Data (MB)')
    ax2.set_title('Online time')
    #ax2.set_yscale('log')
    ax2.set_xlabel('Number of players')
    ax2.set_ylabel('Time (s)')
    ax3.set_title('Online data')
    ax3.set_xlabel('Number of players')
    ax3.set_ylabel('Comm. Data (MB)')
    players = [2,3,4,5]
    for primitive in data2.keys():
        views = [df[(df['primitive'] == primitive) & (df['message length'] == mlen)] for df in dfs]
        ax0.plot(players, [v['avg_offline_time'] for v in views], marker='x', label=primitive)
        ax1.plot(players, [v['avg_offline_data'] for v in views], marker='x', label=primitive)
        ax2.plot(players, [v['avg_online_time'] for v in views], marker='x', label=primitive)
        ax3.plot(players, [v['avg_online_data'] for v in views], marker='x', label=primitive)
        #ax4.plot(v['message length'], v['avg_combined_time'], marker='x', label=primitive)
        #ax5.plot(v['message length'], v['avg_combined_data'], marker='x', label=primitive)
        ax0.legend()

    df = pd.concat(dfs)
    d = Path('nplayers')
    d.mkdir(parents=True, exist_ok=True)
    primitives = set(df['primitive'])
    message_lengths = set(df['message length'])
    for primitive in primitives:
        view = df[df['primitive'] == primitive]
        for mlen in message_lengths:
            view2 = view[view['message length'] == mlen]
            view2.to_csv(d / f'{primitive}-m{mlen}-nplayers.csv')
    plt.show()

prepare_data_lat(data_2players, data_lat)
#plot_online_offline_2players(data_2players)
#plot_nplayers(128, data_2players, data_3players, data_4players, data_5players)