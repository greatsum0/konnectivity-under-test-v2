
# 🚀 Konnectivity Under Test (KUT)

Konnectivity Under Test (KUT) is a modular C++ benchmarking tool designed to simulate network traffic and measure performance in a controlled Kubernetes environment. It supports both **server** and **client** roles and is optimized for deployment on lightweight clusters like `k3s`.

---

## ✨ Features

- Dual-mode architecture: **Client** (Job) and **Server** (Deployment)
- Protocols: TCP and UDP
- Traffic Patterns: Constant, burst, sinusoidal, random, ramp-up, ramp-down
- Scenario-based testing: YAML-driven configuration
- Metric Collection: Throughput, CPU utilization (tool/system/normalized), and more
- Designed for containerized deployment with **Docker** and **Kubernetes**
- Built-in support for **Persistent Volume Claims (PVCs)** and result collection

---

## 🛠️ Project Structure (Kubernetes)

```
k3s-deployment/
├── config/                # YAML configuration files (mounted as ConfigMap)
│   └── server-config.yaml
├── configmaps/            # Generated ConfigMaps from scenario YAMLs
├── deployments/           # Server deployment manifest
├── jobs/                  # One-shot client Jobs (auto-generated)
├── pvc/                   # Persistent Volume Claims
├── services/              # Server service (ClusterIP)
├── tools/                 # PVC viewer
├── scenarios/             # YAML scenarios for each traffic pattern
├── scripts (*.sh)         # Lifecycle scripts (build, deploy, status, reset, rerun)
├── kustomization.yaml     # Top-level Kustomize entrypoint
└── .env                   # DockerHub credentials and namespace
```

---

## 🧠 Deployment Model

- Server: Long-running `Deployment` on `raspberrypi-b`
- Client: Single-run `Job` on `raspberrypi-a`
- Master: `pi-c` manages control plane and orchestration
- All configurations mounted via **ConfigMaps**
- Results exported to shared PVCs and accessible via `pvc-viewer`

---

## 🚀 Deployment Steps

1. **Clone the repository**
2. **Create a `.env` file** with DockerHub credentials and desired namespace:
```bash
AUTH_USERNAME=your-dockerhub-username
AUTH_TOKEN=your-dockerhub-token
NAMESPACE=kut-app
IMAGE_NAME=temp-image/placeholder
IMAGE_TAG=latest
```
3. **Run the deployment**

```bash
./build.sh
```
- `build.sh` → Generates all manifests from `scenarios/`


```bash
./run.sh
```

This executes:
- `reset.sh` → Cleans the previous state
- `auth.sh` → Applies Docker registry secret
- `deploy.sh` → Deploys the base infrastructure
- `run-all-jobs.sh` → Launches all client jobs sequentially

---

## 🔧 Configuration

- All client scenarios are written in `scenarios/*.yaml`
- The server configuration is located at `config/server-config.yaml`
- Each client config is automatically converted into a `ConfigMap` and a `Job`

---

## 📦 Kustomize

- Each resource category (`pvc/`, `deployments/`, `services/`, `jobs/`, `configmaps/`) has its own `kustomization.yaml`
- The root `kustomization.yaml` includes them all
- Image override is done via:
```yaml
images:
  - name: placeholder/temp-image
    newName: oguznagak/kut-tester
    newTag: latest
```

---

## 🔁 Rerunning Clients

To rerun all clients after config changes:

```bash
./run-all-jobs.sh
```

---

## 🛠️ Monitoring

Use the `status.sh` script for a live snapshot:

```bash
./status.sh
```

Shows:
- Nodes
- Deployments
- Jobs and Pods
- PVCs
- Events

---

## 📤 Output

Each test job stores its metrics as CSV under the shared PVC, e.g.:

```
/measurements/data/scenario_tcp_burst.csv
```

Accessible via:
```bash
kubectl exec -it pvc-viewer-client -- cat /measurements/data/<filename>.csv
```

You can use the copy_results.sh script to copy all results to the master node and run the following command to copy the results to your local machine:

on your local machine:

```bash
scp -P 12347 -r <user>@<master-machine-ip>:/home/<user>/Desktop ~/Desktop/
```


---

## 📋 Notes

- ConfigMaps must match the scenario names (`kut-client-<scenario>-config`)
- Server ConfigMap is `kut-server-config` and mounted from `config/`
- Image placeholder must be overridden correctly in all `kustomization.yaml`

---

## 📬 Contact

For any questions or issues: [oguz.altas@tum.de](mailto:oguz.altas@tum.de)

---
