/**
 * AI-generated documentation begins.
 * RANE ONE MIDI controls. Motorized platters require an additional protocol.
 * Address evidence and hardware validation limits: RANE-ONE.md.
 * SPDX-License-Identifier: GPL-2.0-or-later
 * AI-generated documentation ends.
 */
// eslint-disable-next-line no-var -- AI-generated: expose the namespace to QJSEngine. End of AI-generated text.
var RaneOne = {};

RaneOne.bindings = {};
RaneOne.held = {};

RaneOne.bind = function(status, control, component, momentary) {
    RaneOne.bindings[`${status}:${control}`] = {component: component, momentary: !!momentary};
};

RaneOne.button = function(status, control, group, key, outKey, toggle) {
    const component = new components.Button({
        group: group,
        inKey: key,
        outKey: outKey,
        midi: [status, control],
        type: toggle ? components.Button.prototype.types.toggle : components.Button.prototype.types.push,
        on: 127,
        off: 0
    });
    RaneOne.bind(status, control, component, !toggle);
};

RaneOne.pot = function(status, control, group, key, invert) {
    RaneOne.bind(status, control, new components.Pot({
        group: group,
        inKey: key,
        invert: !!invert
    }));
};

RaneOne.init = function() {
    RaneOne.bindings = {};
    RaneOne.held = {};
    for (let deck = 0; deck < 2; deck++) {
        const group = `[Channel${  deck + 1  }]`;
        const note = 0x90 + deck;
        const pads = 0x94 + deck;
        const cc = 0xB0 + deck;
        RaneOne.bind(note, 0x00, new components.PlayButton({group: group, midi: [note, 0x00]}));
        RaneOne.bind(note, 0x01, new components.CueButton({group: group, midi: [note, 0x01]}), true);
        RaneOne.button(note, 0x02, group, "sync_enabled", "sync_enabled", true);
        RaneOne.bind(note, 0x03, {
            input: function(_channel, _control, value) {
                if (value) {
                    engine.setValue(group, "sync_enabled", 0);
                }
            }
        });
        RaneOne.button(note, 0x05, group, "start");
        RaneOne.bind(note, 0x06, {
            input: function(_channel, _control, value) {
                if (value && !engine.getValue(group, "play")) {
                    engine.setValue(group, "LoadSelectedTrack", 1);
                }
            }
        });
        RaneOne.bind(note, 0x07, {
            input: function(_channel, _control, value) {
                if (value) {
                    engine.setValue("[Library]", "MoveFocusBackward", 1);
                }
            }
        });
        RaneOne.button(note, 0x0B, group, "rate_temp_up");
        RaneOne.button(note, 0x0C, group, "rate_temp_down");
        RaneOne.button(note, 0x0D, group, "keylock", "keylock", true);
        RaneOne.button(note, 0x10, group, "reverseroll");
        RaneOne.button(note, 0x11, group, "reverse", "reverse", true);
        RaneOne.button(note, 0x1B, group, "pfl", "pfl", true);
        RaneOne.bind(note, 0x2B, {
            input: function(_channel, _control, value) {
                if (value) {
                    const ranges = [0.08, 0.16, 0.5];
                    const current = engine.getValue(group, "rateRange");
                    const next = ranges.findIndex(range => range > current + 0.001);
                    engine.setValue(group, "rateRange", ranges[next < 0 ? 0 : next]);
                }
            }
        });
        RaneOne.pot(cc, 0x09, group, "rate", true);
        RaneOne.pot(cc, 0x16, group, "pregain");
        const eq = `[EqualizerRack1_${  group  }_Effect1]`;
        RaneOne.pot(cc, 0x17, eq, "parameter3");
        RaneOne.pot(cc, 0x18, eq, "parameter2");
        RaneOne.pot(cc, 0x19, eq, "parameter1");
        RaneOne.pot(cc, 0x1A, `[QuickEffectRack1_${  group  }]`, "super1");
        RaneOne.pot(cc, 0x1C, group, "volume");
        RaneOne.bind(cc, 0x00, {
            input: function(_channel, _control, value) {
                const steps = value - 64;
                if (steps) {
                    engine.setValue("[Library]", "MoveVertical", steps);
                }
            }
        });
        RaneOne.button(pads, 0x09, group, "slip_enabled", "slip_enabled", true);
        RaneOne.button(pads, 0x34, group, "loop_halve");
        RaneOne.button(pads, 0x35, group, "loop_double");
        RaneOne.button(pads, 0x36, group, "loop_in");
        RaneOne.button(pads, 0x37, group, "loop_out");
        RaneOne.button(pads, 0x3F, group, "beatloop_activate", "loop_enabled");
        RaneOne.button(pads, 0x41, group, "reloop_toggle");
        const colors = [112, 76, 67, 124, 79, 120, 123, 99];
        for (let pad = 0; pad < 8; pad++) {
            const hotcue = new components.HotcueButton({
                number: pad + 1,
                group: group,
                midi: [pads, 0x14 + pad],
                on: colors[pad],
                off: 0
            });
            RaneOne.bind(pads, 0x14 + pad, hotcue, true);
            RaneOne.button(pads, 0x1C + pad, group, `hotcue_${  pad + 1  }_clear`);
        }
    }
    RaneOne.pot(0xBF, 0x08, "[Master]", "crossfader");
    RaneOne.bind(0xBF, 0x04, {
        input: function(_channel, _control, value) {
            for (let unit = 1; unit <= 2; unit++) {
                engine.setParameter(`[EffectRack1_EffectUnit${  unit  }]`, "mix", value / 127);
            }
        }
    });
};

RaneOne.input = function(channel, control, value, status) {
    const isNoteOff = (status & 0xF0) === 0x80;
    const normalizedStatus = isNoteOff ? status + 0x10 : status;
    const id = `${normalizedStatus}:${control}`;
    const binding = RaneOne.bindings[id];
    if (!binding) {
        return;
    }
    const normalizedValue = isNoteOff ? 0 : value;
    binding.component.input(channel, control, normalizedValue, normalizedStatus);
    if (binding.momentary) {
        if (normalizedValue) {
            RaneOne.held[id] = {binding: binding, channel: channel, control: control, status: normalizedStatus};
        } else {
            delete RaneOne.held[id];
        }
    }
};

RaneOne.shutdown = function() {
    Object.keys(RaneOne.held).forEach(id => {
        const held = RaneOne.held[id];
        held.binding.component.input(held.channel, held.control, 0, held.status);
    });
    Object.keys(RaneOne.bindings).forEach(id => {
        const component = RaneOne.bindings[id].component;
        if (component.disconnect) {
            component.disconnect();
        }
        if (component.shutdown) {
            component.shutdown();
        }
    });
    RaneOne.held = {};
    RaneOne.bindings = {};
};
