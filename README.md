<p align="center">
  <img src="https://kirzo.dev/content/images/plugins/KzGameplay_banner.jpg" alt="KzGameplay Banner" width="512">
</p>

# KzGameplay

**KzGameplay** is a modular, data-driven gameplay framework for Unreal Engine 5. It provides a robust foundation for building games by offering highly decoupled systems for Input, Interaction, Inventory, Equipment, and the Gameplay Ability System (GAS).

## ⚠️ Dependencies & Installation

This plugin relies on the following custom modules to function:
* **[KzLib](https://github.com/kirzo/KzLib)**: Core math, spatial hashing, and utility libraries.
* **[ScriptableFramework](https://github.com/kirzo/ScriptableFramework)**: Scriptable logic evaluation for requirements and actions.

### Installation Steps

1. Navigate to your Unreal Engine project's root directory.
2. If it doesn't exist, create a folder named `Plugins`.
3. Clone or download this repository and its dependencies into the `Plugins` folder. Your directory structure should look like this:
   ```text
   YourProject/
   └── Plugins/
       ├── KzGameplay/
       │   ├── KzGameplay/
       │   ├── KzGameplayLimbic/
       │   └── KzGameplayUI/
       ├── KzLib/
       └── ScriptableFramework/
   ```
4. Right-click your project's `.uproject` file and select **Generate Visual Studio project files** (or your IDE's equivalent).
5. Open your IDE and compile the project, or simply launch the `.uproject` file to let Unreal Engine build the missing modules.
6. Ensure the plugins are enabled in your project via **Edit -> Plugins**.

### Optional Integrations

Integrations with other plugins live in this repository next to KzGameplay, each as its own plugin and disabled by default, so KzGameplay never depends on them. Enable the ones you need in **Edit -> Plugins**:

* **KzGameplay - Limbic Integration** (`KzGameplayLimbic`): requires **[Limbic](https://github.com/kirzo/Limbic)**. Adds the *Limbic Pose* item fragment, which holds a Limbic pose while the item is equipped.
* **KzGameplay - KzUI Integration** (`KzGameplayUI`): requires **[KzUI](https://github.com/kirzo/KzUI)**. Adds the *Input Tag* icon source and `{Input.Tag}` text tokens, which show the key of the action an input tag fires for the player right now, through the input profiles of their pawn, `KzInputKeysWidget`, a row with the keys an input tag needs (the ones its chords hold, then its own), each on its own or, in the Diamond style, the face buttons lit together in the face-button diamond, `KzInteractionPromptWidget`, a prompt for the interactable the player's interactor focuses, placed over it or at a fixed point of the screen, and `KzAttributeBarWidget`, a bar for a gameplay attribute of the player's pawn over its maximum, such as health. It also hides on a `KzHUD` the *Hidden UI Layers* of a player's cinematic mode.

## 🌟 Core Systems

### 🎮 Agnostic Input Routing & Modifiers
A completely tag-based input architecture that separates physical buttons from gameplay logic.

* **Tag-Based Routing:** Maps Enhanced Input actions to `FGameplayTag` identifiers, routing them directly into GAS or triggering custom delegates.
* **Dynamic Modifiers:** Stackable `UKzInputModifier` system to alter input vectors on the fly. Includes built-in modifiers for:
    * Camera-relative movement and twin-stick directional aiming.
    * Centripetal tether constraints (ideal for physics-based cooperative interactions).
    * Primitive sweeps to prevent wall-clipping before movement.
    * Target focus locking with cone constraints.

### 🔍 High-Performance Interaction
A highly optimized interaction system powered by a dual Spatial Hash Grid.

* **Spatial Hash Subsystem:** Tracks static and dynamic interactables with `O(1)` grid lookups, completely avoiding expensive physics overlaps.
* **Smart Target Evaluation:** Uses `UKzTargetScoringProfile` to evaluate the best candidate based on customizable scorers (e.g., Distance, Angle).
* **Scriptable Requirements:** Interactions can be gated behind node-based logic (e.g., "Requires Key Item" or "Has Minimum Strength").
* **Interaction Types:** Supports Instant, Ignored, and Continuous interactions out of the box.
* **Prompt Arguments:** A Prompt Text such as `Pick up {Item}` is filled by the interactable's handlers through `GetPromptArguments`; an item fills `{Item}` with its Display Name.
* **Nearby:** an interactable with a *Nearby Margin* tells an interactor it is near enough to notice before it can be used, through *On Begin Nearby* and *On End Nearby*: an icon over someone to talk to, shown at a distance, while the prompt waits for the shape itself.
* **Interactions that play something:** with *End With Interaction Action*, each interaction lasts as long as its own run of the *Interaction Action* and ends as Completed when it finishes; ending the interaction first stops the action. A conversation or a cinematic as the action and the *Cinematic Mode* task as the *While Action* hold the mode exactly as long as what plays.
* **Shared Keys:** Interaction can share a key with another action, such as dodge. The *Yield To Interaction* trigger on that action's mapping hands the key to interaction for presses that begin with something focused; neither action may consume input.

### 🎬 Cinematic Mode
What a cinematic or a conversation takes away from gameplay while it lasts, per player.

* **Push / Remove Cinematic Mode:** requests come by source and add up, so a conversation that hides the HUD and a sequence inside it that hides the player both hold. When the last one goes, everything is back as it was, and what was already hidden or off before stays so.
* **Settings:** *Hide Player* hides the pawn and every actor attached to it, such as its equipment, pauses their animation (so notify sounds such as breathing stop) and sounds, and turns their collision off. *Block Input* takes the gameplay input from the pawn and stops it; UI input keeps working. *Hidden UI Layers* names UI layers for a UI that hides them (the KzUI integration does, on a `KzHUD`).
* **Cinematic Mode task:** keeps the mode while it runs and takes it back when stopped, for an interactable's *While Action*: an interaction that plays a cinematic or a conversation holds it exactly as long.

### 🎒 Fragment-Based Inventory & Items
A data-driven item architecture using `UPrimaryDataAsset` definitions and modular fragments.

* **Item Definitions & Fragments:** Define base items (`UKzItemDefinition`) and extend their functionality using interchangeable fragments like `Storable`, `Equippable`, `InitialStats`, and `MeleeWeapon`.
* **Instanced Data:** `FKzItemInstance` bridges static data with runtime state (quantity, dynamic stats, spawned actors).
* **Networked Inventory:** `UKzInventoryComponent` handles stack sizes, capacity, and replication with FastArray serialization for item stats.

### ⚔️ Layout-Driven Equipment
A versatile equipment manager that handles visual representation and ability granting.

* **Equipment Layouts:** `UKzEquipmentLayout` defines available character slots (e.g., MainHand, Head) and supports complex inheritance and slot aliasing.
* **Visual Spawning:** Can spawn items as simple Static/Skeletal Meshes or fully functional Actors based on their `Equippable` fragment.
* **GAS Integration:** Automatically grants passive `FGameplayTag` effects and abilities to the owner when an item is equipped or placed in the inventory.

### ⚡ Gameplay Ability System (GAS) Extensions
Enhances the native Unreal Engine GAS implementation with quality-of-life features.

* **Custom ASC:** `UKzAbilitySystemComponent` automatically routes tag-based input and supports dynamic ability cancellation via tags or events.
* **Custom Ability Tasks:**
    * `AbilityTask_MoveToLocationUsingInput`: Navigates to a location using simulated input for natural, collision-aware pathing.
    * `AbilityTask_WaitTraceCollision`: Performs continuous volumetric sweeps per frame, caching ignored actors to prevent multi-hits in melee combos.
    * `AbilityTask_MatchRotation`: Smoothly aligns the character's rotation to a target over time.
* **Animation Notifies:** Send Gameplay Events directly from animation sequences or states to precisely time ability logic.
