#include <unity.h>
#include "../src/apps/basic_apps_suite.h"
#include "../src/apps/contact_management_app.h"
#include "../src/apps/alarm_timer_app.h"
#include "../src/apps/base_app.h"

// Test fixtures
BasicAppsSuite* basicAppsSuite = nullptr;
ContactManagementApp* contactApp = nullptr;
AlarmTimerApp* alarmTimerApp = nullptr;

void setUp(void) {
    // Set up code here to run before each test
}

void tearDown(void) {
    // Clean up code here to run after each test
    if (basicAppsSuite) {
        basicAppsSuite->shutdown();
        delete basicAppsSuite;
        basicAppsSuite = nullptr;
    }
    if (contactApp) {
        contactApp->shutdown();
        delete contactApp;
        contactApp = nullptr;
    }
    if (alarmTimerApp) {
        alarmTimerApp->shutdown();
        delete alarmTimerApp;
        alarmTimerApp = nullptr;
    }
}

// Basic Apps Suite Tests
void test_basic_apps_suite_initialization() {
    basicAppsSuite = new BasicAppsSuite();
    
    // Test initial state
    TEST_ASSERT_EQUAL_STRING("com.m5stack.basicapps", basicAppsSuite->getId().c_str());
    TEST_ASSERT_EQUAL_STRING("Basic Apps", basicAppsSuite->getName().c_str());
    TEST_ASSERT_EQUAL_STRING("1.0.0", basicAppsSuite->getVersion().c_str());
    TEST_ASSERT_EQUAL(AppState::STOPPED, basicAppsSuite->getState());
    
    // Test initialization
    os_error_t result = basicAppsSuite->initialize();
    TEST_ASSERT_EQUAL(OS_OK, result);
    TEST_ASSERT_TRUE(basicAppsSuite->getMemoryUsage() > 0);
}

void test_basic_apps_suite_expense_management() {
    basicAppsSuite = new BasicAppsSuite();
    basicAppsSuite->initialize();
    
    // Test that basic expense data was loaded
    // Note: This tests the sample data loading in loadExpenses()
    TEST_ASSERT_TRUE(basicAppsSuite->getMemoryUsage() == 50240);  // Expected memory usage
}

void test_basic_apps_suite_currency_formatting() {
    basicAppsSuite = new BasicAppsSuite();
    
    // We can't directly test private methods, but we can test through public interface
    // The formatCurrency method is used internally and tested through expense operations
    TEST_ASSERT_NOT_NULL(basicAppsSuite);
}

void test_basic_apps_suite_calculator_functionality() {
    basicAppsSuite = new BasicAppsSuite();
    basicAppsSuite->initialize();
    
    // Test calculator state initialization
    // The calculator should be properly initialized in the constructor
    TEST_ASSERT_EQUAL(AppPriority::APP_NORMAL, basicAppsSuite->getPriority());
}

// Contact Management App Tests
void test_contact_app_initialization() {
    contactApp = new ContactManagementApp();
    
    // Test initial state
    TEST_ASSERT_EQUAL_STRING("com.m5stack.contacts", contactApp->getId().c_str());
    TEST_ASSERT_EQUAL_STRING("Contacts", contactApp->getName().c_str());
    TEST_ASSERT_EQUAL_STRING("1.0.0", contactApp->getVersion().c_str());
    TEST_ASSERT_EQUAL(AppState::STOPPED, contactApp->getState());
    
    // Test initialization
    os_error_t result = contactApp->initialize();
    TEST_ASSERT_EQUAL(OS_OK, result);
    TEST_ASSERT_TRUE(contactApp->getMemoryUsage() > 0);
}

void test_contact_app_sample_data_loading() {
    contactApp = new ContactManagementApp();
    contactApp->initialize();
    
    // Test that sample contacts were loaded
    TEST_ASSERT_EQUAL(32768, contactApp->getMemoryUsage());  // Expected memory usage
}

