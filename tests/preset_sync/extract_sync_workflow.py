"""Compile the production sync decisions against deterministic test dependencies."""
import sys
from pathlib import Path

root, output = map(Path, sys.argv[1:])
functions = {
    "src/libslic3r/Preset.cpp": [
        "int PresetCollection::get_user_presets(",
        "void PresetCollection::set_sync_info_and_save(",
    ],
    "src/slic3r/GUI/CommunicateWithCXCloud.cpp": [
        "bool CXCloudDataCenter::isNetworkErrorRetryTimeout(",
        "void CXCloudDataCenter::resetNetworkErrorRetryTime(",
        "void CXCloudDataCenter::setNetworkError(",
        "static long long preset_sync_retry_after(",
        "static bool defer_limited_preset_sync(",
        "int CommunicateWithCXCloud::preUpdateProfile_create(",
        "int CommunicateWithCXCloud::preUpdateProfile_update(",
        "int CommunicateWithCXCloud::deleteProfile(",
    ],
    "src/slic3r/GUI/SyncUserPresets.cpp": [
        "bool SyncUserPresets::canSyncToCXCloud(",
        "static bool preset_sync_snapshot_current(",
        "int SyncUserPresets::doCheckNeedSyncPrinterToCXCloud(",
        "int SyncUserPresets::doCheckNeedSyncFilamentToCXCloud(",
        "int SyncUserPresets::doCheckNeedSyncProcessToCXCloud(",
        "int SyncUserPresets::doCheckNeedDeleteFromCXCloud(",
        "int SyncUserPresets::doCheckNeedSyncToCXCloud(",
        "int SyncUserPresets::doCheckNeedSyncConfigToCXCloud(",
    ],
}
result = []
for filename, signatures in functions.items():
    source = (root / filename).read_text(encoding="utf-8-sig")
    for signature in signatures:
        start = source.index(signature)
        end = source.index("{", start) + 1
        depth = 1
        while depth:
            depth += (source[end] == "{") - (source[end] == "}")
            end += 1
        result.append("// " + filename + "\n" + source[start:end])
output.write_text("\n\n".join(result), encoding="utf-8")
