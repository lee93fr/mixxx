/* AI-generated documentation begins.
 * Simulated MIDI integration tests; a physical RANE ONE is still required.
 * Run from any directory: node --test tools/test_rane_one_mapping.cjs
 * AI-generated documentation ends. */
const { test } = require("node:test");
const assert = require("node:assert/strict");
const fs = require("node:fs");
const path = require("node:path");
const vm = require("node:vm");
const { execFileSync } = require("node:child_process");

const root = path.resolve(__dirname, "..");
const resources = path.join(root, "res/controllers");
const xmlPath = path.join(resources, "RANE ONE.midi.xml");
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
    const engine = {
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
        assert.equal(matches[0].key, "RaneOne.input");
        assert.equal(matches[0].script, true);
        mapping.input(status & 15, control, value, status, matches[0].group);
    };
    return { mapping, send, get, set, sent, connections, takeover, writes };
};

test("every XML address has an initialized handler, including both release formats", () => {
    const h = setup();
    const addresses = new Set();
    for (const b of preset.bindings) {
        const status = (b.status & 0xf0) === 0x80 ? b.status + 16 : b.status;
        const id = `${status}:${b.control}`;
        assert.ok(h.mapping.bindings[id], id);
        addresses.add(id);
    }
    assert.equal(addresses.size, Object.keys(h.mapping.bindings).length);
    h.mapping.shutdown();
});

for (const deck of [0, 1]) {
    const group = `[Channel${deck + 1}]`;
    test(`deck ${deck + 1}: play ignores note-off velocity and does not alter the other deck`, () => {
        const h = setup();
        h.send(0x90 + deck, 0, 127);
        assert.equal(h.get(group, "play"), 1);
        h.send(0x80 + deck, 0, 64);
        assert.equal(h.get(group, "play"), 1);
        h.send(0x90 + deck, 0, 0);
        assert.equal(h.get(group, "play"), 1);
        assert.equal(h.get(`[Channel${2 - deck}]`, "play"), 0);
        h.send(0x90 + deck, 0, 1);
        assert.equal(h.get(group, "play"), 0);
    });

    test(`deck ${deck + 1}: cue, pitch bend and censor release with both MIDI conventions`, () => {
        const h = setup();
        for (const [note, key] of [[1, "cue_default"], [11, "rate_temp_up"],
            [12, "rate_temp_down"], [16, "reverseroll"]]) {
            h.send(0x90 + deck, note, 127);
            assert.equal(h.get(group, key), 1);
            h.send(0x80 + deck, note, 127);
            assert.equal(h.get(group, key), 0);
            h.send(0x90 + deck, note, 1);
            h.send(0x90 + deck, note, 0);
            assert.equal(h.get(group, key), 0);
        }
    });

    test(`deck ${deck + 1}: loading is blocked during playback and allowed when stopped`, () => {
        const h = setup();
        h.set(group, "play", 1);
        h.send(0x90 + deck, 6, 127);
        assert.equal(h.get(group, "LoadSelectedTrack"), 0);
        h.set(group, "play", 0);
        h.send(0x80 + deck, 6, 64);
        assert.equal(h.get(group, "LoadSelectedTrack"), 0);
        h.send(0x90 + deck, 6, 1);
        assert.equal(h.get(group, "LoadSelectedTrack"), 1);
    });

    test(`deck ${deck + 1}: all hotcues, clear notes and LED feedback`, () => {
        const h = setup();
        for (let pad = 0; pad < 8; pad++) {
            const key = `hotcue_${pad + 1}`;
            h.send(0x94 + deck, 20 + pad, 127);
            assert.equal(h.get(group, key + "_activate"), 1);
            h.send(0x84 + deck, 20 + pad, 70);
            assert.equal(h.get(group, key + "_activate"), 0);
            h.send(0x94 + deck, 28 + pad, 127);
            assert.equal(h.get(group, key + "_clear"), 1);
            h.send(0x84 + deck, 28 + pad, 64);
            assert.equal(h.get(group, key + "_clear"), 0);
            h.set(group, key + "_status", 1);
            assert.ok(h.sent.some(b => b[0] === 0x94 + deck && b[1] === 20 + pad && b[2] > 0));
        }
    });

    test(`deck ${deck + 1}: sync off, loop controls, tempo ranges and slip`, () => {
        const h = setup();
        h.send(0x90 + deck, 2, 127);
        assert.equal(h.get(group, "sync_enabled"), 1);
        h.send(0x90 + deck, 3, 127);
        assert.equal(h.get(group, "sync_enabled"), 0);
        for (const [note, key] of [[52, "loop_halve"], [53, "loop_double"],
            [54, "loop_in"], [55, "loop_out"], [63, "beatloop_activate"], [65, "reloop_toggle"]]) {
            h.send(0x94 + deck, note, 1);
            assert.equal(h.get(group, key), 1);
            h.send(0x84 + deck, note, 100);
            assert.equal(h.get(group, key), 0);
        }
        h.set(group, "rateRange", 0.08);
        for (const range of [0.16, 0.5, 0.08]) {
            h.send(0x90 + deck, 43, 127);
            assert.equal(h.get(group, "rateRange"), range);
        }
        h.send(0x94 + deck, 9, 1);
        assert.equal(h.get(group, "slip_enabled"), 1);
    });

    test(`deck ${deck + 1}: pots reach both endpoints and the center with soft takeover`, () => {
        const h = setup();
        const eq = `[EqualizerRack1_${group}_Effect1]`;
        for (const [cc, target, key] of [[22, group, "pregain"], [23, eq, "parameter3"],
            [24, eq, "parameter2"], [25, eq, "parameter1"],
            [26, `[QuickEffectRack1_${group}]`, "super1"], [28, group, "volume"]]) {
            for (const [value, expected] of [[0, 0], [64, 0.5], [127, 1]]) {
                h.send(0xb0 + deck, cc, value);
                assert.equal(h.get(target, key), expected);
            }
            assert.ok(h.takeover.some(x => x[0] === target && x[1] === key && x[2]));
        }
        for (const [value, expected] of [[0, 1], [64, 0.5], [127, 0]]) {
            h.send(0xb0 + deck, 9, value);
            assert.equal(h.get(group, "rate"), expected);
        }
    });
}

