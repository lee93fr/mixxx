/* AI-generated documentation begins.
 * Simulated MIDI integration tests; a physical DDJ-1000 is still required.
 * Run from the repository root: node --test tools/test_ddj_1000_mapping.cjs
 * AI-generated documentation ends. */
const { test } = require("node:test");
const assert = require("node:assert/strict");
const fs = require("node:fs");
const path = require("node:path");
const vm = require("node:vm");
const { execFileSync } = require("node:child_process");

const root = path.resolve(__dirname, "..");
const resources = path.join(root, "res/controllers");
const xmlPath = path.join(resources, "Pioneer-DDJ-1000.midi.xml");
const preset = JSON.parse(execFileSync("python3", ["-c", `
import json, sys, xml.etree.ElementTree as E
r = E.parse(sys.argv[1]).getroot()
c = r.find('controller')
print(json.dumps({
    'prefix': c.find('scriptfiles/file[@functionprefix]').get('functionprefix'),
    'files': [f.get('filename') for f in c.findall('scriptfiles/file')],
    'bindings': [dict(group=x.findtext('group'), key=x.findtext('key'),
                      status=int(x.findtext('status'), 0), control=int(x.findtext('midino'), 0),
                      script=x.find('options/script-binding') is not None)
                 for x in c.findall('controls/control')]
}))
`, xmlPath], { encoding: "utf8" }));

const setup = () => {
    const values = new Map();
    const connections = [];
    const sent = [];
    const takeover = [];
    const writes = [];
    const id = (group, key) => `${group}:${key}`;
    const get = (group, key) => values.get(id(group, key)) || 0;
    const set = (group, key, value) => {
        assert.equal(typeof group, "string");
        assert.equal(typeof key, "string");
        const number = Number(value);
        assert.ok(Number.isFinite(number));
        values.set(id(group, key), number);
        writes.push([group, key, number]);
        for (const c of connections) {
            if (c.active && c.group === group && c.key === key) c.callback(number, group, key);
        }
    };
    const scratches = new Map();
    const ticks = [];
    const engine = {
        scratchEnable: (deck, ...args) => scratches.set(deck, args),
        scratchDisable: deck => scratches.delete(deck),
        isScratching: deck => scratches.has(deck),
        scratchTick: (...args) => ticks.push(args),
        getValue: get, getParameter: get, setValue: set, setParameter: set,
        softTakeover: (group, key, enabled) => takeover.push([group, key, enabled]),
        softTakeoverIgnoreNextValue: () => {},
        makeConnection: (group, key, callback) => {
            const c = { group, key, callback, active: true,
                trigger: () => callback(get(group, key), group, key),
                disconnect() { this.active = false; }
            };
            connections.push(c);
            return c;
        }
    };
    const context = vm.createContext({
        engine, console,
        midi: { sendShortMsg: (...bytes) => sent.push(bytes) }
    });
    for (const file of preset.files) {
        vm.runInContext(fs.readFileSync(path.join(resources, file), "utf8"), context, { filename: file });
    }
    const mapping = context[preset.prefix];
    mapping.init("test", false);
    const send = (status, control, value) => {
        const matches = preset.bindings.filter(b => b.status === status && b.control === control);
        assert.equal(matches.length, 1, `XML dispatch ${status.toString(16)}:${control}`);
        assert.equal(matches[0].key, "PioneerDDJ1000.input");
        assert.equal(matches[0].script, true);
        mapping.input(status & 15, control, value, status, matches[0].group);
    };
    return { mapping, send, get, set, sent, connections, takeover, writes, scratches, ticks };
};

test("preset addresses are unique and match every script handler, including note-off", () => {
    const h = setup();
    const addresses = new Set();
    for (const b of preset.bindings) {
        const address = `${b.status}:${b.control}`;
        assert.ok(!addresses.has(address), address);
        addresses.add(address);
        const status = (b.status & 0xf0) === 0x80 ? b.status + 16 : b.status;
        assert.ok(h.mapping.bindings[`${status}:${b.control}`], address);
    }
    for (const id of Object.keys(h.mapping.bindings)) {
        assert.ok(addresses.has(id), id);
        const [status, control] = id.split(":").map(Number);
        if ((status & 0xf0) === 0x90) assert.ok(addresses.has(`${status - 16}:${control}`));
    }
    h.mapping.shutdown();
});

