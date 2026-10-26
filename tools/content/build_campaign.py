#!/usr/bin/env python3
"""Write the three-floor campaign into data/.

Floors are 16x10. The party always enters at (2,2) facing north.
North is decreasing Y. Closed doors are clicked open. The works gate
starts as a wall and becomes an open door when the plate is stepped on.
"""

import os
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
DATA = os.path.join(ROOT, "data")

W, H = 16, 10
WALL, FLOOR, DOOR, DOOR_OPEN, DOOR_LOCKED, TRIGGER = 1, 0, 2, 3, 4, 5

EVENT_SHOWMESSAGE = 8
EVENT_OPENDOOR = 9
EVENT_BATTERY_CHARGER = 221
EVENT_CHANGELEVEL = 222
EVENT_WIN = 33

ITEM_EQUIP = 0x01
ITEM_CONSUMABLE = 0x02
ITEM_STACK = 0x04
ITEM_KEY = 0x08
ITEM_USE = 0x40
USE_HEAL = 1
USE_UNLOCK = 4
MOD_ATTACK = 1
MOD_DEFENSE = 2
DOORLOCK_KEY = 0

# Ground / drop indices.
PIPE, VEST, CELL_KEY, VAULT_KEY, MEDPACK, CORE_KEY = 0, 1, 2, 3, 4, 5
RAT, GUARD, SKELETON = 0, 1, 2


def u16be(n):
    return bytes(((n >> 8) & 0xFF, n & 0xFF))


def path64(s):
    raw = s.encode("ascii")
    if len(raw) >= 64:
        raise SystemExit("path too long: " + s)
    return raw + bytes(64 - len(raw))


def blank():
    grid = [[WALL] * W for _ in range(H)]
    for y in range(1, H - 1):
        for x in range(1, W - 1):
            grid[y][x] = FLOOR
    return grid


def seal(grid, y, x, value):
    for i in range(W):
        grid[y][i] = WALL
    grid[y][x] = value


def setcell(grid, x, y, value):
    grid[y][x] = value


class Maze:
    def __init__(self):
        self.grid = blank()
        self.events = []
        self.strings = []

    def say(self, text):
        idx = len(self.strings)
        self.strings.append(text)
        return idx

    def event(self, x, y, etype, data=b""):
        self.events.append((x, y, etype, bytes(data)))

    def pack(self):
        cells = bytes(self.grid[y][x] for y in range(H) for x in range(W))
        zeros = bytes(W * H)
        ev = bytearray()
        for x, y, etype, data in self.events:
            ev += bytes((x, y, etype, len(data))) + data
        st = bytearray()
        for text in self.strings:
            raw = text.encode("ascii")
            st += u16be(len(raw)) + raw
        return bytes((W, H)) + cells + zeros + zeros + u16be(len(self.events)) + ev + u16be(len(self.strings)) + st


class Level:
    def __init__(self):
        self.locks = []
        self.plates = []
        self.ground = []
        self.monsters = []

    def pack(self):
        out = bytearray(b"LVLE\x01")
        out += bytes((0, 0))  # wall buttons, door buttons
        out.append(len(self.locks))
        for x, y, key_id in self.locks:
            out += bytes((x, y, DOORLOCK_KEY, key_id, 0))
        out.append(len(self.plates))
        for x, y, data in self.plates:
            out += bytes((x, y, EVENT_OPENDOOR, len(data))) + data
        out.append(len(self.ground))
        for x, y, item in self.ground:
            out += bytes((x, y, item, 1))
        out.append(len(self.monsters))
        for type_id, x, y in self.monsters:
            out += bytes((type_id, x, y))
        return bytes(out)


def item(name, flags, mod_type, mod, value, usage, key_id, slot, kind):
    raw = name.encode("ascii")
    return bytes((kind, 0, flags, mod_type, mod, value, 1, 0, usage, key_id, slot, len(raw))) + raw


