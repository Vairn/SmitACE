# Maze format (`.dat`, `MAZE.DAT`)

Binary layout written by [`mazeSave()`](../../src/maze/maze.c) and read by
[`mazeLoad()`](../../src/maze/maze.c).

**Multi-byte fields (`UWORD` counts / string lengths) are big-endian on
disk.** These are written/read with `fileWriteWords` / `fileReadWords`,
which swap on a little-endian host. All other fields are single bytes or
raw byte arrays and use the `Bytes` file functions.

```
| Field                  | Size        | Endianness |
|------------------------|-------------|------------|
| Width                  | 1           | —          |
| Height                 | 1           | —          |
| Maze data              | W * H       | raw bytes  |
| Maze col               | W * H       | raw bytes  |
| Maze floor             | W * H       | raw bytes  |
| Event count            | 2 (UWORD)   | **big-endian** |
| Events                 | per event   | see below  |
| String count           | 2 (UWORD)   | **big-endian** |
| Strings                | per string  | see below |

Per event (event count ×):

| Field         | Size   |
|---------------|--------|
| X             | 1 byte |
| Y             | 1 byte |
| Event type    | 1 byte |
| Event data sz | 1 byte |
| Event data    | `eventDataSize` bytes (0 allowed) |

Per string (string count ×):

| Field  | Size      | Endianness            |
|--------|-----------|-----------------------|
| Length | 2 (UWORD) | **big-endian**        |
| String | `length` bytes | raw ASCII, no NUL |

> The demo maze (`mazeCreateDemoData()`) is generated in memory and never
> serialized; the file format above is only used when levels bundle a
> `.maze` asset via the manifest (`level.mazePath`).