#!/bin/bash

# scripts/setup.sh - One-time setup script for SWG:ANH Modern

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

log_info() {
    echo -e "${BLUE}[SETUP]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

show_banner() {
    echo -e "${CYAN}"
    echo "╔══════════════════════════════════════════════════════════════╗"
    echo "║               SWG:ANH Modern Project Setup                  ║"
    echo "╚══════════════════════════════════════════════════════════════╝"
    echo -e "${NC}"
}

show_banner

log_info "Setting up SWG:ANH Modern development environment..."

# Make all scripts executable
log_info "Making scripts executable..."
chmod +x scripts/*.sh

# Create necessary directories
log_info "Creating directory structure..."
mkdir -p data/config
mkdir -p data/sql  
mkdir -p logs
mkdir -p build
mkdir -p tests

# Create .env file if it doesn't exist
if [[ ! -f ".env" ]]; then
    log_info "Creating .env file..."
    cat > .env << 'EOF'
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
EOF
else
    log_info ".env file already exists"
fi

# Create server config if it doesn't exist
if [[ ! -f "data/config/server.json" ]]; then
    log_info "Creating server configuration..."
    cat > data/config/server.json << 'EOF'
{
    "login_server": {
        "bind_address": "0.0.0.0",
        "port": 44453,
        "galaxy_name": "SWG:ANH Modern",
        "galaxy_id": 1,
        "max_players": 3000
    },
    "database": {
        "host": "postgres",
        "port": 5432,
        "database": "swganh_dev",
        "username": "swganh",
        "password": "devpassword"
    },
    "logging": {
        "level": "DEBUG",
        "file": "logs/server.log"
    }
}
EOF
else
    log_info "Server configuration already exists"
fi

# Create database schema if it doesn't exist
if [[ ! -f "data/sql/001_initial_schema.sql" ]]; then
    log_info "Creating database schema..."
    cat > data/sql/001_initial_schema.sql << 'EOF'
-- Initial database schema for SWG:ANH Modern

-- Account management
CREATE TABLE IF NOT EXISTS accounts (
    id SERIAL PRIMARY KEY,
    username VARCHAR(50) UNIQUE NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    email VARCHAR(100),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    last_login TIMESTAMP,
    is_active BOOLEAN DEFAULT true
);

-- Characters (basic structure for future)
CREATE TABLE IF NOT EXISTS characters (
    id SERIAL PRIMARY KEY,
    account_id INTEGER REFERENCES accounts(id),
    name VARCHAR(50) UNIQUE NOT NULL,
    species VARCHAR(20) NOT NULL,
    profession VARCHAR(20) NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    last_played TIMESTAMP
);

-- Server status tracking
CREATE TABLE IF NOT EXISTS server_status (
    id SERIAL PRIMARY KEY,
    galaxy_id INTEGER NOT NULL,
    galaxy_name VARCHAR(50) NOT NULL,
    online BOOLEAN DEFAULT true,
    current_players INTEGER DEFAULT 0,
    max_players INTEGER DEFAULT 3000,
    last_update TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Insert default galaxy
INSERT INTO server_status (galaxy_id, galaxy_name, online, max_players)
VALUES (1, 'SWG:ANH Modern', true, 3000)
ON CONFLICT DO NOTHING;
EOF
else
    log_info "Database schema already exists"
fi

# Create basic gitignore additions if needed
if ! grep -q "# SWG:ANH Modern specific" .gitignore 2>/dev/null; then
    log_info "Adding to .gitignore..."
    cat >> .gitignore << 'EOF'

# SWG:ANH Modern specific
logs/
*.log
server.pid
.env.local
data/config/local/
EOF
fi

log_success "Setup completed successfully!"
echo ""
log_info "Next steps:"
echo "  1. Build and test:"
echo "     ./scripts/build.sh -T"
echo ""
echo "  2. Start development environment:"
echo "     ./scripts/dev.sh start"
echo ""
echo "  3. OR start locally:"
echo "     ./scripts/start.sh"
echo ""
echo "  4. Connect SWG client to localhost:44453"
echo ""
log_info "For help with any script, use: ./scripts/SCRIPT.sh --help"
