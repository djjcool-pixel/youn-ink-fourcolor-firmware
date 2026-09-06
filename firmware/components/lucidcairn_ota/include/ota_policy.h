// SPDX-License-Identifier: MIT
#pragma once
#include <cstddef>
#include <cstdint>
#include <string>

namespace lucidcairn::ota {
constexpr size_t kMaxManifestBytes = 8192;
constexpr size_t kSlotBytes = 0x3f0000;
constexpr char kLayout[] = "note4c-16m-v2";
struct Manifest {
    std::string version, download_url, sha256;
    size_t size = 0;
    int minimum_battery_percent = 30;
};
bool ParseManifest(const std::string& json, Manifest& out);
bool NewerVersion(const std::string& candidate, const std::string& current);
std::string HttpsOrigin(const std::string& url);
bool ValidToken(const std::string& token);
bool PowerAllowsUpdate(bool known, int percent, bool external, int minimum);
bool LocalHealthPasses(bool power, bool nvs, bool heap, bool display, bool application);

// The transaction is shared by the real ESP-IDF adapter and fault-injection tests.
// Read: positive bytes, zero for complete EOF, negative for transport failure.
// Finish validates the ESP image and its descriptor; only SelectBoot changes otadata.
class InstallIo {
public:
    virtual ~InstallIo() = default;
    virtual bool Open(const Manifest&) = 0;
    virtual bool Begin(size_t size) = 0;
    virtual int Read(uint8_t* buffer, size_t capacity) = 0;
    virtual bool Write(const uint8_t* buffer, size_t size) = 0;
    virtual bool Hash(const uint8_t* buffer, size_t size) = 0;
    virtual std::string Digest() = 0;
    virtual bool Finish(const Manifest&) = 0;
    virtual bool SelectBoot() = 0;
    virtual void Abort() = 0;
};
enum class InstallResult { Installed, Transport, Flash, Integrity, Image, BootSelection };
InstallResult Install(const Manifest& manifest, InstallIo& io);
}  // namespace lucidcairn::ota
