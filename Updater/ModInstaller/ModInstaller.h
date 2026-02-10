#pragma once

#include <string>
#include <vector>

constexpr int MAX_BACKUPS = 3;

// Parse the GitHub releases API JSON and return the first asset download URL.
std::wstring ParseReleaseDownloadUrl(const std::string& json);

// Parse "tag_name" from the GitHub release JSON (used as version).
std::string ParseReleaseVersion(const std::string& json);

// Read the installed version from the version file in Mods folder.
std::string ReadInstalledVersion(const std::wstring& gamePath);

// Write the installed version to the version file in Mods folder.
void WriteInstalledVersion(const std::wstring& gamePath, const std::string& version);

// Clean the Mods folder: remove everything except protected folder names.
bool CleanModsFolder(const std::wstring& modsPath,
                     const std::vector<std::wstring>& protect);

// Backup the Mods folder as a ZIP to the game's Backups directory.
bool BackupModsFolder(const std::wstring& gamePath);

// Rotate old backups, keeping only MAX_BACKUPS most recent.
void RotateBackups(const std::wstring& gamePath);

// Extract a ZIP file into the destination directory.
bool ExtractZipToFolder(const std::wstring& zipPath, const std::wstring& destPath);

// Full pipeline: fetch latest release, download, clean mods, extract.
bool InstallLatestMods(const std::wstring& gamePath);