for (const deck of [0, 1, 2, 3]) {
    const group = `[Channel${deck + 1}]`;
    const note = 0x90 + deck;
    const cc = 0xb0 + deck;
    const pads = 0x97 + deck * 2;
    test(`deck ${deck + 1}: play, cue, sync, master tempo and release velocity`, () => {
        const h = setup();
        h.send(note, 0x0b, 127);
        h.send(note - 16, 0x0b, 64);
        assert.equal(h.get(group, "play"), 1);
        for (const other of [0, 1, 2, 3].filter(d => d !== deck))
            assert.equal(h.get(`[Channel${other + 1}]`, "play"), 0);
        for (const [key, control] of [["cue_default", 0x0c], ["back", 0x65], ["fwd", 0x64]]) {
            h.send(note, control, 127);
            assert.equal(h.get(group, key), 1);
            h.send(note - 16, control, 127);
            assert.equal(h.get(group, key), 0);
        }
        for (const [control, key] of [[0x58, "sync_enabled"], [0x1a, "keylock"],
            [0x54, "pfl"], [0x40, "quantize"], [0x15, "slip_enabled"]]) {
            h.send(note, control, 127);
            h.send(note - 16, control, 127);
            assert.equal(h.get(group, key), 1);
        }
        h.send(note, 0x5c, 127);
        assert.equal(h.get(group, "sync_leader"), 1);
        h.set(group, "rateRange", 0.06);
        for (const range of [0.10, 0.16, 1, 0.06]) {
            h.send(note, 0x60, 127);
            assert.equal(h.get(group, "rateRange"), range);
        }
    });

    test(`deck ${deck + 1}: exact 14-bit endpoints, center, byte order and soft takeover`, () => {
        const h = setup();
        const eq = `[EqualizerRack1_${group}_Effect1]`;
        for (const [status, control, target, key, inverse] of [[cc, 0, group, "rate", true],
            [cc, 4, group, "pregain"], [cc, 0x13, group, "volume"],
            [cc, 7, eq, "parameter3"], [cc, 11, eq, "parameter2"], [cc, 15, eq, "parameter1"],
            [0xb6, 0x17 + deck, `[QuickEffectRack1_${group}]`, "super1"]]) {
            const before = h.writes.length;
            h.send(status, control, 64);
            assert.equal(h.writes.length, before, "no write before both bytes");
            h.send(status, control + 32, 0);
            assert.equal(h.get(target, key), 0.5);
            for (const [raw, expected] of [[0, 0], [8192, 0.5], [16383, 1], [128, 128 / 16384]]) {
                h.send(status, control + 32, raw & 127);
                h.send(status, control, raw >> 7);
                assert.equal(h.get(target, key), inverse ? 1 - expected : expected);
            }
            assert.ok(h.takeover.some(x => x[0] === target && x[1] === key && x[2]));
        }
    });

    test(`deck ${deck + 1}: jog scratch, rim bend, search and touch cleanup`, () => {
        const h = setup();
        h.send(note, 0x36, 127);
        assert.ok(h.scratches.has(deck + 1));
        h.send(cc, 0x22, 65);
        h.send(cc, 0x22, 62);
        assert.deepEqual(h.ticks, [[deck + 1, 1], [deck + 1, -2]]);
        h.send(cc, 0x21, 63);
        assert.equal(h.get(group, "jog"), -0.1);
        h.send(note - 16, 0x36, 70);
        assert.ok(!h.scratches.has(deck + 1));
        h.send(note, 0x36, 127);
        h.send(cc, 0x23, 65);
        assert.ok(!h.scratches.has(deck + 1));
        assert.equal(h.get(group, "jog"), 0.1);
        h.send(cc, 0x1f, 66);
        assert.equal(h.get(group, "jog"), 4);
        h.send(note, 0x67, 127);
        assert.ok(!h.scratches.has(deck + 1));
        h.send(cc, 0x22, 65);
        assert.ok(h.scratches.has(deck + 1));
        h.send(note, 0x3f, 127);
        assert.ok(!h.scratches.has(deck + 1));
    });

    test(`deck ${deck + 1}: hotcue pages, shifted clear, mode changes and feedback`, () => {
        const h = setup();
        for (let pad = 0; pad < 16; pad++) {
            const key = pad < 8 ? `performance_pad_${pad + 1}` : `hotcue_${pad + 1}_activate`;
            h.send(pads, pad, 127);
            assert.equal(h.get(group, key), 1);
            h.send(pads - 16, pad, 64);
            assert.equal(h.get(group, key), 0);
            h.send(pads + 1, pad, 127);
            assert.equal(h.get(group, pad < 8 ? key + "_clear" : `hotcue_${pad + 1}_clear`), 1);
            h.send(pads - 15, pad, 100);
            assert.equal(h.get(group, pad < 8 ? key + "_clear" : `hotcue_${pad + 1}_clear`), 0);
            h.set(group, pad < 8 ? key + "_active" : `hotcue_${pad + 1}_status`, 1);
            assert.ok(h.sent.some(b => b[0] === pads && b[1] === pad && b[2] > 0));
        }
        h.send(pads, 0, 127);
        h.send(note, 0x20, 127);
        assert.equal(h.get(group, "performance_pad_1"), 0);
        h.send(pads, 0, 127);
        h.send(pads - 16, 0x20, 50);
        assert.equal(h.get(group, "performance_pad_1"), 0);
        h.send(pads, 8, 127);
        h.send(note, 0x24, 127);
        assert.equal(h.get(group, "hotcue_9_activate"), 0);
    });

    test(`deck ${deck + 1}: loop editing, beat jump, sampler and orientation switch releases`, () => {
        const h = setup();
        for (const [active, expected] of [[0, ["loop_in", "loop_out"]], [1, ["loop_halve", "loop_double"]]]) {
            h.set(group, "loop_enabled", active);
            for (const [i, control] of [0x10, 0x11].entries()) {
                h.send(note, control, 127);
                assert.ok(h.writes.some(w => w[0] === group && w[1] === expected[i] && w[2] === 1));
                assert.equal(h.get(group, expected[i]), 0);
            }
        }
        h.send(note, 0x14, 127);
        assert.equal(h.get(group, "beatloop_4_toggle"), 1);
        h.send(note - 16, 0x14, 90);
        assert.equal(h.get(group, "beatloop_4_toggle"), 0);
        for (const [control, mode] of [[0x1b, 0], [0x1e, 1], [0x20, 5], [0x22, 3],
            [0x69, 6], [0x6b, 2], [0x6d, 4], [0x6f, 5]]) {
            h.send(note, control, 127);
            assert.equal(h.get(group, "pad_mode"), mode);
            for (let pad = 0; pad < 8; pad++) {
                for (const base of [0, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70]) {
                    const key = `performance_pad_${pad + 1}`;
                    h.send(pads, base + pad, 127);
                    assert.equal(h.get(group, key), 1);
                    h.send(pads - 16, base + pad, 90);
                    assert.equal(h.get(group, key), 0);
                    h.send(pads + 1, base + pad, 127);
                    assert.equal(h.get(group, key + "_clear"), 1);
                    h.send(pads - 15, base + pad, 90);
                    assert.equal(h.get(group, key + "_clear"), 0);
                }
            }
        }
        h.send(pads, 0, 127);
        h.set(group, "pad_mode", 2);
        assert.equal(h.get(group, "performance_pad_1"), 0);
        for (const [control, orientation] of [[0x16, 0], [0x1d, 1], [0x18, 2]]) {
            h.send(note, control, 127);
            h.send(note - 16, control, 70);
            assert.equal(h.get(group, "orientation"), orientation);
        }
    });
}