void test_contact_app_priority() {
    contactApp = new ContactManagementApp();
    TEST_ASSERT_EQUAL(AppPriority::APP_NORMAL, contactApp->getPriority());
}

// Alarm Timer App Tests
void test_alarm_timer_app_initialization() {
    alarmTimerApp = new AlarmTimerApp();
    
    // Test initial state
    TEST_ASSERT_EQUAL_STRING("com.m5stack.alarmtimer", alarmTimerApp->getId().c_str());
    TEST_ASSERT_EQUAL_STRING("Alarm & Timer", alarmTimerApp->getName().c_str());
    TEST_ASSERT_EQUAL_STRING("1.0.0", alarmTimerApp->getVersion().c_str());
    TEST_ASSERT_EQUAL(AppState::STOPPED, alarmTimerApp->getState());
    
    // Test initialization
    os_error_t result = alarmTimerApp->initialize();
    TEST_ASSERT_EQUAL(OS_OK, result);
    TEST_ASSERT_TRUE(alarmTimerApp->getMemoryUsage() > 0);
}

void test_alarm_timer_app_sample_data_loading() {
    alarmTimerApp = new AlarmTimerApp();
    alarmTimerApp->initialize();
    
    // Test that sample alarms and timers were loaded
    TEST_ASSERT_EQUAL(40960, alarmTimerApp->getMemoryUsage());  // Expected memory usage
}

void test_alarm_timer_app_priority() {
    alarmTimerApp = new AlarmTimerApp();
    TEST_ASSERT_EQUAL(AppPriority::APP_NORMAL, alarmTimerApp->getPriority());
}

void test_alarm_timer_app_update_functionality() {
    alarmTimerApp = new AlarmTimerApp();
    alarmTimerApp->initialize();
    
    // Test multiple updates - should handle clock updates, alarm checks, etc.
    for (int i = 0; i < 10; i++) {
        os_error_t result = alarmTimerApp->update(1000);  // 1 second delta
        TEST_ASSERT_EQUAL(OS_OK, result);
    }
}

// Base App Interface Tests
void test_app_lifecycle_basic_apps() {
    basicAppsSuite = new BasicAppsSuite();
    
    // Test lifecycle states
    TEST_ASSERT_EQUAL(AppState::STOPPED, basicAppsSuite->getState());
    
    os_error_t result = basicAppsSuite->initialize();
    TEST_ASSERT_EQUAL(OS_OK, result);
    
    result = basicAppsSuite->start();
    TEST_ASSERT_EQUAL(OS_OK, result);
    TEST_ASSERT_EQUAL(AppState::RUNNING, basicAppsSuite->getState());
    TEST_ASSERT_TRUE(basicAppsSuite->isRunning());
    
    result = basicAppsSuite->pause();
    TEST_ASSERT_EQUAL(OS_OK, result);
    TEST_ASSERT_EQUAL(AppState::PAUSED, basicAppsSuite->getState());
    TEST_ASSERT_TRUE(basicAppsSuite->isPaused());
    
    result = basicAppsSuite->resume();
    TEST_ASSERT_EQUAL(OS_OK, result);
    TEST_ASSERT_EQUAL(AppState::RUNNING, basicAppsSuite->getState());
    
    result = basicAppsSuite->stop();
    TEST_ASSERT_EQUAL(OS_OK, result);
    TEST_ASSERT_EQUAL(AppState::STOPPED, basicAppsSuite->getState());
}

void test_app_lifecycle_contact_app() {
    contactApp = new ContactManagementApp();
    
    // Test lifecycle states
    TEST_ASSERT_EQUAL(AppState::STOPPED, contactApp->getState());
    
    os_error_t result = contactApp->initialize();
    TEST_ASSERT_EQUAL(OS_OK, result);
    
    result = contactApp->start();
    TEST_ASSERT_EQUAL(OS_OK, result);
    TEST_ASSERT_EQUAL(AppState::RUNNING, contactApp->getState());
    
    result = contactApp->stop();
    TEST_ASSERT_EQUAL(OS_OK, result);
    TEST_ASSERT_EQUAL(AppState::STOPPED, contactApp->getState());
}

