#!/bin/bash

# scripts/dev.sh - Development environment setup and management

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Logging functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
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

log_dev() {
    echo -e "${PURPLE}[DEV]${NC} $1"
}

# Show banner
show_banner() {
    echo -e "${CYAN}"
    echo "╔══════════════════════════════════════════════════════════════╗"
    echo "║                 SWG:ANH Modern Development                  ║"
    echo "║                     Environment Manager                     ║"
    echo "╚══════════════════════════════════════════════════════════════╝"
    echo -e "${NC}"
}

# Show help
show_help() {
    show_banner
    echo ""
    echo "Usage: $0 [command] [options]"
    echo ""
    echo "Commands:"
    echo "  start       Start development environment"
    echo "  stop        Stop development environment"
    echo "  restart     Restart development environment"
    echo "  build       Build the project in development container"
    echo "  test        Run tests in development container"
    echo "  shell       Open shell in development container"
    echo "  logs        Show container logs"
    echo "  status      Show development environment status"
    echo "  clean       Clean up development environment"
    echo "  setup       Initial development setup"
    echo ""
    echo "Options:"
    echo "  -d, --detach    Run in background (for start command)"
    echo "  -v, --verbose   Verbose output"
    echo "  -f, --force     Force operation (for clean command)"
    echo "  -h, --help      Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0 start                # Start development environment"
    echo "  $0 start --detach       # Start in background"
    echo "  $0 build                # Build project"
    echo "  $0 shell                # Open development shell"
    echo "  $0 logs login_server    # Show login server logs"
}

# Check if Docker is available
check_docker() {
    if ! command -v docker &> /dev/null; then
        log_error "Docker is not installed or not in PATH"
        echo "Please install Docker Desktop from: https://www.docker.com/products/docker-desktop"
        exit 1
    fi
    
    if ! command -v docker-compose &> /dev/null; then
        log_error "docker-compose is not installed or not in PATH"
        echo "Please install docker-compose or use Docker Desktop which includes it"
        exit 1
    fi
    
    # Check if Docker daemon is running
    if ! docker info &> /dev/null; then
        log_error "Docker daemon is not running"
        echo "Please start Docker Desktop or the Docker daemon"
        exit 1
    fi
}

# Get project root directory
get_project_root() {
    # Try to find project root by looking for CMakeLists.txt
    local dir="$(pwd)"
    while [[ "$dir" != "/" ]]; do
        if [[ -f "$dir/CMakeLists.txt" ]]; then
            echo "$dir"
            return 0
        fi
        dir="$(dirname "$dir")"
    done
    
    log_error "Could not find project root (looking for CMakeLists.txt)"
    exit 1
}

# Docker Compose file path
get_compose_file() {
    local project_root="$(get_project_root)"
    echo "$project_root/docker/dev/docker-compose.dev.yml"
}

# Start development environment
start_dev() {
    local detach=0
    local verbose=0
    
    # Parse options
    while [[ $# -gt 0 ]]; do
        case $1 in
            -d|--detach)
                detach=1
                shift
                ;;
            -v|--verbose)
                verbose=1
                shift
                ;;
            *)
                log_error "Unknown option for start: $1"
                exit 1
                ;;
        esac
    done
    
    local compose_file="$(get_compose_file)"
    
    if [[ ! -f "$compose_file" ]]; then
        log_error "Docker compose file not found: $compose_file"
        exit 1
    fi
    
    log_info "Starting SWG:ANH Modern development environment..."
    
    # Pull latest images
    log_info "Pulling latest Docker images..."
    docker-compose -f "$compose_file" pull
    
    # Start services
    if [[ $detach -eq 1 ]]; then
        log_info "Starting services in background..."
        docker-compose -f "$compose_file" up -d --build
        
        log_success "Development environment started in background"
        log_info "Use '$0 logs' to view logs"
        log_info "Use '$0 shell' to access development container"
    else
        log_info "Starting services (use Ctrl+C to stop)..."
        docker-compose -f "$compose_file" up --build
    fi
}

# Stop development environment
stop_dev() {
    local compose_file="$(get_compose_file)"
    
    log_info "Stopping SWG:ANH Modern development environment..."
    docker-compose -f "$compose_file" down
    
    log_success "Development environment stopped"
}

# Restart development environment
restart_dev() {
    log_info "Restarting SWG:ANH Modern development environment..."
    stop_dev
    start_dev --detach
}

# Build project in development container
build_dev() {
    local compose_file="$(get_compose_file)"
    
    log_info "Building project in development container..."
    
    # Make sure development container is running
    docker-compose -f "$compose_file" up -d swganh-dev
    
    # Run build
    docker-compose -f "$compose_file" exec swganh-dev bash -c "
        cd /workspace &&
        ./scripts/build.sh -T
    "
    
    if [[ $? -eq 0 ]]; then
        log_success "Build completed successfully"
    else
        log_error "Build failed"
        exit 1
    fi
}

