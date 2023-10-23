
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

PREP_EXP = re.compile(r"""Player: Kernel (\d+\.?\d*)s User (\d+\.?\d*)s""")
ONLINE_EXP = re.compile(r"""Time1 = (\d+\.?\d*) seconds \((\d+\.?\d*(e\+\d+)?) MB\)""")
TIME_EXP = re.compile(r"""Time = (\d+\.?\d*) seconds""")
DATA_EXP = re.compile(r"""Data sent = (\d+\.?\d*(e\+\d+)?) MB""")

def parse_prep(path):
    m = parse_line(path, PREP_EXP)
    return float(m.group(1)) + float(m.group(2))

def parse_online(path):
    m = parse_line(path, ONLINE_EXP)
    return float(m.group(1)), float(m.group(2))

def parse_time(path):
    m_time = parse_line(path, TIME_EXP)
    m_data = parse_line(path, DATA_EXP)
    return float(m_time.group(1)), float(m_data.group(1))

def parse_meta(name):
    parts = name.split('-')
    assert parts[0] == 'eevee_benchmark'
    assert len(parts) == 4
    primitive = parts[1]
    simd = int(parts[2])
    message_length = int(parts[3])
    return primitive, simd, message_length

def collect(folder):
    folder = Path(folder)
    df = pd.DataFrame(data=None, columns=['primitive', 'message_length', 'n', 'offline_time', 'online_time', 'combined_time', 'online_data', 'combined_data'])
    for benchmark in folder.iterdir():
        if benchmark.is_dir():
            primitive, simd, message_length = parse_meta(benchmark.name)
            offline_time = parse_prep(benchmark / 'run-prep0.log')
            online_time, online_data = parse_online(benchmark / 'run-online0.log')
            time, data = parse_time(benchmark / 'run-online0.log')
            combined_time, combined_data = parse_online(benchmark / 'run-both0.log')
            d = {
                'primitive': [primitive],
                'message_length': [message_length],
                'n': [simd],
                'offline_time': [offline_time + (time - online_time)],
                'online_time': [online_time],
                'combined_time': [combined_time],
                'online_data': [online_data],
                'combined_data': [combined_data],
                'offline_data': [combined_data - online_data + (data - online_data)]
            }
            d = pd.DataFrame.from_dict(d)
            df = pd.concat([df, d])
    return df

def prepare_data(df):
    # compute average
    def average(df, field):
        df[f'avg_{field}'] = df[field]/df['n']
    average(df, 'offline_time')
    average(df, 'online_time')
    average(df, 'combined_time')
    average(df, 'online_data')
    average(df, 'offline_data')
    average(df, 'combined_data')
    
    
    # sort by primitive and message length
    df.sort_values(by=['primitive', 'message_length'], inplace=True)
    
    return df


df = collect('results')
df = prepare_data(df)

df.to_csv('results.csv')

def plot_online_offline_players(data, simd=None, primitives=None):
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
        'jolteon_forkmimc64': ('Jolteon[MiMC64]', 'tab:blue', 'dashed'),
        'espeon_forkmimc64': ('Espeon[MiMC64]', 'tab:green', 'dashed'),
        'umbreon_forkmimc64': ('Umbreon[MiMC64]', 'tab:red', 'dashed'),
        'htmac_mimc64': ('HtMAC[MiMC64]', 'tab:orange', 'dashed'),
        'pmac_mimc64': ('PMAC[MiMC64]', 'tab:purple', 'dashed'),
        'jolteon_forkaes64': ('Jolteon[AES]', 'tab:blue', 'solid'),
        'aes_gcm64': ('AES-GCM', 'tab:orange', 'solid'),
        'espeon_forkaes64': ('Espeon[AES]', 'tab:green', 'solid'),
        'aes_gcm_siv64': ('AES-GCM-SIV', 'black', 'solid'),
    }
    
    if primitives == None:
        primitives = set(data['primitive'])
    for primitive in primitives:
        label, color, linestyle = style[primitive]
        if simd == None:
            v = df[df['primitive'] == primitive]
        else:
            v = df[(df['primitive'] == primitive) & (df['n'] == simd)]
        
        ax0.plot(v['message_length'], v['avg_offline_time'], marker='x', label=label, color=color, linestyle=linestyle)
        ax1.plot(v['message_length'], v['avg_offline_data'], marker='x', label=label, color=color, linestyle=linestyle)
        ax2.plot(v['message_length'], v['avg_online_time'], marker='x', label=label, color=color, linestyle=linestyle)
        ax3.plot(v['message_length'], v['avg_online_data'], marker='x', label=label, color=color, linestyle=linestyle)

    ax0.legend()
    plt.show()

def plot_total(data):
    fig, (ax0, ax1) = plt.subplots(1,2)
    ax0.set_title('Time')
    ax0.set_ylabel('Time (s)')
    ax0.set_xlabel('Message size (byte)')
    ax1.set_title('Offline data')
    ax1.set_ylabel('Comm. Data (MB)')
    ax1.set_xlabel('Message size (byte)')
    
    style = {
        'jolteon_forkmimc64': ('Jolteon[MiMC64]', 'tab:blue', 'dashed'),
        'espeon_forkmimc64': ('Espeon[MiMC64]', 'tab:green', 'dashed'),
        'umbreon_forkmimc64': ('Umbreon[MiMC64]', 'tab:red', 'dashed'),
        'htmac_mimc64': ('HtMAC[MiMC64]', 'tab:orange', 'dashed'),
        'pmac_mimc64': ('PMAC[MiMC64]', 'tab:purple', 'dashed'),
    }
    
    primitives = set(data['primitive'])
    for primitive in primitives:
        label, color, linestyle = style[primitive]
        v = df[df['primitive'] == primitive]
        
        ax0.plot(v['message_length'], v['avg_combined_time'], marker='x', label=label, color=color, linestyle=linestyle)
        ax1.plot(v['message_length'], v['avg_combined_data'], marker='x', label=label, color=color, linestyle=linestyle)
    ax0.legend()
    plt.show()
plot_online_offline_players(df, simd=100, primitives=['jolteon_forkaes64', 'aes_gcm64', 'espeon_forkaes64', 'aes_gcm_siv64'])
#plot_total(df)