# QMovement Plugin

**GoldSrc / QuakeWorld Movement in Unreal Engine 5**

This plugin is a faithful C++ port of the original GoldSrc (`pmove.c`) movement system. It provides the authentic feel of games like **Counter-Strike 1.6**, **Quake**, and **Half-Life** natively as an `APawn` in UE5.

It fully supports classic mechanics without compromises:

- Bunnyhopping (Bhop)
- Strafe-jumping and Air-strafing
- Surfing (sliding on curved walls)
- Ladder climbing and ramp trajectories

## 🚀 Installation

1. Copy or clone this `QMovement` folder directly into the `Plugins` directory of your Unreal Engine project. The final path should look like this:
   `[YourProject]/Plugins/QMovement/`
   _(If the `Plugins` folder doesn't exist in your project, simply create it)._
2. Right-click on your project's `.uproject` file and select **Generate Visual Studio project files**.
3. Open the generated Solution (`.sln`) file and `Build` the project in Visual Studio or Rider.
4. Open the project in Unreal Engine 5 and ensure the plugin is enabled (_Edit -> Plugins_, search for "QMovement").

## 🎮 Usage Guide

### Step 1: Create a Character

1. Create a new Blueprint class that inherits from `AQuakePawn` (a class provided by this plugin).
2. You will receive a ready-to-use character with a Capsule Component and attached Camera.

### Step 2: Setup Enhanced Input

The `AQuakePawn` class is ready to accept the standard **Enhanced Input** system from Unreal Engine 5.
In the Details panel of your new Blueprint, assign your `Input Mapping Context` and corresponding `Input Actions` under the "Input" category:

- **MoveAction** (WASD - 2D Vector)
- **LookAction** (Mouse - 2D Vector)
- **JumpAction** (Jump - Digital/Bool)
- **DuckAction** (Crouch - Digital/Bool)

### Step 3: Configure Movement Settings (Data Asset)

In Unreal Engine:

1. Right-click in the Content Browser -> _Miscellaneous_ -> _Data Asset_.
2. Select the `UMovementSettings` class.
3. Name and open the new file. This is your control panel for all movement physics.
4. Assign this created Data Asset to the `Movement Settings` slot inside your Player Blueprint.

_Note: Default properties perfectly match original QuakeWorld values. If you do not assign a Data Asset, the engine will automatically use defaults known from CS 1.6._

---

## 💻 Console Commands

The plugin features built-in `sv_*` console commands that allow you to tweak physics live during gameplay, preserving the original engine's nomenclature. To use them, press the tilde `~` key during the game and type a command with a value (e.g., `sv_gravity 800`). Typing just the command will display its current value on the screen.

- `sv_gravity` - **[Default: 800]** Sets gravity (lower value = moon jump).
- `sv_maxspeed` - **[Default: 320 in QW / 250 in CS]** Maximum running speed on flat ground holding W.
- `sv_airaccelerate` - **[Default: 0.7 in QW / 10-100 on Surf servers]** The most important value for Bhop/Surfing. Determines how strongly the player can change flight direction while airborne.
- `sv_accelerate` - **[Default: 10]** The force with which the player accelerates from a standstill on the ground.
- `sv_friction` - **[Default: 6 in QW / 4 in CS]** Ground friction. Setting this to 0 creates ice, higher values instantly stop the player on the ground.
- `sv_jumpspeed` - **[Default: 270]** Upward velocity applied when jumping (jump height).
- `sv_stopspeed` - **[Default: 100]** Speed below which the engine applies full instantaneous static friction to prevent micro-sliding.
- `sv_stepsize` - **[Default: 18]** Maximum "step" height (in units) the player will automatically walk up without jumping.
- `sv_airspeedcap` - **[Default: 30]** Hidden Quake engine feature. Maximum sideways vector velocity considered during air-strafing.
- `sv_maxvelocity` - **[Default: 2000]** Emergency cap - absolute highest physical velocity (e.g. from explosions/bhop) to prevent wall clipping.
- `sv_autobhop` - **[Default: 0]** Set to `1` (or check in Data Asset) to make the player automatically hop off the ground by holding the jump key, avoiding the need for perfect mouse-wheel timing.
- `noclip` - Ghost mode. Character ignores walls and can freely fly around the map. Typing it again turns physics back on.

---

## 🤝 Contribution (Fork & Pull Request)

This plugin is open for community improvements! If you want to contribute (e.g., add wall-jump mechanics, improve surfacing, etc.), please follow the standard GitHub workflow:

1. **Fork:** Click the `Fork` button at the top right of the main GitHub repository. This creates your own copy.
2. **Clone:** Download your forked version to your PC:
   `git clone https://github.com/YourName/QMovement.git`
3. **Create a Branch:** Start working on a separate branch to keep things organized:
   `git checkout -b feature-new-walljump`
4. **Code:** Make your C++ modifications, compile, and test in Unreal Engine.
5. **Commit:**
   `git add .`
   `git commit -m "Added professional wall-jump with UE support"`
6. **Push:**
   `git push origin feature-new-walljump`
7. **Pull Request (PR):** Go back to the official **main** repository page and click the green `Compare & pull request` button. Briefly describe what your change does. We will review the code and merge it!
