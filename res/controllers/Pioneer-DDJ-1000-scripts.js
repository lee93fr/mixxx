/**
 * AI-generated documentation begins.
 * Original DDJ-1000 MIDI mapping; see Pioneer-DDJ-1000.md for setup and limits.
 * SPDX-License-Identifier: GPL-2.0-or-later
 * AI-generated documentation ends.
 */
// eslint-disable-next-line no-var -- AI-generated: QJSEngine namespace. End of AI-generated text.
var PioneerDDJ1000 = {};

PioneerDDJ1000.jogResolution = 720;
PioneerDDJ1000.bendScale = 0.1;
PioneerDDJ1000.bindings = {};
PioneerDDJ1000.held = {};

PioneerDDJ1000.bind = function(status, control, component, momentary) {
    PioneerDDJ1000.bindings[`${status}:${control}`] = {component: component, momentary: !!momentary};
};

PioneerDDJ1000.button = function(status, control, group, key, outKey, toggle) {
    PioneerDDJ1000.bind(status, control, new components.Button({
        group: group, inKey: key, outKey: outKey, midi: [status, control],
        type: toggle ? components.Button.prototype.types.toggle : components.Button.prototype.types.push,
        on: 127, off: 0
    }), !toggle);
};

PioneerDDJ1000.press = function(status, control, action) {
    PioneerDDJ1000.bind(status, control, {
        input: function(_channel, _control, value) {
            if (value) {
                action();
            }
        }
    });
};

PioneerDDJ1000.pot14 = function(status, msb, group, key, invert) {
    const pot = new components.Pot({group: group, inKey: key, max: 16383, invert: !!invert});
    const pair = {};
    const receive = function(part, channel, control, value, message) {
        pair[part] = value;
        if (pair.msb !== undefined && pair.lsb !== undefined) {
            pot.input(channel, control, pair.msb * 128 + pair.lsb, message);
            delete pair.msb;
            delete pair.lsb;
        }
    };
    PioneerDDJ1000.bind(status, msb, {
        input: function(channel, control, value, message) {
            receive("msb", channel, control, value, message);
        },
        disconnect: function() { pot.disconnect(); }
    });
    PioneerDDJ1000.bind(status, msb + 32, {
        input: function(channel, control, value, message) {
            receive("lsb", channel, control, value, message);
        }
    });
};

PioneerDDJ1000.release = function(predicate) {
    Object.keys(PioneerDDJ1000.held).forEach(id => {
        const held = PioneerDDJ1000.held[id];
        if (predicate(held)) {
            held.binding.component.input(held.channel, held.control, 0, held.status);
            delete PioneerDDJ1000.held[id];
        }
    });
};

PioneerDDJ1000.releaseDeck = function(deck) {
    PioneerDDJ1000.release(held => {
        const status = held.status;
        return status === 0x90 + deck || status === 0x97 + deck * 2 || status === 0x98 + deck * 2;
    });
    engine.scratchDisable(deck + 1);
};

