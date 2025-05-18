# TCP Congestion Control & Fat-Tree Topology Simulations using NS-3

This repository contains NS-3 simulations focusing on:

- **TCP Congestion Control Algorithms**: Evaluating various TCP variants under different network scenarios.
- **Fat-Tree Topology**: Simulating a data center network using a fat-tree architecture.

---

## 📁 Project Structure

```
.
├── tcp_congestion_control_simulation.cc  # Simulates TCP variants across four scenarios
├── fat_tree_simulation.cc                # Simulates a fat-tree network topology
├── README.md                             # Project documentation
└── results/                              # Output graphs and logs
```

---

## 🧪 Simulation Scenarios

### 1. `tcp_congestion_control_simulation.cc`

This simulation evaluates TCP variants (e.g., Tahoe, Reno, Cubic, Vegas) across four scenarios:

- **Scenario 1: Single Flow** – Basic TCP flow between two nodes.
- **Scenario 2: MultiFlow** – Multiple concurrent TCP flows.
- **Scenario 3: Dumbbell Topology (Uniform CBR)** – Simulates a dumbbell topology with uniform Constant Bit Rate traffic.
- **Scenario 4: Dumbbell Topology (Bursty CBR)** – Similar to Scenario 3 but with bursty traffic patterns.

### 2. `fat_tree_simulation.cc`

Simulates a data center network using a fat-tree topology, analyzing performance metrics like throughput and latency.

---

## 🛠️ NS-3 Installation

### Prerequisites

Ensure your system has the following:

- **Operating System**: Ubuntu 20.04 LTS or compatible
- **Packages**:
  - C++ compiler (e.g., `g++`)
  - Python 3 (version 3.6 or higher)
  - CMake
  - Git
  - Development tools (`build-essential`, `python3-dev`, etc.)

### Installation Steps

1. **Clone the ns-3 Repository**:

```bash
git clone https://gitlab.com/nsnam/ns-3-dev.git
cd ns-3-dev
```

2. **Configure the Build**:

```bash
./ns3 configure -d release --enable-examples --enable-tests
```

3. **Build NS-3**:

```bash
./ns3 build
```

> 💡 *You can also use `./waf` if you are using the older build system.*

For detailed instructions, refer to the [NS-3 Installation Guide](https://www.nsnam.org/docs/installation/singlehtml/).

---

## 🚀 Running Simulations

1. **Copy Simulation Files**:

Move the `.cc` files into NS-3's `scratch/` directory:

```bash
cp /path/to/tcp_congestion_control_simulation.cc scratch/
cp /path/to/fat_tree_simulation.cc scratch/
```

2. **Build NS-3 Again** (if needed):

```bash
./ns3 build
```

3. **Run the Simulations**:

```bash
./ns3 run scratch/tcp_congestion_control_simulation
./ns3 run scratch/fat_tree_simulation
```

> Replace filenames if you've renamed the simulation scripts.

---

## 📊 Output and Visualization

Simulation results are saved under the `results/` directory and may include:

- **Throughput Graphs**
- **Latency & RTT Plots**
- **Congestion Window (CWND) Evolution**
- **Packet Drop & Retransmission Logs**

You can further analyze the results using:

- Python scripts with `matplotlib`
- `NetAnim` (for animated visualizations)

---

## 📚 References

- [NS-3 Official Docs](https://www.nsnam.org/docs/)
- [NS-3 Tutorial](https://www.nsnam.org/docs/tutorial/html/)
- [BBR: Bottleneck Bandwidth and RTT (Cardwell et al.)](https://queue.acm.org/detail.cfm?id=3022184)

---

## 🤝 Contributions

Pull requests are welcome! If you have suggestions for additional scenarios, TCP variants, or improvements to the visualization, feel free to contribute.

---

## 👥 Authors

- **Aditya Dawadikar**
- **Aniket Mali**

Developed as part of an academic project to study and visualize the behavior of various TCP congestion control algorithms and data center topologies using NS-3.

---
