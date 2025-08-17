#!/bin/bash

# M5Stack Tab5 App Store Server - Docker Management Commands
# Professional application marketplace deployment

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Configuration
IMAGE_NAME="m5stack-tab5-app-store"
CONTAINER_NAME="m5tab5-app-store"
PORT="8080"
DOCKER_TAG="latest"

echo -e "${CYAN}🚀 M5Stack Tab5 App Store Server - Docker Management${NC}"
echo -e "${CYAN}======================================================${NC}"

# Function to print colored output
print_step() {
    echo -e "${BLUE}▶ $1${NC}"
}

print_success() {
    echo -e "${GREEN}✅ $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}⚠️ $1${NC}"
}

print_error() {
    echo -e "${RED}❌ $1${NC}"
}

# Function to check if Docker is running
check_docker() {
    if ! command -v docker &> /dev/null; then
        print_error "Docker is not installed. Please install Docker first."
        exit 1
    fi

    if ! docker info &> /dev/null; then
        print_error "Docker daemon is not running. Please start Docker."
        exit 1
    fi
}

# Function to build the Docker image
build_image() {
    print_step "Building M5Stack Tab5 App Store Docker image..."
    
    docker build \
        --tag ${IMAGE_NAME}:${DOCKER_TAG} \
        --tag ${IMAGE_NAME}:$(date +%Y%m%d) \
        --label "org.opencontainers.image.title=M5Stack Tab5 App Store" \
        --label "org.opencontainers.image.description=Professional application marketplace for M5Stack Tab5 ESP32-P4 devices" \
        --label "org.opencontainers.image.version=1.0.0" \
        --label "org.opencontainers.image.created=$(date -u +'%Y-%m-%dT%H:%M:%SZ')" \
        --label "org.opencontainers.image.source=https://github.com/Paqurin/m5tab5-lvgl" \
        .

    if [ $? -eq 0 ]; then
        print_success "Docker image built successfully"
        docker images ${IMAGE_NAME}
    else
        print_error "Failed to build Docker image"
        exit 1
    fi
}

# Function to run the container
run_container() {
    print_step "Starting M5Stack Tab5 App Store container..."

    # Stop existing container if running
    if docker ps -q -f name=${CONTAINER_NAME} | grep -q .; then
        print_warning "Stopping existing container..."
        docker stop ${CONTAINER_NAME}
        docker rm ${CONTAINER_NAME}
    fi

    # Run new container
    docker run -d \
        --name ${CONTAINER_NAME} \
        --restart unless-stopped \
        -p ${PORT}:80 \
        -v $(pwd)/packages:/var/www/html/packages:ro \
        -v $(pwd)/docs:/var/www/html/docs:ro \
        -e TZ=UTC \
        --label "service=m5stack-tab5-app-store" \
        --label "environment=production" \
        ${IMAGE_NAME}:${DOCKER_TAG}

    if [ $? -eq 0 ]; then
        print_success "Container started successfully"
        echo -e "${GREEN}📱 App Store URL: http://localhost:${PORT}${NC}"
        echo -e "${GREEN}🏥 Health Check: http://localhost:${PORT}/health${NC}"
        echo -e "${GREEN}📚 Documentation: http://localhost:${PORT}/docs/${NC}"
    else
        print_error "Failed to start container"
        exit 1
    fi
}

# Function to run with Docker Compose
run_compose() {
    print_step "Starting M5Stack Tab5 App Store with Docker Compose..."

    # Build and start services
    docker-compose up --build -d

    if [ $? -eq 0 ]; then
        print_success "Services started successfully with Docker Compose"
        echo -e "${GREEN}📱 App Store URL: http://localhost:8080${NC}"
        echo -e "${GREEN}🏥 Health Check: http://localhost:8080/health${NC}"
        
        # Show service status
        echo ""
        docker-compose ps
    else
        print_error "Failed to start services with Docker Compose"
        exit 1
    fi
}

# Function to stop the container
stop_container() {
    print_step "Stopping M5Stack Tab5 App Store container..."

    if docker ps -q -f name=${CONTAINER_NAME} | grep -q .; then
        docker stop ${CONTAINER_NAME}
        docker rm ${CONTAINER_NAME}
        print_success "Container stopped and removed"
    else
        print_warning "Container is not running"
    fi
}

