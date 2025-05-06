import pandas as pd
import matplotlib.pyplot as plt

data = pd.read_csv('tcp_congestion_control_benchmark.csv')

data['Variant'] = data['Variant'].str.strip()

# Group and average
avg_metrics = data.groupby('Variant')[['Throughput(Mbps)', 'AvgRTT(ms)', 'DropRate']].mean().reset_index()

# Convert DropRate to percentage
avg_metrics['DropRate (%)'] = avg_metrics['DropRate'] * 100

# Plotting with value labels
fig, axs = plt.subplots(1, 3, figsize=(18, 5), constrained_layout=True)

# Throughput
axs[0].bar(avg_metrics['Variant'], avg_metrics['Throughput(Mbps)'])
axs[0].set_title('Avg Throughput per TCP Variant')
axs[0].set_ylabel('Throughput (Mbps)')
axs[0].tick_params(axis='x', labelrotation=20)
for i, v in enumerate(avg_metrics['Throughput(Mbps)']):
    axs[0].text(i, v + 0.05, f"{v:.2f}", ha='center')

# RTT
axs[1].bar(avg_metrics['Variant'], avg_metrics['AvgRTT(ms)'])
axs[1].set_title('Avg RTT per TCP Variant')
axs[1].set_ylabel('RTT (ms)')
axs[1].tick_params(axis='x', labelrotation=20)
for i, v in enumerate(avg_metrics['AvgRTT(ms)']):
    axs[1].text(i, v + 0.5, f"{v:.1f}", ha='center')

# Drop Rate (percentage)
axs[2].bar(avg_metrics['Variant'], avg_metrics['DropRate (%)'])
axs[2].set_title('Avg Drop Rate per TCP Variant')
axs[2].set_ylabel('Drop Rate (%)')
axs[2].set_ylim(0, max(avg_metrics['DropRate (%)']) + 1)
axs[2].tick_params(axis='x', labelrotation=20)
for i, v in enumerate(avg_metrics['DropRate (%)']):
    axs[2].text(i, v + 0.05, f"{v:.2f}%", ha='center')

plt.suptitle('TCP Variant Comparison: Throughput, RTT, and Drop Rate (%)', fontsize=14)
# plt.show()
plt.savefig("tcp_congestion_control_benchmark.png")