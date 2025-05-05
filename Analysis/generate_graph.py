import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns

# Load CSV
df = pd.read_csv("sim_results.csv")

# Clean and prepare
df.columns = df.columns.str.strip()
df["Variant"] = df["Variant"].str.strip()
df["Scenario"] = df["Scenario"].str.strip()
df["CBR(Mbps)"] = df["CBR(Mbps)"].astype(float)
df["Throughput(Mbps)"] = df["Throughput(Mbps)"].astype(float)
df["AvgRTT(ms)"] = df["AvgRTT(ms)"].astype(float)
df["DropRate"] = df["DropRate"].astype(float)

print(df)

# Set seaborn style
sns.set(style="whitegrid")

# Helper function to create combined metric plots per scenario
def plot_all_metrics_per_scenario(df, scenario_label, output_prefix):
    fig, axs = plt.subplots(1, 3, figsize=(18, 5))

    sns.lineplot(data=df, x="CBR(Mbps)", y="Throughput(Mbps)", hue="Variant", marker="o", ax=axs[0])
    axs[0].set_title("Throughput vs CBR")
    axs[0].set_xlabel("CBR (Mbps)")
    axs[0].set_ylabel("Throughput (Mbps)")

    sns.lineplot(data=df, x="CBR(Mbps)", y="AvgRTT(ms)", hue="Variant", marker="o", ax=axs[1])
    axs[1].set_title("Avg RTT vs CBR")
    axs[1].set_xlabel("CBR (Mbps)")
    axs[1].set_ylabel("RTT (ms)")

    sns.lineplot(data=df, x="CBR(Mbps)", y="DropRate", hue="Variant", marker="o", ax=axs[2])
    axs[2].set_title("Drop Rate vs CBR")
    axs[2].set_xlabel("CBR (Mbps)")
    axs[2].set_ylabel("Drop Rate")

    plt.suptitle(f"{scenario_label} - TCP Variant Comparison", fontsize=16)
    plt.tight_layout(rect=[0, 0.03, 1, 0.95])
    plt.savefig(f"{output_prefix}_comparison.png")
    # plt.show()

for i in range(1,5):
    dfx = df[df["Scenario"]== f"Scenario{i}"]
    plot_all_metrics_per_scenario(dfx, f"Scenario {i}", f"Scenario{i}")