// M5Stack Tab5 App Store JavaScript

// Application data following M5Tab5 development standards
const applications = [
    // Personal Assistant Apps
    {
        id: "com.m5stack.contacts",
        name: "Contact Management",
        version: "1.0.0",
        category: "personal-assistant",
        icon: "📞",
        size: "245KB",
        author: "M5Stack",
        description: "Complete address book solution with advanced search, categorization, and VCard import/export capabilities.",
        features: [
            "Full CRUD operations for contacts",
            "Advanced search by name, phone, email, company",
            "Category grouping (Family, Work, Friends)",
            "VCard (.vcf) import/export support",
            "Profile picture support",
            "Direct calling integration"
        ],
        tags: ["contacts", "address-book", "productivity"],
        memoryUsage: {
            ram: 51200,
            flash: 245760,
            psram: 65536
        },
        packageUrl: "packages/contact-management-v1.0.0.m5app"
    },
    {
        id: "com.m5stack.tasks",
        name: "Task Management",
        version: "1.0.0",
        category: "personal-assistant", 
        icon: "✅",
        size: "198KB",
        author: "M5Stack",
        description: "Smart to-do list application with priority system, due dates, and comprehensive progress tracking.",
        features: [
            "Smart to-do lists with priorities",
            "Due date tracking with calendar integration",
            "Category filtering (Work, Personal, Shopping, Health)",
            "Search and filter functionality",
            "Progress tracking with visual indicators",
            "Task completion statistics"
        ],
        tags: ["tasks", "todo", "productivity", "planning"],
        memoryUsage: {
            ram: 49152,
            flash: 198656,
            psram: 32768
        },
        packageUrl: "packages/task-management-v1.0.0.m5app"
    },
    {
        id: "com.m5stack.voice",
        name: "Voice Assistant",
        version: "1.0.0",
        category: "personal-assistant",
        icon: "🎤",
        size: "456KB",
        author: "M5Stack",
        description: "AI-powered voice assistant with ChatGPT integration, multi-language support, and system voice commands.",
        features: [
            "ChatGPT integration for natural conversations",
            "Multi-language support (EN, ES, FR, DE, ZH, JA)",
            "Voice commands for system control",
            "Real-time voice visualizer",
            "Chat interface with history",
            "Secure API key management"
        ],
        tags: ["ai", "voice", "chatgpt", "assistant", "multilingual"],
        memoryUsage: {
            ram: 65536,
            flash: 456704,
            psram: 131072
        },
        packageUrl: "packages/voice-assistant-v1.0.0.m5app"
    },
    {
        id: "com.m5stack.basicapps",
        name: "Basic Apps Suite",
        version: "1.0.0",
        category: "personal-assistant",
        icon: "🧮",
        size: "678KB",
        author: "M5Stack",
        description: "Multi-feature productivity suite with expense tracker, calculator, spreadsheet, and entertainment games.",
        features: [
            "Expense tracker with income/expense management",
            "Scientific calculator with Unicode operators",
            "Basic spreadsheet with formulas",
            "Memory game with difficulty levels",
            "3x3 sliding puzzle game",
            "Financial reports and budgeting"
        ],
        tags: ["calculator", "expense", "spreadsheet", "games", "finance"],
        memoryUsage: {
            ram: 50240,
            flash: 678912,
            psram: 98304
        },
        packageUrl: "packages/basic-apps-suite-v1.0.0.m5app"
    },
    {
        id: "com.m5stack.alarms",
        name: "Alarm & Timer",
        version: "1.0.0",
        category: "personal-assistant",
        icon: "⏰",
        size: "156KB",
        author: "M5Stack",
        description: "Comprehensive timekeeping solution with smart alarms, multiple timers, stopwatch, and world clock.",
        features: [
            "Smart alarms with recurring patterns",
            "Multiple countdown timers",
            "Precision stopwatch with lap times",
            "World clock with timezone support",
            "Custom ringtones and volume control",
            "Snooze functionality"
        ],
        tags: ["alarm", "timer", "clock", "stopwatch", "scheduling"],
        memoryUsage: {
            ram: 32768,
            flash: 156672,
            psram: 16384
        },
        packageUrl: "packages/alarm-timer-v1.0.0.m5app"
    },

    // Productivity Apps
    {
        id: "com.m5stack.filemanager",
        name: "File Manager",
        version: "1.2.0",
        category: "productivity",
        icon: "📁",
        size: "324KB",
        author: "M5Stack",
        description: "Professional file management with multi-storage support, advanced operations, and storage analytics.",
        features: [
            "Multi-storage support (SD, USB, Flash)",
            "File operations (copy, move, delete, rename)",
            "Directory navigation and search",
            "Storage usage analytics",
            "File properties and permissions",
            "Progress tracking for operations"
        ],
        tags: ["files", "storage", "manager", "utility"],
        memoryUsage: {
            ram: 73728,
            flash: 324608,
            psram: 65536
        },
        packageUrl: "packages/file-manager-v1.2.0.m5app"
    },
    {
        id: "com.m5stack.camera",
        name: "Camera",
        version: "1.1.0",
        category: "productivity",
        icon: "📷",
        size: "412KB",
        author: "M5Stack",
        description: "Advanced camera application with photo/video capture, gallery management, and QR code scanning.",
        features: [
            "High-resolution photo capture",
            "MP4 video recording with audio",
            "Gallery with organization tools",
            "QR code and barcode scanner",
            "Basic photo editing (crop, rotate, filters)",
            "Camera settings and effects"
        ],
        tags: ["camera", "photo", "video", "qr-scanner", "gallery"],
        memoryUsage: {
            ram: 98304,
            flash: 412672,
            psram: 196608
        },
        packageUrl: "packages/camera-v1.1.0.m5app"
    },
    {
        id: "com.m5stack.calendar",
        name: "Calendar",
        version: "1.0.0",
        category: "productivity",
        icon: "📅",
        size: "287KB",
        author: "M5Stack",
        description: "Full-featured calendar with event management, reminders, recurring events, and iCal support.",
        features: [
            "Event creation and management",
            "Multiple calendar views (day, week, month)",
            "Event reminders and notifications",
            "Recurring event patterns",
            "iCal (.ics) import/export",
            "Holiday calendar integration"
        ],
        tags: ["calendar", "events", "scheduling", "reminders"],
        memoryUsage: {
            ram: 61440,
            flash: 287744,
            psram: 49152
        },
        packageUrl: "packages/calendar-v1.0.0.m5app"
    },
    {
        id: "com.m5stack.modular",
        name: "Modular App System",
        version: "2.0.0",
        category: "productivity",
        icon: "📱",
        size: "534KB",
        author: "M5Stack",
        description: "APK-style dynamic app loading system with app store interface and dependency management.",
        features: [
            "Dynamic application installation/removal",
            "App store browsing interface",
            "Dependency management system",
            "Version control and updates",
            "Sandboxed app execution",
            "Security validation"
        ],
        tags: ["system", "apps", "dynamic", "installer", "apk"],
        memoryUsage: {
            ram: 131072,
            flash: 534528,
            psram: 262144
        },
        packageUrl: "packages/modular-apps-v2.0.0.m5app"
    },

    // System & Utility Apps
    {
        id: "com.m5stack.terminal",
        name: "Enhanced Terminal",
        version: "1.3.0",
        category: "system-utility",
        icon: "💻",
        size: "445KB",
        author: "M5Stack",
        description: "Professional terminal with Telnet, SSH, RS-485 support and multiple concurrent sessions.",
        features: [
            "Telnet client with connection management",
            "SSH support with key authentication",
            "RS-485 communication protocol",
            "Multiple concurrent sessions",
            "Command history and search",
            "VT100/ANSI escape sequences"
        ],
        tags: ["terminal", "ssh", "telnet", "rs485", "networking"],
        memoryUsage: {
            ram: 98304,
            flash: 445440,
            psram: 131072
        },
        packageUrl: "packages/enhanced-terminal-v1.3.0.m5app"
    },
    {
        id: "com.m5stack.rs485terminal",
        name: "RS-485 Terminal",
        version: "1.1.0", 
        category: "system-utility",
        icon: "🔧",
        size: "267KB",
        author: "M5Stack",
        description: "Industrial communication terminal for Modbus RTU/ASCII with device scanning and protocol analysis.",
        features: [
            "Modbus RTU/ASCII protocol support",
            "Automatic device discovery",
            "Real-time packet analysis",
            "Communication error detection",
            "Data logging and monitoring",
            "Custom command sequences"
        ],
        tags: ["industrial", "modbus", "rs485", "protocol", "analysis"],
        memoryUsage: {
            ram: 65536,
            flash: 267264,
            psram: 49152
        },
        packageUrl: "packages/rs485-terminal-v1.1.0.m5app"
    },
    {
        id: "com.m5stack.integration",
        name: "App Integration",
        version: "1.0.0",
        category: "system-utility",
        icon: "🔗",
        size: "178KB",
        author: "M5Stack",
        description: "Inter-app communication framework with shared services and cross-app workflow management.",
        features: [
            "Inter-app communication APIs",
            "Shared data services",
            "Cross-app workflow automation",
            "Unified settings management",
            "Service discovery and registration",
            "Event broadcasting system"
        ],
        tags: ["integration", "api", "services", "framework"],
        memoryUsage: {
            ram: 40960,
            flash: 178176,
            psram: 32768
        },
        packageUrl: "packages/app-integration-v1.0.0.m5app"
    },

    // Industrial IoT Apps
    {
        id: "com.m5stack.iot-gateway",
        name: "IoT Gateway",
        version: "1.0.0",
        category: "industrial",
        icon: "🏭",
        size: "612KB",
        author: "M5Stack",
        description: "Industrial IoT gateway with MQTT, Modbus, and cloud connectivity for factory automation.",
        features: [
            "MQTT broker and client support",
            "Modbus TCP/RTU gateway",
            "Cloud service integration",
            "Device management dashboard",
            "Real-time data visualization",
            "Alert and notification system"
        ],
        tags: ["iot", "mqtt", "modbus", "gateway", "industrial"],
        memoryUsage: {
            ram: 147456,
            flash: 612352,
            psram: 294912
        },
        packageUrl: "packages/iot-gateway-v1.0.0.m5app"
    },
    {
        id: "com.m5stack.plc-monitor",
        name: "PLC Monitor",
        version: "1.0.0",
        category: "industrial",
        icon: "⚙️",
        size: "389KB",
        author: "M5Stack",
        description: "PLC monitoring and control interface with real-time data visualization and alarm management.",
        features: [
            "Real-time PLC data monitoring",
            "Control interface for outputs",
            "Alarm and event logging",
            "Trend charts and analytics",
            "Recipe and parameter management",
            "Remote access capabilities"
        ],
        tags: ["plc", "monitoring", "control", "automation", "hmi"],
        memoryUsage: {
            ram: 98304,
            flash: 389120,
            psram: 163840
        },
        packageUrl: "packages/plc-monitor-v1.0.0.m5app"
    }
];

