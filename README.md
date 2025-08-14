ffmpeg mp4 video player

A simple FFmpeg + SDL project in C for extracting and processing audio and video.
Based on a tutorial by Murage Kibicho.
---
## Description

This project demonstrates how to use FFmpeg’s C API to decode video/audio streams, resample audio, and optionally display video frames using SDL. It’s intended as a learning tool for multimedia programming and to develop new skills with different API in C.

---
## Prerequisites
Before running or compiling this project, make sure you have the following installed:

- **C compiler** (GCC, Clang or compatible)
- **FFmpeg development libraries** (`libavcodec`, `libavformat`, `libavutil`, `libswscale`, `libswresample`)
- **SDL2 development libraries** (for video rendering)
- **pkg-config** (optional, for easier compilation)

On Ubuntu/Debian, you can install dependencies with:

```bash
sudo apt-get update
sudo apt-get install build-essential pkg-config libavcodec-dev libavformat-dev libavutil-dev libswscale-dev libswresample-dev libsdl2-dev
```
To compile run:
gcc main.c -o Part1.o -lm -lavcodec -lavutil -lavformat -lswscale

To run the program:
./main.o <filename>.mp4
where <filename>.mp4 is the mp4 file you wish to run
