# SWG:ANH Modern Server

A modern, beginner-friendly Star Wars Galaxies server emulator targeting Publish 14.1 compatibility.

## 🚀 Quick Start

```bash
# Clone the repository
git clone https://github.com/SWGANHServices/swganh-modern.git
cd swganh-modern

# Start development environment
docker-compose -f docker/dev/docker-compose.dev.yml up
```

## 🎯 Project Goals

- **Beginner-Friendly**: Easy setup and contribution process
- **Modern Architecture**: C++ core with Lua scripting layer
- **SWG 14.1 Compatible**: Full Pre-CU experience
- **Modular Design**: Plugin-based extensibility
- **Production Ready**: Docker deployment and monitoring

## 🏗️ Project Relationship

This project is part of the [SWGANHServices](https://github.com/SWGANHServices) organization's effort to modernize Star Wars Galaxies emulation:

- **swganh-modern** (this project) - Modern C++/Lua server core
- **SWGANHJava** - Java-based emulation reference
- **swg-anh** - Unity client projects

Our goal is to create a beginner-friendly, maintainable codebase while preserving compatibility with existing SWGANH community standards.

## 📖 Documentation

### Getting Started
- [Quick Start Guide](docs/getting-started/quickstart.md) - Get up and running in 10 minutes
- [Development Setup](docs/getting-started/development-setup.md) - Complete development environment
- [First Contribution](docs/getting-started/first-contribution.md) - Make your first code contribution

### Architecture
- [System Overview](docs/architecture/overview.md) - High-level architecture design
- [Networking Layer](docs/architecture/networking.md) - SOE protocol implementation
- [Database Design](docs/architecture/database.md) - Data persistence strategy
- [Plugin System](docs/architecture/plugins.md) - Modular plugin architecture

### API Reference
- [Core API](docs/api/core-api.md) - C++ core system APIs
- [Lua API](docs/api/lua-api.md) - Scripting interface reference
- [Plugin API](docs/api/plugin-api.md) - Plugin development interface

### Deployment
- [Docker Guide](docs/deployment/docker.md) - Container deployment
- [Production Setup](docs/deployment/production.md) - Production deployment guide
- [Monitoring](docs/deployment/monitoring.md) - System monitoring and alerting

## 🤝 Contributing

We welcome contributors of all skill levels! 

**For Beginners:**
- Look for issues labeled [`good-first-issue`](https://github.com/SWGANHServices/swganh-modern/labels/good-first-issue)
- Join our [Discord Community](https://discord.gg/your-discord) for real-time help
- Read our [First Contribution Guide](docs/getting-started/first-contribution.md)

**For Experienced Developers:**
- Check our [Contributing Guide](.github/CONTRIBUTING.md)
- Review our [Architecture Documentation](docs/architecture/overview.md)
- Explore [Open Issues](https://github.com/SWGANHServices/swganh-modern/issues)

## 🛠️ Current Status

**Phase 1: Foundation (In Progress)**
- [x] Project structure and documentation
- [x] Docker development environment
- [ ] Basic SOE protocol implementation
- [ ] Plugin system framework
- [ ] Database schema design

**Phase 2: Core Systems (Planned)**
- [ ] Character creation and management
- [ ] World state synchronization
- [ ] Basic game mechanics
- [ ] Administrative tools

## 📊 Project Stats

![GitHub issues](https://img.shields.io/github/issues/SWGANHServices/swganh-modern)
![GitHub pull requests](https://img.shields.io/github/issues-pr/SWGANHServices/swganh-modern)
![GitHub contributors](https://img.shields.io/github/contributors/SWGANHServices/swganh-modern)
![GitHub last commit](https://img.shields.io/github/last-commit/SWGANHServices/swganh-modern)

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- Original SWGANH development team
- SWGEmu/SWG:ANH community for protocol research
- All contributors and community members

---

**Ready to contribute?** Start with our [Quick Start Guide](docs/getting-started/quickstart.md)!

---
