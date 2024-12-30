Teensy MultiFX

A simple hardware platform for Gutiar based FX. Mostly mod and delay - no distortion. Audio board has no Hi-Z input, so this is meant to be used after
your main distortion pedal (ToneX-One is what I use) which will buffer the input to low-z and make it usable on the Audio board.

Libs Used:

- LVGL 9.22, Teensyduino

Libs Planned:

- DRWav,DRFlac,DRMp3 (for media playback)
- Teensy T4 screen lib
  
Current Status:

- SD card - working and reading.
- Screen Working, spi running at 70mhz = around 50fps full screen updates
- Screen Cap Touch working
- LVGL rendering
- One FX running
- Pots working
- Encoder Working
- USB Audio in and out working (USB input in reaper causes major stuttering for some reason!)
- Line in/out working
- Headphone out and control working
- VU Meters - usb in,line in and audio out working
- LVGL TileView for separate FX UIs, scrolls with encoder
  
Code style is wildcat frenzy - so a complete mess :) - early days until everything falls into place and refactoring to a neat and tidy system.
Code is un-optimized, but Teensy 4.1 is a beastly MCU!, even at 600mhz Reverse-O-Tron uses 90 delay taps and 18 Allpass filters - takes around 17% APU - completely unoptimized.

FX Planned:

- FFT Reverb - from my github
- Riser Reverb - from my github
- Pitch Shifter/Harmonizer with per note mapping and FX routing.
- Loop Recorder/Multitracker
- Full Midiverb/Midiflex implementation from: https://github.com/thement/midiverb_emulator
- Chug-O-Tron - Rhythmic delay with duckin for folk too lazy to learn Messuggah style patterns

Current Parts are:

- Teensy 4.1
- Teensy Audio Sheild rev D for 4.0
- 11 x 10k pots
- 1 ALPS rotary encoder
- Hosyond 3.2 inch 240x320 IPS Capacitive Touch Screen LCD Module SPI Serial ILI9341V 
- 128gb SDCard
- 2 Stereo 3.5mm sockets
- 1 PCB board
  
![](./Images/MultiFX1.jpg)
