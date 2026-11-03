<!-- AI-generated documentation begins. -->

# RANE ONE — experimental MIDI mapping

This mapping targets the original RANE ONE, not the ONE MKII. It is independent of
the Mixxx skin, including Gameflorr. It has automated MIDI simulation coverage;
it has **not been tested on a physical RANE ONE**.

**Motorized platters and scratching are not implemented.** Their initialization,
position reporting and motor commands need hardware protocol captures. The rotating
markers in Gameflorr are a screen display and do not imply hardware platter support.
Do not use this mapping for a performance until you have checked your controller.

## Installation

The mapping is included in this fork's controller resources. With the existing local
Gameflorr launcher, use the rebuilt binary of this fork: performance pad controls
are not present in stock Mixxx. For another installation of this fork,
copy `RANE ONE.midi.xml` and `RANE-ONE-scripts.js` into its user controller directory
(Preferences → Controllers → Open User Mapping Folder), then restart Mixxx.
Mixxx supplies `midi-components-0.0.js`.

1. Connect and power on the original RANE ONE. Install the manufacturer's audio
   driver/control panel if your operating system requires it.
2. Set both deck source switches to the connected **USB A** or **USB B** port.
3. Open Preferences → Controllers, select the RANE One MIDI port, choose
   **RANE ONE (experimental MIDI; no motorized platters)** and enable it.
4. Configure audio separately under Preferences → Sound Hardware. This mapping does
   not install an audio driver or configure USB audio routing. Check the available
   device output pairs at low volume, including headphone cueing, before mixing.

## Controls

| Hardware | Mixxx action |
| --- | --- |
| Start/Stop, Cue, Sync | Play/pause, cue, toggle sync |
| Shift+Sync | Disable sync |
| Shift+Cue | Return to track start |
| Load | Load selected track, blocked while the destination deck is playing |
| Browse encoder, Back | Move through the library, move focus back |
| Pitch slider, Pitch Bend −/+ | Tempo, temporary tempo bend |
| Range, key lock | Cycle ±8/16/50%, toggle key lock |
| Censor / Reverse | Hold reverse roll / toggle reverse |
| Slip | Toggle slip mode |
| Channel Level, EQ, Filter, faders | Gain, three-band EQ, quick effect, volume, crossfader |
| Headphone Cue | Toggle Mixxx PFL; hardware headphone controls still require audio setup |
| Pads 1–8 | Follow the deck pad mode selected on screen: Hotcue, Roll, Slicer, Sampler, Loop, Jump, Slicer Loop |
| Shift+pad | Clear a hotcue, stop a sampler, or exit a loop, according to the active mode |
| Parameter −/+ | Cycle the seven pad modes; changing mode releases held pads |
| Loop On/Off, Reloop | Toggle beat loop, reloop |
| Loop ½/×2, In/Out | Resize or set the loop |
| FX Depth | Wet/dry of effect units 1 and 2; enable effects in the Mixxx interface |

Hotcue LED colors use the observed RANE palette. Transport and cue feedback follow
Mixxx state. Slicer requires a valid beat grid and a complete eight-beat region;
Sampler targets the shared slots 1–8. See the [pad guide](../../docs/gameflorr-interface.md).
The hardware pad-mode selection buttons are not decoded: use Parameter or the UI.
Motor commands, platter LEDs, touch strip, stop-time knob, FX selection/paddles and
meters are not implemented.
Master, booth, microphone and headphone level controls are left to the hardware.
Sliders and pots use Components JS soft takeover after the first received position.

## Verification and protocol evidence

Run `node --test tools/test_rane_one_mapping.cjs` from the repository root.
The tests load the real Components JS library and dispatch the XML's MIDI messages.
They check both decks, release messages, loading protection, pots, pads, loops,
LED feedback and shutdown. They do not replace a hardware test.

Message addresses were checked against the mapping attached by its author to
[Sharing my edited RANE ONE mapping for iPad](https://community.algoriddim.com/t/sharing-my-edited-rane-one-mapping-for-ipad/15719).
Only protocol facts were used; its custom djay actions and private `RaneOne` class
are not included. The private class means this attachment is insufficient to
implement motors or hardware initialization.

Hardware behavior was checked against the
[RANE ONE user guide](https://cdn.inmusicbrands.com/rane/one/RANE-ONE-User-Guide-v1_5.pdf)
and [VirtualDJ's RANE ONE setup guide](https://virtualdj.com/manuals/hardware/rane/one/setup.html).

For the hardware check, start at low volume and verify load protection, both decks,
note release, faders, headphone cueing, pads, loops and LEDs. To investigate missing
controls on Linux, close Mixxx, run `amidi -l`, then use `amidi -p <port> -d` and
record one named control at a time. MIDI dumps alone may not expose platter data;
motor integration can require USB captures with software that already supports it.

<!-- AI-generated documentation ends. -->