PioneerDDJ1000.init = function() {
    PioneerDDJ1000.bindings = {};
    PioneerDDJ1000.held = {};
    PioneerDDJ1000.vinyl = [true, true, true, true];
    PioneerDDJ1000.modeConnections = [];
    for (const entry of [["num_decks", 4], ["num_samplers", 8]]) {
        if (engine.getValue("[App]", entry[0]) < entry[1]) {
            engine.setValue("[App]", entry[0], entry[1]);
        }
    }
    for (let deck = 0; deck < 4; deck++) {
        const group = `[Channel${deck + 1}]`;
        const note = 0x90 + deck;
        const cc = 0xB0 + deck;
        const pads = 0x97 + deck * 2;
        PioneerDDJ1000.modeConnections.push(engine.makeConnection(group, "pad_mode", () => {
            PioneerDDJ1000.release(held => held.status === pads || held.status === pads + 1);
        }));
        PioneerDDJ1000.bind(note, 0x0B, new components.PlayButton({group: group, midi: [note, 0x0B]}));
        PioneerDDJ1000.bind(note, 0x0C, new components.CueButton({group: group, midi: [note, 0x0C]}), true);
        PioneerDDJ1000.button(note, 0x48, group, "start");
        for (const entry of [[0x58, "sync_enabled"], [0x1A, "keylock"],
            [0x54, "pfl"], [0x40, "quantize"], [0x15, "slip_enabled"]]) {
            PioneerDDJ1000.button(note, entry[0], group, entry[1], entry[1], true);
        }
        PioneerDDJ1000.press(note, 0x5C, () => engine.setValue(group, "sync_leader", 1));
        PioneerDDJ1000.press(note, 0x60, () => {
            const ranges = [0.06, 0.10, 0.16, 1.0];
            const current = engine.getValue(group, "rateRange");
            const next = ranges.findIndex(range => range > current + 0.001);
            engine.setValue(group, "rateRange", ranges[next < 0 ? 0 : next]);
        });
        for (const entry of [[0x65, "back"], [0x64, "fwd"], [0x4C, "loop_in"], [0x4D, "reloop_toggle"]]) {
            PioneerDDJ1000.button(note, entry[0], group, entry[1]);
        }
        for (const entry of [[0x10, "loop_in", "loop_halve"], [0x11, "loop_out", "loop_double"]]) {
            PioneerDDJ1000.press(note, entry[0], () => {
                const key = engine.getValue(group, "loop_enabled") ? entry[2] : entry[1];
                engine.setValue(group, key, 1);
                engine.setValue(group, key, 0);
            });
        }
        PioneerDDJ1000.button(note, 0x14, group, "beatloop_4_toggle", "loop_enabled");
        for (const entry of [[0x16, 0], [0x1D, 1], [0x18, 2]]) {
            PioneerDDJ1000.press(note, entry[0], () => engine.setValue(group, "orientation", entry[1]));
        }
        PioneerDDJ1000.bind(note, 0x3F, {
            input: function() { PioneerDDJ1000.releaseDeck(deck); }
        });
        for (const mode of [0x1B, 0x1E, 0x20, 0x22, 0x69, 0x6B, 0x6D, 0x6F,
            0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B,
            0x2C, 0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x32, 0x33,
            0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
            0x09, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F, 0x00]) {
            PioneerDDJ1000.press(note, mode, () => {
                PioneerDDJ1000.release(held => held.status === pads || held.status === pads + 1);
                const modes = {
                    0x1B: 0, 0x1E: 1, 0x20: 5, 0x22: 3,
                    0x69: 6, 0x6B: 2, 0x6D: 4, 0x6F: 5
                };
                if (modes[mode] !== undefined) {
                    engine.setValue(group, "pad_mode", modes[mode]);
                }
            });
        }
        for (const touch of [0x36, 0x67]) {
            PioneerDDJ1000.bind(note, touch, {
                input: function(_channel, _control, value) {
                    if (value && PioneerDDJ1000.vinyl[deck]) {
                        engine.scratchEnable(deck + 1, PioneerDDJ1000.jogResolution, 33 + 1 / 3, 1 / 8, 1 / 256);
                    } else {
                        engine.scratchDisable(deck + 1);
                    }
                }
            }, true);
        }
        for (const jog of [0x21, 0x22, 0x23, 0x26, 0x1F, 0x29]) {
            PioneerDDJ1000.bind(cc, jog, {
                input: function(_channel, _control, value) {
                    const delta = value - 64;
                    if (jog === 0x23) {
                        PioneerDDJ1000.vinyl[deck] = false;
                        engine.scratchDisable(deck + 1);
                    } else if (jog === 0x22) {
                        PioneerDDJ1000.vinyl[deck] = true;
                        if (!engine.isScratching(deck + 1) &&
                            (PioneerDDJ1000.held[`${note}:54`] || PioneerDDJ1000.held[`${note}:103`])) {
                            engine.scratchEnable(deck + 1, PioneerDDJ1000.jogResolution, 33 + 1 / 3, 1 / 8, 1 / 256);
                        }
                    }
                    if (jog === 0x22 && engine.isScratching(deck + 1)) {
                        engine.scratchTick(deck + 1, delta);
                    } else {
                        engine.setValue(group, "jog", delta * (jog === 0x1F || jog === 0x29 ? 2 : PioneerDDJ1000.bendScale));
                    }
                }
            });
        }
        PioneerDDJ1000.pot14(cc, 0x00, group, "rate", true);
        PioneerDDJ1000.pot14(cc, 0x04, group, "pregain");
        PioneerDDJ1000.pot14(cc, 0x13, group, "volume");
        const eq = `[EqualizerRack1_${group}_Effect1]`;
        for (const entry of [[0x07, "parameter3"], [0x0B, "parameter2"], [0x0F, "parameter1"]]) {
            PioneerDDJ1000.pot14(cc, entry[0], eq, entry[1]);
        }
        PioneerDDJ1000.pot14(0xB6, 0x17 + deck, `[QuickEffectRack1_${group}]`, "super1");
        for (let pad = 8; pad < 16; pad++) {
            PioneerDDJ1000.bind(pads, pad, new components.HotcueButton({
                group: group, number: pad + 1, midi: [pads, pad], on: 5, off: 0
            }), true);
            PioneerDDJ1000.button(pads + 1, pad, group, `hotcue_${pad + 1}_clear`);
        }
        for (let pad = 0; pad < 8; pad++) {
            for (const base of [0x00, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70]) {
                PioneerDDJ1000.button(pads, base + pad, group,
                    `performance_pad_${pad + 1}`, `performance_pad_${pad + 1}_active`);
                PioneerDDJ1000.button(pads + 1, base + pad, group, `performance_pad_${pad + 1}_clear`);
            }
        }
        PioneerDDJ1000.press(0x96, 0x46 + deck, () => {
            if (!engine.getValue(group, "play")) {
                engine.setValue(group, "LoadSelectedTrack", 1);
            }
        });
    }
    for (const status of [0x92, 0x93]) {
        PioneerDDJ1000.press(status, 0x72, () => {
            const side = status - 0x92;
            PioneerDDJ1000.releaseDeck(side);
            PioneerDDJ1000.releaseDeck(side + 2);
        });
    }
    PioneerDDJ1000.pot14(0xB6, 0x1F, "[Master]", "crossfader");
    PioneerDDJ1000.pot14(0xB6, 0x0C, "[Master]", "headMix");
    for (const control of [0x40, 0x64]) {
        PioneerDDJ1000.bind(0xB6, control, {
            input: function(_channel, _control, value) {
                const delta = value < 64 ? value : value - 128;
                if (delta) {
                    engine.setValue("[Library]", "MoveVertical", delta * (control === 0x64 ? 10 : 1));
                }
            }
        });
    }
    PioneerDDJ1000.press(0x96, 0x65, () => engine.setValue("[Library]", "MoveFocusBackward", 1));
    PioneerDDJ1000.button(0x96, 0x7A, "[Master]", "maximize_library", undefined, true);
};