// DOM elements
let appsGrid;
let filterButtons;

// Initialize the app store
document.addEventListener('DOMContentLoaded', function() {
    appsGrid = document.getElementById('apps-grid');
    filterButtons = document.querySelectorAll('.filter-btn');
    
    // Setup event listeners
    setupFilterButtons();
    setupCategoryCards();
    
    // Load all apps initially
    displayApps(applications);
});

// Setup filter button functionality
function setupFilterButtons() {
    filterButtons.forEach(button => {
        button.addEventListener('click', function() {
            // Remove active class from all buttons
            filterButtons.forEach(btn => btn.classList.remove('active'));
            // Add active class to clicked button
            this.classList.add('active');
            
            const filter = this.getAttribute('data-filter');
            filterApps(filter);
        });
    });
}

// Setup category card functionality
function setupCategoryCards() {
    const categoryCards = document.querySelectorAll('.category-card');
    categoryCards.forEach(card => {
        card.addEventListener('click', function() {
            const category = this.getAttribute('data-category');
            
            // Update filter buttons
            filterButtons.forEach(btn => btn.classList.remove('active'));
            const filterBtn = document.querySelector(`[data-filter="${category}"]`);
            if (filterBtn) {
                filterBtn.classList.add('active');
            }
            
            // Filter apps and scroll to apps section
            filterApps(category);
            document.getElementById('apps').scrollIntoView({ behavior: 'smooth' });
        });
    });
}

