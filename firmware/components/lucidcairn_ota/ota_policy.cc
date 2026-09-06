// SPDX-License-Identifier: MIT
#include "ota_policy.h"
#include "cJSON.h"
#include <array>
#include <cmath>
#include <cstring>
#include <limits>
#include <memory>

namespace lucidcairn::ota {
namespace {
bool Version(const std::string& s, std::array<uint32_t, 3>& parts) {
    size_t i = (!s.empty() && s[0] == 'v') ? 1 : 0;
    for (size_t p = 0; p < parts.size(); ++p) {
        const size_t begin = i;
        uint32_t value = 0;
        while (i < s.size() && s[i] >= '0' && s[i] <= '9') {
            const uint32_t digit = s[i++] - '0';
            if (value > (std::numeric_limits<uint32_t>::max() - digit) / 10) return false;
            value = value * 10 + digit;
        }
        if (i == begin || (i - begin > 1 && s[begin] == '0')) return false;
        parts[p] = value;
        if (p < 2 && (i == s.size() || s[i++] != '.')) return false;
    }
    return i == s.size();
}
bool StringField(const cJSON* root, const char* name, std::string& out, size_t limit) {
    const auto* value = cJSON_GetObjectItemCaseSensitive(root, name);
    if (!cJSON_IsString(value) || !value->valuestring) return false;
    out = value->valuestring;
    return !out.empty() && out.size() <= limit;
}
bool Integer(const cJSON* root, const char* name, int minimum, int maximum, int& out) {
    const auto* value = cJSON_GetObjectItemCaseSensitive(root, name);
    if (!cJSON_IsNumber(value) || !std::isfinite(value->valuedouble) ||
        value->valuedouble < minimum || value->valuedouble > maximum ||
        std::floor(value->valuedouble) != value->valuedouble) return false;
    out = static_cast<int>(value->valuedouble);
    return true;
}
}  // namespace

std::string HttpsOrigin(const std::string& url) {
    if (url.size() > 2048 || url.compare(0, 8, "https://") != 0) return {};
    for (unsigned char c : url) {
        if (c <= 32 || c >= 127 || c == '\\' || c == '#') return {};
    }
    const size_t end = url.find_first_of("/?", 8);
    std::string host = url.substr(8, end == std::string::npos ? end : end - 8);
    if (host.empty()) return {};
    // DNS/IPv4 and optional decimal port only. Reject userinfo, ambiguous escaping,
    // IPv6 literals and all non-ASCII hostnames rather than guessing an origin.
    const size_t colon = host.find(':');
    std::string name = host.substr(0, colon);
    if (name.empty() || name.front() == '.' || name.back() == '.' ||
        name.front() == '-' || name.back() == '-' || name.find("..") != std::string::npos) return {};
    for (char& c : name) {
        if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
        if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '.' || c == '-')) return {};
    }
    unsigned port = 443;
    if (colon != std::string::npos) {
        const auto text = host.substr(colon + 1);
        if (text.empty() || text.size() > 5) return {};
        port = 0;
        for (char c : text) {
            if (c < '0' || c > '9') return {};
            port = port * 10 + (c - '0');
        }
        if (port == 0 || port > 65535) return {};
    }
    return "https://" + name + (port == 443 ? "" : ":" + std::to_string(port));
}

bool ValidToken(const std::string& token) {
    if (token.empty() || token.size() > 512) return false;
    for (char c : token) {
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~')) return false;
    }
    return true;
}

