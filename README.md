# ESP32SoundModule
This repository contains code for a sound playback module to replace a very popular MP3 player which is used in many robotics projects, but comes with a lot of problems that we will hopefully fix. And we will add a lot of features.

The sound module is based on ESP32S3 hardware ([Xiao ESP32S3](https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/)) or [Adafruit Qt PY ESP](https://www.adafruit.com/product/5426)) and Adafruit's [Audio BFF](https://www.adafruit.com/product/5769) combination SD card and D/A converter bundle.

**Current Features:**
* Drop in protocol replacement for "that popular MP3 player" (partly)
* Configuration via INI file

**Planned Features:**
* Drop in protocol replacement for "that popular MP3 player" (full)
* Monaco implementation for direct sound triggering (via ESPnow, Bluetooth, and UDP)
* Monaco bridge to add ESPnow communication to controlling robot
* WAV playback
* MP3 playback
* Sound generation for robot motion sounds
* Sound generation for robot utterings

## Primary Purpose
Many many folks are using a specific MP3 player module in their robot projects. I won't name it here for liability reasons but you know which one I'm talking about. 

And everyone hates these things. Many different clones all with slightly different firmwares. Most support the basic feature: Play a MP3 file from the root of the SD card, given by the index of when the file was uploaded (not the file name, nooo...) If you want to do any sort of file management, like playing files from a folder, you have to luck out to get a chip/firmware combination that actually supports folders. Dotfiles (like the ones Macs generate) are a problem. Of course you can't debug  because there is no separate output path. In short, not optimal.

Because these things are so ubiquitous, many control boards have receptacles for them, and many control software packages are using code that talks to them. So the primary purpose of this project is to build a proper working sound player that is open source, debuggable, actually works as designed, and hopefully can do a lot more.

## Documentation
Please see the [Wiki](https://github.com/bjoerngiesler/ESP32SoundModule/wiki) for docs.
