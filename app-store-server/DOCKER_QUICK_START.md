# M5Stack Tab5 App Store Server - Quick Start Guide

## 🚀 One-Command Setup

```bash
# Navigate to the app store server directory
cd app-store-server

# Build and start the entire app store server
./docker-commands.sh setup
```

This will:
1. Build the Docker image
2. Start all services with Docker Compose
3. Make the app store available at http://localhost:8080

## 📦 Docker Commands Reference

### Essential Commands
```bash
# Quick setup (recommended for first-time users)
./docker-commands.sh setup

# Build the Docker image
./docker-commands.sh build

# Start with Docker Compose (production ready)
./docker-commands.sh compose

# View container status and health
./docker-commands.sh status

# View real-time logs
./docker-commands.sh logs

# Stop all services
./docker-commands.sh stop-compose

# Clean up Docker resources
./docker-commands.sh cleanup
```

### Manual Docker Commands
```bash
# Build the image manually
docker build -t m5stack-tab5-app-store .

# Run the container manually
docker run -d \
  --name m5tab5-app-store \
  -p 8080:80 \
  -v $(pwd)/packages:/var/www/html/packages:ro \
  -v $(pwd)/docs:/var/www/html/docs:ro \
  m5stack-tab5-app-store

# Check container status
docker ps | grep m5tab5-app-store

# View logs
docker logs -f m5tab5-app-store

# Stop and remove
docker stop m5tab5-app-store && docker rm m5tab5-app-store
```

### Docker Compose Commands
```bash
# Start all services in background
docker-compose up -d

# Start with rebuild
docker-compose up --build -d

# View service status
docker-compose ps

# View logs from all services
docker-compose logs -f

# Stop all services
docker-compose down

# Stop and remove volumes
docker-compose down -v
```

## 🌐 Access URLs

Once deployed, access the app store at:

- **🏠 Main App Store**: http://localhost:8080
- **📚 Documentation**: http://localhost:8080/docs/
- **📦 Package Downloads**: http://localhost:8080/packages/
- **🏥 Health Check**: http://localhost:8080/health

## 🔧 Configuration

### Port Configuration
```bash
# Use different port (e.g., 3000)
docker run -p 3000:80 m5stack-tab5-app-store

# Or modify docker-compose.yml:
ports:
  - "3000:80"
```

### Environment Variables
```bash
# Set timezone
docker run -e TZ=America/New_York m5stack-tab5-app-store

# Set server name
docker run -e NGINX_HOST=your-domain.com m5stack-tab5-app-store
```

### Volume Mounts
```bash
# Mount additional directories
docker run \
  -v $(pwd)/custom-packages:/var/www/html/packages:ro \
  -v $(pwd)/custom-docs:/var/www/html/docs:ro \
  -v $(pwd)/logs:/var/log/nginx \
  m5stack-tab5-app-store
```

## 🔍 Troubleshooting

### Check Docker Status
```bash
# Verify Docker is running
docker info

# Check if port is available
netstat -tulpn | grep :8080

# View container logs for errors
docker logs m5tab5-app-store
```

### Common Issues
```bash
# Permission denied on docker-commands.sh
chmod +x docker-commands.sh

# Port already in use
./docker-commands.sh stop
# Or use different port: docker run -p 9090:80 ...

# Container won't start
docker logs m5tab5-app-store
# Check for configuration errors

# Out of disk space
./docker-commands.sh cleanup
docker system prune -a
```

## 📊 Monitoring

### Resource Usage
```bash
# View resource consumption
docker stats m5tab5-app-store

# Check container health
docker inspect m5tab5-app-store --format='{{.State.Health.Status}}'

# Monitor logs in real-time
./docker-commands.sh logs
```

### Performance Testing
```bash
# Test app store response
curl -I http://localhost:8080

# Test health endpoint
curl http://localhost:8080/health

# Download speed test
curl -o test-package.m5app http://localhost:8080/packages/com.m5stack.contacts-v1.0.0.m5app
```

## 🚀 Production Deployment

### SSL/HTTPS Setup
```bash
# Add SSL certificates and modify nginx config
docker run \
  -v /path/to/ssl:/etc/nginx/ssl:ro \
  -p 443:443 \
  m5stack-tab5-app-store
```

### Reverse Proxy
```nginx
# Nginx reverse proxy
upstream m5tab5-app-store {
    server localhost:8080;
}

server {
    listen 80;
    server_name m5tab5-store.yourdomain.com;
    
    location / {
        proxy_pass http://m5tab5-app-store;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
    }
}
```

### Backup and Updates
```bash
# Backup app packages
docker cp m5tab5-app-store:/var/www/html/packages ./backup-packages-$(date +%Y%m%d)

# Update to new version
docker pull m5stack-tab5-app-store:latest
docker-compose up -d

# Rollback if needed
docker-compose down
docker run -d m5stack-tab5-app-store:previous-version
```

---

**Ready to deploy your M5Stack Tab5 App Store server!** 🎉

For detailed documentation, see [DEPLOYMENT.md](DEPLOYMENT.md)