# Network Performance & Safety Optimizations (Absolute Build)

This document outlines the specific differences between the optimized "Absolute" networking stack and the stock OpenJK/MB2 engine. These changes prioritize high-throughput stability, CPU hitch elimination, and intelligent entity culling for high-population Movie Battles II servers (up to 32 players).

## 🟢 Why the Change? (Modernizing the 1999 Stack)
The stock engine was built for an era of low bandwidth and small player counts. Modern MB2 environments utilize a massive **49,152 (48KB)** message buffer. In high-intensity 32-player matches, the original engine's "Adaptive Huffman" compression and "Lazy Loading" filesystem create severe bottlenecks.

* **The Problem:** The engine stalls to re-calculate compression trees or search the disk for 700+ player models. This leads to "muddy" movement and 100ms+ "entry hitches" when players join.
* **The Solution:** This build transitions to **Static High-Burst Networking** and **Proactive RAM Caching**. It ensures the network pipe and filesystem are optimized to handle 32-player bursts instantly without blocking the main thread.

---

## 1. Static Huffman Compression (CPU Efficiency)
**Implementation:** `huffman_static.cpp` / `msg.cpp`

* **Elimination of Adaptive Hitches:** Replaces the stock `msgHuff` adaptive tree with pre-computed Static Huffman lookup tables.
* **Constant-Time Processing:** The server no longer spends CPU cycles "learning" frequency patterns or re-balancing trees during heavy combat.
* **Surgical Bypass:** The system bypasses `Huff_addRef` and `Huff_Init` calls, removing the primary cause of server-side micro-stutters during player join/spawn events.

## 2. Command Spam Gatekeeper (Security)
**Function:** `sv_client.cpp` (`SV_ExecuteClientCommand`)

* **Early-Exit Logic:** Implements a high-performance "Gatekeeper" that checks for command spam *before* the engine performs expensive string tokenization.
* **Flood Protection:** If flood protection is tripped, common chat/voice commands are discarded instantly. This prevents malicious users from lagging the server by spamming the `say` or `engage` commands.
* **Enhanced Packet Reliability:** Modified `SV_ClientCommand` to always return `qtrue`, ensuring that even if a text command is muted for spam, the player's movement data (UserMove) in the same packet is still processed.

## 3. Dedicated Server "Duel Isolation" (DuelCull)
**Implementation:** `duel_cull.cpp` / `sv_snapshot.cpp` / `sv_world.cpp`

* **Intelligent Culling:** Automatically hides players from those involved in a private duel. This significantly reduces the data footprint for dueling players.
* **Direct Pointer Passing:** Replaces slow internal table lookups with direct `playerState_t` pointer passing in `sv_world.cpp` to keep physics and snapshot generation loops fast.
* **NPC Persistence:** Ensures that `ET_NPC` (training dummies/bots) are never culled, maintaining visibility for all players.
* **Performance Gate:** Controlled via `sv_snapShotDuelCull`. If disabled, the logic exits in a single cycle to save CPU.

## 4. "Warm Cache" Snapshot Optimization
**Function:** `sv_snapshot.cpp` (`SV_EmitPacketEntities`)

* **Byte-Level Comparison:** Uses `std::equal` to perform a lightning-fast memory check between current and previous entity states.
* **Delta Efficiency:** If an entity hasn't changed at the byte level, the engine writes a single "no change" bit and skips field-by-field delta calculation, maximizing headroom within the 48KB buffer.

## 5. Engine-Side Logging
**Function:** `sv_main.cpp` (`SV_LogPrintf`)

* **Direct I/O:** Provides a bridge for the engine to write `DuelStart` and `DuelEnd` events directly to `games.log` or a custom log file (e.g., `duel-games.log`).
* **Timestamp Accuracy:** Syncs logs with the engine's internal time (e.g., `3:09`) for precise match review.
* **Reliability:** Uses `fputs` instead of `fprintf` for faster, unformatted string writing, ensuring logs are preserved even in high-stress scenarios.

## 6. Proactive Asset Indexing & Pre-Caching
**Function:** `sv_main.cpp` (`SV_IndexAllModels`) / `sv_world.cpp`

* **Instant Path Resolution:** Replaces the engine's expensive "Linear PK3 Scan" with a high-performance `std::unordered_map` lookup table.
* **Page Cache Warming (Hitch Elimination):** At startup, the engine proactively "touches" all 700+ `.glm` files. This pulls the model data into the **OS Page Cache (RAM)** before any players join. 
* **Zero-Hitch Player Entry:** Because model data is already "hot" in RAM, the engine initializes new players in microseconds, eliminating the 100-200ms "entry hitch" common in MB2.
* **Transform Caching:** Works in tandem with `G2API_CollisionDetectCache` in `sv_world.cpp` to ensure that once a model is located, its skeletal transforms are reused across all player traces in a single frame.

---

### Implementation Summary
* **Standard:** Compiled using **C++14** for cross-platform compatibility (Visual Studio & GCC).
* **OS Support:** Verified for **OpenJK/MB2** on **Ubuntu 24.04** and **Windows Server**.
* **Impact:** By combining Static Huffman compression with proactive RAM caching and intelligent culling, the Absolute Build ensures that the networking stack and filesystem are no longer bottlenecks in high-population environments.


# OpenJK

OpenJK is an effort by the JACoders group to maintain and improve the game engines on which the Jedi Academy (JA) and Jedi Outcast (JO) games run on, while maintaining *full backwards compatibility* with the existing games. *This project does not attempt to rebalance or otherwise modify core gameplay*.

