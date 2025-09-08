#!/bin/bash

# scripts/start.sh - Quick start script for SWG:ANH Modern

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

# Configuration
BUILD_TYPE=${BUILD_TYPE:-Debug}
BUILD_DIR=${BUILD_DIR:-build}
SERVER_ADDRESS=${SERVER_ADDRESS:-0.0.0.0}
SERVER_PORT=${SERVER_PORT:-44453}
SKIP_BUILD=${SKIP_BUILD:-0}
BACKGROUND=${BACKGROUND:-0}

# Show banner
show_banner() {
    echo -e "${CYAN}"
    echo "╔══════════════════════════════════════════════════════════════╗"
    echo "║                   SWG:ANH Modern Server                     ║"
    echo "║                      Quick Start                            ║"
    echo "╚══════════════════════════════════════════════════════════════╝"
    echo -e "${NC}"
}

# Show help
show_help() {
    show_banner
    echo ""
    echo "Usage: $0 [options]"
    echo ""
    echo "Options:"
    echo "  -a, --address ADDR   Server bind address [default: 0.0.0.0]"
    echo "  -p, --port PORT      Server port [default: 44453]"
    echo "  -t, --type TYPE      Build type (Debug, Release) [default: Debug]"
    echo "  -s, --skip-build     Skip building, run existing binary"
    echo "  -b, --background     Run server in background"
    echo "  -d, --docker         Use Docker development environment"
    echo "  -h, --help           Show this help message"
    echo ""
    echo "Environment Variables:"
    echo "  BUILD_TYPE           Build type (Debug, Release)"
    echo "  SERVER_ADDRESS       Server bind address"
    echo "  SERVER_PORT          Server port"
    echo "  SKIP_BUILD           Skip build step (1 to skip)"
    echo ""
    echo "Examples:"
    echo "  $0                              # Build and start server"
    echo "  $0 -p 44454                     # Start on different port"
    echo "  $0 -s                           # Skip build, use existing binary"
    echo "  $0 -t Release                   # Release build"
    echo "  $0 -d                           # Use Docker environment"
    echo "  SKIP_BUILD=1 $0                 # Skip build via environment"
}

