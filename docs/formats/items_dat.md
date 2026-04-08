# `items.dat`

Binary item table loaded by [`loadItems()`](../../src/items/item.c) and written by `saveItems()`.

| Field | Size |
|-------|------|
| Count | 1 | 1–128 |

Per item:

| Field | Size |
|-------|------|
| Type | 1 |
| SubType | 1 |
| Flags | 1 |
| ModifierType | 1 |
| Modifier | 1 |
| Value | 1 |
| Weight | 1 |
| Icon | 1 |
| UsageType | 1 |
| KeyId | 1 |
| EquipSlot | 1 |
| Name length | 1 | 0–63 |
| Name | *n* | no trailing NUL in file |

Indices are used by inventory and script opcodes (`EVENT_GIVEITEM`, ground items, etc.).