def monster(name, level, hp, atk, defense, xp, aggro, drop_item, drop_chance):
    drops = bytes(8)
    chances = bytes(8)
    if drop_item:
        drops = bytes((drop_item,)) + bytes(7)
        chances = bytes((drop_chance,)) + bytes(7)
    raw = name.encode("ascii")
    return bytes((level,)) + u16be(hp) + bytes((atk, defense)) + u16be(xp) + bytes((aggro, 0)) + drops + chances + bytes((len(raw),)) + raw


def floor_intake():
    maze = Maze()
    # String 0 is spoken the moment the floor loads.
    maze.say(
        "{c:12}INTAKE{c:8}\n"
        "The lamps are dead. Rust in the air, and claws on the grating ahead.\n"
        "A brass key opens the south lock. The lift is past it."
    )
    hall = maze.say(
        "{c:8}They nested where the pipes were warm.\n"
        "The white case is a medpack. The length of pipe on the floor\n"
        "is the only weapon in the shaft."
    )
    charge = maze.say(
        "{c:14}The wall plate is still live.{c:8}\n"
        "The lamp on your belt drinks from it."
    )
    setcell(maze.grid, 1, 1, TRIGGER)
    setcell(maze.grid, 2, 1, TRIGGER)
    seal(maze.grid, 3, 4, DOOR)
    seal(maze.grid, 6, 7, DOOR_LOCKED)
    setcell(maze.grid, 7, 8, TRIGGER)
    maze.event(1, 1, EVENT_SHOWMESSAGE, bytes((charge,)))
    maze.event(1, 1, EVENT_BATTERY_CHARGER, bytes((80,)))
    maze.event(2, 1, EVENT_SHOWMESSAGE, bytes((hall,)))
    maze.event(7, 8, EVENT_CHANGELEVEL, bytes((1,)))
    lvl = Level()
    lvl.locks.append((7, 6, 1))
    lvl.ground += [(6, 1, MEDPACK), (9, 1, CELL_KEY), (2, 4, PIPE)]
    lvl.monsters.append((RAT, 4, 1))
    return maze, lvl


def floor_works():
    maze = Maze()
    maze.say(
        "{c:12}THE WORKS{c:8}\n"
        "A pale plate in the floor in front of you opens the gate.\n"
        "The blue key is stamped for the lock past it.\n"
        "Something in a dead uniform is still walking the hall."
    )
    hall = maze.say(
        "{c:8}Grease and bone. It kept the vest and the vault key\n"
        "like the shift never ended."
    )
    charge = maze.say(
        "{c:14}Another live plate.{c:8}\n"
        "Take what the dark will give you."
    )
    plate = maze.say(
        "{c:10}The plate sinks.{c:8}\n"
        "Somewhere ahead, a bolt lets go."
    )
    setcell(maze.grid, 1, 1, TRIGGER)
    setcell(maze.grid, 2, 1, TRIGGER)
    setcell(maze.grid, 3, 2, TRIGGER)
    seal(maze.grid, 3, 7, WALL)  # plate turns this wall into an open door
    seal(maze.grid, 6, 7, DOOR_LOCKED)
    setcell(maze.grid, 7, 8, TRIGGER)
    maze.event(1, 1, EVENT_SHOWMESSAGE, bytes((charge,)))
    maze.event(1, 1, EVENT_BATTERY_CHARGER, bytes((80,)))
    maze.event(2, 1, EVENT_SHOWMESSAGE, bytes((hall,)))
    maze.event(3, 2, EVENT_SHOWMESSAGE, bytes((plate,)))
    maze.event(7, 8, EVENT_CHANGELEVEL, bytes((2,)))
    lvl = Level()
    lvl.locks.append((7, 6, 2))
    lvl.plates.append((3, 2, bytes((7, 3))))
    lvl.ground += [(3, 1, MEDPACK), (5, 1, VEST), (6, 1, VAULT_KEY)]
    lvl.monsters.append((SKELETON, 11, 1))
    return maze, lvl