// Filter apps by category
function filterApps(category) {
    let filteredApps;
    
    if (category === 'all') {
        filteredApps = applications;
    } else {
        filteredApps = applications.filter(app => app.category === category);
    }
    
    displayApps(filteredApps);
}

// Display apps in the grid
function displayApps(apps) {
    appsGrid.innerHTML = '';
    
    apps.forEach((app, index) => {
        const appCard = createAppCard(app);
        appCard.style.animationDelay = `${index * 0.1}s`;
        appsGrid.appendChild(appCard);
    });
}

// Create individual app card
function createAppCard(app) {
    const card = document.createElement('div');
    card.className = 'app-card';
    card.setAttribute('data-category', app.category);
    
    card.innerHTML = `
        <div class="app-header">
            <div class="app-icon">${app.icon}</div>
            <div class="app-title">
                <h3>${app.name}</h3>
                <div class="app-meta">
                    <span class="app-version">v${app.version}</span>
                    <span class="app-size">${app.size}</span>
                </div>
            </div>
        </div>
        <div class="app-body">
            <p class="app-description">${app.description}</p>
            <ul class="app-features">
                ${app.features.slice(0, 4).map(feature => `<li>${feature}</li>`).join('')}
                ${app.features.length > 4 ? `<li>+ ${app.features.length - 4} more features</li>` : ''}
            </ul>
            <div class="app-tags">
                ${app.tags.map(tag => `<span class="app-tag">${tag}</span>`).join('')}
            </div>
            <div class="app-actions">
                <button class="btn btn-primary" onclick="downloadApp('${app.id}')">
                    📥 Download
                </button>
                <button class="btn btn-secondary" onclick="viewAppDetails('${app.id}')">
                    📋 Details
                </button>
            </div>
        </div>
    `;
    
    return card;
}

