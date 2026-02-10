# Vortex 7 Days to Die — Mod Updater

Automated mod updater for the **Vortex** mod pack for [7 Days to Die](https://store.steampowered.com/app/251570/7_Days_to_Die/). Detects the game installation via Steam, checks for the latest release on GitHub, downloads and installs mods — all from a single executable.

---

## Features

- **Automatic game detection** — reads the Windows registry and parses Steam library folders (`libraryfolders.vdf`) to locate the 7 Days to Die installation across all Steam library paths.
- **Version tracking** — stores the installed mod version in `Mods/.vortex_version` and skips the update if already on the latest release.
- **GitHub Releases integration** — queries the GitHub API for the latest release of [Building_Mods_Vortex](https://github.com/PavelDeep/Building_Mods_Vortex) and downloads the attached ZIP asset.
- **Download progress bar** — displays a real-time progress bar with percentage and MB counters in the console.
- **Mod backup with rotation** — before updating, prompts the user to create a backup of the current `Mods` folder. Old backups are automatically rotated, keeping only the 3 most recent (configurable via `MAX_BACKUPS`).
- **Protected folders** — the folders `0_TFP_Harmony` and `Jahy7Days` are never deleted during cleanup.
- **Colored console output** — all messages use color-coded output (Russian locale) for clear status indication.

---

## Requirements

| Requirement       | Version                     |
|-------------------|-----------------------------|
| OS                | Windows 10 / 11 (x64)      |
| Compiler          | MSVC (Visual Studio 2022+)  |
| C++ Standard      | C++20                       |
| Steam             | Installed with 7 Days to Die |

No third-party libraries are required. The project uses only the Windows SDK (`WinHTTP`, `Shell32`, `Ole32`, `Advapi32`).

---

## Project Structure

```
Updater/
??? Console/
?   ??? Console.h          # Console color output, progress bar, Y/N prompt
?   ??? Console.cpp
??? Http/
?   ??? Http.h             # HTTPS downloads (text and binary with progress)
?   ??? Http.cpp
??? Parser/
?   ??? Parser.h           # Update info file parser
?   ??? Parser.cpp
??? Steam/
?   ??? Steam.h            # Steam game path detection
?   ??? Steam.cpp
??? ModInstaller/
?   ??? ModInstaller.h     # Mod install pipeline, backup, version check
?   ??? ModInstaller.cpp
??? Updater.cpp            # Entry point
```

---

## Build

### Visual Studio

1. Open `Updater.vcxproj` in Visual Studio 2022 or later.
2. Select the **Release | x64** configuration.
3. Build the solution (`Ctrl+Shift+B`).

### Command Line (Developer Command Prompt)

```bat
msbuild Updater\Updater.vcxproj /p:Configuration=Release /p:Platform=x64
```

The output binary will be located in the `x64/Release/` directory.

---

## Usage

Run the executable. The updater will perform the following steps automatically:

```
============================================
   Vortex 7 Days to Die — ???????? ??????????
============================================

[INFO] ????? ????????????? ???? 7 Days to Die...
[OK] ???? ???????: D:\SteamLibrary\steamapps\common\7 Days to Die

[INFO] ???????? ?????? ?????????? ? GitHub...
[OK] ?????? ??????? ????????.

?? ?????????? ?? ?????????? ??????????????????????????
  Version: beta-0.07
  Date: 10-02-2026
  Telegram: https://t.me/Mods_Vortex
  Steam: https://steamcommunity.com/chat/invite/LbKpd9T6

[INFO] ?????? ?????????? ?????? ? GitHub...
[INFO] ??????? ??????: old_tag
[INFO] ????? ??????:    beta-0.07
[OK] ?????? ???????.
??????? ????? ??????? ?????? (Y/N): Y
[INFO] ???????? ??????...
[OK] ????? ?????? (????: 3).
[INFO] ?????????? ??????...
[????????????????????????????????????????] 100% (46 / 46 MB)
[OK] ????? ???????? (46 MB).
[INFO] ??????? ????? Mods...
[OK] ????? ??????? (?????????: 0_TFP_Harmony, Jahy7Days).
[INFO] ?????????? ?????...
[OK] ???? ??????? ???????????!

[??????] ?????????? ?????????.

??????? Enter ??? ????????...
```

If the installed version already matches the latest release, the updater will skip the download:

```
[OK] ??? ??????????? ????????? ??????: beta-0.07
```

---

## Configuration

| Constant            | File                    | Default  | Description                                    |
|---------------------|-------------------------|----------|------------------------------------------------|
| `SEVEN_DAYS_APP_ID` | `Steam/Steam.h`        | `251570` | Steam Application ID for 7 Days to Die         |
| `MAX_BACKUPS`       | `ModInstaller/ModInstaller.h` | `3` | Maximum number of backup copies to retain      |

Protected mod folders (excluded from cleanup) are defined in `ModInstaller.cpp`:

```cpp
std::vector<std::wstring> protectedFolders = { L"0_TFP_Harmony", L"Jahy7Days", L".vortex_version" };
```

---

## How It Works

1. **Steam detection** — reads `HKLM\SOFTWARE\WOW6432Node\Valve\Steam\InstallPath` and `HKCU\SOFTWARE\Valve\Steam\SteamPath` from the registry. Parses `libraryfolders.vdf` to discover all Steam library paths, then searches for `appmanifest_251570.acf` to find the game directory.

2. **Update info** — downloads the `Update_Vortex` file from the `Building_Mods_Vortex` repository on GitHub to display version, date, and community links.

3. **Version check** — queries the GitHub Releases API (`/repos/.../releases/latest`) and compares the `tag_name` against the locally stored version in `Mods/.vortex_version`.

4. **Backup** — if the user accepts, copies the entire `Mods` folder to `<game>/Mods_Backups/Mods_Backup_YYYYMMDD_HHMMSS/`. Oldest backups beyond `MAX_BACKUPS` are deleted automatically.

5. **Download** — downloads the release ZIP asset over HTTPS with automatic redirect handling. Progress is displayed in real time.

6. **Install** — cleans the `Mods` folder (preserving protected folders), extracts the ZIP via the Windows Shell COM API, and writes the new version to `.vortex_version`.

---

## Links

- **Mod repository:** [PavelDeep/Building_Mods_Vortex](https://github.com/PavelDeep/Building_Mods_Vortex)
- **Telegram:** [https://t.me/Mods_Vortex](https://t.me/Mods_Vortex)
- **Steam group:** [https://steamcommunity.com/chat/invite/LbKpd9T6](https://steamcommunity.com/chat/invite/LbKpd9T6)

---

## License

This project is provided as-is for personal use. See the repository for details.