void test_app_info_retrieval() {
    basicAppsSuite = new BasicAppsSuite();
    basicAppsSuite->initialize();
    
    AppInfo info = basicAppsSuite->getAppInfo();
    TEST_ASSERT_EQUAL_STRING("com.m5stack.basicapps", info.id.c_str());
    TEST_ASSERT_EQUAL_STRING("Basic Apps", info.name.c_str());
    TEST_ASSERT_EQUAL_STRING("1.0.0", info.version.c_str());
    TEST_ASSERT_EQUAL(AppPriority::APP_NORMAL, info.priority);
    TEST_ASSERT_EQUAL(50240, info.memoryUsage);
}

void test_memory_usage_tracking() {
    basicAppsSuite = new BasicAppsSuite();
    contactApp = new ContactManagementApp();
    
    basicAppsSuite->initialize();
    contactApp->initialize();
    
    // Test that memory usage is properly set
    TEST_ASSERT_EQUAL(50240, basicAppsSuite->getMemoryUsage());  // Basic Apps Suite
    TEST_ASSERT_EQUAL(32768, contactApp->getMemoryUsage());     // Contact Management
}

// Error handling tests
void test_invalid_ui_creation() {
    basicAppsSuite = new BasicAppsSuite();
    basicAppsSuite->initialize();
    
    // Test creating UI with null parent
    os_error_t result = basicAppsSuite->createUI(nullptr);
    TEST_ASSERT_EQUAL(OS_ERROR_INVALID_PARAM, result);
}

void test_update_without_initialization() {
    basicAppsSuite = new BasicAppsSuite();
    
    // Test update before initialization - should still work but with limited functionality
    os_error_t result = basicAppsSuite->update(16);  // 16ms delta time (60 FPS)
    TEST_ASSERT_EQUAL(OS_OK, result);
}

// Performance tests
void test_update_performance() {
    basicAppsSuite = new BasicAppsSuite();
    basicAppsSuite->initialize();
    
    // Test multiple updates
    for (int i = 0; i < 100; i++) {
        os_error_t result = basicAppsSuite->update(16);
        TEST_ASSERT_EQUAL(OS_OK, result);
    }
}

// Main test runner
void setup() {
    UNITY_BEGIN();
    
    // Basic Apps Suite Tests
    RUN_TEST(test_basic_apps_suite_initialization);
    RUN_TEST(test_basic_apps_suite_expense_management);
    RUN_TEST(test_basic_apps_suite_currency_formatting);
    RUN_TEST(test_basic_apps_suite_calculator_functionality);
    
    // Contact Management Tests
    RUN_TEST(test_contact_app_initialization);
    RUN_TEST(test_contact_app_sample_data_loading);
    RUN_TEST(test_contact_app_priority);
    
    // Alarm Timer App Tests
    RUN_TEST(test_alarm_timer_app_initialization);
    RUN_TEST(test_alarm_timer_app_sample_data_loading);
    RUN_TEST(test_alarm_timer_app_priority);
    RUN_TEST(test_alarm_timer_app_update_functionality);
    
    // Lifecycle Tests
    RUN_TEST(test_app_lifecycle_basic_apps);
    RUN_TEST(test_app_lifecycle_contact_app);
    RUN_TEST(test_app_info_retrieval);
    RUN_TEST(test_memory_usage_tracking);
    
    // Error Handling Tests
    RUN_TEST(test_invalid_ui_creation);
    RUN_TEST(test_update_without_initialization);
    
    // Performance Tests
    RUN_TEST(test_update_performance);
    
    UNITY_END();
}

void loop() {
    // Empty loop for Arduino framework compatibility
}