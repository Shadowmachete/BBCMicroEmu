# Devlog

## 20260602 - Tuesday: Day 1

---

- spent incredibly long searching for documentation
- implemented memory management module with mapping and 32KB RAM, 16KB BASIC and
  OS ROM
- drafted out 6502 CPU code

---

## 20260603 - Wednesday: Day 2

---

- implemented all different instructions for 6502
- spent an excessive amount of time reading disassembly of the MOS ROM and
  comparing against my execution loop to bug-fix instructions
  - fixed bugs in branches not updating PC properly
- encountered OS loop waiting for response from peripherals
- outlined VIA registers

---

## 20260604 - Thursday: Day 3

---

- spent a brief amount of time reading the documentation on system VIA
- trying to understand reading and writing for register B
- implemented reading code from VIA registers

---

## 20260608 - Monday: Day 4

---

- finished up VIA read and write

---

## 20260613 - Saturday: Day 5

---

- implemented CRTC and Video ULA
- add Raylib to vendor and draw the screen from frame buffer
- add keyboard support for ASCII keys, backspace and enter
- BASIC works
- weird corruption bug when scroll down past screen end
- MODEs with colours other than black & white / MODE 4 not implemented yet

---

## 20260615 - Monday: Day 6

---

- fixed the corruption bug by wrapping addr around while reading from vram
- added cursor
- MODE 0-3 dont do anything but MODE 4-7 set the correct mode (just weird display)
- tried to generalise away from just MODE 4 for drawing screen, MODE 5 "seems"
  to draw properly for now
- idk how to get colours drawing, the palette map doesnt seem to correspond to
  the colours when i try `GCOL`

---

## 20260617 - Wednesday: Day 7

---

- fixed cursor size to match the character width in different modes
- able to draw colours, issue was the logical location of the colours are stored
  as 1-4 bits in the pixel data but corresponding to b3 / b3 & b1 / b3-0 respectively
- allow flashing colors
- MODE 6 working

---

## 20260623 - Tuesday: Day 8

---

- implemented paged rom switching

---

## 20260624 - Wednesday: Day 9

---

- spending lots of time searching for 8271 FDC spec
- implementing fdc module
  - implemented fdc_read, fdc_write, fdc_reset and execution of specify command

---

## 20260625 - Thursday: Day 10

---

- still implementing fdc module
  - implemented all init commands (seek, read status (stub), specify, read and
    write sp reg)
- struggling with read data
- gated by a proper interrupt system
- adding cycling count is so stupid if i dont hard-code
- struggling on cycle counting last i saw