bool ParseManifest(const std::string& json, Manifest& out) {
    if (json.empty() || json.size() > kMaxManifestBytes || json.find('\0') != std::string::npos ||
        json.find("\\u0000") != std::string::npos) return false;
    // cJSON is recursive. Bound nesting before parsing even ignored fields, so
    // a small hostile document cannot exhaust the device worker's stack.
    unsigned depth = 0;
    bool quoted = false, escaped = false;
    for (char c : json) {
        if (quoted) {
            if (escaped) escaped = false;
            else if (c == '\\') escaped = true;
            else if (c == '"') quoted = false;
        } else if (c == '"') quoted = true;
        else if (c == '{' || c == '[') { if (++depth > 4) return false; }
        else if (c == '}' || c == ']') { if (!depth) return false; --depth; }
    }
    const char* end = nullptr;
    std::unique_ptr<cJSON, decltype(&cJSON_Delete)> root(
        cJSON_ParseWithLengthOpts(json.c_str(), json.size() + 1, &end, true), cJSON_Delete);
    if (!root || !cJSON_IsObject(root.get())) return false;
    for (const auto* a = root->child; a; a = a->next) {
        for (const auto* b = a->next; b; b = b->next) {
            if (std::strcmp(a->string, b->string) == 0) return false;
        }
    }
    Manifest next;
    int schema = 0, size = 0;
    std::string board, layout, project;
    if (!Integer(root.get(), "schema_version", 1, 1, schema) ||
        !StringField(root.get(), "board", board, 32) || board != "note4c" ||
        !StringField(root.get(), "layout", layout, 32) || layout != kLayout ||
        !StringField(root.get(), "project", project, 32) || project != "xiaozhi" ||
        !StringField(root.get(), "version", next.version, 31) ||
        !StringField(root.get(), "download_url", next.download_url, 2048) ||
        HttpsOrigin(next.download_url).empty() ||
        !StringField(root.get(), "sha256", next.sha256, 64) || next.sha256.size() != 64 ||
        !Integer(root.get(), "size", 1, kSlotBytes, size)) return false;
    std::array<uint32_t, 3> version;
    if (!Version(next.version, version)) return false;
    for (char& c : next.sha256) {
        if (c >= 'A' && c <= 'F') c += 'a' - 'A';
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return false;
    }
    if (cJSON_HasObjectItem(root.get(), "minimum_battery_percent") &&
        !Integer(root.get(), "minimum_battery_percent", 30, 100, next.minimum_battery_percent)) return false;
    next.size = static_cast<size_t>(size);
    out = std::move(next);
    return true;
}

bool NewerVersion(const std::string& candidate, const std::string& current) {
    std::array<uint32_t, 3> a, b;
    return Version(candidate, a) && Version(current, b) && a > b;
}
bool PowerAllowsUpdate(bool known, int percent, bool external, int minimum) {
    return known && percent >= 0 && percent <= 100 && minimum >= 30 && minimum <= 100 &&
           (external || percent >= minimum);
}
bool LocalHealthPasses(bool power, bool nvs, bool heap, bool display, bool application) {
    return power && nvs && heap && display && application;
}

InstallResult Install(const Manifest& manifest, InstallIo& io) {
    // Close/abort on every exit, including failures after begin or image validation.
    struct Cleanup { InstallIo& io; ~Cleanup() { io.Abort(); } } cleanup{io};
    if (!manifest.size || manifest.size > kSlotBytes || manifest.sha256.size() != 64 ||
        HttpsOrigin(manifest.download_url).empty()) return InstallResult::Integrity;
    if (!io.Open(manifest)) return InstallResult::Transport;
    if (!io.Begin(manifest.size)) return InstallResult::Flash;
    uint8_t bytes[4096];
    size_t total = 0;
    for (;;) {
        const int read = io.Read(bytes, sizeof(bytes));
        if (read < 0) return InstallResult::Transport;
        if (read == 0) break;
        const size_t count = static_cast<size_t>(read);
        if (count > sizeof(bytes) || count > manifest.size - total) return InstallResult::Integrity;
        if (!io.Hash(bytes, count)) return InstallResult::Integrity;
        if (!io.Write(bytes, count)) return InstallResult::Flash;
        total += count;
    }
    if (total != manifest.size || io.Digest() != manifest.sha256) return InstallResult::Integrity;
    if (!io.Finish(manifest)) return InstallResult::Image;
    if (!io.SelectBoot()) return InstallResult::BootSelection;
    return InstallResult::Installed;
}
}  // namespace lucidcairn::ota
