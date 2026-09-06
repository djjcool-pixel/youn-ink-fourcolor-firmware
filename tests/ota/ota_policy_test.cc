// SPDX-License-Identifier: MIT
#include "ota_policy.h"
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <vector>
using namespace lucidcairn::ota;
static int checks = 0;
#define CHECK(test) do { ++checks; if (!(test)) { std::cerr << "failed line " << __LINE__ << ": " #test << '\n'; std::exit(1); } } while (0)
const std::string valid = R"({"schema_version":1,"board":"note4c","layout":"note4c-16m-v2","project":"xiaozhi","version":"6.5.10","download_url":"https://firmware.example.org/app.bin?signature=example","sha256":"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa","size":12})";
std::string Replace(std::string text, const std::string& from, const std::string& to) {
    const auto p = text.find(from);
    CHECK(p != std::string::npos);
    text.replace(p, from.size(), to);
    return text;
}
struct Fake final : InstallIo {
    std::vector<std::string> calls;
    size_t available = 12, read = 0, written = 0;
    std::string fail, digest = std::string(64, 'a');
    bool selected = false;
    bool Op(const std::string& name) { calls.push_back(name); return fail != name; }
    bool Open(const Manifest&) override { return Op("open"); }
    bool Begin(size_t) override { return Op("begin"); }
    int Read(uint8_t* bytes, size_t) override {
        if (!Op("read")) return -1;
        if (read == available) return 0;
        const size_t count = std::min<size_t>(5, available - read);
        std::fill(bytes, bytes + count, 0xaa); read += count;
        return count;
    }
    bool Write(const uint8_t*, size_t size) override { written += size; return Op("write"); }
    bool Hash(const uint8_t*, size_t) override { return Op("hash"); }
    std::string Digest() override { Op("digest"); return digest; }
    bool Finish(const Manifest&) override { return Op("finish"); }
    bool SelectBoot() override { if (!Op("select")) return false; selected = true; return true; }
    void Abort() override { Op("abort"); }
};
int main() {
    Manifest manifest;
    CHECK(ParseManifest(valid, manifest));
    CHECK(manifest.size == 12 && manifest.minimum_battery_percent == 30);
    CHECK(ParseManifest(Replace(valid, "aaaaaaaa", "AAAAAAAA"), manifest));
    CHECK(manifest.sha256 == std::string(64, 'a'));
    for (const auto& pair : std::vector<std::pair<std::string, std::string>>{
        {"\"schema_version\":1", "\"schema_version\":2"}, {"note4c\"", "note4\""},
        {"note4c-16m-v2", "legacy-3mb"}, {"xiaozhi", "note4c-ota-terminal"},
        {"6.5.10", "6.5.10-dev"}, {"6.5.10", "06.5.10"}, {"6.5.10", "6.5.10 garbage"},
        {"6.5.10", "4294967296.5.10"}, {"https://", "http://"},
        {"firmware.example.org", "token@firmware.example.org"},
        {"aaaaaaaa", "aaaaaaag"}, {"\"size\":12", "\"size\":0"},
        {"\"size\":12", "\"size\":-1"}, {"\"size\":12", "\"size\":12.5"},
        {"\"size\":12", "\"size\":4128769"}, {"\"size\":12", "\"size\":1e100"},
        {"\"size\":12", "\"size\":\"12\""}, {"\"size\":12", "\"size\":12,\"size\":13"},
        {"\"size\":12", "\"size\":12,\"minimum_battery_percent\":29"},
        {"\"size\":12", "\"size\":12,\"minimum_battery_percent\":101"},
        {"\"size\":12", "\"size\":12,\"minimum_battery_percent\":30.5"},
        {"note4c\"", "note4c\\u0000evil\""}}) {
        CHECK(!ParseManifest(Replace(valid, pair.first, pair.second), manifest));
    }
    for (const auto& text : {std::string(), std::string("[]"), valid + "{}", valid + std::string(1, '\0'), std::string(8193, ' ')})
        CHECK(!ParseManifest(text, manifest));
    CHECK(ParseManifest(valid + " \n", manifest));
    CHECK(!ParseManifest(Replace(valid, "\"size\":12", "\"size\":12,\"ignored\":[[[[0]]]]"), manifest));
    CHECK(NewerVersion("v6.5.10", "6.5.9"));
    CHECK(!NewerVersion("6.5.9", "6.5.9"));
    CHECK(!NewerVersion("6.5.8", "6.5.9"));
    CHECK(!NewerVersion("7.0.0", "6.5.9-dev"));
    CHECK(!NewerVersion("7.0.0x", "6.5.9"));
    CHECK(HttpsOrigin("https://EXAMPLE.org:443/a") == "https://example.org");
    CHECK(HttpsOrigin("https://example.org:8443/a") == "https://example.org:8443");
    for (const auto* url : {"https://", "http://example.org", "https://evil@host/a", "https://host/a#fragment",
         "https://host\\evil/a", "https://host/a\r\nX:1", "https://host:0", "https://host:65536",
         "https://host:x", "https://host:", "https://host..name", "https://%65vil/a"}) CHECK(HttpsOrigin(url).empty());
    CHECK(ValidToken("device-A_123.jwt~x"));
    for (const auto& token : {std::string(), std::string("a\r\nX:evil"), std::string("a b"), std::string(513, 'a')}) CHECK(!ValidToken(token));
    CHECK(PowerAllowsUpdate(true, 30, false, 30));
    CHECK(PowerAllowsUpdate(true, 10, true, 30));
    CHECK(!PowerAllowsUpdate(false, 100, true, 30));
    CHECK(!PowerAllowsUpdate(true, 29, false, 30));
    CHECK(!PowerAllowsUpdate(true, -1, true, 30));
    CHECK(!PowerAllowsUpdate(true, 101, true, 30));
    for (unsigned bits = 0; bits < 32; ++bits)
        CHECK(LocalHealthPasses(bits&1, bits&2, bits&4, bits&8, bits&16) == (bits == 31));
    CHECK(ParseManifest(valid, manifest));
    Fake good;
    CHECK(Install(manifest, good) == InstallResult::Installed && good.selected && good.written == 12);
    CHECK(good.calls[good.calls.size()-3] == "finish" && good.calls[good.calls.size()-2] == "select");
    for (const auto* operation : {"open", "begin", "read", "hash", "write", "finish", "select"}) {
        Fake failed; failed.fail = operation;
        CHECK(Install(manifest, failed) != InstallResult::Installed);
        CHECK(!failed.selected && failed.calls.back() == "abort");
        if (failed.fail != "select") CHECK(std::find(failed.calls.begin(), failed.calls.end(), "select") == failed.calls.end());
    }
    for (size_t size : {0u, 5u, 11u, 13u, 100u}) {
        Fake truncated; truncated.available = size;
        CHECK(Install(manifest, truncated) == InstallResult::Integrity);
        CHECK(!truncated.selected && truncated.written <= manifest.size);
    }
    Fake wrong_hash; wrong_hash.digest = std::string(64, 'b');
    CHECK(Install(manifest, wrong_hash) == InstallResult::Integrity && !wrong_hash.selected);
    Fake empty_hash; empty_hash.digest.clear();
    CHECK(Install(manifest, empty_hash) == InstallResult::Integrity && !empty_hash.selected);
    Fake oversized; manifest.size = kSlotBytes + 1;
    CHECK(Install(manifest, oversized) == InstallResult::Integrity && oversized.calls.size() == 1);
    std::cout << checks << " OTA policy/transaction checks passed\n";
}
