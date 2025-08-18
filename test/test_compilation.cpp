// Simple compilation test for the three main applications
#include <Arduino.h>

// Include only the applications we want to test
#include "../src/apps/basic_apps_suite.h"
#include "../src/apps/contact_management_app.h"
#include "../src/apps/alarm_timer_app.h"

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("=== M5Stack Tab5 Application Compilation Test ===");
    
    // Test Basic Apps Suite compilation
    Serial.println("Testing Basic Apps Suite...");
    BasicAppsSuite* basicApps = new BasicAppsSuite();
    if (basicApps) {
        Serial.printf("✓ Basic Apps Suite created: %s v%s\n", 
                     basicApps->getName().c_str(), 
                     basicApps->getVersion().c_str());
        delete basicApps;
    } else {
        Serial.println("✗ Failed to create Basic Apps Suite");
    }
    
    // Test Contact Management App compilation
    Serial.println("Testing Contact Management App...");
    ContactManagementApp* contactApp = new ContactManagementApp();
    if (contactApp) {
        Serial.printf("✓ Contact Management App created: %s v%s\n", 
                     contactApp->getName().c_str(), 
                     contactApp->getVersion().c_str());
        delete contactApp;
    } else {
        Serial.println("✗ Failed to create Contact Management App");
    }
    
    // Test Alarm Timer App compilation
    Serial.println("Testing Alarm Timer App...");
    AlarmTimerApp* alarmApp = new AlarmTimerApp();
    if (alarmApp) {
        Serial.printf("✓ Alarm Timer App created: %s v%s\n", 
                     alarmApp->getName().c_str(), 
                     alarmApp->getVersion().c_str());
        delete alarmApp;
    } else {
        Serial.println("✗ Failed to create Alarm Timer App");
    }
    
    Serial.println("=== Compilation Test Complete ===");
}

void loop() {
    // Empty loop
    delay(1000);
}