// Download app function
function downloadApp(appId) {
    const app = applications.find(a => a.id === appId);
    if (!app) return;
    
    // Simulate download process
    const downloadBtn = event.target;
    const originalText = downloadBtn.innerHTML;
    
    downloadBtn.innerHTML = '⏳ Downloading...';
    downloadBtn.disabled = true;
    
    setTimeout(() => {
        downloadBtn.innerHTML = '✅ Downloaded';
        setTimeout(() => {
            downloadBtn.innerHTML = originalText;
            downloadBtn.disabled = false;
        }, 2000);
        
        // Create and trigger download
        createAppPackage(app);
    }, 2000);
}

// Create app package following M5Tab5 standards
function createAppPackage(app) {
    const manifest = {
        app: {
            id: app.id,
            name: app.name,
            version: app.version,
            description: app.description,
            author: app.author,
            category: app.category,
            tags: app.tags
        },
        system: {
            min_os_version: "4.0.0",
            target_platform: "m5stack-tab5",
            architecture: "esp32p4"
        },
        requirements: {
            memory: app.memoryUsage,
            permissions: [
                "STORAGE_READ",
                "STORAGE_WRITE",
                "UI_CREATE"
            ]
        },
        resources: {
            icon: `assets/${app.icon}.png`,
            screenshots: [
                `assets/screenshots/${app.id}_1.png`,
                `assets/screenshots/${app.id}_2.png`
            ]
        },
        build: {
            entry_point: app.name.replace(/\s+/g, ''),
            factory_function: `create${app.name.replace(/\s+/g, '')}`
        }
    };
    
    // Create downloadable manifest file
    const manifestBlob = new Blob([JSON.stringify(manifest, null, 2)], {
        type: 'application/json'
    });
    
    const url = URL.createObjectURL(manifestBlob);
    const a = document.createElement('a');
    a.href = url;
    a.download = `${app.id}-manifest.json`;
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);
    URL.revokeObjectURL(url);
    
    // Show installation instructions
    showInstallationInstructions(app);
}

// View app details
function viewAppDetails(appId) {
    const app = applications.find(a => a.id === appId);
    if (!app) return;
    
    // Create modal with app details
    const modal = document.createElement('div');
    modal.className = 'modal-overlay';
    modal.innerHTML = `
        <div class="modal-content">
            <div class="modal-header">
                <h2>${app.icon} ${app.name}</h2>
                <button class="modal-close">&times;</button>
            </div>
            <div class="modal-body">
                <div class="app-detail-grid">
                    <div class="app-detail-section">
                        <h3>Description</h3>
                        <p>${app.description}</p>
                    </div>
                    <div class="app-detail-section">
                        <h3>Features</h3>
                        <ul>
                            ${app.features.map(feature => `<li>${feature}</li>`).join('')}
                        </ul>
                    </div>
                    <div class="app-detail-section">
                        <h3>Technical Specifications</h3>
                        <table class="spec-table">
                            <tr><td>Version</td><td>${app.version}</td></tr>
                            <tr><td>Size</td><td>${app.size}</td></tr>
                            <tr><td>RAM Usage</td><td>${(app.memoryUsage.ram / 1024).toFixed(1)}KB</td></tr>
                            <tr><td>Flash Usage</td><td>${(app.memoryUsage.flash / 1024).toFixed(1)}KB</td></tr>
                            <tr><td>PSRAM Usage</td><td>${(app.memoryUsage.psram / 1024).toFixed(1)}KB</td></tr>
                            <tr><td>Category</td><td>${app.category}</td></tr>
                        </table>
                    </div>
                    <div class="app-detail-section">
                        <h3>Package Information</h3>
                        <p><strong>Package ID:</strong> ${app.id}</p>
                        <p><strong>Author:</strong> ${app.author}</p>
                        <p><strong>Tags:</strong> ${app.tags.join(', ')}</p>
                    </div>
                </div>
                <div class="modal-actions">
                    <button class="btn btn-primary" onclick="downloadApp('${app.id}')">📥 Download Package</button>
                </div>
            </div>
        </div>
    `;
    
    document.body.appendChild(modal);
    
    // Close modal functionality
    const closeBtn = modal.querySelector('.modal-close');
    const overlay = modal;
    
    closeBtn.addEventListener('click', () => {
        document.body.removeChild(modal);
    });
    
    overlay.addEventListener('click', (e) => {
        if (e.target === overlay) {
            document.body.removeChild(modal);
        }
    });
}

