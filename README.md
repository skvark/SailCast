# SailCast
Screencasting app for Sailfish OS. Streams MJPEG over socket, screen contents grabbed through Linux framebuffer.

# Usage

Just hit start button and the app will start listening on address provided in below buttons.

# Supported devices

 - Jolla

## Notes

Tested only on Firefox and VLC. Chrome requires that the mjpeg stream is embedded into html element so the stream won't show up when the plain address is accessed.

TODO:

- Serve an html page which embeds the stream element? (won't work with VLC)
- Add remote control, might be a bit complicated but doable

## Framebuffer information

``fbset -i`` ouput for different devices.

#### Jolla

```
mode "540x960"
    geometry 540 960 540 2884 32
    timings 0 20 40 4 4 10 2
    rgba 8/24,8/16,8/8,8/0
endmode

Frame buffer device information:
    Name        : msmfb43_90501
    Address     : 0x82478000
    Size        : 6266880
    Type        : PACKED PIXELS
    Visual      : TRUECOLOR
    XPanStep    : 1
    YPanStep    : 1
    YWrapStep   : 0
    LineLength  : 2176
    Accelerator : No
```

#### Jolla C

Reversed pixel layout when compared to Jolla.

```
mode "720x1280-2"
    # D: 2.347 MHz, H: 2.637 kHz, V: 1.983 Hz
    geometry 720 1280 720 2560 32
    timings 426132 80 80 20 20 10 10
    rgba 8/0,8/8,8/16,8/24
endmode

Frame buffer device information:
    Name        : mdssfb_80000
    Address     : 0x600000
    Size        : 12582912
    Type        : PACKED PIXELS
    Visual      : TRUECOLOR
    XPanStep    : 1
    YPanStep    : 1
    YWrapStep   : 0
    LineLength  : 2944
    Accelerator : No
```

#### Jolla Tablet

To be added (I guess it's reversed too).

```

```
