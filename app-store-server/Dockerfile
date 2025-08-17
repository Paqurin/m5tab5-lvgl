# M5Stack Tab5 App Store Server
# Professional app marketplace for M5Stack Tab5 ESP32-P4 devices

FROM nginx:alpine

# Metadata
LABEL maintainer="M5Stack Tab5 Team <support@m5stack-tab5.dev>"
LABEL description="M5Stack Tab5 App Store - Professional application marketplace"
LABEL version="1.0.0"
LABEL platform="m5stack-tab5"

# Install additional tools
RUN apk add --no-cache \
    python3 \
    py3-pip \
    zip \
    unzip \
    curl \
    && rm -rf /var/cache/apk/*

# Create app directories
RUN mkdir -p /var/www/html \
    && mkdir -p /var/www/packages \
    && mkdir -p /var/www/docs \
    && mkdir -p /var/www/assets \
    && mkdir -p /scripts

# Copy website files
COPY index.html /var/www/html/
COPY styles.css /var/www/html/
COPY script.js /var/www/html/
COPY README.md /var/www/html/
COPY assets/ /var/www/html/assets/
COPY docs/ /var/www/html/docs/
COPY packages/ /var/www/html/packages/

# Copy package generator
COPY generate-packages.py /scripts/
RUN chmod +x /scripts/generate-packages.py

# Create Nginx configuration
RUN cat > /etc/nginx/nginx.conf << 'EOF'
user nginx;
worker_processes auto;
error_log /var/log/nginx/error.log warn;
pid /var/run/nginx.pid;

events {
    worker_connections 1024;
    use epoll;
    multi_accept on;
}

http {
    include /etc/nginx/mime.types;
    default_type application/octet-stream;

    # Logging
    log_format main '$remote_addr - $remote_user [$time_local] "$request" '
                    '$status $body_bytes_sent "$http_referer" '
                    '"$http_user_agent" "$http_x_forwarded_for"';
    access_log /var/log/nginx/access.log main;

    # Performance
    sendfile on;
    tcp_nopush on;
    tcp_nodelay on;
    keepalive_timeout 65;
    types_hash_max_size 2048;
    client_max_body_size 50M;

    # Compression
    gzip on;
    gzip_vary on;
    gzip_min_length 1024;
    gzip_proxied any;
    gzip_comp_level 6;
    gzip_types
        text/plain
        text/css
        text/xml
        text/javascript
        application/json
        application/javascript
        application/xml+rss
        application/atom+xml
        image/svg+xml;

    # Security headers
    add_header X-Frame-Options "SAMEORIGIN" always;
    add_header X-XSS-Protection "1; mode=block" always;
    add_header X-Content-Type-Options "nosniff" always;
    add_header Referrer-Policy "no-referrer-when-downgrade" always;
    add_header Content-Security-Policy "default-src 'self' http: https: data: blob: 'unsafe-inline'" always;

    server {
        listen 80;
        listen [::]:80;
        server_name localhost;
        root /var/www/html;
        index index.html;

        # Cache static assets
        location ~* \.(css|js|jpg|jpeg|png|gif|ico|svg|woff|woff2|ttf|eot)$ {
            expires 1y;
            add_header Cache-Control "public, immutable";
            access_log off;
        }

        # App packages with special handling
        location /packages/ {
            alias /var/www/html/packages/;
            add_header Content-Type "application/zip";
            add_header Content-Disposition "attachment";
            access_log /var/log/nginx/downloads.log main;
        }

        # API endpoint for package generation
        location /api/generate-packages {
            proxy_pass http://127.0.0.1:8000;
            proxy_set_header Host $host;
            proxy_set_header X-Real-IP $remote_addr;
            proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        }

        # Main application
        location / {
            try_files $uri $uri/ /index.html;
            add_header Cache-Control "no-cache, no-store, must-revalidate";
        }

        # Documentation
        location /docs/ {
            alias /var/www/html/docs/;
            autoindex on;
            autoindex_exact_size off;
            autoindex_localtime on;
        }

        # Health check
        location /health {
            access_log off;
            return 200 "healthy\n";
            add_header Content-Type text/plain;
        }

        # Robots.txt
        location /robots.txt {
            return 200 "User-agent: *\nDisallow: /packages/\nDisallow: /api/\n";
            add_header Content-Type text/plain;
        }

        # Error pages
        error_page 404 /404.html;
        error_page 500 502 503 504 /50x.html;
    }
}
EOF

# Create 404 error page
RUN cat > /var/www/html/404.html << 'EOF'
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>404 - Page Not Found | M5Stack Tab5 App Store</title>
    <link rel="stylesheet" href="/styles.css">
</head>
<body>
    <div class="container" style="text-align: center; padding: 4rem 0;">
        <h1 style="font-size: 4rem; margin-bottom: 1rem;">404</h1>
        <h2>Page Not Found</h2>
        <p style="margin: 2rem 0;">The requested page could not be found on the M5Stack Tab5 App Store.</p>
        <a href="/" class="btn btn-primary">Return to App Store</a>
    </div>
</body>
</html>
EOF

# Create startup script
RUN cat > /scripts/startup.sh << 'EOF'
#!/bin/sh

echo "🚀 Starting M5Stack Tab5 App Store Server..."

# Check if packages need to be regenerated
if [ ! -f /var/www/html/packages/.generated ]; then
    echo "📦 Generating app packages..."
    cd /scripts
    python3 generate-packages.py
    touch /var/www/html/packages/.generated
    echo "✅ Packages generated successfully"
fi

# Set proper permissions
chown -R nginx:nginx /var/www/html
chmod -R 755 /var/www/html

# Start Nginx
echo "🌐 Starting web server..."
nginx -g "daemon off;"
EOF

RUN chmod +x /scripts/startup.sh

# Expose port
EXPOSE 80

# Health check
HEALTHCHECK --interval=30s --timeout=3s --start-period=5s --retries=3 \
    CMD curl -f http://localhost/health || exit 1

# Set working directory
WORKDIR /var/www/html

# Start the server
CMD ["/scripts/startup.sh"]