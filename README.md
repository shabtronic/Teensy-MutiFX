# Teensy MultiFX

A simple hardware platform for Gutiar based FX. Mostly mod and delay - no distortion. Audio board has no Hi-Z input, so this is meant to be used after
your main distortion pedal (ToneX-One is what I use, ToneX-one uses a NXP chip similar to the Teensy 4.1) which will buffer the input to low-z and make it usable on the Audio board. You may be able to use active pickups directly into the Audio board without a pedal to buffer - untested.

Libs Used:

- LVGL 9.22, Teensyduino

Libs Planned:

- DRWav, DRFlac, DRMp3 (for media playback)
- Teensy T4 screen lib, for speed and DMA freeing up CPU.
  
Current Status:

- SD card - working and reading.
- Screen Working, SPI running at 110mhz = around 79fps full screen updates (110,000,000)/(320x240x18) (9 SPI bits per byte)
  I think that's the max the LCD ILI9341V can handle
- Screen Cap Touch working
- LVGL rendering
- One FX running and usable (Reverse-O-Tron)
- Pots working - Volumes and parameters
- Encoder Working - Lvgl TileView slides to separate FX screens
- USB Audio in and out working (usb buffer is 512 samples, USB input in reaper causes major stuttering for some reason!)
    So this can be used as a full blown usb audio interface - with no OS drivers needed.
- Line in/out working
- Headphone out and control working
- VU Meters - usb in,line in and audio out working
- LVGL TileView for separate FX UIs, scrolls with encoder
  
Code style is "wildcat frenzy" - so it's currently a complete mess :) - early days until everything falls into place and refactoring to a neat and tidy system. 

So this is currently **Pre-Alpha** - lets get the hardware connected and working stage.

Code is un-optimized, but Teensy 4.1 is a beastly MCU!, even at the default 600mhz Reverse-O-Tron at max uses 90 delay taps and 18 Allpass filters - and takes around 17% APU - completely unoptimized.

FX/Features Planned:

- BiQuad EQ - from my github https://github.com/shabtronic/Audio-Filters
- FFT Reverb - from my github https://github.com/shabtronic/FFT-Reverb
- Riser Reverb - from my github https://github.com/shabtronic/FDN-Reverb-Riser
- LPC monophonic Pitch Shifter/Harmonizer with per note mapping and FX routing.
- Poly ring buffer Pitchshifter
- Loop Recorder/Multitracker
- Full Midiverb/Midiflex implementation from: https://github.com/thement/midiverb_emulator
- Chug-O-Tron - Sequenced rhythmic delay with ducking/gating for folk too lazy to learn Meshuggah style patterns
- Chorus/Flanger/Phaser
- Tremolo
- Presets
- Footswitch Control
- Selectable processing USB/Line In - currently only Line In
- Simple Streaming Sampler and Sequencer
    
Far off in the future may never happen blue sky optimism ideas:

- Flexible Parallel FX routing
- Jeskola Buzz/Kyma Capabyra style graph editing
- Selectable sample rate (48khz,96khz), currently fixed at 44.1khz
   
Current Parts are:

- Teensy 4.1 £34
- Teensy Audio Shield rev D for 4.0 £18
- 11 x 10k pots £12
- 1 ALPS rotary encoder £10
- Hosyond 3.2 inch 240x320 IPS Capacitive Touch Screen LCD Module SPI Serial ILI9341V £18
- 128gb micro SDCard £10
- 2 Stereo 3.5mm sockets £5
- 1 PCB board £5
- 2x10k resistors (for encoder) £???

Total Parts cost circa 2024: £112

# Development Notes

  The teensy platform has been a real dream to code so far - almost everything worked out of the box (USB Audio, SDIO Card, Encoders, Audio Plumbing), even on the Ardinuo platform all this stuff just worked. The Hardware side has been the tricky part - trying to get a stable board with zero mechanical engineering skills, zero soldering skills and little Patience has been frustrating. Getting a small form factor/footprint with parts from yestayear has been really challenging.
  
  There's only one issue with the Teensy that I've hit so far - and it's not too big a thing and that's the ADC noise - it doesn't have a separate 3.3v ref, it uses the 3.3v power for that - so it's as noisy as your power supply (usb) and soldering skills (very poor :) ) - to the point that I use 8 bit resolution and long ADC sample read times to make it usable.

Info on the Teensy ADC grounding stuff: https://forum.pjrc.com/index.php?threads/teensy-4-1-agnd-retired.65599/

# CNC Aluminium case

Cost £20

# Video of hardware running

# Current Hardware Build
![](./Images/MultiFX1.jpg)

connections:

LCD RST is tied to VCC with resistor
LCD CS is tied to GND

13 LCD SCK
10 LCD DS/RS
11 LCD MOSI

18 LCD CPT SDA
19 LCD CPT SCL

14 POT1
15 POT2
16 POT3
24 POT4
25 POT5
26 POT6
27 POT6
41 POT8
40 POT9
39 POT10
38 POT11

34 ENC 1
33 ENC 2

Encoder pins are tied to VCC with resistors as per: https://forum.pjrc.com/index.php?threads/using-5v-rotary-encoders-with-teensy-4-1.72834/