def floor_core():
    maze = Maze()
    maze.say(
        "{c:9}THE CORE{c:8}\n"
        "One guard. The orange key stays in his fist until he drops.\n"
        "Right-click the pipe and the vest in your pack before you go to him."
    )
    hall = maze.say(
        "{c:8}He was posted to keep the last door shut.\n"
        "He is still doing the job."
    )
    charge = maze.say(
        "{c:14}The last plate.{c:8}\n"
        "After this, the dark keeps what it takes."
    )
    win = maze.say(
        "{c:12}The core goes dark.{c:8}\n"
        "For a moment the shaft is only your breathing.\n"
        "Then the lock takes the key, and the night lets you out."
    )
    setcell(maze.grid, 1, 1, TRIGGER)
    setcell(maze.grid, 2, 1, TRIGGER)
    seal(maze.grid, 6, 7, DOOR_LOCKED)
    setcell(maze.grid, 7, 8, TRIGGER)
    maze.event(1, 1, EVENT_SHOWMESSAGE, bytes((charge,)))
    maze.event(1, 1, EVENT_BATTERY_CHARGER, bytes((80,)))
    maze.event(2, 1, EVENT_SHOWMESSAGE, bytes((hall,)))
    maze.event(7, 8, EVENT_SHOWMESSAGE, bytes((win,)))
    maze.event(7, 8, EVENT_WIN, b"")
    lvl = Level()
    lvl.locks.append((7, 6, 3))
    lvl.ground.append((4, 1, MEDPACK))
    lvl.monsters.append((GUARD, 12, 1))
    return maze, lvl


def write_items(path):
    records = [
        item("Rusty pipe", ITEM_EQUIP, MOD_ATTACK, 4, 0, 0, 0, 0, 1),
        item("Scrap vest", ITEM_EQUIP, MOD_DEFENSE, 3, 0, 0, 0, 1, 2),
        item("Cell key", ITEM_KEY, 0, 0, 0, USE_UNLOCK, 1, 255, 3),
        item("Vault key", ITEM_KEY, 0, 0, 0, USE_UNLOCK, 2, 255, 3),
        item("Medpack", ITEM_CONSUMABLE | ITEM_STACK | ITEM_USE, 0, 0, 18, USE_HEAL, 0, 255, 4),
        item("Core key", ITEM_KEY, 0, 0, 0, USE_UNLOCK, 3, 255, 3),
    ]
    blob = bytes((len(records),)) + b"".join(records)
    with open(path, "wb") as f:
        f.write(blob)


def write_monsters(path):
    records = [
        monster("rat", 1, 14, 4, 1, 8, 6, MEDPACK, 40),
        monster("guard", 3, 40, 8, 3, 40, 4, CORE_KEY, 100),
        monster("skeleton", 2, 24, 6, 2, 18, 4, MEDPACK, 60),
    ]
    blob = b"MONS\x01" + bytes((len(records),)) + b"".join(records)
    with open(path, "wb") as f:
        f.write(blob)


def write_manifest(path):
    wall = "data/factory2/factory2.wll"
    pal = "data/playfield.plt"
    blob = bytearray(b"SMTE\x01\x00\x03")
    blob += path64("data/items.dat")
    blob += path64("data/monsters.dat")
    blob += path64(pal)
    for i in range(3):
        blob += path64("data/level%02d.maze" % i)
        blob += path64(wall)
        blob += path64("data/level%02d.lvl" % i)
    with open(path, "wb") as f:
        f.write(blob)


def main():
    os.makedirs(DATA, exist_ok=True)
    floors = [floor_intake(), floor_works(), floor_core()]
    for i, (maze, lvl) in enumerate(floors):
        with open(os.path.join(DATA, "level%02d.maze" % i), "wb") as f:
            f.write(maze.pack())
        with open(os.path.join(DATA, "level%02d.lvl" % i), "wb") as f:
            f.write(lvl.pack())
    write_items(os.path.join(DATA, "items.dat"))
    write_monsters(os.path.join(DATA, "monsters.dat"))
    write_manifest(os.path.join(DATA, "game.smt"))
    print("wrote 3 floors to", DATA)


if __name__ == "__main__":
    main()