# Run tests in development container
test_dev() {
    local compose_file="$(get_compose_file)"
    
    log_info "Running tests in development container..."
    
    # Make sure development container is running
    docker-compose -f "$compose_file" up -d swganh-dev
    
    # Run tests
    docker-compose -f "$compose_file" exec swganh-dev bash -c "
        cd /workspace/build &&
        ctest --output-on-failure --parallel 4
    "
}

# Open shell in development container
shell_dev() {
    local compose_file="$(get_compose_file)"
    
    log_info "Opening shell in development container..."
    
    # Make sure development container is running
    docker-compose -f "$compose_file" up -d swganh-dev
    
    # Open interactive shell
    docker-compose -f "$compose_file" exec swganh-dev bash
}

# Show logs
logs_dev() {
    local compose_file="$(get_compose_file)"
    local service="$1"
    
    if [[ -n "$service" ]]; then
        log_info "Showing logs for service: $service"
        docker-compose -f "$compose_file" logs -f "$service"
    else
        log_info "Showing logs for all services"
        docker-compose -f "$compose_file" logs -f
    fi
}

# Show status
status_dev() {
    local compose_file="$(get_compose_file)"
    
    log_info "Development environment status:"
    echo ""
    
    # Show container status
    docker-compose -f "$compose_file" ps
    
    echo ""
    
    # Show resource usage
    log_info "Resource usage:"
    docker stats --no-stream --format "table {{.Container}}\t{{.CPUPerc}}\t{{.MemUsage}}\t{{.NetIO}}" $(docker-compose -f "$compose_file" ps -q) 2>/dev/null || echo "No containers running"
    
    echo ""
    
    # Show exposed ports
    log_info "Exposed services:"
    echo "  Login Server:    http://localhost:44453"
    echo "  PostgreSQL:      postgresql://localhost:5432"
    echo "  Redis:           redis://localhost:6379"
    echo "  pgAdmin:         http://localhost:5050"
}

# Clean up development environment
clean_dev() {
    local force=0
    
    # Parse options
    while [[ $# -gt 0 ]]; do
        case $1 in
            -f|--force)
                force=1
                shift
                ;;
            *)
                log_error "Unknown option for clean: $1"
                exit 1
                ;;
        esac
    done
    
    local compose_file="$(get_compose_file)"
    
    if [[ $force -eq 0 ]]; then
        echo -n "This will remove all containers, volumes, and networks. Continue? [y/N] "
        read -r response
        if [[ ! "$response" =~ ^[Yy]$ ]]; then
            log_info "Cleanup cancelled"
            exit 0
        fi
    fi
    
    log_warning "Cleaning up development environment..."
    
    # Stop and remove containers
    docker-compose -f "$compose_file" down -v --remove-orphans
    
    # Remove images
    docker-compose -f "$compose_file" down --rmi all 2>/dev/null || true
    
    # Clean up dangling images and volumes
    docker system prune -f
    
    log_success "Development environment cleaned up"
}

# Initial development setup
setup_dev() {
    log_info "Setting up SWG:ANH Modern development environment..."
    
    local project_root="$(get_project_root)"
    cd "$project_root"
    
    # Create necessary directories
    log_info "Creating directory structure..."
    mkdir -p data/config
    mkdir -p data/sql
    mkdir -p logs
    mkdir -p build
    
    # Make scripts executable
    log_info "Making scripts executable..."
    chmod +x scripts/*.sh
    
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
    fi
    
    # Create basic server config if it doesn't exist
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
    fi
    
    # Create basic SQL schema if it doesn't exist
    if [[ ! -f "data/sql/001_initial_schema.sql" ]]; then
        log_info "Creating initial database schema..."
        mkdir -p data/sql
        cat > data/sql/001_initial_schema.sql << 'EOF'
-- Initial database schema for SWG:ANH Modern
-- This will be expanded in future weeks

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
    fi
    
    log_success "Initial setup completed!"
    log_info "Next steps:"
    echo "  1. Run: $0 start"
    echo "  2. Wait for services to start"
    echo "  3. Run: $0 build"
    echo "  4. Test with SWG client on localhost:44453"
}

# Main function
main() {
    local command="$1"
    shift || true
    
    # Show banner for most commands
    case "$command" in
        logs|status)
            # Skip banner for these commands
            ;;
        *)
            show_banner
            ;;
    esac
    
    # Check Docker availability
    check_docker
    
    case "$command" in
        start)
            start_dev "$@"
            ;;
        stop)
            stop_dev "$@"
            ;;
        restart)
            restart_dev "$@"
            ;;
        build)
            build_dev "$@"
            ;;
        test)
            test_dev "$@"
            ;;
        shell)
            shell_dev "$@"
            ;;
        logs)
            logs_dev "$@"
            ;;
        status)
            status_dev "$@"
            ;;
        clean)
            clean_dev "$@"
            ;;
        setup)
            setup_dev "$@"
            ;;
        help|--help|-h)
            show_help
            ;;
        "")
            log_error "No command specified"
            echo "Use '$0 --help' for usage information"
            exit 1
            ;;
        *)
            log_error "Unknown command: $command"
            echo "Use '$0 --help' for usage information"
            exit 1
            ;;
    esac
}

# Run main function with all arguments
main "$@"
