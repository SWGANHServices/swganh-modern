#!/bin/bash

# scripts/build.sh - Cross-platform build script for SWG:ANH Modern

set -e

# Configuration
BUILD_TYPE=${BUILD_TYPE:-Debug}
BUILD_DIR=${BUILD_DIR:-build}
JOBS=${JOBS:-$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)}
VERBOSE=${VERBOSE:-0}

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
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

# Help function
show_help() {
    echo "SWG:ANH Modern Build Script"
    echo ""
    echo "Usage: $0 [options]"
    echo ""
    echo "Options:"
    echo "  -t, --type TYPE     Build type (Debug, Release, RelWithDebInfo) [default: Debug]"
    echo "  -d, --dir DIR       Build directory [default: build]"
    echo "  -j, --jobs JOBS     Number of parallel jobs [default: auto-detect]"
    echo "  -c, --clean         Clean build directory before building"
    echo "  -v, --verbose       Verbose output"
    echo "  -T, --test          Run tests after building"
    echo "  -I, --install       Install after building"
    echo "  -h, --help          Show this help message"
    echo ""
    echo "Environment Variables:"
    echo "  CC                  C compiler"
    echo "  CXX                 C++ compiler"
    echo "  CMAKE_GENERATOR     CMake generator (e.g., Ninja, Unix Makefiles)"
    echo ""
    echo "Examples:"
    echo "  $0                           # Debug build"
    echo "  $0 -t Release -j 8           # Release build with 8 jobs"
    echo "  $0 -c -T                     # Clean build and run tests"
    echo "  CMAKE_GENERATOR=Ninja $0     # Use Ninja generator"
}

# Parse command line arguments
CLEAN=0
RUN_TESTS=0
INSTALL=0

while [[ $# -gt 0 ]]; do
    case $1 in
        -t|--type)
            BUILD_TYPE="$2"
            shift 2
            ;;
        -d|--dir)
            BUILD_DIR="$2"
            shift 2
            ;;
        -j|--jobs)
            JOBS="$2"
            shift 2
            ;;
        -c|--clean)
            CLEAN=1
            shift
            ;;
        -v|--verbose)
            VERBOSE=1
            shift
            ;;
        -T|--test)
            RUN_TESTS=1
            shift
            ;;
        -I|--install)
            INSTALL=1
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

# Validate build type
case $BUILD_TYPE in
    Debug|Release|RelWithDebInfo|MinSizeRel)
        ;;
    *)
        log_error "Invalid build type: $BUILD_TYPE"
        log_error "Valid types: Debug, Release, RelWithDebInfo, MinSizeRel"
        exit 1
        ;;
esac

# Detect platform
PLATFORM="unknown"
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    PLATFORM="linux"
elif [[ "$OSTYPE" == "darwin"* ]]; then
    PLATFORM="macos"
elif [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "cygwin" ]]; then
    PLATFORM="windows"
fi

# Print build configuration
echo "=== SWG:ANH Modern Build Script ==="
echo "Platform: $PLATFORM"
echo "Build Type: $BUILD_TYPE"
echo "Build Directory: $BUILD_DIR"
echo "Jobs: $JOBS"
echo "Verbose: $VERBOSE"
echo "Clean: $CLEAN"
echo "Run Tests: $RUN_TESTS"
echo "Install: $INSTALL"
echo "=================================="

# Check for required tools
check_tool() {
    if ! command -v "$1" &> /dev/null; then
        log_error "$1 is required but not installed"
        return 1
    fi
}

log_info "Checking required tools..."
check_tool cmake || exit 1

# Check for compiler
if [[ -n "$CXX" ]]; then
    log_info "Using C++ compiler: $CXX"
elif command -v clang++ &> /dev/null; then
    export CXX=clang++
    log_info "Using C++ compiler: clang++"
elif command -v g++ &> /dev/null; then
    export CXX=g++
    log_info "Using C++ compiler: g++"
else
    log_error "No C++ compiler found"
    exit 1
fi

# Clean build directory if requested
if [[ $CLEAN -eq 1 ]]; then
    log_info "Cleaning build directory: $BUILD_DIR"
    rm -rf "$BUILD_DIR"
fi

