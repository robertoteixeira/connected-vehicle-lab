#include "AzureIoTManager.h"

#include <Arduino.h>
#include <WiFi.h>
#include <esp_heap_caps.h>
#include <mbedtls/x509_crt.h>
#include <time.h>

#include "AzureIoTAuth.h"
#include "AzureIoTConfig.h"

// Arduino shadows ESP-IDF's esp_crt_bundle.h with a different API. Declare the
// ESP-IDF function linked from libmbedtls to use its built-in CA bundle.
extern "C" esp_err_t esp_crt_bundle_attach(void* conf);

extern const char azureRootCertificates[] asm("_binary_certs_azure_roots_pem_start");

namespace {
void onMqttEvent(void* handlerArgs, esp_event_base_t eventBase, int32_t eventId, void* eventData) {
    switch (eventId) {
        case MQTT_EVENT_CONNECTED:
            Serial.println("Azure MQTT connected");
            break;

        case MQTT_EVENT_DISCONNECTED:
            Serial.println("Azure MQTT disconnected");
            break;

        case MQTT_EVENT_ERROR: {
            Serial.println("Azure MQTT connection error");

            const auto* event = static_cast<esp_mqtt_event_t*>(eventData);

            if (event != nullptr && event->error_handle != nullptr) {
                const auto* error = event->error_handle;

                Serial.printf("TLS error: %d, certificate flags: 0x%X\n",
                              error->esp_tls_stack_err,
                              static_cast<unsigned>(error->esp_tls_cert_verify_flags));

                if (error->esp_tls_cert_verify_flags != 0) {
                    char details[512] = {};
                    mbedtls_x509_crt_verify_info(
                        details, sizeof(details), "  ", error->esp_tls_cert_verify_flags);
                    Serial.print(details);
                }
            }

            Serial.printf("Current Unix time: %lu\n",
                          static_cast<unsigned long>(time(nullptr)));

            Serial.printf("At MQTT error: free internal heap=%u, largest block=%u\n",
                          static_cast<unsigned>(
                              heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)),
                          static_cast<unsigned>(heap_caps_get_largest_free_block(
                              MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)));
            break;
        }

        default:
            break;
    }
}
}  // namespace

bool AzureIoTManager::begin() {
    if (client_ != nullptr) {
        return true;
    }

    // Temporary diagnostic: confirm the running firmware can parse every embedded root.
    mbedtls_x509_crt roots;
    mbedtls_x509_crt_init(&roots);

    const int parseResult = mbedtls_x509_crt_parse(
        &roots, reinterpret_cast<const unsigned char*>(azureRootCertificates),
        strlen(azureRootCertificates) + 1);

    Serial.printf("Embedded CA parse result: %d\n", parseResult);

    unsigned rootCount = 0;

    for (const mbedtls_x509_crt* root = &roots;
         root != nullptr && root->raw.p != nullptr; root = root->next) {
        char subject[256] = {};
        mbedtls_x509_dn_gets(subject, sizeof(subject), &root->subject);
        Serial.printf("Embedded CA %u: %s\n", ++rootCount, subject);
    }

    mbedtls_x509_crt_free(&roots);

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
        client_, static_cast<esp_mqtt_event_id_t>(ESP_EVENT_ANY_ID), onMqttEvent, nullptr);

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

    Serial.printf(
        "Before MQTT start: free internal heap=%u, largest block=%u\n",
        static_cast<unsigned>(heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)),
        static_cast<unsigned>(
            heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)));

    result = esp_mqtt_client_start(client_);

    if (result != ESP_OK) {
        Serial.printf("Azure MQTT start failed: %d\n", result);
        return;
    }

    mqttStarted_ = true;
    Serial.println("Azure MQTT client started");
}
