#include "AzureIoTAuth.h"

#include <mbedtls/base64.h>
#include <mbedtls/md.h>

#include "AzureIoTConfig.h"
#include "AzureIoTSecrets.h.local"

namespace AzureIoTAuth {

String urlEncode(const String& value) {
    constexpr char HEX_DIGITS[] = "0123456789ABCDEF";
    String encoded;
    encoded.reserve(value.length() * 3);

    for (size_t index = 0; index < value.length(); ++index) {
        const unsigned char character = static_cast<unsigned char>(value[index]);

        const bool isUnreserved = (character >= 'A' && character <= 'Z') ||
                                  (character >= 'a' && character <= 'z') ||
                                  (character >= '0' && character <= '9') || character == '-' ||
                                  character == '_' || character == '.' || character == '~';

        if (isUnreserved) {
            encoded += static_cast<char>(character);
        } else {
            encoded += '%';
            encoded += HEX_DIGITS[character >> 4];
            encoded += HEX_DIGITS[character & 0x0F];
        }
    }

    return encoded;
}

String createSasToken(time_t expiresAt) {
    if (expiresAt <= time(nullptr)) {
        return {};
    }

    String resource = String(AzureIoTConfig::HOST_NAME) + "/devices/" + AzureIoTConfig::DEVICE_ID;
    resource.toLowerCase();

    const String encodedResource = urlEncode(resource);
    const String expiry = String(static_cast<unsigned long>(expiresAt));
    const String stringToSign = encodedResource + "\n" + expiry;

    unsigned char decodedKey[64] = {};
    size_t decodedKeyLength = 0;

    int result =
        mbedtls_base64_decode(decodedKey, sizeof(decodedKey), &decodedKeyLength,
                              reinterpret_cast<const unsigned char*>(AzureIoTSecrets::DEVICE_KEY),
                              strlen(AzureIoTSecrets::DEVICE_KEY));

    if (result != 0 || decodedKeyLength == 0) {
        return {};
    }

    const mbedtls_md_info_t* sha256 = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);

    if (sha256 == nullptr) {
        return {};
    }

    unsigned char signature[32] = {};

    result = mbedtls_md_hmac(sha256, decodedKey, decodedKeyLength,
                             reinterpret_cast<const unsigned char*>(stringToSign.c_str()),
                             stringToSign.length(), signature);

    if (result != 0) {
        return {};
    }

    unsigned char encodedSignature[64] = {};
    size_t encodedSignatureLength = 0;

    result = mbedtls_base64_encode(encodedSignature, sizeof(encodedSignature),
                                   &encodedSignatureLength, signature, sizeof(signature));

    if (result != 0) {
        return {};
    }

    const String signatureText(reinterpret_cast<const char*>(encodedSignature));

    return "SharedAccessSignature sr=" + encodedResource + "&sig=" + urlEncode(signatureText) +
           "&se=" + expiry;
}

}  // namespace AzureIoTAuth