
# Konnectivity Under Test (KUT)

Konnectivity Under Test (KUT) is a modular C++ benchmarking tool designed to simulate network traffic and measure performance characteristics under controlled test environments. It supports both **server** and **client** modes and is built with deployment flexibility in mind, including integration with Kubernetes (`k3s`) clusters.

---

## 🚀 Features

- **Dual Modes**: Run as either a traffic-generating client or a traffic-receiving server
- **Protocol Support**: TCP and UDP
- **Traffic Patterns**: Constant, burst, sinusoidal, random, ramp-up/down
- **Metrics Collection**: Configurable performance metrics like CPU usage, packet stats, throughput
- **Scenario Configuration**: YAML-based setup for repeatable test environments
- **Modular CLI Interface**: Fine-grained control via command-line arguments
- **Dockerized**: Ready to be built and deployed as a container image

---

## 🏗️ Building the Project

### 🔧 Prerequisites

- C++20 compatible compiler
- CMake >= 3.20

### ⚙️ Build Instructions

```bash
git clone <repo-url>
cd konnectivity-under-test
mkdir build && cd build
cmake ..
make
```

This builds the `kut` executable with:
- C++20 standard
- Header-only ASIO (standalone, no Boost)
- Integrated `yaml-cpp` (via `external/yaml-cpp`)
- Modular code from `src/network`, `src/traffic`, `src/metrics`, `src/config`, `src/utils`

The resulting binary will be located in the `build/` directory.

---

## 🔄 GitHub Actions & CI/CD

A GitHub Actions workflow builds the KUT application when code is pushed to the `release` branch:

- 🛠️ Builds the C++ binary using the above CMake config
- 📦 Packages it into a Docker image
- 🚀 Pushes to a private Docker registry

This enables streamlined, hands-off deployment across distributed nodes.

---

## 🧠 Deployment Architecture

The KUT system is deployed on a 3-node Raspberry Pi `k3s` cluster:

- **pi-c**: master node, runs orchestration logic and CLI tools
- **pi-a**: worker node, runs the client Pod (benchmark generator)
- **pi-b**: worker node, runs the server Pod (traffic receiver)

Deployment is fully automated using:

- `Kustomize` for manifest management and image override
- `kubectl apply -k .` for resource provisioning
- Shell scripts for lifecycle management

---

## 🧪 Test Flow Overview

1. `kut-server` runs as a `Deployment`
2. `kut-client` runs as a one-shot `Job`
3. Server exposes a `ClusterIP` service used by the client
4. Both share PVCs for test data export and logs
5. `pvc-viewer` Pod helps inspect results

---

## 📂 Repo Structure

```
.
├── src/                  # C++ source files (modular)
│   ├── network/
│   ├── traffic/
│   ├── metrics/
│   ├── config/
│   └── utils/
├── external/             # Third-party libraries (ASIO, yaml-cpp)
├── config/               # YAML configMaps (client & server)
├── deployments/          # Deployment & Job manifests
├── service/              # ClusterIP service for server
├── tools/                # PVC viewer
├── scripts/              # Shell automation (auth, deploy, status)
├── CMakeLists.txt
├── kustomization.yaml
├── utils.sh
└── .env                  # Auth + namespace config (ignored by git)
```

---

## 📊 Diagrams

### 🧱 Dev & CI/CD Workflow

![Development Process](./readme_assets/development-process.png)

### 📡 Deployment Architecture

![Deployment Process](./readme_assets/basic-infra-measurement.png)

---

# KUT Konnectivity Under Test - Kubernetes Deployment

This repository contains a fully automated deployment system for a test/benchmark application called **KUT**, running on a lightweight Kubernetes distribution (`k3s`).

It consists of the following components:

- ⚙️ `kut-server` - A long-running server component (TCP/UDP listener)
- 🧪 `kut-client` - A one-shot benchmark/test client executed as a Kubernetes Job
- 📂 `PVC Viewer` - A small utility Pod to inspect shared Persistent Volume Claims
- 🔁 `Kustomize` - Used for image overrides, environment setup, and clean configuration layering

---

## 🌐 Architecture Overview

```
                      +--------------------+
                      |    kut-server      |
                      | (TCP/UDP listener) |
                      +---------+----------+
                                ^
                                |
                                |   ClusterIP Service (DNS: `kut-server`)
                                |
                      +---------v----------+
                      |    kut-client      |
                      |  (Job, one-shot)   |
                      +--------------------+
```

- The server runs as a **Deployment**
- The client runs as a **Job**, and can be re-triggered anytime
- All components share PVCs (data output/input)

---

## 📦 Deployment Features

- 🧠 Built for `k3s`, using `local-path` storage
- 🔐 Docker image auth via `kut-regcred` secret (loaded from `.env`)
- 🔧 Configuration via `ConfigMaps`
- 📜 Kustomize used for:
    - Setting image names/tags
    - Managing all resources cleanly
- 🛠️ Scripts (`*.sh`) for full lifecycle: deploy, reset, rerun, status, auth...

---

## 🚀 Quick Start

### 1. Clone and prepare:
```bash
git clone <url-to-this-repo>
cd repo
```

### 2. Fill in your `.env` file:
```env
AUTH_USERNAME=your-dockerhub-username
AUTH_TOKEN=your-dockerhub-token
NAMESPACE=kut-app
```

### 3. Deploy everything:
```bash
./run.sh
```

This runs:
- `reset.sh` → deletes previous cluster setup
- `auth.sh` → creates the image pull secret
- `deploy.sh` → applies everything using Kustomize

---

## 🔁 Rerunning only the client

To rerun just the `kut-client` Job (e.g. new config, or repeat test):

```bash
./rerun-client.sh
```

This will:
- Delete the old Job (if it exists)
- Re-apply the manifests using Kustomize
- Trigger a new test run
- The configuration file for client can be updated in `config/client-config.yaml`

---

## 📋 Status Check

To inspect current state of the namespace:

```bash
./status.sh
```

Outputs info about:
- Pods, Jobs, PVCs, Services, Events, etc.

---

## 🔄 Reset

To completely wipe the deployment:

```bash
./reset.sh
```

This deletes the entire namespace and everything inside.

---

## 📂 File Structure

```
k8s-deployment/
├── deployments/           # Job and Deployment YAMLs
├── config/                # ConfigMaps for client/server
├── pvc/                   # PVC definitions
├── service/               # Service for kut-server
├── tools/                 # PVC viewer
├── utils.sh               # Common env loader
├── kustomization.yaml     # Kustomize config
├── .env                   # DockerHub auth + namespace
└── *.sh                   # deploy.sh, auth.sh, etc.
```

---

## 📦 Tech Stack

- Kubernetes (k3s)
- DockerHub registry access
- Bash scripting
- Kustomize (built into `kubectl`)
- TCP/UDP networking
- Persistent storage with local-path-provisioner

---

## 🧊 Notes

- All manifests are safe to commit — `.env` is ignored and contains sensitive values
- Image overrides happen via `kustomization.yaml`
- Works well on Raspberry Pi clusters or any K3s-compatible environment

---

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for more details.

## Contributing

Contributions are welcome! Please open an issue or submit a pull request for any changes or improvements.

## Contact

For questions or support, please contact [oguz.altas@tum.de](mailto:oguz.altas@tum.de).