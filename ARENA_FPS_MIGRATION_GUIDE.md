# Arena FPS Migration Guide

This guide records the current migration state and the recommended next steps for continuing work in the new project/repository.

## Current Repositories

Old repository:

```text
throusea/ue5-fps-cpp
```

New repository:

```text
throusea/arena-fps
```

The old Unreal project name is `FirstPersonCPP`. The new project should use a cleaner project/module name such as:

```text
ArenaFPS
```

or, if you want the AI theme to be explicit:

```text
AIArenaFPS
```

## GitHub Issues Migration Status

Issues have already been transferred from `throusea/ue5-fps-cpp` to `throusea/arena-fps`.

Transferred:

```text
Epic issues:    9
Feature issues: 35
Total:          44
```

New issue numbering in `throusea/arena-fps`:

```text
#1  - #9   Epic issues
#10 - #20  MVP feature issues
#21 - #33  Alpha feature issues
#34 - #44  Polish feature issues
```

Milestone distribution in the new repository:

```text
MVP           11
Alpha         13
Polish        11
No milestone   9  // Epic issues
```

The old repository currently has:

```text
0 open issues
```

## Recommended New Git Setup

Use only `main` as the default branch. Avoid recreating the old `master` / `main` split.

Suggested branch naming:

```text
feature/3-core-project-architecture
feature/4-network-listen-server
feature/5-player-movement-camera
feature/8-combat-health-death
feature/24-ai-statetree
```

Recommended workflow:

```text
1. Create one branch per GitHub Issue.
2. Keep each branch focused.
3. Open a PR back to main.
4. Merge.
5. Delete the feature branch.
```

## What To Migrate From The Old Project

Do not blindly copy the entire old `FirstPersonCPP` project. Treat it as a reference/source depot and move only the pieces you still want.

High-value C++ files to inspect and selectively port:

```text
Source/FirstPersonCPP/FirstPersonCPPCharacter.*
Source/FirstPersonCPP/FirstPersonCPPPlayerController.*
Source/FirstPersonCPP/FirstPersonCPPGameMode.*
Source/FirstPersonCPP/NetPlayerStateBase.*
Source/FirstPersonCPP/AS_Character.*
Source/FirstPersonCPP/AbilityLab/
Source/FirstPersonCPP/Variant_Shooter/
```

High-value content/assets to inspect and selectively port:

```text
Content/0_/
Content/0_/Maps/Lvl_NetFirstPerson.umap
Content/__ExternalActors__/0_/Maps/Lvl_NetFirstPerson/
Content/__ExternalObjects__/0_/Maps/Lvl_NetFirstPerson/
Content/Input/
Content/FirstPerson/
Content/Variant_Shooter/
```

Important: if you migrate a World Partition / external actor map, migrate the `.umap`, `__ExternalActors__`, and `__ExternalObjects__` folders together. Missing external actor files can make the map incomplete.

## Old Project Stash Warning

The old project still had a stash at the time of inspection:

```text
stash@{0}: On master: Auto stash before rebase of "refs/heads/master"
```

That stash contained at least:

```text
Content/0_/BP_NetPlayerController.uasset
Content/0_/Maps/Lvl_NetFirstPerson.umap
Content/__ExternalActors__/0_/Maps/Lvl_NetFirstPerson/...
Content/__ExternalObjects__/0_/Maps/Lvl_NetFirstPerson/...
UE5 Multiplayer FPS Demo GitHub Kanban.md
```

The map files were committed on the old `3-featurecore-setup-ue5-c-project-archit` branch, but these two files were not present in the working tree at the time:

```text
Content/0_/BP_NetPlayerController.uasset
UE5 Multiplayer FPS Demo GitHub Kanban.md
```

Before deleting the stash, recover anything you still need:

```powershell
git restore --source="stash@{0}" -- "Content/0_/BP_NetPlayerController.uasset" "UE5 Multiplayer FPS Demo GitHub Kanban.md"
```

Then either commit useful files or discard them. Only drop the stash after confirming it contains nothing important:

```powershell
git stash drop "stash@{0}"
```

## Naming Migration Suggestions

Avoid keeping template-style names in the new project.

Suggested class/module naming:

```text
FirstPersonCPP                 -> ArenaFPS
AFirstPersonCPPCharacter       -> AArenaFPSCharacter
AFirstPersonCPPPlayerController -> AArenaFPSPlayerController
AFirstPersonCPPGameMode        -> AArenaFPSGameMode
ANetPlayerStateBase            -> AArenaPlayerState
UAS_Character                  -> UArenaCharacterAttributeSet
ULabAbilitySystemComponent     -> UArenaAbilitySystemComponent
ULabHealthAttributeSet         -> UArenaHealthAttributeSet
```

Suggested folders:

```text
Source/ArenaFPS/
Source/ArenaFPS/AbilitySystem/
Source/ArenaFPS/Combat/
Source/ArenaFPS/Player/
Source/ArenaFPS/AI/
Source/ArenaFPS/GameFlow/
Source/ArenaFPS/UI/
```

## GAS Direction

Pick one AbilitySystem ownership model early:

```text
Option A: ASC on Character
```

Good for simple pawn-owned health/combat. Easier for the course project.

```text
Option B: ASC on PlayerState
```

Better for persistent player state across respawn, but requires cleaner PlayerState setup and client initialization.

For this project, unless respawn persistence becomes important, prefer:

```text
ASC on Character
```

Then keep all code and Blueprint references consistent with that decision.

## Immediate Next Steps In New Project

1. Create or open the new UE C++ project.
2. Confirm the module name and API macro are clean, e.g. `ARENAFPS_API`.
3. Set up git remote to `throusea/arena-fps`.
4. Create the first branch:

```powershell
git switch -c feature/3-core-project-architecture
```

5. Port only core framework classes first:

```text
GameMode
GameState
PlayerState
PlayerController
Character
```

6. Build after each small migration chunk.
7. Commit early and often.

Suggested first commit sequence:

```text
1. Add clean ArenaFPS C++ framework classes
2. Add multiplayer map assets
3. Add GAS health attribute set
4. Add basic player controller input setup
```

## GitHub Project Follow-Up

The issues were transferred to the new repo, but GitHub Projects may need re-linking manually or with GitHub CLI.

Check projects:

```powershell
& "C:\Program Files\GitHub CLI\gh.exe" project list --owner throusea --format json
```

If you create a new Project for `arena-fps`, add issues to it after migration:

```powershell
& "C:\Program Files\GitHub CLI\gh.exe" project item-add <project-number> --owner throusea --url https://github.com/throusea/arena-fps/issues/<issue-number>
```

## Do Not Bring These Problems Forward

Avoid carrying over:

```text
master/main branch split
Auto stash commits
Revert/Reapply repair commits
old FirstPersonCPP names
ambiguous ASC ownership
uncommitted UE map external actor files
```

The new project is an opportunity to make the history and names boring, clean, and easy to explain.