test("library encoder decodes centered relative values from either deck", () => {
    const h = setup();
    for (const cc of [0xb0, 0xb1]) {
        h.send(cc, 0, 65);
        assert.equal(h.get("[Library]", "MoveVertical"), 1);
        h.send(cc, 0, 63);
        assert.equal(h.get("[Library]", "MoveVertical"), -1);
        const count = h.writes.length;
        h.send(cc, 0, 64);
        assert.equal(h.writes.length, count);
    }
});

test("crossfader and shared FX depth", () => {
    const h = setup();
    for (const [value, expected] of [[0, 0], [64, 0.5], [127, 1]]) {
        h.send(0xbf, 8, value);
        assert.equal(h.get("[Master]", "crossfader"), expected);
    }
    h.send(0xbf, 4, 127);
    for (const unit of [1, 2]) assert.equal(h.get(`[EffectRack1_EffectUnit${unit}]`, "mix"), 1);
});

test("shutdown releases held controls and disconnects feedback without stopping playback", () => {
    const h = setup();
    h.set("[Channel1]", "play", 1);
    for (const deck of [0, 1]) {
        h.send(0x90 + deck, 1, 127);
        h.send(0x90 + deck, 11, 127);
        h.send(0x90 + deck, 16, 127);
        h.send(0x94 + deck, 20, 127);
    }
    h.mapping.shutdown();
    for (const group of ["[Channel1]", "[Channel2]"]) {
        for (const key of ["cue_default", "rate_temp_up", "reverseroll", "hotcue_1_activate"])
            assert.equal(h.get(group, key), 0);
    }
    assert.equal(h.get("[Channel1]", "play"), 1);
    assert.ok(h.connections.every(c => !c.active));
    const sentCount = h.sent.length;
    h.set("[Channel1]", "play_indicator", 1);
    assert.equal(h.sent.length, sentCount);
    h.mapping.shutdown();
    h.mapping.init();
    h.send(0x91, 0, 1);
    assert.equal(h.get("[Channel2]", "play"), 1);
});