Our aims are to:
* Improve the stability of the engine by fixing bugs and improving performance.
* Provide a clean base from which new JO and JA code modifications can be made.
* Make available this engine to more operating systems. To date, we have ports on Linux and macOS.

Currently, the most stable portion of this project is the Jedi Academy multiplayer code, with the single player code in a reasonable state.

Rough support for Jedi Outcast single player is also available, however this should be considered heavily work in progress. This is not currently actively worked on or tested. OpenJK does not have Jedi Outcast multiplayer support.

Please use discretion when making issue requests on GitHub. The [JKHub sub-forum](https://jkhub.org/forums/forum/49-openjk/) is a better place for support queries, discussions, and feature requests.

<a href="https://discord.gg/dPNCfeQ"><img src="https://img.shields.io/badge/discord-join-7289DA.svg?logo=discord&longCache=true&style=flat" /></a>
[![Forum](https://img.shields.io/badge/forum-JKHub.org%20OpenJK-brightgreen.svg)](https://jkhub.org/forums/forum/49-openjk/)

[![Coverity Scan Build Status](https://scan.coverity.com/projects/1153/badge.svg)](https://scan.coverity.com/projects/1153)

## License

OpenJK is licensed under GPLv2 as free software. You are free to use, modify and redistribute OpenJK following the terms in LICENSE.txt.


## For players

To install OpenJK, you will first need Jedi Academy installed. If you don't already own the game you can buy it from online stores such as [Steam](http://store.steampowered.com/app/6020/), [Amazon](http://www.amazon.com/Star-Wars-Jedi-Knight-Academy-Pc/dp/B0000A2MCN) or [GOG](https://www.gog.com/game/star_wars_jedi_knight_jedi_academy).

Installing and running OpenJK:

1. [Download the latest build](http://builds.openjk.org) for your operating system.
2. Extract the contents of the file into the Jedi Academy `GameData/` folder. For Steam users, this will be in `<Steam Folder>/steamapps/common/Jedi Academy/GameData`.
3. Run `openjk.x86.exe` (Windows), `openjk.i386` (Linux 32-bit), `openjk.x86_64` (Linux 64-bit) or the `OpenJK` app bundle (macOS), depending on your operating system.


**Linux Instructions**

If you do not have a windows partition and need to download the game base.

1. Download  and Install SteamCMD [SteamCMD](https://developer.valvesoftware.com/wiki/SteamCMD#Linux) .
2. Set the download path using steamCMD, force_install_dir <path> .
3. Using SteamCMD Set the platform to windows to download any windows game on steam. @sSteamCmdForcePlatformType "windows"
4. Using SteamCMD download the game,  app_update 6020.
5. [Download the latest build](http://builds.openjk.org) for your operating system.
6. Extract the contents of the file into the Jedi Academy `GameData/` folder. For Steam users, this will be in `<Steam Folder>/steamapps/common/Jedi Academy/GameData`.


**macOS Instructions**

If you have the Mac App Store Version of Jedi Academy, follow these steps to get OpenJK runnning under macOS:

1. Install [Homebrew](http://brew.sh/) if you don't have it.
2. Open the Terminal app, and enter the command `brew install sdl2`.
3. Extract the contents of the OpenJK DMG ([Download the latest build](http://builds.openjk.org)) into the game directory `/Applications/Star Wars Jedi Knight: Jedi Academy.app/Contents/`
4. Run `OpenJK.app` or `OpenJK SP.app` 
5. Savegames, Config Files and Log Files are stored in `/Users/<USER>/Library/Application Support/OpenJK/`


## For Developers


### Vulkan support

Support Initially started by porting to [Quake-III-Arena-Kenny-Edition](https://github.com/kennyalive/Quake-III-Arena-Kenny-Edition).<br />
After that, I found [vkQuake3](https://github.com/suijingfeng/vkQuake3/tree/master/code), hence the file structure.

Lastly, I stumbled across [Quake3e](https://github.com/ec-/Quake3e).<br />
Which is highly maintained, and is packed with many additions compared to the other repositories.

Therefore the vulkan renderer is now based on Quake3e. <br />A list of the additions can be found on [here](https://github.com/ec-/Quake3e#user-content-vulkan-renderer).

Many thanks to all the contributors that worked & are still working hard on improving the Q3 engine!

~Sunny

### Building OpenJK

* [Compilation guide](https://github.com/JACoders/OpenJK/wiki/Compilation-guide)
* [Debugging guide](https://github.com/JACoders/OpenJK/wiki/Debugging)


### Contributing to OpenJK

* [Fork](https://github.com/JACoders/OpenJK/fork) the project on GitHub
* Create a new branch and make your changes
* Send a [pull request](https://help.github.com/articles/creating-a-pull-request) to upstream (JACoders/OpenJK)


### Using OpenJK as a base for a new mod

* [Fork](https://github.com/JACoders/OpenJK/fork) the project on GitHub
* Change the GAMEVERSION define in codemp/game/g_local.h from "OpenJK" to your project name
* If you make a nice change, please consider back-porting to upstream via pull request as described above. This is so everyone benefits without having to reinvent the wheel for every project.


## Maintainers (in alphabetical order)

* Ensiform
* Razish
* Xycaleth


## Significant contributors (in alphabetical order)

* eezstreet
* exidl
* ImperatorPrime
* mrwonko
* redsaurus
* Scooper
* Sil
* smcv
