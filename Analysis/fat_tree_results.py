import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns

df = pd.read_csv("sim_results_fattree.csv")


# Remove duplicate header rows (if any) from concatenated data
df_clean = df[df["CBR(Mbps)"] != "CBR(Mbps)"].copy()

# Convert columns to correct types
df_clean["CBR(Mbps)"] = df_clean["CBR(Mbps)"].astype(float)
df_clean["Throughput(Mbps)"] = df_clean["Throughput(Mbps)"].astype(float)
df_clean["AvgRTT(ms)"] = df_clean["AvgRTT(ms)"].astype(float)
df_clean["DropRate"] = df_clean["DropRate"].astype(float)

# Plot for Scenario 1 using seaborn style
def plot_all_metrics_per_scenario(df, scenario_label):
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
    # plt.show()
    plt.savefig(f"{scenario_label}_comparison.png")

# Filter and plot for Scenario 1
df_scenario1 = df_clean[df_clean["Scenario"] == "Scenario1"]
plot_all_metrics_per_scenario(df_scenario1, "Fat Tree (K=4)")
