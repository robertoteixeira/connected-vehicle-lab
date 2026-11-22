#include "AzureIoTManager.h"

#include <Arduino.h>
#include <WiFi.h>
#include <mbedtls/x509_crt.h>
#include <time.h>

#include "AzureIoTAuth.h"
#include "AzureIoTConfig.h"

// Arduino shadows ESP-IDF's esp_crt_bundle.h with a different API. Declare the
// ESP-IDF function linked from libmbedtls to use its built-in CA bundle.
extern "C" esp_err_t esp_crt_bundle_attach(void* conf);

void AzureIoTManager::onMqttEvent(void* handlerArgs, esp_event_base_t eventBase, int32_t eventId,
                                  void* eventData) {
    auto* manager = static_cast<AzureIoTManager*>(handlerArgs);

    switch (eventId) {
        case MQTT_EVENT_CONNECTED: {
            manager->connected_.store(true);
            Serial.println("Azure MQTT connected");
            break;
        }

        case MQTT_EVENT_PUBLISHED: {
            const auto* event = static_cast<esp_mqtt_event_t*>(eventData);

            Serial.printf("Azure acknowledged message: %d\n", event->msg_id);
            break;
        }

        case MQTT_EVENT_DISCONNECTED:
            manager->connected_.store(false);
            Serial.println("Azure MQTT disconnected");
            break;

        case MQTT_EVENT_ERROR: {
            manager->connected_.store(false);
            Serial.println("Azure MQTT connection error");

            const auto* event = static_cast<esp_mqtt_event_t*>(eventData);

            if (event != nullptr && event->error_handle != nullptr) {
                const auto* error = event->error_handle;

                Serial.printf("TLS error: %d, certificate flags: 0x%X\n", error->esp_tls_stack_err,
                              static_cast<unsigned>(error->esp_tls_cert_verify_flags));

                if (error->esp_tls_cert_verify_flags != 0) {
                    char details[512] = {};
                    mbedtls_x509_crt_verify_info(details, sizeof(details), "  ",
                                                 error->esp_tls_cert_verify_flags);
                    Serial.print(details);
                }
            }

            break;
        }

        default:
            break;
    }
}

bool AzureIoTManager::begin() {
    if (client_ != nullptr) {
        return true;
    }

    esp_mqtt_client_config_t config = {};
    config.host = AzureIoTConfig::HOST_NAME;
    config.port = 8883;
    config.transport = MQTT_TRANSPORT_OVER_SSL;
    config.crt_bundle_attach = esp_crt_bundle_attach;
    config.client_id = AzureIoTConfig::DEVICE_ID;

    client_ = esp_mqtt_client_init(&config);

    if (client_ == nullptr) {
        return false;
    }

    const esp_err_t result = esp_mqtt_client_register_event(
        client_, static_cast<esp_mqtt_event_id_t>(ESP_EVENT_ANY_ID), onMqttEvent, this);

    if (result != ESP_OK) {
        esp_mqtt_client_destroy(client_);
        client_ = nullptr;
        return false;
    }

    return true;
}

void AzureIoTManager::update() {
    if (client_ == nullptr || WiFi.status() != WL_CONNECTED) {
        return;
    }

    if (!timeSyncStarted_) {
        configTime(0, 0, "pool.ntp.org", "time.google.com");
        timeSyncStarted_ = true;

        Serial.println("Clock synchronization requested");
    }

    if (!clockReady_) {
        constexpr time_t MIN_VALID_TIME = 1704067200;

        if (time(nullptr) >= MIN_VALID_TIME) {
            clockReady_ = true;
            Serial.println("Clock ready for Azure connection");
        }
    }

    if (!clockReady_ || mqttStarted_) {
        return;
    }

    const unsigned long nowMs = millis();

    if (nowMs - lastConnectionAttemptMs_ < 5000) {
        return;
    }

    lastConnectionAttemptMs_ = nowMs;

    const String username = String(AzureIoTConfig::HOST_NAME) + "/" + AzureIoTConfig::DEVICE_ID +
                            "/?api-version=2021-04-12";

    const String sasToken = AzureIoTAuth::createSasToken(time(nullptr) + 3600);

    if (sasToken.isEmpty()) {
        Serial.println("Azure SAS token generation failed");
        return;
    }

    esp_mqtt_client_config_t config = {};
    config.host = AzureIoTConfig::HOST_NAME;
    config.port = 8883;
    config.transport = MQTT_TRANSPORT_OVER_SSL;
    config.crt_bundle_attach = esp_crt_bundle_attach;
    config.client_id = AzureIoTConfig::DEVICE_ID;
    config.username = username.c_str();
    config.password = sasToken.c_str();
    config.protocol_ver = MQTT_PROTOCOL_V_3_1_1;

    esp_err_t result = esp_mqtt_set_config(client_, &config);

    if (result != ESP_OK) {
        Serial.printf("Azure MQTT configuration failed: %d\n", result);
        return;
    }

    result = esp_mqtt_client_start(client_);

    if (result != ESP_OK) {
        Serial.printf("Azure MQTT start failed: %d\n", result);
        return;
    }

    mqttStarted_ = true;
    Serial.println("Azure MQTT client started");
}

bool AzureIoTManager::publishSpeed(float speedKph) {
    if (client_ == nullptr || !connected_.load()) {
        return false;
    }

    char payload[64];
    snprintf(payload, sizeof(payload), "{\"speedKph\":%.1f}", speedKph);

    const String topic = String("devices/") + AzureIoTConfig::DEVICE_ID +
                         "/messages/events/$.ct=application%2Fjson&$.ce=utf-8";

    const int messageId = esp_mqtt_client_enqueue(client_, topic.c_str(), payload, 0, 1, 0, true);

    if (messageId < 0) {
        Serial.println("Azure speed message queue failed");
        return false;
    }

    Serial.printf("Azure speed queued: %s\n", payload);
    return true;
}
