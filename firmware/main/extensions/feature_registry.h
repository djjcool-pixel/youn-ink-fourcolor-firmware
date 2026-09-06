// SPDX-License-Identifier: MIT
#pragma once
#include <vector>
namespace rawdraw { struct SettingsItemDef; }

namespace extensions {
// Fixed compile-time services; pages continue to use upstream PageRenderer.
void BeforeBoot();
void LocalReady();
void AddSettings(std::vector<rawdraw::SettingsItemDef>& items);
bool Busy();
bool PendingVerify();
void RejectPendingImage();
}