# Get project root directory
get_project_root() {
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

# Check if binary exists
check_binary() {
    local binary_path="$1"
    if [[ ! -f "$binary_path" ]]; then
        log_error "Binary not found: $binary_path"
        log_error "Try running without --skip-build to build first"
        return 1
    fi
    
    if [[ ! -x "$binary_path" ]]; then
        log_error "Binary is not executable: $binary_path"
        return 1
    fi
    
    return 0
}

# Build project
build_project() {
    local project_root="$1"
    
    log_info "Building SWG:ANH Modern ($BUILD_TYPE)..."
    
    if [[ -f "$project_root/scripts/build.sh" ]]; then
        cd "$project_root"
        ./scripts/build.sh -t "$BUILD_TYPE" -T
    else
        log_warning "build.sh not found, using manual build"
        
        mkdir -p "$BUILD_DIR"
        cd "$BUILD_DIR"
        
        cmake .. -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
        cmake --build . --parallel $(nproc 2>/dev/null || echo 4)
        
        # Run quick test
        if command -v ctest &> /dev/null; then
            ctest --output-on-failure
        fi
    fi
}

# Start with Docker
start_docker() {
    local project_root="$(get_project_root)"
    
    log_info "Starting SWG:ANH Modern with Docker..."
    
    if [[ ! -f "$project_root/scripts/dev.sh" ]]; then
        log_error "dev.sh script not found"
        log_error "Please ensure the development scripts are properly set up"
        exit 1
    fi
    
    cd "$project_root"
    
    # Use dev script to start
    if [[ $BACKGROUND -eq 1 ]]; then
        ./scripts/dev.sh start --detach
        log_success "Server started in background via Docker"
        log_info "Use './scripts/dev.sh logs' to view logs"
        log_info "Use './scripts/dev.sh shell' to access container"
    else
        log_info "Starting Docker development environment..."
        log_info "Server will be available on port $SERVER_PORT"
        log_info "Press Ctrl+C to stop all services"
        ./scripts/dev.sh start
    fi
}

# Start local binary
start_local() {
    local project_root="$(get_project_root)"
    local binary_path="$project_root/$BUILD_DIR/bin/login_server"
    
    # Build if needed
    if [[ $SKIP_BUILD -eq 0 ]]; then
        build_project "$project_root"
    fi
    
    # Check if binary exists
    if ! check_binary "$binary_path"; then
        exit 1
    fi
    
    # Change to project root for relative paths
    cd "$project_root"
    
    # Create logs directory
    mkdir -p logs
    
    log_success "Starting SWG:ANH Modern Login Server..."
    echo ""
    log_info "Configuration:"
    echo "  Address: $SERVER_ADDRESS"
    echo "  Port: $SERVER_PORT"
    echo "  Build Type: $BUILD_TYPE"
    echo "  Binary: $binary_path"
    echo ""
    
    # Prepare server arguments
    local server_args=(
        "--address" "$SERVER_ADDRESS"
        "--port" "$SERVER_PORT"
    )
    
    # Start server
    if [[ $BACKGROUND -eq 1 ]]; then
        log_info "Starting server in background..."
        nohup "$binary_path" "${server_args[@]}" > logs/server.log 2>&1 &
        local pid=$!
        echo $pid > logs/server.pid
        
        log_success "Server started in background (PID: $pid)"
        log_info "Log file: logs/server.log"
        log_info "To stop: kill $pid"
        
        # Wait a moment and check if it's still running
        sleep 2
        if kill -0 $pid 2>/dev/null; then
            log_success "Server is running successfully"
            echo ""
            echo "Connect your SWG client to: $SERVER_ADDRESS:$SERVER_PORT"
        else
            log_error "Server failed to start, check logs/server.log"
            exit 1
        fi
    else
        log_info "Starting server (use Ctrl+C to stop)..."
        echo ""
        
        # Set up signal handlers for graceful shutdown
        trap 'log_info "Shutting down server..."; exit 0' INT TERM
        
        # Start server in foreground
        "$binary_path" "${server_args[@]}"
    fi
}

# Check prerequisites
check_prerequisites() {
    # Check for CMake
    if ! command -v cmake &> /dev/null; then
        log_error "CMake is required but not installed"
        echo "Please install CMake from: https://cmake.org/download/"
        exit 1
    fi
    
    # Check for C++ compiler
    if ! command -v g++ &> /dev/null && ! command -v clang++ &> /dev/null; then
        log_error "C++ compiler (g++ or clang++) is required but not installed"
        echo "Please install a C++ compiler"
        exit 1
    fi
}

# Show connection info
show_connection_info() {
    echo ""
    log_success "SWG:ANH Modern is ready for connections!"
    echo ""
    echo "╔══════════════════════════════════════════════════════════════╗"
    echo "║                    Connection Information                    ║"
    echo "╠══════════════════════════════════════════════════════════════╣"
    echo "║  Server Address: $SERVER_ADDRESS"
    echo "║  Server Port:    $SERVER_PORT"
    echo "║"
    echo "║  To connect your SWG client:"
    echo "║  1. Edit swganh.cfg in your SWG directory:"
    echo "║     loginServerAddress0=$SERVER_ADDRESS"
    echo "║     loginServerPort0=$SERVER_PORT"
    echo "║"
    echo "║  2. Launch SWG client:"
    echo "║     SWGClient_r.exe -- -s Station subscriptionFeatures=1"
    echo "║"
    echo "║  3. Look for 'SWG:ANH Modern' in the galaxy list"
    echo "╚══════════════════════════════════════════════════════════════╝"
    echo ""
}

# Parse command line arguments
USE_DOCKER=0

while [[ $# -gt 0 ]]; do
    case $1 in
        -a|--address)
            SERVER_ADDRESS="$2"
            shift 2
            ;;
        -p|--port)
            SERVER_PORT="$2"
            shift 2
            ;;
        -t|--type)
            BUILD_TYPE="$2"
            shift 2
            ;;
        -s|--skip-build)
            SKIP_BUILD=1
            shift
            ;;
        -b|--background)
            BACKGROUND=1
            shift
            ;;
        -d|--docker)
            USE_DOCKER=1
            shift
            ;;
        -h|--help)
            show_help
            exit 0
            ;;
        *)
            log_error "Unknown option: $1"
            echo "Use -h or --help for usage information."
            exit 1
            ;;
    esac
done

# Main execution
show_banner

if [[ $USE_DOCKER -eq 1 ]]; then
    # Docker mode
    if ! command -v docker &> /dev/null; then
        log_error "Docker is required but not installed"
        echo "Please install Docker Desktop from: https://www.docker.com/products/docker-desktop"
        exit 1
    fi
    
    start_docker
else
    # Local mode
    check_prerequisites
    start_local
    
    if [[ $BACKGROUND -eq 0 ]]; then
        show_connection_info
    fi
fi
