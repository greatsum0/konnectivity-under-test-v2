# Konnectivity Under Test (KUT) v2

A comprehensive network performance testing framework designed for testing network connectivity and performance under various traffic patterns. The system consists of a high-performance C++ traffic generator/analyzer and a Python-based Kubernetes controller for orchestration.

## 🏗️ Architecture Overview

KUT v2 is a distributed system with two main components:

1. **C++ Core Engine (`kut`)**: High-performance network traffic generator and analyzer
2. **Python Controller**: Kubernetes-based orchestration and management system

### System Components

```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   Python        │    │   C++ Core      │    │   Kubernetes    │
│   Controller    │◄──►│   Engine (kut)  │◄──►│   Cluster       │
│                 │    │                 │    │                 │
│ • Flask API     │    │ • Traffic Gen   │    │ • Pods          │
│ • K8s Jobs      │    │ • Monitoring    │    │ • Jobs          │
│ • Scenario Mgmt │    │ • Metrics       │    │ • Namespaces    │
└─────────────────┘    └─────────────────┘    └─────────────────┘
```

## 🚀 Quick Start

### Prerequisites

- **C++ Dependencies**:
  - CMake 3.28+
  - C++20 compatible compiler
  - Boost 1.70+ (system, thread)
  - yaml-cpp
  - libpcap
  - Threads library

- **Python Dependencies**:
  - Python 3.8+
  - Flask 3.1.0
  - Kubernetes Python client 32.0.1
  - PyYAML 6.0.2

### Building the C++ Core

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt-get install build-essential cmake libboost-all-dev libyaml-cpp-dev libpcap-dev

# Install dependencies (macOS)
brew install cmake boost yaml-cpp libpcap

# Build
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### Running with Docker

```bash
# Build Docker image
docker build -t kut .

# Run client mode
docker run --rm -v $(pwd)/config:/app/config kut -m client -c /app/config/client.yaml

# Run server mode  
docker run --rm -v $(pwd)/config:/app/config kut -m server -c /app/config/server.yaml
```

### Python Controller Setup

```bash
cd controller
pip install -r requirements.txt
python main.py
```

The controller will start on `http://localhost:5050` with Swagger API documentation available at `http://localhost:5050/apidocs`.

## 📋 Usage

### Command Line Interface

The C++ core supports two modes of operation:

```bash
# Client mode - generates traffic
./kut -m client -c scenario.yaml

# Server mode - receives and measures traffic
./kut -m server -c server.yaml
```

### Configuration Files

#### Client Configuration Example

```yaml
scenario:
  meta:
    name: "high_load_test"
  
  network:
    protocol: "tcp"  # or "udp"
    endpoint:
      ip: "192.168.1.100"
      port: 8080
      control_port: 60001
  
  traffic_pattern:
    type: "constant"
    duration_sec: 60.0
    interval_sec: 0.001
    packet_size: 1024
    parameters:
      rate_mbps: 100.0
  
  metric_collection:
    interval_sec: 1.0
    output_path: "/tmp/metrics"
    interface: "eth0"
    metrics: ["net_pck_len", "net_lat_owd"]
```

#### Server Configuration Example

```yaml
server:
  meta:
    name: "traffic_receiver"
  
  network:
    protocol: "tcp"
    endpoint:
      port: 8080
      control_port: 60001
  
  metric_collection:
    interval_sec: 1.0
    output_path: "/tmp/metrics"
    interface: "eth0"
```

## 🎯 Traffic Patterns

KUT supports multiple traffic generation patterns:

### 1. Constant Traffic
Steady rate traffic generation.

```yaml
traffic_pattern:
  type: "constant"
  parameters:
    rate_mbps: 50.0
```

### 2. Burst Traffic
Periodic burst patterns with configurable intervals.

```yaml
traffic_pattern:
  type: "burst"
  parameters:
    rate_mbps: 100.0
    burst_size_mb: 10.0
    burst_duration_sec: 5.0
    burst_interval_sec: 10.0
```

### 3. Pure Burst
Burst-only traffic (no background traffic).

```yaml
traffic_pattern:
  type: "pure_burst"
  parameters:
    rate_mbps: 200.0
    burst_size_mb: 5.0
    burst_duration_sec: 2.0
    burst_interval_sec: 8.0
```

### 4. Random Burst
Randomized burst patterns with configurable ranges.

```yaml
traffic_pattern:
  type: "random_burst"
  parameters:
    rate_mbps: 150.0
    min_burst_size_mb: 2.0
    max_burst_size_mb: 8.0
    min_burst_duration_ms: 1000
    max_burst_duration_ms: 5000
    min_burst_interval_ms: 2000
    max_burst_interval_ms: 10000
    random_seed: 42
```

### 5. Sinusoidal Traffic
Sinusoidal rate variation.

```yaml
traffic_pattern:
  type: "sinusoidal"
  parameters:
    rate_mbps: 75.0
    amplitude: 25.0
    frequency: 0.1  # Hz
```

### 6. Ramp Up/Down
Gradual rate increase or decrease.

```yaml
traffic_pattern:
  type: "ramp_up"
  parameters:
    rate_mbps: 100.0
    ramp_up_time: 30.0  # seconds
```

## 📊 Monitoring and Metrics

### Network Metrics

- **Packet Length**: `net_pck_len` - Size of captured packets
- **One-Way Delay**: `net_lat_owd` - Network latency measurements
- **CPU Usage**: System CPU utilization during tests

### Metric Collection

Metrics are collected using:
- **libpcap**: For network packet capture and analysis
- **CPU Monitoring**: System resource utilization tracking
- **CSV Output**: Structured data export for analysis

### Output Format

Metrics are exported as CSV files with the following structure:

