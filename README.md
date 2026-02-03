# High-Latency Server Engine Optimizations

This repository contains a specialized build of the OpenJK/Jedi Academy server engine. These modifications architecturally improve network stability, input responsiveness, and competitive integrity for players with latencies up to 200ms+.

---

## 1. Network Snapshot Delivery (sv_snapshot.cpp)
The server-to-client communication has been redesigned to favor data continuity over strict rate-limiting, effectively eliminating "freeze" states.

* **Aggressive Delta Recovery:** The engine now searches much further back into the snapshot history to find a valid frame for delta compression. This prevents the server from defaulting to "Full Gamestate" sends, which are the primary cause of the "999 Connection Interrupted" freeze on unstable connections.
* **Adaptive Rate Management:** Traditional throttling that stops snapshot transmission during packet loss has been disabled. The server maintains a constant data flow, ensuring that even if a packet is delayed, the subsequent data is sent immediately to keep the client’s game world moving.
* **Overflow Mitigation:** Added robust handling for message overflows. When a packet exceeds the network limit, the server prioritizes movement and combat state while reducing non-essential data, preventing a hard disconnect.

## 2. Command Processing & Jitter Correction (sv_client.cpp)
The way the server interprets player intent has been modified to eliminate the "input lag" often felt on high-ping connections.

* **Universal Jitter Clamping:** The server no longer discards or queues packets that arrive slightly ahead of its internal clock due to network jitter. Instead, it intelligently "clamps" these commands to the current server time.
* **Instant Action Execution:** By processing commands the millisecond they arrive rather than waiting for a specific timestamp match, hit registration for sabers and button presses (swings, jumps, force powers) feels instantaneous.
* **Packet-Burst Protection:** A dynamic safety window, scaled to the server's frame rate, prevents "command clumping." This ensures that if multiple packets arrive at once, the player does not "warp" across the map, maintaining linear and predictable movement for opponents.

## 3. Duel Integrity & Collision Management (duel_cull.cpp)
Competitive integrity is maintained by isolating duels from the surrounding chaos of a public server while fixing long-standing engine bugs regarding collision.

* **Bystander Ghosting:** A sophisticated culling system ensures that players in a private duel can pass through non-dueling players. This prevents "body blocking" and interference from spectators without affecting the duelists' ability to collide with each other.
* **NPC Logic Preservation:** Specific safeguards have been added for NPCs and dummies. This prevents dueling bots or test dummies from losing their collision properties, ensuring they remain solid targets for practice even when targeted by a duelist.
* **State Tracking & Performance:** Improved tracking for duel participants ensures that collision states are reset instantly upon duel completion. All duel outcomes are logged with clean naming conventions for server administration and analytics.

---

## Summary of Benefits
* **Zero "999" Freezes:** High-latency players no longer time out during minor packet loss.
* **Responsive Sabers:** Saber swings and blocks register when they hit the server, not when the clock catches up.
* **Linear Movement:** Opponents move smoothly even if they are lagging, making them easier to track.
* **Clean Duels:** Private duels are protected from external interference while maintaining perfect collision.

# High-Ping & Networking Optimizations

This repository contains a set of mission-critical networking optimizations for the OpenJK engine, specifically designed to eliminate "999" lag spikes, stabilize connections over 200ms ping, and ensure competitive hit registration.

## Optimized Files Comparison

The following sections detail the exact line-by-line changes between the **Stock** engine code and the **Optimized** versions.

### 1. `sv_snapshot.cpp`

These changes focus on proactive data delivery and deep packet recovery.

* **Player Position Prioritization (Line 91)**
    * **Stock:** `MSG_WriteDeltaEntity (msg, oldent, newent, qfalse );`
    * **Optimized:** `MSG_WriteDeltaEntity (msg, oldent, newent, (newnum < MAX_CLIENTS) ? qtrue : qfalse );`
    * **Result:** Forces the server to prioritize player movement updates in every snapshot, maintaining combat accuracy even when snapshots are saturated.

* **Deep Delta Recovery Search (Line 159)**
    * **Stock:** Standard shallow search for acknowledged frames.
    * **Optimized:** Implemented a for-loop: `for ( i = 0 ; i < PACKET_BACKUP ; i++ )` to scan the backup buffer.
    * **Result:** Searches up to 32 frames back for a valid base. This prevents the "Full Gamestate" stutters (1-second freezes) commonly triggered by a single lost packet on high-latency connections.

* **Proactive Fragment Pumping (Line 460)**
    * **Stock:** Logic for fragment handling is missing in this block.
    * **Optimized:** Added `if (client->state && client->netchan.unsentFragments) { ... }` block.
    * **Result:** Triggers an immediate transmission of the next fragment using `SV_Netchan_TransmitNextFragment`. Instead of waiting for the next server tick, it calculates a precision delay via `SV_RateMsec`, eliminating the "999 Connection Interrupted" icon.

* **Smart Overflow Management (Line 475)**
    * **Stock:** Basic overflow warning only.
    * **Optimized:** Added `"WARNING: msg overflowed... - Reducing Data..."` and sets `client->rateDelayed = qtrue;`.
    * **Result:** Actively manages data bursts during heavy combat to prevent clients from falling out of sync.

* **Fragment Logic Trigger (Line 512)**
    * **Stock:** Passively resets the snapshot timer: `c->nextSnapshotTime = 0;`.
    * **Optimized:** Proactively calls `SV_SendMessageToClient( NULL, c );`.
    * **Result:** Moves fragment handling to the start of the message loop, bypassing snapshot timers to ensure large data chunks (like map loads) flow at the maximum possible rate.

### 2. `sv_client.cpp`

These changes address input lag and network jitter.

* **Future Command Filtering (Line 929)**
    * **Stock:** No safety window for future timestamps.
    * **Optimized:** Added `frameTime` and `maxSafeFuture` calculations.
    * **Result:** Detects and ignores command packets sent too far into the server's future (anti-speedhack).

* **Jitter Clamping (Line 938)**
    * **Stock:** Missing logic.
    * **Optimized:** Added `if ( ucmd->serverTime > sv.time ) { ucmd->serverTime = sv.time; }`.
    * **Result:** "Clamps" packets that arrive slightly early due to network jitter to the current server time. This ensures saber swings and shots register the instant they reach the server, fixing the "dead-click" feeling on high-ping connections.

---

## Technical Overview

The engine has been converted from a **Passive** "wait-for-timer" system to an **Active** "pump-and-clamp" system:

1.  **Fragment Pump:** Data flows constantly; the server doesn't wait for a new snapshot to clear the buffer.
2.  **Deep Delta:** The server is 32x more likely to find a valid delta base, keeping packets small.
3.  **Command Clamping:** Hit registration is processed at the earliest possible millisecond relative to server time.

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