# Create build directory
log_info "Creating build directory: $BUILD_DIR"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Determine CMake generator
CMAKE_GENERATOR=${CMAKE_GENERATOR:-}
if [[ -z "$CMAKE_GENERATOR" ]]; then
    if command -v ninja &> /dev/null; then
        CMAKE_GENERATOR="Ninja"
        log_info "Using Ninja generator"
    else
        case $PLATFORM in
            windows)
                CMAKE_GENERATOR="Visual Studio 17 2022"
                ;;
            *)
                CMAKE_GENERATOR="Unix Makefiles"
                ;;
        esac
        log_info "Using generator: $CMAKE_GENERATOR"
    fi
fi

# Configure CMake options
CMAKE_OPTIONS=(
    "-DCMAKE_BUILD_TYPE=$BUILD_TYPE"
    "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
    "-DENABLE_TESTING=ON"
)

if [[ $VERBOSE -eq 1 ]]; then
    CMAKE_OPTIONS+=("-DCMAKE_VERBOSE_MAKEFILE=ON")
fi

# Platform-specific options
case $PLATFORM in
    windows)
        CMAKE_OPTIONS+=("-DCMAKE_WINDOWS_EXPORT_ALL_SYMBOLS=ON")
        ;;
    macos)
        CMAKE_OPTIONS+=("-DCMAKE_OSX_DEPLOYMENT_TARGET=10.15")
        ;;
esac

# Configure
log_info "Configuring build..."
if [[ $VERBOSE -eq 1 ]]; then
    cmake .. -G "$CMAKE_GENERATOR" "${CMAKE_OPTIONS[@]}"
else
    cmake .. -G "$CMAKE_GENERATOR" "${CMAKE_OPTIONS[@]}" > /dev/null
fi

if [[ $? -ne 0 ]]; then
    log_error "CMake configuration failed"
    exit 1
fi

log_success "Configuration completed"

# Build
log_info "Building with $JOBS jobs..."
if [[ $VERBOSE -eq 1 ]]; then
    cmake --build . --config "$BUILD_TYPE" --parallel "$JOBS"
else
    cmake --build . --config "$BUILD_TYPE" --parallel "$JOBS" | while read line; do
        if [[ "$line" == *"error"* ]] || [[ "$line" == *"Error"* ]]; then
            log_error "$line"
        elif [[ "$line" == *"warning"* ]] || [[ "$line" == *"Warning"* ]]; then
            log_warning "$line"
        elif [[ $VERBOSE -eq 1 ]]; then
            echo "$line"
        fi
    done
fi

if [[ $? -ne 0 ]]; then
    log_error "Build failed"
    exit 1
fi

log_success "Build completed successfully"

# Show build artifacts
log_info "Build artifacts:"
if [[ -d "bin" ]]; then
    find bin -type f -executable 2>/dev/null | head -10 | while read file; do
        size=$(du -h "$file" 2>/dev/null | cut -f1)
        echo "  $file ($size)"
    done
else
    echo "  No bin directory found"
fi

# Run tests if requested
if [[ $RUN_TESTS -eq 1 ]]; then
    log_info "Running tests..."
    if command -v ctest &> /dev/null; then
        if [[ $VERBOSE -eq 1 ]]; then
            ctest --output-on-failure --parallel "$JOBS"
        else
            ctest --output-on-failure --parallel "$JOBS" --quiet
        fi
        
        if [[ $? -eq 0 ]]; then
            log_success "All tests passed"
        else
            log_error "Some tests failed"
            exit 1
        fi
    else
        log_warning "ctest not found, skipping tests"
    fi
fi

# Install if requested
if [[ $INSTALL -eq 1 ]]; then
    log_info "Installing..."
    cmake --install . --config "$BUILD_TYPE"
    
    if [[ $? -eq 0 ]]; then
        log_success "Installation completed"
    else
        log_error "Installation failed"
        exit 1
    fi
fi

# Summary
echo ""
log_success "Build script completed successfully!"
echo "Build directory: $(pwd)"
if [[ -d "bin" ]]; then
    echo "Executables: $(pwd)/bin/"
    echo ""
    echo "To run the login server:"
    echo "  ./bin/login_server --address 0.0.0.0 --port 44453"
fi

# Copy compile commands for IDE support
if [[ -f "compile_commands.json" ]] && [[ ! -f "../compile_commands.json" ]]; then
    cp compile_commands.json ..
    log_info "Copied compile_commands.json for IDE support"
fi