# Function to stop Docker Compose services
stop_compose() {
    print_step "Stopping Docker Compose services..."
    docker-compose down
    print_success "Services stopped"
}

# Function to show container logs
show_logs() {
    print_step "Showing M5Stack Tab5 App Store logs..."
    
    if docker ps -q -f name=${CONTAINER_NAME} | grep -q .; then
        docker logs -f ${CONTAINER_NAME}
    else
        print_error "Container is not running"
        exit 1
    fi
}

# Function to show container status
show_status() {
    print_step "Container Status:"
    
    if docker ps -q -f name=${CONTAINER_NAME} | grep -q .; then
        echo -e "${GREEN}🟢 Container is running${NC}"
        docker ps -f name=${CONTAINER_NAME} --format "table {{.ID}}\t{{.Image}}\t{{.Status}}\t{{.Ports}}"
        
        echo ""
        print_step "Resource Usage:"
        docker stats ${CONTAINER_NAME} --no-stream
        
        echo ""
        print_step "Health Status:"
        docker inspect ${CONTAINER_NAME} --format='{{.State.Health.Status}}'
    else
        echo -e "${RED}🔴 Container is not running${NC}"
    fi
}

# Function to clean up Docker resources
cleanup() {
    print_step "Cleaning up Docker resources..."
    
    # Remove stopped containers
    docker container prune -f
    
    # Remove unused images
    docker image prune -f
    
    # Remove unused volumes
    docker volume prune -f
    
    print_success "Cleanup completed"
}

# Function to show help
show_help() {
    echo -e "${PURPLE}Available Commands:${NC}"
    echo -e "  ${YELLOW}build${NC}          Build the Docker image"
    echo -e "  ${YELLOW}run${NC}            Run the container (standalone)"
    echo -e "  ${YELLOW}compose${NC}        Run with Docker Compose (recommended)"
    echo -e "  ${YELLOW}stop${NC}           Stop the standalone container"
    echo -e "  ${YELLOW}stop-compose${NC}   Stop Docker Compose services"
    echo -e "  ${YELLOW}logs${NC}           Show container logs"
    echo -e "  ${YELLOW}status${NC}         Show container status and health"
    echo -e "  ${YELLOW}cleanup${NC}        Clean up unused Docker resources"
    echo -e "  ${YELLOW}help${NC}           Show this help message"
    echo ""
    echo -e "${PURPLE}Examples:${NC}"
    echo -e "  ${CYAN}./docker-commands.sh build${NC}     # Build the image"
    echo -e "  ${CYAN}./docker-commands.sh compose${NC}   # Start with Docker Compose"
    echo -e "  ${CYAN}./docker-commands.sh status${NC}    # Check status"
    echo -e "  ${CYAN}./docker-commands.sh logs${NC}      # View logs"
}

# Function to run all commands for quick setup
quick_setup() {
    print_step "Quick Setup - Building and running M5Stack Tab5 App Store..."
    
    check_docker
    build_image
    run_compose
    
    echo ""
    print_success "🎉 M5Stack Tab5 App Store is now running!"
    echo -e "${GREEN}📱 Access the app store at: http://localhost:8080${NC}"
    echo -e "${GREEN}📚 View documentation at: http://localhost:8080/docs/${NC}"
    echo -e "${GREEN}📦 Download packages from: http://localhost:8080/packages/${NC}"
    echo ""
    echo -e "${YELLOW}Use './docker-commands.sh logs' to view real-time logs${NC}"
    echo -e "${YELLOW}Use './docker-commands.sh stop-compose' to stop the services${NC}"
}

# Main command handling
case "$1" in
    "build")
        check_docker
        build_image
        ;;
    "run")
        check_docker
        build_image
        run_container
        ;;
    "compose")
        check_docker
        run_compose
        ;;
    "stop")
        stop_container
        ;;
    "stop-compose")
        stop_compose
        ;;
    "logs")
        show_logs
        ;;
    "status")
        show_status
        ;;
    "cleanup")
        cleanup
        ;;
    "quick"|"setup")
        quick_setup
        ;;
    "help"|"--help"|"-h")
        show_help
        ;;
    "")
        print_warning "No command specified. Use 'help' to see available commands."
        echo ""
        show_help
        ;;
    *)
        print_error "Unknown command: $1"
        echo ""
        show_help
        exit 1
        ;;
esac