PioneerDDJ1000.input = function(channel, control, value, status) {
    const isNoteOff = (status & 0xF0) === 0x80;
    const message = isNoteOff ? status + 16 : status;
    const normalized = isNoteOff ? 0 : value;
    if (!normalized && message >= 0x97 && message <= 0x9E) {
        const deck = Math.floor((message - 0x97) / 2);
        PioneerDDJ1000.release(held => Math.floor((held.status - 0x97) / 2) === deck && held.control % 8 === control % 8);
    }
    const id = `${message}:${control}`;
    const binding = PioneerDDJ1000.bindings[id];
    if (!binding) {
        return;
    }
    binding.component.input(channel, control, normalized, message);
    if (binding.momentary) {
        if (normalized) {
            PioneerDDJ1000.held[id] = {binding: binding, channel: channel, control: control, status: message};
        } else {
            delete PioneerDDJ1000.held[id];
        }
    }
};

PioneerDDJ1000.shutdown = function() {
    PioneerDDJ1000.release(() => true);
    for (let deck = 1; deck <= 4; deck++) {
        engine.scratchDisable(deck);
    }
    Object.keys(PioneerDDJ1000.bindings).forEach(id => {
        const component = PioneerDDJ1000.bindings[id].component;
        if (component.disconnect) {
            component.disconnect();
        }
    });
    PioneerDDJ1000.modeConnections.forEach(connection => connection.disconnect());
    PioneerDDJ1000.bindings = {};
};