// Show installation instructions
function showInstallationInstructions(app) {
    const instructions = `
# Installation Instructions for ${app.name}

## Method 1: Using M5Stack Tab5 Package Manager
1. Copy the downloaded manifest file to your M5Stack Tab5 device
2. Open the Modular App System on your device
3. Select "Install from File" and choose the manifest
4. Follow the on-screen installation process

## Method 2: Manual Installation
1. Download the full source code from the GitHub repository
2. Copy the app files to src/apps/${app.id.split('.').pop()}/
3. Add the app to your platformio.ini build configuration
4. Compile and flash the updated firmware

## Prerequisites
- M5Stack Tab5 v4 OS (minimum version 4.0.0)
- Available memory: ${(app.memoryUsage.ram / 1024).toFixed(1)}KB RAM, ${(app.memoryUsage.flash / 1024).toFixed(1)}KB Flash
- Required permissions: Storage access, UI creation

## Support
For installation issues, visit: https://github.com/Paqurin/m5tab5-lvgl/issues
    `;
    
    const instructionsBlob = new Blob([instructions], {
        type: 'text/markdown'
    });
    
    const url = URL.createObjectURL(instructionsBlob);
    const a = document.createElement('a');
    a.href = url;
    a.download = `${app.name.replace(/\s+/g, '-')}-installation.md`;
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);
    URL.revokeObjectURL(url);
}

// Add modal styles
const modalStyles = `
<style>
.modal-overlay {
    position: fixed;
    top: 0;
    left: 0;
    width: 100%;
    height: 100%;
    background: rgba(0, 0, 0, 0.8);
    display: flex;
    justify-content: center;
    align-items: center;
    z-index: 1000;
    backdrop-filter: blur(5px);
}

.modal-content {
    background: var(--card-bg);
    border-radius: 16px;
    max-width: 800px;
    max-height: 90vh;
    overflow-y: auto;
    border: 1px solid var(--border-color);
    box-shadow: var(--shadow-hover);
}

.modal-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 1.5rem;
    border-bottom: 1px solid var(--border-color);
}

.modal-header h2 {
    margin: 0;
    color: var(--text-primary);
}

.modal-close {
    background: none;
    border: none;
    color: var(--text-secondary);
    font-size: 2rem;
    cursor: pointer;
    transition: color 0.3s ease;
}

.modal-close:hover {
    color: var(--danger-color);
}

.modal-body {
    padding: 1.5rem;
}

.app-detail-grid {
    display: grid;
    gap: 2rem;
    margin-bottom: 2rem;
}

.app-detail-section h3 {
    color: var(--primary-color);
    margin-bottom: 1rem;
}

.app-detail-section p,
.app-detail-section li {
    color: var(--text-secondary);
    line-height: 1.6;
}

.app-detail-section ul {
    list-style: none;
    padding-left: 0;
}

.app-detail-section li {
    padding: 0.3rem 0;
    position: relative;
    padding-left: 1.5rem;
}

.app-detail-section li::before {
    content: '✓';
    position: absolute;
    left: 0;
    color: var(--secondary-color);
    font-weight: bold;
}

.spec-table {
    width: 100%;
    border-collapse: collapse;
}

.spec-table td {
    padding: 0.5rem 0;
    border-bottom: 1px solid var(--border-color);
    color: var(--text-secondary);
}

.spec-table td:first-child {
    font-weight: 600;
    color: var(--text-primary);
}

.modal-actions {
    text-align: center;
}

@media (max-width: 768px) {
    .modal-content {
        margin: 1rem;
        max-height: calc(100vh - 2rem);
    }
}
</style>
`;

document.head.insertAdjacentHTML('beforeend', modalStyles);

// Smooth scrolling for navigation links
document.querySelectorAll('a[href^="#"]').forEach(anchor => {
    anchor.addEventListener('click', function (e) {
        e.preventDefault();
        const target = document.querySelector(this.getAttribute('href'));
        if (target) {
            target.scrollIntoView({
                behavior: 'smooth',
                block: 'start'
            });
        }
    });
});