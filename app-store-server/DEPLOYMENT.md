# M5Stack Tab5 App Store Server - Deployment Guide

## Quick Start

### Option 1: One-Command Setup (Recommended)
```bash
# Build and start the entire app store server
./docker-commands.sh setup
```

### Option 2: Docker Compose (Production)
```bash
# Start all services with Docker Compose
docker-compose up --build -d

# View logs
docker-compose logs -f

# Stop services
docker-compose down
```

### Option 3: Standalone Docker
```bash
# Build the image
docker build -t m5stack-tab5-app-store .

# Run the container
docker run -d \
  --name m5tab5-app-store \
  -p 8080:80 \
  -v $(pwd)/packages:/var/www/html/packages:ro \
  m5stack-tab5-app-store

# Check status
docker ps | grep m5tab5-app-store
```

## Access URLs

Once deployed, the app store will be available at:

- **🏠 Main App Store**: http://localhost:8080
- **📚 Documentation**: http://localhost:8080/docs/
- **📦 Package Downloads**: http://localhost:8080/packages/
- **🏥 Health Check**: http://localhost:8080/health

## Docker Commands Reference

### Build and Run
```bash
# Quick setup (recommended for first time)
./docker-commands.sh setup

# Build image only
./docker-commands.sh build

# Run with Docker Compose
./docker-commands.sh compose

# Run standalone container
./docker-commands.sh run
```

### Management
```bash
# View container status
./docker-commands.sh status

# View real-time logs
./docker-commands.sh logs

# Stop services
./docker-commands.sh stop-compose

# Clean up resources
./docker-commands.sh cleanup
```

### Manual Docker Commands
```bash
# Build with custom tag
docker build -t m5stack-tab5-app-store:v1.0.0 .

# Run with custom configuration
docker run -d \
  --name m5tab5-app-store \
  --restart unless-stopped \
  -p 8080:80 \
  -v $(pwd)/packages:/var/www/html/packages:ro \
  -v $(pwd)/docs:/var/www/html/docs:ro \
  -e TZ=UTC \
  m5stack-tab5-app-store:latest

# View logs
docker logs -f m5tab5-app-store

# Stop and remove
docker stop m5tab5-app-store
docker rm m5tab5-app-store

# Execute commands inside container
docker exec -it m5tab5-app-store sh
```

## Production Deployment

### Environment Configuration
```bash
# Set production environment variables
export NGINX_HOST=your-domain.com
export NGINX_PORT=443
export TZ=America/New_York

# Use production Docker Compose
docker-compose -f docker-compose.prod.yml up -d
```

### SSL/HTTPS Setup
```bash
# Add SSL certificates to nginx configuration
# Mount certificates volume:
docker run -d \
  -v /path/to/ssl:/etc/nginx/ssl:ro \
  -p 443:443 \
  m5stack-tab5-app-store
```

### Reverse Proxy (Nginx/Traefik)
```nginx
# Nginx reverse proxy configuration
server {
    listen 80;
    server_name m5tab5-app-store.yourdomain.com;
    
    location / {
        proxy_pass http://localhost:8080;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;
    }
}
```

## Monitoring and Maintenance

### Health Checks
```bash
# Check application health
curl http://localhost:8080/health

# Check Docker container health
docker inspect m5tab5-app-store --format='{{.State.Health.Status}}'

# View resource usage
docker stats m5tab5-app-store
```

### Log Management
```bash
# View Nginx access logs
docker exec m5tab5-app-store tail -f /var/log/nginx/access.log

# View Nginx error logs
docker exec m5tab5-app-store tail -f /var/log/nginx/error.log

# View download statistics
docker exec m5tab5-app-store tail -f /var/log/nginx/downloads.log
```

### Backup and Updates
```bash
# Backup packages
docker cp m5tab5-app-store:/var/www/html/packages ./backup-packages

# Update to new version
docker pull m5stack-tab5-app-store:latest
docker-compose up -d

# Rollback if needed
docker-compose down
docker run -d --name m5tab5-app-store m5stack-tab5-app-store:previous-tag
```

## Configuration Options

### Environment Variables
- `NGINX_HOST`: Server hostname (default: localhost)
- `NGINX_PORT`: Server port (default: 80)
- `TZ`: Timezone (default: UTC)

### Volume Mounts
- `/var/www/html/packages`: App packages (read-only recommended)
- `/var/www/html/docs`: Documentation files
- `/var/log/nginx`: Nginx log files
- `/etc/nginx/ssl`: SSL certificates (for HTTPS)

### Port Configuration
- `80`: HTTP port (internal)
- `8080`: Default external port mapping
- `443`: HTTPS port (if SSL configured)

## Troubleshooting

### Common Issues

#### Container Won't Start
```bash
# Check Docker daemon
docker info

# Check port availability
netstat -tulpn | grep :8080

# View container logs
docker logs m5tab5-app-store
```

#### Package Downloads Not Working
```bash
# Check packages volume mount
docker exec m5tab5-app-store ls -la /var/www/html/packages

# Verify Nginx configuration
docker exec m5tab5-app-store nginx -t

# Check file permissions
docker exec m5tab5-app-store ls -la /var/www/html/
```

#### Performance Issues
```bash
# Monitor resource usage
docker stats m5tab5-app-store

# Check memory usage
docker exec m5tab5-app-store free -h

# View active connections
docker exec m5tab5-app-store netstat -an | grep :80
```

### Debug Mode
```bash
# Run container in debug mode
docker run -it --rm \
  -p 8080:80 \
  -v $(pwd):/app \
  m5stack-tab5-app-store sh

# Enable Nginx debug logging
docker exec m5tab5-app-store nginx -s reload
```

## Security Considerations

### Container Security
- Runs as non-root user (nginx)
- Read-only file system where possible
- Security headers enabled
- No unnecessary packages installed

### Network Security
- Only exposes necessary ports
- HTTPS recommended for production
- Reverse proxy recommended
- Regular security updates

### Content Security
- Package integrity verification
- Secure download headers
- Access logging enabled
- Rate limiting configured

## Performance Optimization

### Nginx Configuration
- Gzip compression enabled
- Static file caching
- Keep-alive connections
- Worker process optimization

### Docker Optimization
- Multi-stage build (if needed)
- Minimal base image (Alpine)
- Layer caching optimization
- Resource limits configured

### Monitoring
- Health check endpoints
- Resource usage monitoring
- Log rotation configured
- Performance metrics available

---

**M5Stack Tab5 App Store Server** - Professional application marketplace deployment