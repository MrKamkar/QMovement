# QMovement Plugin

**GoldSrc / QuakeWorld Movement in Unreal Engine 5**

This plugin is a faithful C++ port of the original GoldSrc (`pmove.c`) movement system. It provides the authentic feel of games like **Counter-Strike 1.6**, **Quake**, and **Half-Life** natively as an `APawn` in UE5.

It fully supports classic mechanics without compromises:

- Bunnyhopping (Bhop)
- Strafe-jumping and Air-strafing
- Surfing (sliding on curved walls)
- Ladder climbing and orientation-independent jump-off
- Water movement and swimming
- Conveyor belts (using BaseVelocity)
- **Crouch-Jumping (Duck-Jump)** with authentic mid-air origin behavior
- **Delayed Ground Crouching** (TIME_TO_DUCK) matching GoldSrc delay
- Fully data-driven view heights and physics parameters

### 🌟 Gameplay Showcase

|              Bunnyhopping               |            Surfing             |
| :-------------------------------------: | :----------------------------: |
| ![Bunnyhopping](Images/Bunnyhoping.gif) | ![Surfing](Images/Surfing.gif) |

|       Ladder Climbing        |       Water Movement       |              Conveyor Belts               |
| :--------------------------: | :------------------------: | :---------------------------------------: |
| ![Ladder](Images/Ladder.gif) | ![Water](Images/Water.gif) | ![Conveyor Belt](Images/ConveyorBelt.gif) |

## ⚠️ Important Prerequisite (For Blueprint-Only Projects)

If your project was created strictly as a Blueprint project and does not have a `Source` folder, Unreal Engine will block you from compiling new Plugins with the error: _"This project does not have any source code"_.

**To quickly convert your project to support C++ Plugins:**

1. Open your project normally in Unreal Engine Editor.
2. Go to **Tools** (or **File** in older versions) at the top -> **New C++ Class...**
3. Choose `None` (or `Actor`), click **Next**, and then **Create Class**.
4. The Engine will generate a Source folder and automatically open Visual Studio.
5. Close Visual Studio and Unreal Engine. You are now ready to install the plugin below!

## 🚀 Installation (ZIP Method)

1. Click the green `Code` button at the top of this repository and select **Download ZIP**.
2. Extract the downloaded ZIP file. Rename the extracted folder from `QMovement-main` to strictly **`QMovement`**.
3. Go to your Unreal Engine project's main directory (where your `.uproject` file is located).
4. Create a new folder named **`Plugins`** there (if you don't already have one).
5. Paste your extracted `QMovement` folder inside the `Plugins` folder. _(Your path should look like: `YourGame/Plugins/QMovement/`)_.
6. Right-click on your project's `.uproject` file (e.g., `YourGame.uproject`) and select **Generate Visual Studio project files**.
   _(This step is crucial! It forces the engine to link your Blueprint project with this C++ plugin)._
7. Open the generated `.sln` file and `Build` the project in Visual Studio. **Alternatively**, just open the project in Unreal Editor 5 — the engine will prompt: _"Missing Modules. Would you like to rebuild them now?"_. Click **Yes**.
8. Wait for the background compilation to finish. Once the Editor opens, the plugin is ready to go! (Ensure it is enabled under _Edit -> Plugins_).

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

_Note: Default properties perfectly match original QuakeWorld values. If you do not assign a Data Asset, the engine will automatically use defaults known from Quake._

---

## 💻 Console Commands

The plugin features built-in `sv_*` console commands that allow you to tweak physics live during gameplay, preserving the original engine's nomenclature. To use them, press the tilde `~` key during the game and type a command with a value (e.g., `sv_gravity 800`). Typing just the command will display its current value on the screen.

- `sv_gravity` - **[Default: 800]** Sets world gravity (lower = moon jump).
- `sv_maxspeed` - **[Default: 320 in QW / 250 in CS]** Maximum running speed on flat ground.
- `sv_airaccelerate` - **[Default: 10]** Air control strength. High values (10-100) enable Bhop/Surf.
- `sv_accelerate` - **[Default: 10]** Ground acceleration force.
- `sv_friction` - **[Default: 4]** Ground friction. Set to 0 for ice, higher for instant stops.
- `sv_jumpspeed` - **[Default: 268.3]** Upward velocity applied on jump.
- `sv_stopspeed` - **[Default: 100]** Speed threshold for applying static friction.
- `sv_stepsize` - **[Default: 18]** Maximum height (in units) for automatic step-up.
- `sv_airspeedcap` - **[Default: 30]** Max sideways velocity cap during air-strafing.
- `sv_maxvelocity` - **[Default: 2000]** Hard physical velocity limit.
- `sv_autobhop` - **[0/1]** Enable auto-jumping by holding the jump key.
- `sv_disablebhopcap` - **[0/1]** Disable the 1.7x speed cap found in classic HL/CS.
- `sv_camerapunch` - **[0/1]** Enable/disable camera impact shake on landing.
- `sv_cameraroll` - **[0/1]** Enable/disable camera leaning during strafing.
- `cl_bob` - **[0/1]** Enable/disable view bobbing (head bob) while moving.
- `noclip` - Toggle ghost mode (fly through walls).

---

## 🤝 Contribution (Fork & Pull Request)

This plugin is open for community improvements! If you want to contribute (e.g., add wall-jump mechanics, improve surfacing, etc.), please follow the standard GitHub workflow:

1. **Fork:** Click the `Fork` button at the top right of the main GitHub repository. This creates your own copy.
2. **Clone:** Download your forked version to your PC:
   `git clone https://github.com/MrKamkar/BH_Quake.git`
3. **Create a Branch:** Start working on a separate branch to keep things organized:
   `git checkout -b feature-new-walljump`
4. **Code:** Make your C++ modifications, compile, and test in Unreal Engine.
5. **Commit:**
   `git add .`
   `git commit -m "Added professional wall-jump with UE support"`
6. **Push:**
   `git push origin feature-new-walljump`
7. **Pull Request (PR):** Go back to the official **main** repository page and click the green `Compare & pull request` button. Briefly describe what your change does. We will review the code and merge it!

---

**Developed by MrKamkar** | [GitHub Profile](https://github.com/MrKamkar) | [Project Repository](https://github.com/MrKamkar/BH_Quake)
