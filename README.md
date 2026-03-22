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
       ├── KzLib/
       └── ScriptableFramework/
4. Right-click your project's `.uproject` file and select **Generate Visual Studio project files** (or your IDE's equivalent).
5. Open your IDE and compile the project, or simply launch the `.uproject` file to let Unreal Engine build the missing modules.
6. Ensure the plugins are enabled in your project via **Edit -> Plugins**.

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
