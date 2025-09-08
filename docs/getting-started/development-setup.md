# Development Setup Guide

Complete guide for setting up a development environment for SWG:ANH Modern.

## 🛠️ Prerequisites

### Required Software

| Software | Version | Download |
|----------|---------|----------|
| Docker Desktop | Latest | [Download](https://www.docker.com/products/docker-desktop) |
| Git | 2.30+ | [Download](https://git-scm.com/downloads) |
| Visual Studio Code | Latest | [Download](https://code.visualstudio.com/) |

### Optional (for advanced development)

| Software | Purpose | Download |
|----------|---------|----------|
| Visual Studio 2022 | Windows C++ development | [Download](https://visualstudio.microsoft.com/) |
| CMake | Build system | [Download](https://cmake.org/download/) |
| vcpkg | C++ package manager | [GitHub](https://github.com/Microsoft/vcpkg) |

## 🐳 Docker Development Environment

### Starting the Environment

```bash
# Clone and enter project
git clone https://github.com/SWGANHServices/swganh-modern.git
cd swganh-modern

# Start all services
docker-compose -f docker/dev/docker-compose.dev.yml up -d

# Verify all services are running
docker-compose -f docker/dev/docker-compose.dev.yml ps
```

### Environment Variables

Create `.env` file in project root:

```bash
# Database Configuration
POSTGRES_DB=swganh_dev
POSTGRES_USER=swganh
POSTGRES_PASSWORD=devpassword

# Server Configuration
DEBUG_MODE=1
LOG_LEVEL=DEBUG
PLUGIN_AUTO_RELOAD=1

# Development Settings
HOT_RELOAD_SCRIPTS=1
ENABLE_PROFILING=1
```

## 🔧 Visual Studio Code Setup

### Required Extensions

Install these extensions:

```bash
# C++ development
code --install-extension ms-vscode.cpptools-extension-pack
code --install-extension ms-vscode.cmake-tools

# Docker support
code --install-extension ms-azuretools.vscode-docker

# Lua development
code --install-extension sumneko.lua

# Git integration
code --install-extension eamodio.gitlens
```

### Workspace Configuration

Create `.vscode/settings.json`:

```json
{
    "C_Cpp.default.configurationProvider": "ms-vscode.cmake-tools",
    "C_Cpp.default.cppStandard": "c++20",
    "C_Cpp.default.intelliSenseMode": "gcc-x64",
    "cmake.buildDirectory": "${workspaceFolder}/build",
    "cmake.generator": "Unix Makefiles",
    "files.associations": {
        "*.lua": "lua",
        "*.sql": "sql"
    },
    "Lua.workspace.library": [
        "${workspaceFolder}/scripts/api"
    ],
    "docker.compose.files": [
        "docker/dev/docker-compose.dev.yml"
    ]
}
```

## 🏗️ Build System

### Build Commands

```bash
# Enter development container
docker-compose -f docker/dev/docker-compose.dev.yml exec swganh-dev bash

# Configure build
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug -DENABLE_TESTING=ON

# Build project
make -j$(nproc)

# Run specific targets
make login_server      # Build login server only
make zone_server       # Build zone server only
make core_tests        # Build core tests
make plugins          # Build all plugins
```

### Build Options

| Option | Description | Default |
|--------|-------------|---------|
| `CMAKE_BUILD_TYPE` | Build type (Debug/Release/RelWithDebInfo) | Debug |
| `ENABLE_TESTING` | Enable unit tests | ON |
| `ENABLE_PROFILING` | Enable performance profiling | OFF |
| `BUILD_PLUGINS` | Build plugin system | ON |
| `BUILD_TOOLS` | Build development tools | ON |

## 🧪 Testing Framework

### Running Tests

```bash
# Run all tests
make test

# Run specific test suites
ctest -R core_tests
ctest -R network_tests
ctest -R plugin_tests

# Run with verbose output
ctest -V
```

## 🚀 Next Steps

1. **[Make Your First Contribution](first-contribution.md)**
2. **[Understanding the Architecture](../architecture/overview.md)**
3. **[Plugin Development](../api/plugin-api.md)**
4. **[Core API Reference](../api/core-api.md)**

---

**Ready to start coding?** Check out our [First Contribution Guide](first-contribution.md)!

---
