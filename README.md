
# 🚀 Konnectivity Under Test (KUT)

Konnectivity Under Test (KUT) is a modular C++ benchmarking tool designed to simulate network traffic and measure performance in a controlled Kubernetes environment. It supports both **server** and **client** roles and is optimized for deployment on lightweight clusters like `k3s`.

---

## ✨ Features

- Dual-mode architecture: **Client** (Job) and **Server** (Deployment)
- Protocols: TCP and UDP
- Traffic Patterns: Constant, burst (burst, random, pure), sinusoidal, ramp (up, down)
- Scenario-based testing: YAML-driven configuration
- Metric Collection: Throughput, Transferred bytes, CPU utilization (tool/system), and more
- Designed for containerized deployment with **Docker** and **Kubernetes**
- Built-in support for **Persistent Volume Claims (PVCs)** and result collection
- Built-in API for easy integration with other tools

---

## 🛠️ Project Structure (Kubernetes)

```
k3s-deployment/
├── api/                   # Flask API for the deployment controller
├── config/                # YAML configuration files (mounted as ConfigMap)
│   └── server-config.yaml
├── configmaps/            # Generated ConfigMaps from scenario YAMLs
├── deployments/           # Server deployment manifest
├── jobs/                  # One-shot client Jobs (auto-generated)
├── pvc/                   # Persistent Volume Claims
├── scenarios/             # YAML scenarios for each traffic pattern
├── services/              # Server service (ClusterIP)
├── templates/             # Kustomize templates for Jobs
├── tools/                 # PVC viewer
├── kustomization.yaml     # Top-level Kustomize entrypoint
└── .env                   # DockerHub credentials and namespace
```

---

## Deployment Model

- Server: Long-running `Deployment` on `raspberrypi-b`
- Client: Single-run `Job` on `raspberrypi-a`
- Master: `pi-c` manages control plane and orchestration
- All configurations mounted via **ConfigMaps**
- Results exported to shared PVCs and accessible via `pvc-viewer`

---

## Deployment Steps

1. **Clone the repository**
2. **Create a `.env` file** with DockerHub credentials and desired namespace:
```
AUTH_USERNAME=your-dockerhub-username
AUTH_TOKEN=your-dockerhub-token
NAMESPACE=kut-app
IMAGE_NAME=temp-image/placeholder
IMAGE_TAG=latest
```
3. **Start the controller API**

4. **Create a Python virtual environment**:

```bash
cd k3s-deployment/api

python3 -m venv .venv

source .venv/bin/activate

pip install -r requirements.txt

python app.py
```
The server will start on `localhost:5050` and you can port-forward it for your cluster.

5. **Deploy the server**:

On client side, you can access the `swagger` API documentation at `localhost:5050/apidocs` to see the available endpoints.

6. **Call the following endpoints for preparation**:

```
/reset: Cleans the previous state
/base-deployment: Deploys the base infrastructure
/run-all: Launches all client jobs sequentially | /trigger-scenarios: Triggers the scenarios
/copy-results: Copies the results to the master node
/download-results: Downloads the results as a zip file

```

List of endpoints and their examples:
```bash
curl -X POST http://localhost:5050/trigger-scenarios   -H "Content-Type: application/json"   -d '{"message":"Test","changed_files":[<scenario-name>.yaml"]}'
curl -X POST http://localhost:5050/copy-results
curl -X POST http://localhost:5050/base-deployment
curl -X POST http://localhost:5050/reset
curl -X POST http://localhost:5050/run-all
curl -X GET http://localhost:5050/status
curl -X GET http://localhost:5050/download-results -o <zip-name>.zip

```


## Configuration

- All client scenarios are written in `scenarios/*.yaml`
- The server configuration is located at `config/server-config.yaml`
- Each client config is automatically converted into a `ConfigMap` and a `Job`

---

## Kustomize

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

## Rerunning Clients

To rerun all clients or a specific client Job after config changes you can simply call the following endpoint:

```bash
curl -X POST http://localhost:5050/run-all
```

Or 

```bash
curl -X POST http://localhost:5050/trigger-scenarios
```

---

## Monitoring

Use the /status endpoint to get the current status of the cluster and the jobs:

```bash
curl -X GET http://localhost:5050/status
```

Shows:
- Nodes
- Deployments
- Jobs and Pods
- PVCs
- Events

---

## Output

Each test job stores its metrics as CSV under the shared PVC, e.g.:

```
filename = <timestamp>_<scenario-name>.csv
```

1. For client jobs:
```
/measurements/client/filename
```
2. For server jobs:
```
/measurements/server/filename
```



Accessible via:
```
/copy-results
/download-results
```



## Notes
- Image placeholder must be overridden correctly in all `kustomization.yaml`
---

## Contact
For any questions or issues: oguz.altas[a*t]tum.de
---
