# Konnectivity Under Test (KUT)

This project is a C++ command-line tool designed to measure network performance metrics such as throughput, latency, and CPU usage under various traffic conditions. The tool supports both client and server modes, enabling comprehensive network testing in diverse scenarios.

## Features

- **Client and Server Modes:**
    - `server` mode to receive traffic.
    - `client` mode to generate and send traffic to a specified server.
- **Traffic Shaping:** Supports different traffic shapes for better simulation of real-world scenarios.
- **Metrics Collection:** Measures various network performance metrics, specified via a comma-separated list.
- **Protocol Support:** Supports both TCP and UDP protocols.
- **Logging and Verbosity:** Optional logging to a file and adjustable verbosity levels for detailed output.
- **Command-Line Argument Parsing:** Extensive support for command-line arguments for flexibility and configurability.

## Command-Line Arguments

| Short Option | Long Option        | Description                                                 | Required          |
|--------------|--------------------|-------------------------------------------------------------|--------------------|
| `-m`         | `--mode`           | Mode of operation (`server` or `client`).                   | Yes                |
| `-s`         | `--server`         | Server IP address (required in `client` mode).              | Yes (in client)    |
| `-p`         | `--port`           | Port number (range: 1024–65535).                            | Yes                |
| `-v`         | `--verbose`        | Enable verbose logging.                                     | No                 |
| `-f`         | `--file`           | Log file path.                                              | No                 |
| `-d`         | `--duration`       | Duration of the experiment in seconds (required in `client` mode). | Yes (in client) |
| `-i`         | `--interval`       | Data logging interval in seconds (default: 1s).             | No                 |
| `-r`         | `--rate`           | Bandwidth rate in Mbps (required in `client` mode).         | Yes (in client)    |
| `-t`         | `--traffic-shape`  | Traffic shape (e.g., constant).                             | No                 |
| `-l`         | `--metric-list`    | Comma-separated list of metrics to measure (e.g., throughput, latency). | Yes         |
|              | `--protocol`       | Protocol to use (`tcp` or `udp`, default: `tcp`).           | No                 |
| `-h`         | `--help`           | Show help message.                                          | No                 |

### Usage Examples

#### Server Mode

```bash
./kut -m server -p 8080
```

#### Client Mode

```bash
./kut -m client -s 192.168.1.1 -p 8080 -d 60 -r 100 -l throughput,latency --protocol tcp
```

## Building the Project

### Prerequisites

- A C++17 compatible compiler.
- CMake version 3.10 or higher.

### Build Instructions

1. Clone the repository:

   ```bash
   git clone <repo-url>
   cd konnectivity-under-test
   ```

2. Create a build directory and navigate to it:

   ```bash
   mkdir build && cd build
   ```

3. Run CMake and build the project:

   ```bash
   cmake ..
   make
   ```

4. The executable `kut` will be generated in the `build` directory.

## Code Structure

## Error Handling

The application performs comprehensive error checking on the command-line arguments:

- Ensures required arguments are provided (`mode`, `port`, etc.).
- Validates numeric ranges for arguments like `port` (1024–65535), `duration`, `interval`, and `rate`.
- Provides detailed error messages and usage instructions for incorrect or missing arguments.

## GitHub Actions
A CI/CD pipeline and a lightweight Kubernetes setup (`k3s`) is provided to build, package, and deploy the kut application across a fleet of Raspberry Pi devices in this framework.

## Development & CI/CD
Development happens on a local machine, and all code changes are pushed to GitHub. When updates are merged into the branch named `release`, a GitHub Actions workflow triggers automatically. It builds the application, creating a Docker image, and pushes it to a private Docker registry.

![Development Process](./readme_assets/development-process.png)

## Deployment Architecture
The `k3s` cluster is consist of three Raspberry Pi devices:

- `pi-c` acts as the master node and handles orchestration.
- `pi-a` runs as the client pod.
- `pi-b` runs as the server pod.

Deployment is managed directly from `pi-c` using a `deployment.yaml` file. Both the client and server run from the same Docker image, with different cli arguments applied at runtime.

![Deployment Process](./readme_assets/basic-infra-measurement.png)

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for more details.

## Contributing

Contributions are welcome! Please open an issue or submit a pull request for any changes or improvements.

## Contact

For questions or support, please contact [oguz.altas@tum.de](mailto:oguz.altas@tum.de).

---