test("browser uses signed relative counts; load is guarded on all four decks", () => {
    const h = setup();
    for (const [control, value, expected] of [[0x40, 1, 1], [0x40, 127, -1],
        [0x40, 98, -30], [0x64, 2, 20], [0x64, 126, -20]]) {
        h.send(0xb6, control, value);
        assert.equal(h.get("[Library]", "MoveVertical"), expected);
    }
    for (let deck = 0; deck < 4; deck++) {
        const group = `[Channel${deck + 1}]`;
        h.set(group, "play", 1);
        h.send(0x96, 0x46 + deck, 127);
        assert.equal(h.get(group, "LoadSelectedTrack"), 0);
        h.set(group, "play", 0);
        h.send(0x86, 0x46 + deck, 127);
        assert.equal(h.get(group, "LoadSelectedTrack"), 0);
        h.send(0x96, 0x46 + deck, 127);
        assert.equal(h.get(group, "LoadSelectedTrack"), 1);
    }
    for (const [control, key] of [[0x1f, "crossfader"], [0x0c, "headMix"]]) {
        h.send(0xb6, control, 127);
        h.send(0xb6, control + 32, 127);
        assert.equal(h.get("[Master]", key), 1);
    }
});

test("deck switches release both decks on that side; shutdown disconnects and preserves playback", () => {
    const h = setup();
    h.set("[Channel1]", "play", 1);
    for (let deck = 0; deck < 4; deck++) {
        h.send(0x90 + deck, 0x0c, 127);
        h.send(0x90 + deck, 0x36, 127);
        h.send(0x97 + deck * 2, 0, 127);
    }
    h.send(0x92, 0x72, 127);
    for (const deck of [1, 3]) {
        assert.equal(h.get(`[Channel${deck}]`, "cue_default"), 0);
        assert.equal(h.get(`[Channel${deck}]`, "performance_pad_1"), 0);
        assert.ok(!h.scratches.has(deck));
    }
    assert.ok(h.scratches.has(2));
    h.mapping.shutdown();
    h.mapping.shutdown();
    assert.equal(h.scratches.size, 0);
    assert.ok(h.connections.every(c => !c.active));
    assert.equal(h.get("[Channel1]", "play"), 1);
    for (const deck of [2, 4]) assert.equal(h.get(`[Channel${deck}]`, "cue_default"), 0);
    h.mapping.init();
    h.send(0x93, 0x0b, 127);
    assert.equal(h.get("[Channel4]", "play"), 1);
});
