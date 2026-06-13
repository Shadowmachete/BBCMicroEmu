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

- implement CRTC and Video ULA
- add Raylib to vendor and draw the screen from frame buffer
- add keyboard support for ASCII keys, backspace and enter
- BASIC works
- weird corruption bug when scroll down past screen end
- MODEs with colours other than black & white / MODE 4 not implemented yet

---
