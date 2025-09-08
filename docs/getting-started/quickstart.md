# Quick Start Guide

Get up and running with SWG:ANH Modern in under 10 minutes!

## Prerequisites

- [Docker Desktop](https://www.docker.com/products/docker-desktop) (Windows/Mac/Linux)
- [Git](https://git-scm.com/downloads)
- [Visual Studio Code](https://code.visualstudio.com/) (recommended)

## 🚀 5-Minute Setup

### 1. Clone the Repository

```bash
git clone https://github.com/SWGANHServices/swganh-modern.git
cd swganh-modern
```

### 2. Start Development Environment

```bash
# Start all services (database, cache, server)
docker-compose -f docker/dev/docker-compose.dev.yml up -d

# View logs
docker-compose -f docker/dev/docker-compose.dev.yml logs -f
```

### 3. Verify Installation

Open your browser and navigate to:
- **Server Status**: http://localhost:8080/status
- **Admin Panel**: http://localhost:8080/admin
- **API Documentation**: http://localhost:8080/docs

### 4. Test Client Connection

```bash
# Test basic connectivity
telnet localhost 44453
```

## 🎮 Connect Your SWG Client

### Client Requirements

- Star Wars Galaxies client patched to **version 14.1**
- SWG:ANH client modifications (provided in `data/client-patches/`)

### Configuration

1. Copy `swganh.cfg` from `data/client-patches/` to your SWG client directory
2. Update the server address in `swganh.cfg`:
   ```ini
   [ClientGame]
   loginServerAddress0=127.0.0.1
   loginServerPort0=44453
   ```

3. Launch the client:
   ```bash
   SWGClient_r.exe -- -s Station subscriptionFeatures=1 gameFeatures=255
   ```

## 🧩 Your First Plugin

Create a simple "Hello World" plugin:

1. **Create plugin directory:**
   ```bash
   mkdir src/plugins/hello-world
   ```

2. **Create `hello_world.lua`:**
   ```lua
   -- src/plugins/hello-world/hello_world.lua
   local HelloWorld = {}

   function HelloWorld:onPlayerLogin(player)
       player:sendSystemMessage("Hello, " .. player:getName() .. "! Welcome to SWG:ANH Modern!")
   end

   function HelloWorld:onServerStart()
       print("Hello World plugin loaded!")
   end

   return HelloWorld
   ```

3. **Register the plugin in `data/config/plugins.json`:**
   ```json
   {
       "enabled_plugins": [
           "hello-world"
       ]
   }
   ```

4. **Restart the server and test!**

## ⚡ Quick Commands

```bash
# Start development environment
docker-compose -f docker/dev/docker-compose.dev.yml up -d

# Stop environment
docker-compose -f docker/dev/docker-compose.dev.yml down

# View server logs
docker-compose -f docker/dev/docker-compose.dev.yml logs -f swganh-dev

# Rebuild containers
docker-compose -f docker/dev/docker-compose.dev.yml build

# Access development container
docker-compose -f docker/dev/docker-compose.dev.yml exec swganh-dev bash

# Reset database
docker-compose -f docker/dev/docker-compose.dev.yml down -v
docker-compose -f docker/dev/docker-compose.dev.yml up -d
```

## 🎯 Next Steps

1. **[Complete Development Setup](development-setup.md)** - Full development environment
2. **[Make Your First Contribution](first-contribution.md)** - Contribute to the project
3. **[Understanding the Architecture](../architecture/overview.md)** - Learn how it all works
4. **[Plugin Development Guide](../api/plugin-api.md)** - Create custom game features

## 🤝 Need Help?

- **Discord**: [Join our community](https://discord.gg/your-discord)
- **GitHub Issues**: [Report bugs or ask questions](https://github.com/SWGANHServices/swganh-modern/issues)
- **Documentation**: Browse the full documentation in the `docs/` folder

Welcome to the SWG:ANH Modern community! 🌟

---
