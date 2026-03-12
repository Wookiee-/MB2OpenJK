# Network Performance & Safety Optimizations (Absolute VPS Build)

This document outlines the specific differences between this optimized "Absolute" networking stack and the stock OpenJK/MB2 engine. These changes prioritize high-throughput stability, the elimination of disk-access hitches, and intelligent entity culling for high-population Movie Battles II servers.

## 🟢 Why the Change?
Standard engines utilize "Lazy Loading," searching the disk for models only when a player joins. With MB2's massive asset library (800+ models), this creates severe bottlenecks.

* **The Problem:** The server stalls to perform disk I/O when players join with un-cached skins. This leads to 100ms+ "entry hitches" that affect everyone on the server.
* **The Solution:** This build implements **Proactive RAM Caching** and **Soft Entity Culling**. It ensures the filesystem is "warm" before players join and that the network stream remains smooth for high-latency (200ms+) clients.

---

## 1. Proactive Asset Indexing & RAM Warming
**Function:** `sv_main.cpp` (`SV_IndexAllModels`)

* **Instant Path Resolution:** Replaces the engine's expensive "Linear PK3 Scan" with a high-performance `std::unordered_map` lookup table for all 800+ `.glm` files.
* **Page Cache Warming (Hitch Elimination):** At startup, the engine proactively "touches" model files via `FS_FOpenFileRead`. This pulls the model data into the **Linux OS Page Cache (RAM)** before any players join.
* **VPS Safety Circuit:** Includes a **100MB RAM budget** for pre-caching. This prevents the server from triggering an Out-Of-Memory (OOM) shutdown on 2GB VPS systems while ensuring the most popular models are always "hot" in memory.

### 2. Duel Isolation (Soft-Cull & Physics Bypass)
* **Visuals:** Players are ghosted via `EF_NODRAW` in the snapshot.
* **Physics:** Implemented in `sv_world.cpp` inside `SV_ClipMoveToEntities`.
* **Result:** Bystanders can physically pass through active duels. This prevents the "Invisible Wall" and "Rubber-banding" issues caused by server-side collision checks.

## 3. High-Burst Snapshot Efficiency
**Function:** `sv_snapshot.cpp` (`SV_BuildClientSnapshot`)

* **Optimized Loop:** The snapshot generation loop has been stripped of redundant `#ifdef DEDICATED` blocks to ensure consistent performance.
* **Event Silencing:** During Soft-Culling, `state->event` is cleared to prevent "ghost" sounds or sparks from duels leaking to players who shouldn't see or hear them, further reducing unnecessary network traffic.

## 4. Engine-Side Logging & Diagnostics
**Function:** `sv_main.cpp` (`SV_LogPrintf`)

* **Direct I/O Logging:** Provides a high-speed bridge to write `DuelStart` and `DuelEnd` events directly to the server logs.
* **Reliability:** Uses optimized buffer flushing to ensure logs are preserved in real-time, allowing external web-tools or Discord bots to track match results without delay.

---

## 🛠 Implementation Summary
* **Standard:** Compiled using **C++14** for modern Linux VPS environments.
* **Impact:** By combining proactive RAM warming with intelligent soft-culling, the Absolute Build ensures the networking stack and filesystem are no longer bottlenecks, even with over 800 player models installed.
* **Config:** Controlled server-side; no client-side changes or CVARs are required for players to benefit from the increased smoothness.


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
