# GBA-Emulator (WIP)

A Game Boy Advance emulator written in C, designed to replicate the GBA's hardware and provide an accurate emulation experience. This emulator implements the ARM7TDMI CPU (supporting ARM and Thumb instruction sets), memory management, and PPU graphics modes (0–4).  

While still under development, the emulator successfully passes several key test ROMs, showcasing its growing capabilities.  

---

## Table of Contents  
- [Features](#features)  
- [Test Results Overview](#test-results-overview)  
- [Passing Test ROMs](#passing-test-roms)  
  - [arm.gba](#armgba)  
  - [armwrestler.gba](#armwrestlergba)  
  - [hello.gba](#hellogba)  
  - [memory.gba](#memorygba)  
  - [panda.gba](#pandagba)  
  - [shades.gba](#shadesgba)  
  - [stripes.gba](#stripesgba)  
  - [thumb.gba](#thumbgba)  
- [License](#license)  

---

## Features  
- **ARM7TDMI CPU Emulation**: Supports ARM (32-bit) and Thumb (16-bit) instruction sets.  
- **Memory Management**: Accurate memory handling that passes test ROMs.  
- **Graphics Rendering**: Supports GBA video modes 0–4.   

---

## Test Results Overview 
Below is the current status of test ROMs.  

| Test ROM            | Status   | Notes                          |  
|---------------------|----------|--------------------------------|  
| **arm.gba**         | ✅ Pass  | ARM instruction set test.      |  
| **armwrestler.gba** | ✅ Pass  | Stress test for ARM accuracy.  |  
| **hello.gba**       | ✅ Pass  | Basic program functionality.   |  
| **memory.gba**      | ✅ Pass  | Validates memory operations.   |  
| **panda.gba**       | ✅ Pass  | Panda sprite rendering test.   |  
| **shades.gba**      | ✅ Pass  | Tests gradient rendering.      |  
| **stripes.gba**     | ✅ Pass  | Striped pattern rendering.     |  
| **thumb.gba**       | ✅ Pass  | Thumb instruction set test.    |  
| **bios.gba**        | ❌ Fail  | Bios setup test. It is successfully setting up the bios, but due to a video mode issue, the GBA startup animation freezes on the Nintendo text preventing ROM startup.    |  
| **brin_demo.gba**       | ❌ Fail  | Tiling rendering. Successfully renders tiles, but due to video mode issue, it is frozen on initial position preventing user from moving position with inputs.    |  
| **sbb_reg.gba**       | ❌ Fail  | Tiling rendering. Successfully renders tiles, but due to video mode issue, it is frozen on initial position preventing user from moving position with inputs.    |  
| **nes.gba**        | ❌ Fail  | NES test. Have not looked into or attempted this yet.    | 
| **suite.gba**        | ❌ Fail  | General GBA test suite. Due to same video mode issue causing freeze, it is frozen on main menu and can not navigate through test menus.    | 

---

## Passing Test ROMs  

### arm.gba  
ARM instruction set test. Demonstrates accurate execution of ARM-based instructions.  
![arm.gba passing](path/to/screenshot_arm.png)  

---

### armwrestler.gba  
Stress test for ARM instruction set accuracy.  
![armwrestler.gba passing](path/to/screenshot_armwrestler.png)  

---

### hello.gba  
A simple test validating basic program functionality and display rendering.  
![hello.gba passing](path/to/screenshot_hello.png)  

---

### memory.gba  
Validates memory read/write operations and address handling.  
![memory.gba passing](path/to/screenshot_memory.png)  

---

### panda.gba  
Renders a panda sprite on the screen, testing sprite rendering accuracy.  
![panda.gba passing](path/to/screenshot_panda.png)  

---

### shades.gba  
Tests the emulator's ability to render gradients and shades correctly.  
![shades.gba passing](path/to/screenshot_shades.png)  

---

### stripes.gba  
Displays striped patterns to test alignment and rendering precision.  
![stripes.gba passing](path/to/screenshot_stripes.png)  

---

### thumb.gba  
Thumb instruction set test to validate 16-bit instruction handling.  
![thumb.gba passing](path/to/screenshot_thumb.png)  

---

## License

This project is licensed under the GNU General Public License v2.0.

You can view the full text of the license in the LICENSE file or on the GNU website.

---

Stay tuned for updates as this emulator progresses! 🚀

