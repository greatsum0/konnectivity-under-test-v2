# Konnectivity Under Test (KUT)

Konnectivity Under Test (KUT) is a modular C++ benchmarking tool designed to simulate network traffic and measure
performance in a controlled Kubernetes environment. It supports both **server** and **client** roles and is optimized
for deployment on lightweight clusters like `k3s`.

---

## Features

- Dual-mode architecture: **Client** (Job) and **Server** (Deployment)
- Protocols: TCP and UDP
- Traffic Patterns: Constant, burst (burst, random, pure), sinusoidal, ramp (up, down)
- Scenario-based testing: YAML-driven configuration
- Metric Collection: Throughput, transferred bytes, CPU utilization (tool/system), and more
- Built-in API for easy integration (Flask-based)
- Logging and CSV export of results
- Designed for containerized deployment with **Docker** and **Kubernetes**
- Supports **Persistent Volume Claims (PVCs)** for result collection

---

## Project Structure

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

- **Server**: Long-running `Deployment` on `raspberrypi-b`
- **Client**: Single-run `Job` on `raspberrypi-a`
- **Master**: `pi-c` handles control plane and orchestration
- All configurations are mounted using **ConfigMaps**
- Results are stored on shared PVCs, accessible via the `pvc-viewer`

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

3. **Start the Flask API controller**:
   ```bash
   cd k3s-deployment/api
   python3 -m venv .venv
   source .venv/bin/activate
   pip install -r requirements.txt
   python app.py
   ```
   The server starts at `localhost:5050`. Port-forward it as needed for cluster access.

4. **Deploy the server** and use the Swagger UI at `localhost:5050/apidocs` to explore API endpoints.

5. **Use the following API endpoints for orchestration**:
   ```
   /reset             # Cleans the previous state
   /base-deployment   # Deploys the base infrastructure
   /run-all           # Launches all client jobs sequentially
   /trigger-scenarios # Triggers selected scenario jobs
   /copy-results      # Copies results to master node
   /download-results  # Downloads results as a ZIP file
   ```

   Example usage:
   ```bash
   curl -X POST http://localhost:5050/trigger-scenarios -H "Content-Type: application/json" -d '{"message":"Test","changed_files":["<scenario-name>.yaml"]}'
   curl -X POST http://localhost:5050/copy-results
   curl -X POST http://localhost:5050/base-deployment
   curl -X POST http://localhost:5050/reset
   curl -X POST http://localhost:5050/run-all
   curl -X GET  http://localhost:5050/status
   curl -X GET  http://localhost:5050/download-results -o <zip-name>.zip
   ```

---

## Configuration

- Client scenarios: `scenarios/*.yaml`
- Server config: `config/server-config.yaml`
- All client configs are converted into Kubernetes `ConfigMap` and `Job` resources automatically.

---

## Kustomize

Each Kubernetes resource category has its own `kustomization.yaml`, and the root includes all of them.

Image override:

```yaml
images:
  - name: placeholder/temp-image
    newName: oguznagak/kut-tester
    newTag: latest
```

---

## Rerunning Clients

To rerun all clients:

```bash
curl -X POST http://localhost:5050/run-all
```

Or to rerun specific scenarios:

```bash
curl -X POST http://localhost:5050/trigger-scenarios
```

---

## Monitoring

Check current job and node status:

```bash
curl -X GET http://localhost:5050/status
```

Returns:

- Nodes
- Deployments
- Jobs and Pods
- PVCs
- Kubernetes Events

---

## Output

Metrics are saved as CSV files under the shared PVC:

```
<timestamp>_<scenario-name>.csv
```

- Client results: `/measurements/client/<filename>`
- Server results: `/measurements/server/<filename>`

You can access these using:

```
/copy-results
/download-results
```

---

## Notes

- The image placeholder must be overridden in all relevant `kustomization.yaml` files.

---

## Contact

For any questions or issues, feel free to reach out via the GitHub repository issues page.