```csv
timestamp,net_pck_len,net_lat_owd,cpu_user,cpu_system
2025-01-15T10:30:01.123456,1024,0.5,25.3,12.1
2025-01-15T10:30:02.123456,1024,0.6,26.1,11.8
```

## 🎮 Python Controller API

The Python controller provides a REST API for managing test scenarios:

### Key Endpoints

- `GET /scenarios` - List available scenario files
- `POST /scenarios/upload` - Upload new scenario configurations
- `GET /jobs?namespace=<ns>` - List Kubernetes jobs
- `DELETE /jobs/delete` - Delete jobs
- `GET /pods?namespace=<ns>` - List pods
- `POST /apply` - Apply Kubernetes manifests

### Example API Usage

```bash
# Upload a scenario
curl -X POST -F "files=@scenario.yaml" http://localhost:5050/scenarios/upload

# List jobs
curl "http://localhost:5050/jobs?namespace=kut-test"

# Delete all jobs in namespace
curl -X DELETE -H "Content-Type: application/json" \
  -d '{"namespace": "kut-test", "jobs": ["*"]}' \
  http://localhost:5050/jobs/delete
```

## 🏗️ Architecture Details

### C++ Core Components

#### Network Layer
- **TCP/UDP Clients**: High-performance network clients
- **TCP/UDP Servers**: Traffic receivers and analyzers
- **Control Channel**: Synchronization between client and server
- **Connection Management**: Robust connection handling with Boost.Asio

#### Traffic Generation
- **Pattern Factory**: Creates traffic patterns based on configuration
- **Timing Engine**: Precise packet timing and rate control
- **Buffer Management**: Efficient packet buffering and transmission

#### Monitoring
- **Packet Sniffer**: Real-time network packet capture using libpcap
- **CPU Monitor**: System resource utilization tracking
- **Metric Aggregation**: Data collection and processing

#### Configuration
- **YAML Parser**: Configuration file parsing and validation
- **Context Management**: Runtime configuration state
- **CLI Parser**: Command-line argument processing

### Python Controller Components

#### Flask Application
- **Blueprint Architecture**: Modular route organization
- **REST API**: Comprehensive API for scenario management
- **Swagger Documentation**: Interactive API documentation

#### Kubernetes Integration
- **Job Management**: Create, monitor, and delete Kubernetes jobs
- **Pod Operations**: Pod lifecycle management
- **Namespace Handling**: Multi-tenant scenario execution

#### File Management
- **Scenario Storage**: YAML configuration file management
- **Directory Utilities**: File system operations
- **Security**: Secure file upload and validation

## 🔧 Development

### Project Structure

```
├── src/                    # C++ source code
│   ├── context/           # Configuration data models
│   ├── monitor/           # Performance monitoring
│   ├── network/           # Network communication
│   ├── parser/            # Configuration parsing
│   ├── pattern/           # Traffic pattern implementations
│   ├── session/           # Client/server sessions
│   └── utils/             # Utility functions
├── controller/            # Python controller
│   ├── app/              # Flask application
│   │   ├── routes/       # API endpoints
│   │   └── utils/        # Utility functions
│   └── requirements.txt  # Python dependencies
├── CMakeLists.txt        # Build configuration
└── Dockerfile           # Container configuration
```

### Building from Source

```bash
# Clone repository
git clone <repository-url>
cd konnectivity-under-test-v2

# Build C++ core
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)

# Setup Python environment
cd ../controller
python -m venv venv
source venv/bin/activate  # On Windows: venv\Scripts\activate
pip install -r requirements.txt
```

### Testing

```bash
# Run C++ tests (if available)
cd build
ctest

# Run Python tests
cd controller
python -m pytest tests/
```

## 📝 Configuration Reference

### Client Context Schema

```yaml
scenario:
  meta:
    name: string              # Test scenario name
  
  network:
    protocol: string          # "tcp" or "udp"
    endpoint:
      ip: string             # Target IP address
      port: integer          # Target port
      control_port: integer  # Control channel port (default: 60001)
  
  traffic_pattern:
    type: string             # Traffic pattern type
    duration_sec: float      # Test duration
    interval_sec: float      # Packet interval
    packet_size: integer     # Packet size in bytes
    parameters:              # Pattern-specific parameters
      # ... (see traffic patterns section)
  
  metric_collection:
    interval_sec: float      # Metric collection interval
    output_path: string      # Output directory
    interface: string        # Network interface
    metrics: [string]        # List of metrics to collect
```

### Server Context Schema

```yaml
server:
  meta:
    name: string              # Server instance name
  
  network:
    protocol: string          # "tcp" or "udp"
    endpoint:
      port: integer          # Listen port
      control_port: integer  # Control channel port (default: 60001)
  
  metric_collection:
    interval_sec: float      # Metric collection interval
    output_path: string      # Output directory
    interface: string        # Network interface
```

## 🐛 Troubleshooting

### Common Issues

1. **Build Failures**
   - Ensure all dependencies are installed
   - Check CMake version (3.28+ required)
   - Verify C++20 compiler support

2. **Network Issues**
   - Check firewall settings
   - Verify network interface permissions
   - Ensure libpcap has proper privileges

3. **Kubernetes Issues**
   - Verify kubeconfig is properly set
   - Check namespace permissions
   - Ensure controller can access cluster

### Debug Mode

Enable debug logging:

```bash
# Set log level to DEBUG in the application
export KUT_LOG_LEVEL=DEBUG
./kut -m client -c scenario.yaml
```

## 📄 License

This project is licensed under the terms specified in the LICENSE file.

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests if applicable
5. Submit a pull request

## 📞 Support

For issues and questions:
- Create an issue in the repository
- Check the troubleshooting section
- Review the API documentation at `/apidocs` when running the controller
