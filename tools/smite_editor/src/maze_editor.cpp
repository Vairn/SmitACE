#include "maze_editor.hpp"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <imgui.h>
#include <imgui_internal.h>
#include <misc/cpp/imgui_stdlib.h>

#include <algorithm>
#include <cstdio>
#include <string>

namespace {

std::string joinPath(const std::string &a, const std::string &b)
{
	if (a.empty())
		return b;
	if (a.back() == '/' || a.back() == '\\')
		return a + b;
	return a + "/" + b;
}

enum Brush : int {
	BrushFloor = 0,
	BrushWall = 1,
	BrushDoor = 2,
	BrushOpen = 3,
	BrushLocked = 4,
	BrushTrigger = 5
};

enum LevelMode : int {
	ModeView = 0,
	ModeEdit = 1,
	ModeDraw = 2
};

struct EvName {
	int id;
	const char *name;
};

int clampByte(int v)
{
	if (v < 0)
		return 0;
	if (v > 255)
		return 255;
	return v;
}

void drawIfCondition(MazeEvent &ev);

const EvName kEvents[] = {
	{0, "Set wall"},
	{1, "Set floor"},
	{2, "Set color"},
	{3, "Clear wall"},
	{4, "Clear floor"},
	{5, "Clear color"},
	{6, "Set wall color"},
	{7, "Clear wall color"},
	{8, "Show message"},
	{9, "Open door"},
	{10, "Close door"},
	{17, "Teleport"},
	{18, "Give item"},
	{19, "Take item"},
	{20, "Set flag"},
	{21, "Clear flag"},
	{22, "Add monster"},
	{23, "Remove monster"},
	{24, "Add item"},
	{25, "Remove item"},
	{26, "Add XP"},
	{27, "Damage"},
	{28, "Launcher"},
	{29, "Turn"},
	{30, "Identify items"},
	{31, "Encounter"},
	{32, "Sound"},
	{33, "Win"},
	{128, "If"},
	{129, "Else"},
	{130, "Endif"},
	{131, "Goto"},
	{132, "Gosub"},
	{133, "Return"},
	{221, "Battery charger"},
	{222, "Change level"},
};

/*
 * Typed event fields.
 *
 * Events are still stored as raw bytes (the .maze format is unchanged), but the
 * editor now knows what each byte means so it can show a real widget instead of
 * eight anonymous number boxes. min/max clamp the value; items/monsters/levels
 * are sentinel bounds meaning "use the loaded table length".
 */
enum FieldKind : int {
	FieldByte,      /* plain 0..255 */
	FieldItem,      /* index into items.dat */
	FieldMonster,   /* index into monsters.dat */
	FieldLevel,     /* index into game.smt level list */
	FieldCoordX,    /* maze column */
	FieldCoordY,    /* maze row */
	FieldDirection, /* 0 = left, 1 = right */
	FieldFlagType,  /* 0 = local, 1 = global */
	FieldMessage,   /* index into the maze string table */
	FieldWallType,  /* maze cell type 0..5 */
};

struct EvField {
	const char *label;
	FieldKind kind;
	bool optional; /* omitted when the trailing value is zero/none */
};

struct EvSchema {
	int id;
	const EvField *fields;
	int count;
};

#define EVF(label, kind) { label, kind, false }
#define EVF_OPT(label, kind) { label, kind, true }

/* Set wall/floor/color: one wall/floor/column index. */
const EvField kF_SetWall[] = {EVF("Wall type", FieldWallType)};
const EvField kF_SetIndex[] = {EVF("Index", FieldByte)};
/* Show message: maze string index. */
const EvField kF_Message[] = {EVF("Message", FieldMessage)};
/* Door: 0 bytes = at this cell, or x/y. */
const EvField kF_Door[] = {EVF_OPT("Door X", FieldCoordX), EVF_OPT("Door Y", FieldCoordY)};
/* Teleport target. */
const EvField kF_Teleport[] = {EVF("Target X", FieldCoordX), EVF("Target Y", FieldCoordY)};
/* Give/take item + quantity. */
const EvField kF_Item[] = {EVF("Item", FieldItem), EVF_OPT("Quantity", FieldByte)};
const EvField kF_FlagSet[] = {EVF("Scope", FieldFlagType), EVF("Flag index", FieldByte), EVF_OPT("Value", FieldByte)};
const EvField kF_FlagClear[] = {EVF("Scope", FieldFlagType), EVF("Flag index", FieldByte)};
/* Add monster: type + spawn cell. */
const EvField kF_AddMonster[] = {EVF("Monster", FieldMonster), EVF("X", FieldCoordX), EVF("Y", FieldCoordY)};
const EvField kF_Position[] = {EVF("X", FieldCoordX), EVF("Y", FieldCoordY)};
/* XP is little-endian across two bytes. */
const EvField kF_Xp[] = {EVF("XP", FieldByte), EVF_OPT("XP high", FieldByte)};
const EvField kF_Damage[] = {EVF("Damage", FieldByte)};
const EvField kF_Turn[] = {EVF("Direction", FieldDirection), EVF("Count", FieldByte)};
const EvField kF_Id[] = {EVF("Id", FieldByte)};
const EvField kF_Charge[] = {EVF("Charges", FieldByte)};
const EvField kF_ChangeLevel[] = {EVF("Level", FieldLevel)};
const EvField kF_Ordinal[] = {EVF("Target ordinal", FieldByte)};
/* IF condition: [operator, operand, a, b?]. Laid out by hand below. */
const EvField kF_If[] = {EVF("Condition", FieldByte), EVF("Operand", FieldByte),
	EVF("A / X", FieldByte), EVF("B / Y", FieldByte), EVF("Compare to", FieldByte)};

const EvSchema kSchemas[] = {
	{0, kF_SetWall, 1}, {1, kF_SetIndex, 1}, {2, kF_SetIndex, 1},
	{6, kF_SetWall, 1},
	{8, kF_Message, 1},
	{9, kF_Door, 2},
	{17, kF_Teleport, 2},
	{18, kF_Item, 2}, {19, kF_Item, 2},
	{20, kF_FlagSet, 3}, {21, kF_FlagClear, 2},
	{22, kF_AddMonster, 3}, {23, kF_Position, 2},
	{24, kF_Item, 2}, {25, kF_Item, 2},
	{26, kF_Xp, 2},
	{27, kF_Damage, 1},
	{29, kF_Turn, 2},
	{30, kF_Id, 1}, {31, kF_Id, 1}, {32, kF_Id, 1},
	{131, kF_Ordinal, 1}, {132, kF_Ordinal, 1},
	{221, kF_Charge, 1},
	{222, kF_ChangeLevel, 1},
	{128, kF_If, 5},
	{208, kF_Ordinal, 1},
};

const EvSchema *findSchema(int id)
{
	/* IF and ELSE_GOTO are laid out by drawIfCondition() because their bytes
	 * are a nested expression, not a flat field list. */
	if (id == 128 || id == 208)
		return nullptr;
	for (const EvSchema &s : kSchemas) {
		if (s.id == id)
			return &s;
	}
	return nullptr;
}

/* Condition operators (byte 0 of an IF payload). */
const EvName kCondOps[] = {
	{220, "="}, {219, "<>"}, {217, ">"}, {218, "<"},
};
/* Condition operands (byte 1). Bit-width differs per operand; see script.c. */
const EvName kCondOperands[] = {
	{209, "Level flag"}, {210, "Global flag"}, {211, "Party at X,Y"},
	{207, "Party direction"}, {202, "Party has class"}, {203, "Party has race"},
	{200, "Party visible"}, {201, "Roll dice"},
	{212, "Monsters at X,Y"}, {213, "Items at X,Y"}, {214, "Wall number at X,Y"},
};

const char *condName(const EvName *tbl, int n, int id, char *fallback, size_t fbLen)
{
	for (int i = 0; i < n; i++) {
		if (tbl[i].id == id)
			return tbl[i].name;
	}
	snprintf(fallback, fbLen, "Type %d", id);
	return fallback;
}

/** Operand byte count by sensor, mirroring conditionOperandBytes() in script.c. */
int condOperandBytes(int operand)
{
	if (operand == 212 || operand == 213 || operand == 214)
		return 3; /* x, y, compare */
	return 2;     /* value, compare */
}

/** Bounds for a field, so the editor can clamp and label it. */
struct FieldBounds {
	int min;
	int max;
};

FieldBounds fieldBounds(FieldKind kind, const MazeFile &maze,
	const std::vector<ItemRow> &items, const std::vector<MonsterRow> &monsters,
	int levelCount, const MazeFile *stringsMaze)
{
	FieldBounds b{0, 255};
	switch (kind) {
	case FieldItem:
		b.max = items.empty() ? 255 : (int)items.size() - 1;
		break;
	case FieldMonster:
		b.max = monsters.empty() ? 255 : (int)monsters.size() - 1;
		break;
	case FieldLevel:
		b.max = (levelCount > 0) ? levelCount - 1 : 255;
		break;
	case FieldCoordX:
		b.max = (maze.width > 0) ? maze.width - 1 : 255;
		break;
	case FieldCoordY:
		b.max = (maze.height > 0) ? maze.height - 1 : 255;
		break;
	case FieldDirection:
		b.max = 1;
		break;
	case FieldFlagType:
		b.max = 1;
		break;
	case FieldMessage:
		if (stringsMaze && !stringsMaze->strings.empty())
			b.max = (int)stringsMaze->strings.size() - 1;
		break;
	case FieldWallType:
		b.max = 5;
		break;
	case FieldByte:
	default:
		break;
	}
	(void)maze;
	return b;
}

const char *kBrush[] = {"Floor", "Wall", "Door", "Open", "Locked", "Trigger"};
const char *kMode[] = {"View", "Edit", "Draw"};
const char *kSide[] = {"North", "East", "South", "West"};

const char *eventName(int id)
{
	for (const EvName &e : kEvents) {
		if (e.id == id)
			return e.name;
	}
	return nullptr;
}

/**
 * Ensure ev.data has exactly `count` bytes so a field change can write safely.
 * Extra bytes are kept (never silently dropped) so hand-authored payloads survive.
 */
void ensureEventSize(MazeEvent &ev, int count)
{
	while ((int)ev.data.size() < count)
		ev.data.push_back(0);
}

/**
 * IF payload layout (see evaluateCondition in script.c):
 *   [operator][operand][a][b?][compare][AND/OR][next condition...]
 * where a/b is 1 byte for simple operands and 2 bytes (x,y) for cell sensors.
 * This editor handles the first condition only; extra chained conditions are
 * left in place and editable as raw bytes so nothing is silently dropped.
 */
void drawIfCondition(MazeEvent &ev)
{
	while (ev.data.size() < 5)
		ev.data.push_back(0);

	int op = ev.data[0];
	int operand = ev.data[1];
	int a = ev.data[2];
	int b = ev.data[3];
	int cmp = ev.data[4];
	char fb[32];

	ImGui::SetNextItemWidth(-1.0f);
	if (ImGui::BeginCombo("##op", condName(kCondOps, IM_ARRAYSIZE(kCondOps), op, fb, sizeof fb))) {
		for (const EvName &n : kCondOps) {
			if (ImGui::Selectable(n.name, op == n.id))
				ev.data[0] = (unsigned char)n.id;
		}
		ImGui::EndCombo();
	}

	op = ev.data[0];
	ImGui::SetNextItemWidth(-1.0f);
	if (ImGui::BeginCombo("##operand", condName(kCondOperands, IM_ARRAYSIZE(kCondOperands), operand, fb, sizeof fb))) {
		for (const EvName &n : kCondOperands) {
			if (ImGui::Selectable(n.name, operand == n.id)) {
				ev.data[1] = (unsigned char)n.id;
				operand = n.id;
			}
		}
		ImGui::EndCombo();
	}

	/* Cell sensors read two coordinates before the compare value. */
	bool cellSensor = (operand == 212 || operand == 213 || operand == 214);
	if (cellSensor) {
		ImGui::SetNextItemWidth(64.0f);
		if (ImGui::InputInt(cellSensor && operand == 214 ? "X" : "X", &a, 0, 0))
			ev.data[2] = (unsigned char)clampByte(a);
		ImGui::SameLine();
		ImGui::SetNextItemWidth(64.0f);
		if (ImGui::InputInt("Y", &b, 0, 0))
			ev.data[3] = (unsigned char)clampByte(b);
	} else {
		ImGui::SetNextItemWidth(64.0f);
		if (ImGui::InputInt("Value", &a, 0, 0))
			ev.data[2] = (unsigned char)clampByte(a);
	}

	/* Compare target sits after the operand. For 3-byte sensors that is byte 4;
	 * for 2-byte operands it is byte 3, so the compare fields differ. */
	int cmpIndex = 1 + condOperandBytes(operand);
	while ((int)ev.data.size() <= cmpIndex)
		ev.data.push_back(0);
	ImGui::SetNextItemWidth(64.0f);
	int cmpVal = ev.data[cmpIndex];
	if (ImGui::InputInt(cellSensor ? "Match count" : "Compare to", &cmpVal, 0, 0))
		ev.data[cmpIndex] = (unsigned char)clampByte(cmpVal);

	if (cellSensor) {
		ImGui::TextDisabled("Cells: value at X,Y is compared to the count.");
	}
}

/** Human label for an item/monster index, falling back to the number. */
void drawEventFields(MazeEvent &ev, const MazeFile &maze,
	const std::vector<ItemRow> &items, const std::vector<MonsterRow> &monsters,
	int levelCount, const MazeFile *stringsMaze)
{
	if (ev.type == 128) {
		drawIfCondition(ev);
		return;
	}

	const EvSchema *schema = findSchema(ev.type);
	if (!schema) {
		/* Unknown opcode: keep the raw byte editor so nothing becomes unreachable. */
		for (size_t b = 0; b < ev.data.size(); b++) {
			if (b % 4 != 0)
				ImGui::SameLine();
			int v = ev.data[b];
			ImGui::PushID((int)b);
			ImGui::SetNextItemWidth(52.0f);
			if (ImGui::InputInt("##byte", &v, 0, 0))
				ev.data[b] = (unsigned char)clampByte(v);
			ImGui::PopID();
		}
		if (ImGui::SmallButton("Byte"))
			ev.data.push_back(0);
		ImGui::SameLine();
		return;
	}

	ensureEventSize(ev, schema->count);
	for (int f = 0; f < schema->count; f++) {
		const EvField &field = schema->fields[f];
		FieldBounds bounds = fieldBounds(field.kind, maze, items, monsters, levelCount, stringsMaze);
		int value = ev.data[f];
		ImGui::PushID(f);

		switch (field.kind) {
		case FieldItem: {
			const char *cur = (value < (int)items.size()) ? items[value].name.c_str() : "(none)";
			ImGui::SetNextItemWidth(-1.0f);
			if (ImGui::BeginCombo(field.label, cur)) {
				for (int i = 0; i < (int)items.size(); i++) {
					char label[96];
					snprintf(label, sizeof label, "%d  %s", i, items[i].name.c_str());
					if (ImGui::Selectable(label, value == i))
						ev.data[f] = (unsigned char)clampByte(i);
				}
				ImGui::EndCombo();
			}
			break;
		}
		case FieldMonster: {
			const char *cur = (value < (int)monsters.size()) ? monsters[value].name.c_str() : "(none)";
			ImGui::SetNextItemWidth(-1.0f);
			if (ImGui::BeginCombo(field.label, cur)) {
				for (int i = 0; i < (int)monsters.size(); i++) {
					char label[96];
					snprintf(label, sizeof label, "%d  %s", i, monsters[i].name.c_str());
					if (ImGui::Selectable(label, value == i))
						ev.data[f] = (unsigned char)clampByte(i);
				}
				ImGui::EndCombo();
			}
			break;
		}
		case FieldMessage: {
			if (stringsMaze && !stringsMaze->strings.empty()) {
				std::string cur = "(none)";
				if (value < (int)stringsMaze->strings.size()) {
					const auto &s = stringsMaze->strings[value];
					cur.assign(s.begin(), s.end());
					if (cur.size() > 40)
						cur = cur.substr(0, 37) + "...";
				}
				ImGui::SetNextItemWidth(-1.0f);
				if (ImGui::BeginCombo(field.label, cur.c_str())) {
					for (int i = 0; i < (int)stringsMaze->strings.size(); i++) {
						const auto &s = stringsMaze->strings[i];
						std::string txt(s.begin(), s.end());
						char label[256];
						snprintf(label, sizeof label, "%d  %s", i, txt.c_str());
						if (ImGui::Selectable(label, value == i))
							ev.data[f] = (unsigned char)clampByte(i);
					}
					ImGui::EndCombo();
				}
			} else {
				ImGui::SetNextItemWidth(64.0f);
				if (ImGui::InputInt(field.label, &value, 0, 0))
					ev.data[f] = (unsigned char)clampByte(value);
			}
			break;
		}
		case FieldWallType: {
			ImGui::SetNextItemWidth(-1.0f);
			if (ImGui::BeginCombo(field.label, kBrush[value <= 5 ? value : 0])) {
				for (int i = 0; i <= 5; i++) {
					if (ImGui::Selectable(kBrush[i], value == i))
						ev.data[f] = (unsigned char)i;
				}
				ImGui::EndCombo();
			}
			break;
		}
		case FieldDirection: {
			ImGui::SetNextItemWidth(-1.0f);
			if (ImGui::BeginCombo(field.label, value ? "Right" : "Left")) {
				if (ImGui::Selectable("Left", value == 0))
					ev.data[f] = 0;
				if (ImGui::Selectable("Right", value != 0))
					ev.data[f] = 1;
				ImGui::EndCombo();
			}
			break;
		}
		case FieldFlagType: {
			ImGui::SetNextItemWidth(-1.0f);
			if (ImGui::BeginCombo(field.label, value ? "Global" : "Local")) {
				if (ImGui::Selectable("Local", value == 0))
					ev.data[f] = 0;
				if (ImGui::Selectable("Global", value != 0))
					ev.data[f] = 1;
				ImGui::EndCombo();
			}
			break;
		}
		case FieldLevel: {
			ImGui::SetNextItemWidth(64.0f);
			if (ImGui::InputInt(field.label, &value, 0, 0)) {
				if (value < bounds.min) value = bounds.min;
				if (value > bounds.max) value = bounds.max;
				ev.data[f] = (unsigned char)clampByte(value);
			}
			break;
		}
		default: {
			ImGui::SetNextItemWidth(64.0f);
			if (ImGui::InputInt(field.label, &value, 0, 0)) {
				if (value < bounds.min) value = bounds.min;
				if (value > bounds.max) value = bounds.max;
				ev.data[f] = (unsigned char)clampByte(value);
			}
			break;
		}
		}
		ImGui::PopID();
	}
}

void resizeMaze(MazeFile &m, int nw, int nh)
{
	nw = clampByte(nw);
	nh = clampByte(nh);
	if (nw < 1)
		nw = 1;
	if (nh < 1)
		nh = 1;
	std::vector<unsigned char> cells((size_t)nw * nh, 1);
	std::vector<unsigned char> cols((size_t)nw * nh, 0);
	std::vector<unsigned char> floors((size_t)nw * nh, 0);
	int cw = (std::min)(m.width, nw);
	int ch = (std::min)(m.height, nh);
	for (int y = 0; y < ch; y++) {
		for (int x = 0; x < cw; x++) {
			size_t si = (size_t)y * (size_t)m.width + (size_t)x;
			size_t di = (size_t)y * (size_t)nw + (size_t)x;
			if (si < m.cells.size())
				cells[di] = m.cells[si];
			if (si < m.cols.size())
				cols[di] = m.cols[si];
			if (si < m.floors.size())
				floors[di] = m.floors[si];
		}
	}
	m.width = nw;
	m.height = nh;
	m.cells.swap(cells);
	m.cols.swap(cols);
	m.floors.swap(floors);
}

bool layersMatch(const MazeFile &m)
{
	size_t n = (size_t)m.width * (size_t)m.height;
	return m.width > 0 && m.height > 0 && m.cells.size() == n && m.cols.size() == n && m.floors.size() == n;
}

ImU32 cellFill(int type, int col, int floor)
{
	int tint = (col * 23) & 31;
	int ft = (floor * 17) & 24;
	switch (type) {
	case BrushWall:
		return IM_COL32(138 + tint, 142 + tint / 2, 154, 255);
	case BrushDoor:
		return IM_COL32(186, 154, 78, 255);
	case BrushOpen:
		return IM_COL32(48 + ft, 56 + ft, 52, 255);
	case BrushLocked:
		return IM_COL32(168, 84, 72, 255);
	case BrushTrigger:
		return IM_COL32(58, 72, 120, 255);
	default:
		return IM_COL32(32 + ft, 34 + ft / 2, 42, 255);
	}
}

void drawCellGlyph(ImDrawList *dl, int type, ImVec2 tl, float cell)
{
	float inset = (std::max)(2.0f, cell * 0.18f);
	ImVec2 a(tl.x + inset, tl.y + inset);
	ImVec2 b(tl.x + cell - inset, tl.y + cell - inset);
	if (type == BrushOpen)
		dl->AddRect(a, b, IM_COL32(186, 154, 78, 255), 0.0f, 0, (std::max)(1.0f, cell * 0.08f));
	else if (type == BrushLocked) {
		float y = (a.y + b.y) * 0.5f;
		dl->AddLine(ImVec2(a.x, y), ImVec2(b.x, y), IM_COL32(40, 24, 22, 255), (std::max)(1.0f, cell * 0.1f));
	} else if (type == BrushTrigger) {
		ImVec2 c((tl.x + tl.x + cell) * 0.5f, (tl.y + tl.y + cell) * 0.5f);
		float r = cell * 0.16f;
		dl->AddQuadFilled(ImVec2(c.x, c.y - r), ImVec2(c.x + r, c.y), ImVec2(c.x, c.y + r), ImVec2(c.x - r, c.y),
			IM_COL32(129, 161, 255, 255));
	}
}

void paintCell(MazeFile &m, int x, int y, int type, bool stampIdx, int wallIdx, int floorIdx, int &selX, int &selY)
{
	if (x < 0 || y < 0 || x >= m.width || y >= m.height)
		return;
	size_t i = (size_t)y * (size_t)m.width + (size_t)x;
	if (i >= m.cells.size())
		return;
	if (type >= 0)
		m.cells[i] = (unsigned char)type;
	if (stampIdx) {
		m.cols[i] = (unsigned char)clampByte(wallIdx);
		m.floors[i] = (unsigned char)clampByte(floorIdx);
	}
	selX = x;
	selY = y;
}

void paintRect(MazeFile &m, int x0, int y0, int x1, int y1, int type, bool stampIdx, int wallIdx, int floorIdx, int &selX, int &selY)
{
	if (x0 > x1)
		std::swap(x0, x1);
	if (y0 > y1)
		std::swap(y0, y1);
	for (int y = y0; y <= y1; y++) {
		for (int x = x0; x <= x1; x++)
			paintCell(m, x, y, type, stampIdx, wallIdx, floorIdx, selX, selY);
	}
}

const char *monsterLabel(const std::vector<MonsterRow> &monsters, int type)
{
	static char buf[96];
	if (type >= 0 && type < (int)monsters.size() && !monsters[type].name.empty())
		snprintf(buf, sizeof buf, "%d  %s", type, monsters[type].name.c_str());
	else
		snprintf(buf, sizeof buf, "Type %d", type);
	return buf;
}

const char *itemLabel(const std::vector<ItemRow> &items, int type)
{
	static char buf[96];
	if (type >= 0 && type < (int)items.size() && !items[type].name.empty())
		snprintf(buf, sizeof buf, "%d  %s", type, items[type].name.c_str());
	else
		snprintf(buf, sizeof buf, "Item %d", type);
	return buf;
}

struct Snap {
	MazeFile maze;
	LvlEntities lvl;
};

struct Hist {
	std::vector<Snap> undo;
	std::vector<Snap> redo;
	Snap committed;
	bool have = false;
	bool applying = false;
	int level = -2;
};

bool sameEvents(const std::vector<MazeEvent> &a, const std::vector<MazeEvent> &b)
{
	if (a.size() != b.size())
		return false;
	for (size_t i = 0; i < a.size(); i++) {
		if (a[i].x != b[i].x || a[i].y != b[i].y || a[i].type != b[i].type || a[i].data != b[i].data)
			return false;
	}
	return true;
}

bool sameLvl(const LvlEntities &a, const LvlEntities &b)
{
	if (a.wallBtns.size() != b.wallBtns.size() || a.doorBtns.size() != b.doorBtns.size()
		|| a.locks.size() != b.locks.size() || a.plates.size() != b.plates.size()
		|| a.ground.size() != b.ground.size() || a.monsters.size() != b.monsters.size())
		return false;
	for (size_t i = 0; i < a.wallBtns.size(); i++) {
		const auto &x = a.wallBtns[i];
		const auto &y = b.wallBtns[i];
		if (x.x != y.x || x.y != y.y || x.wallSide != y.wallSide || x.type != y.type || x.gfx != y.gfx
			|| x.evt != y.evt || x.esz != y.esz || x.ed != y.ed)
			return false;
	}
	for (size_t i = 0; i < a.doorBtns.size(); i++) {
		const auto &x = a.doorBtns[i];
		const auto &y = b.doorBtns[i];
		if (x.x != y.x || x.y != y.y || x.wallSide != y.wallSide || x.type != y.type || x.gfx != y.gfx
			|| x.tx != y.tx || x.ty != y.ty)
			return false;
	}
	for (size_t i = 0; i < a.locks.size(); i++) {
		const auto &x = a.locks[i];
		const auto &y = b.locks[i];
		if (x.doorX != y.doorX || x.doorY != y.doorY || x.type != y.type || x.keyId != y.keyId || x.code != y.code)
			return false;
	}
	for (size_t i = 0; i < a.plates.size(); i++) {
		const auto &x = a.plates[i];
		const auto &y = b.plates[i];
		if (x.x != y.x || x.y != y.y || x.evt != y.evt || x.dsz != y.dsz)
			return false;
		for (int d = 0; d < 8; d++) {
			if (x.d[d] != y.d[d])
				return false;
		}
	}
	for (size_t i = 0; i < a.ground.size(); i++) {
		const auto &x = a.ground[i];
		const auto &y = b.ground[i];
		if (x.x != y.x || x.y != y.y || x.item != y.item || x.qty != y.qty)
			return false;
	}
	for (size_t i = 0; i < a.monsters.size(); i++) {
		const auto &x = a.monsters[i];
		const auto &y = b.monsters[i];
		if (x.type != y.type || x.x != y.x || x.y != y.y)
			return false;
	}
	return true;
}

bool sameDoc(const Snap &s, const MazeFile &m, const LvlEntities &l)
{
	return s.maze.width == m.width && s.maze.height == m.height
		&& s.maze.cells == m.cells && s.maze.cols == m.cols && s.maze.floors == m.floors
		&& s.maze.strings == m.strings && sameEvents(s.maze.events, m.events)
		&& sameLvl(s.lvl, l);
}

void historyReset(Hist &h, const MazeFile &maze, const LvlEntities &lvl, int level)
{
	h.undo.clear();
	h.redo.clear();
	h.committed.maze = maze;
	h.committed.lvl = lvl;
	h.have = true;
	h.level = level;
	h.applying = false;
}

void historySync(Hist &h, const MazeFile &maze, const LvlEntities &lvl, int level, bool hold)
{
	if (!h.have || h.level != level) {
		historyReset(h, maze, lvl, level);
		return;
	}
	if (h.applying) {
		h.committed.maze = maze;
		h.committed.lvl = lvl;
		h.applying = false;
		return;
	}
	if (hold)
		return;
	if (sameDoc(h.committed, maze, lvl))
		return;
	h.undo.push_back(h.committed);
	if (h.undo.size() > 64)
		h.undo.erase(h.undo.begin());
	h.committed.maze = maze;
	h.committed.lvl = lvl;
	h.redo.clear();
}

void historyUndo(Hist &h, MazeFile &maze, LvlEntities &lvl)
{
	if (h.undo.empty())
		return;
	h.redo.push_back(Snap{maze, lvl});
	h.committed = h.undo.back();
	h.undo.pop_back();
	maze = h.committed.maze;
	lvl = h.committed.lvl;
	h.applying = true;
}

void historyRedo(Hist &h, MazeFile &maze, LvlEntities &lvl)
{
	if (h.redo.empty())
		return;
	h.undo.push_back(Snap{maze, lvl});
	h.committed = h.redo.back();
	h.redo.pop_back();
	maze = h.committed.maze;
	lvl = h.committed.lvl;
	h.applying = true;
}

void drawInspector(MazeFile &maze, LvlEntities &lvl, const std::vector<MonsterRow> &monsters, const std::vector<ItemRow> &items, int levelCount, int selX, int selY, bool editable)
{
	ImGui::BeginChild("maze_inspector", ImVec2(320.0f, -1.0f), ImGuiChildFlags_Borders);
	if (selX < 0 || selY < 0 || selX >= maze.width || selY >= maze.height || !layersMatch(maze)) {
		ImGui::TextDisabled("Click a cell.");
		ImGui::EndChild();
		return;
	}

	size_t i = (size_t)selY * (size_t)maze.width + (size_t)selX;
	ImGui::Text("Cell %d, %d", selX, selY);
	ImGui::BeginDisabled(!editable);
	int type = maze.cells[i];
	const char *typeName = (type >= 0 && type <= 5) ? kBrush[type] : "Unknown";
	if (ImGui::BeginCombo("Type", typeName)) {
		for (int t = 0; t <= 5; t++) {
			if (ImGui::Selectable(kBrush[t], type == t))
				maze.cells[i] = (unsigned char)t;
		}
		ImGui::EndCombo();
	}
	int wall = maze.cols[i];
	int floor = maze.floors[i];
	ImGui::SetNextItemWidth(-1.0f);
	if (ImGui::InputInt("Wall index", &wall))
		maze.cols[i] = (unsigned char)clampByte(wall);
	ImGui::SetNextItemWidth(-1.0f);
	if (ImGui::InputInt("Floor index", &floor))
		maze.floors[i] = (unsigned char)clampByte(floor);

	ImGui::SeparatorText("Events");
	int removeEvent = -1;
	int shown = 0;
	for (size_t e = 0; e < maze.events.size(); e++) {
		MazeEvent &ev = maze.events[e];
		if (ev.x != selX || ev.y != selY)
			continue;
		shown++;
		ImGui::PushID((int)e + 1);
		const char *nm = eventName(ev.type);
		char fallback[32];
		if (!nm) {
			snprintf(fallback, sizeof fallback, "Type %d", ev.type);
			nm = fallback;
		}
		if (ImGui::BeginCombo("##ev", nm)) {
			for (const EvName &name : kEvents) {
				if (ImGui::Selectable(name.name, ev.type == name.id))
					ev.type = name.id;
			}
			ImGui::EndCombo();
		}
		drawEventFields(ev, maze, items, monsters, levelCount, &maze);
		ImGui::SameLine();
		if (!ev.data.empty()) {
			if (ImGui::SmallButton("Drop byte"))
				ev.data.pop_back();
			ImGui::SameLine();
		}
		if (ImGui::SmallButton("Remove"))
			removeEvent = (int)e;
		ImGui::PopID();
	}
	if (shown == 0)
		ImGui::TextDisabled("No events here.");
	if (removeEvent >= 0)
		maze.events.erase(maze.events.begin() + removeEvent);
	if (ImGui::Button("Add event")) {
		MazeEvent ev;
		ev.x = selX;
		ev.y = selY;
		ev.type = 8;
		maze.events.push_back(std::move(ev));
	}

	ImGui::SeparatorText("On this cell");
	int removeMon = -1, removeItem = -1, removePlate = -1, removeWall = -1, removeDoor = -1, removeLock = -1;
	for (size_t n = 0; n < lvl.monsters.size(); n++) {
		if (lvl.monsters[n].x != selX || lvl.monsters[n].y != selY)
			continue;
		ImGui::PushID((int)n + 1000);
		ImGui::Text("Monster  %s", monsterLabel(monsters, lvl.monsters[n].type));
		ImGui::SameLine();
		if (ImGui::SmallButton("x"))
			removeMon = (int)n;
		ImGui::PopID();
	}
	for (size_t n = 0; n < lvl.ground.size(); n++) {
		if (lvl.ground[n].x != selX || lvl.ground[n].y != selY)
			continue;
		ImGui::PushID((int)n + 2000);
		ImGui::Text("Item  %s x%d", itemLabel(items, lvl.ground[n].item), lvl.ground[n].qty);
		ImGui::SameLine();
		if (ImGui::SmallButton("x"))
			removeItem = (int)n;
		ImGui::PopID();
	}
	for (size_t n = 0; n < lvl.plates.size(); n++) {
		if (lvl.plates[n].x != selX || lvl.plates[n].y != selY)
			continue;
		ImGui::PushID((int)n + 3000);
		const char *nm = eventName(lvl.plates[n].evt);
		ImGui::Text("Plate  %s", nm ? nm : "event");
		ImGui::SameLine();
		if (ImGui::SmallButton("x"))
			removePlate = (int)n;
		ImGui::PopID();
	}
	for (size_t n = 0; n < lvl.wallBtns.size(); n++) {
		if (lvl.wallBtns[n].x != selX || lvl.wallBtns[n].y != selY)
			continue;
		ImGui::PushID((int)n + 4000);
		int side = lvl.wallBtns[n].wallSide;
		ImGui::Text("Wall button  %s", (side >= 0 && side < 4) ? kSide[side] : "?");
		ImGui::SameLine();
		if (ImGui::SmallButton("x"))
			removeWall = (int)n;
		ImGui::PopID();
	}
	for (size_t n = 0; n < lvl.doorBtns.size(); n++) {
		if (lvl.doorBtns[n].x != selX || lvl.doorBtns[n].y != selY)
			continue;
		ImGui::PushID((int)n + 5000);
		ImGui::Text("Door button  -> %d,%d", lvl.doorBtns[n].tx, lvl.doorBtns[n].ty);
		ImGui::SameLine();
		if (ImGui::SmallButton("x"))
			removeDoor = (int)n;
		ImGui::PopID();
	}
	for (size_t n = 0; n < lvl.locks.size(); n++) {
		if (lvl.locks[n].doorX != selX || lvl.locks[n].doorY != selY)
			continue;
		ImGui::PushID((int)n + 6000);
		ImGui::Text("Lock  key %d", lvl.locks[n].keyId);
		ImGui::SameLine();
		if (ImGui::SmallButton("x"))
			removeLock = (int)n;
		ImGui::PopID();
	}
	if (removeMon >= 0)
		lvl.monsters.erase(lvl.monsters.begin() + removeMon);
	if (removeItem >= 0)
		lvl.ground.erase(lvl.ground.begin() + removeItem);
	if (removePlate >= 0)
		lvl.plates.erase(lvl.plates.begin() + removePlate);
	if (removeWall >= 0)
		lvl.wallBtns.erase(lvl.wallBtns.begin() + removeWall);
	if (removeDoor >= 0)
		lvl.doorBtns.erase(lvl.doorBtns.begin() + removeDoor);
	if (removeLock >= 0)
		lvl.locks.erase(lvl.locks.begin() + removeLock);

	static int addMon = 0, addItem = 0, addQty = 1, addSide = 0, addKey = 0, addPlate = 8, addTx = 0, addTy = 0;
	ImGui::SetNextItemWidth(-1.0f);
	if (ImGui::BeginCombo("##addmon", monsterLabel(monsters, addMon))) {
		int n = (int)monsters.size();
		if (n < 1)
			n = 1;
		for (int t = 0; t < n; t++) {
			if (ImGui::Selectable(monsterLabel(monsters, t), addMon == t))
				addMon = t;
		}
		ImGui::EndCombo();
	}
	if (ImGui::Button("Place monster")) {
		LvlEntities::MS m{};
		m.type = addMon;
		m.x = selX;
		m.y = selY;
		lvl.monsters.push_back(m);
	}
	ImGui::SetNextItemWidth(-1.0f);
	if (ImGui::BeginCombo("##additem", itemLabel(items, addItem))) {
		int n = (int)items.size();
		if (n < 1)
			n = 1;
		for (int t = 0; t < n; t++) {
			if (ImGui::Selectable(itemLabel(items, t), addItem == t))
				addItem = t;
		}
		ImGui::EndCombo();
	}
	ImGui::SetNextItemWidth(80.0f);
	ImGui::InputInt("Qty", &addQty);
	if (addQty < 1)
		addQty = 1;
	if (ImGui::Button("Place item")) {
		LvlEntities::GI g{};
		g.x = selX;
		g.y = selY;
		g.item = addItem;
		g.qty = addQty;
		lvl.ground.push_back(g);
	}
	ImGui::SetNextItemWidth(-1.0f);
	if (ImGui::BeginCombo("Side", kSide[addSide])) {
		for (int s = 0; s < 4; s++) {
			if (ImGui::Selectable(kSide[s], addSide == s))
				addSide = s;
		}
		ImGui::EndCombo();
	}
	if (ImGui::Button("Wall button")) {
		LvlEntities::WB w{};
		w.x = selX;
		w.y = selY;
		w.wallSide = addSide;
		w.gfx = 255;
		lvl.wallBtns.push_back(w);
	}
	ImGui::SetNextItemWidth(70.0f);
	ImGui::InputInt("Door x", &addTx);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(70.0f);
	ImGui::InputInt("Door y", &addTy);
	if (ImGui::Button("Door button")) {
		LvlEntities::DB d{};
		d.x = selX;
		d.y = selY;
		d.wallSide = addSide;
		d.gfx = 254;
		d.tx = clampByte(addTx);
		d.ty = clampByte(addTy);
		lvl.doorBtns.push_back(d);
	}
	ImGui::SetNextItemWidth(80.0f);
	ImGui::InputInt("Key", &addKey);
	if (ImGui::Button("Lock door")) {
		LvlEntities::Lk k{};
		k.doorX = selX;
		k.doorY = selY;
		k.keyId = clampByte(addKey);
		lvl.locks.push_back(k);
	}
	const char *plateName = eventName(addPlate);
	char plateFallback[32];
	if (!plateName) {
		snprintf(plateFallback, sizeof plateFallback, "Type %d", addPlate);
		plateName = plateFallback;
	}
	if (ImGui::BeginCombo("Plate", plateName)) {
		for (const EvName &name : kEvents) {
			if (ImGui::Selectable(name.name, addPlate == name.id))
				addPlate = name.id;
		}
		ImGui::EndCombo();
	}
	if (ImGui::Button("Place plate")) {
		LvlEntities::Pl p{};
		p.x = selX;
		p.y = selY;
		p.evt = addPlate;
		lvl.plates.push_back(p);
	}

	if (ImGui::CollapsingHeader("Strings")) {
		if (ImGui::Button("Add string"))
			maze.strings.emplace_back();
		int removeString = -1;
		for (size_t s = 0; s < maze.strings.size(); s++) {
			ImGui::PushID((int)s + 8000);
			std::string text(maze.strings[s].begin(), maze.strings[s].end());
			char label[24];
			snprintf(label, sizeof label, "%zu", s);
			if (ImGui::InputText(label, &text))
				maze.strings[s].assign(text.begin(), text.end());
			ImGui::SameLine();
			if (ImGui::SmallButton("x"))
				removeString = (int)s;
			ImGui::PopID();
		}
		if (removeString >= 0)
			maze.strings.erase(maze.strings.begin() + removeString);
	}
	ImGui::EndDisabled();
	ImGui::EndChild();
}

std::string defaultLevelFile(const std::string &root, int level, const char *ext)
{
	char name[32];
	snprintf(name, sizeof name, "level%02d.%s", level, ext);
	return joinPath(root, name);
}

void noteErr(char *err, int errBytes, const std::string &er)
{
	if (err && errBytes > 0)
		snprintf(err, errBytes, "%s", er.c_str());
}

} // namespace

bool loadLevelFromManifest(const GameManifest &man, int level, MazeFile &maze, LvlEntities &lvl, std::string &err)
{
	maze = MazeFile();
	lvl = LvlEntities();
	if (level < 0 || level >= (int)man.levels.size()) {
		err = "No level in game.smt";
		return false;
	}
	const GameManifest::Level &lv = man.levels[level];
	if (lv.mazePath.empty())
		loadBuiltinDemoMaze(maze);
	else if (!loadMaze(lv.mazePath, maze, err))
		return false;
	if (!lv.entitiesPath.empty() && !loadLvl(lv.entitiesPath, lvl, err))
		return false;
	return true;
}

bool saveMazeToManifest(GameManifest &man, int level, const MazeFile &maze, const std::string &root, std::string &err)
{
	if (level < 0 || level >= (int)man.levels.size()) {
		err = "No level in game.smt";
		return false;
	}
	GameManifest::Level &lv = man.levels[level];
	bool invented = lv.mazePath.empty();
	if (invented)
		lv.mazePath = defaultLevelFile(root, level, "maze");
	if (!saveMaze(lv.mazePath, maze, err))
		return false;
	if (invented && !saveGameManifest(joinPath(root, "game.smt"), man, err))
		return false;
	return true;
}

bool saveEntitiesToManifest(GameManifest &man, int level, const LvlEntities &lvl, const std::string &root, std::string &err)
{
	if (level < 0 || level >= (int)man.levels.size()) {
		err = "No level in game.smt";
		return false;
	}
	GameManifest::Level &lv = man.levels[level];
	bool invented = lv.entitiesPath.empty();
	if (invented)
		lv.entitiesPath = defaultLevelFile(root, level, "lvl");
	if (!saveLvl(lv.entitiesPath, lvl, err))
		return false;
	if (invented && !saveGameManifest(joinPath(root, "game.smt"), man, err))
		return false;
	return true;
}

const DungeonSheet *sheetFor(const std::vector<DungeonSheet> &sheets, int setIndex)
{
	const DungeonSheet *fallback = nullptr;
	for (const DungeonSheet &s : sheets) {
		if (!s.tex || s.w <= 0 || s.h <= 0)
			continue;
		if (!fallback)
			fallback = &s;
		if (s.index == setIndex)
			return &s;
	}
	if (setIndex == 0)
		return fallback;
	return nullptr;
}

bool tileVisible(int cell, int type)
{
	if (cell == type)
		return true;
	return type == 0 && (cell == 2 || cell == 3 || cell == 4 || cell == 5);
}

bool canStand(const MazeFile &maze, int x, int y)
{
	if (x < 0 || y < 0 || x >= maze.width || y >= maze.height || !layersMatch(maze))
		return false;
	unsigned char c = maze.cells[(size_t)y * (size_t)maze.width + (size_t)x];
	return c == BrushFloor || c == BrushOpen || c == BrushTrigger;
}

void stepFacing(const MazeFile &maze, int face, int &camX, int &camY)
{
	int dx = 0;
	int dy = 0;
	face &= 3;
	if (face == 0)
		dy = -1;
	else if (face == 1)
		dx = 1;
	else if (face == 2)
		dy = 1;
	else
		dx = -1;
	int nx = camX + dx;
	int ny = camY + dy;
	if (canStand(maze, nx, ny)) {
		camX = nx;
		camY = ny;
	}
}

bool clickDoorAhead(MazeFile &maze, const WallsetFile &wallset, int camX, int camY, int facing, float lx, float ly)
{
	static const int kSlot[18][2] = {
		{-3, -3}, {-2, -3}, {-1, -3}, {3, -3}, {2, -3}, {1, -3}, {0, -3},
		{-2, -2}, {-1, -2}, {2, -2}, {1, -2}, {0, -2},
		{-1, -1}, {1, -1}, {0, -1},
		{-1, 0}, {1, 0}, {0, 0},
	};
	static const int kFace[4][2] = {{1, 1}, {-1, 1}, {-1, -1}, {1, -1}};
	if (!layersMatch(maze))
		return false;
	facing &= 3;
	int dx = 0;
	int dy = 0;
	if (facing == 0)
		dy = -1;
	else if (facing == 1)
		dx = 1;
	else if (facing == 2)
		dy = 1;
	else
		dx = -1;
	int doorX = camX + dx;
	int doorY = camY + dy;
	if (doorX < 0 || doorY < 0 || doorX >= maze.width || doorY >= maze.height)
		return false;
	size_t idx = (size_t)doorY * (size_t)maze.width + (size_t)doorX;
	unsigned char cell = maze.cells[idx];
	if (cell != BrushDoor && cell != BrushOpen && cell != BrushLocked)
		return false;
	int xs = kFace[facing][0];
	int ys = kFace[facing][1];
	bool even = (facing % 2) == 0;
	int tx = 0;
	int ty = 0;
	bool slotFound = false;
	for (int slot = 0; slot < 18; slot++) {
		int sx = kSlot[slot][0];
		int sy = kSlot[slot][1];
		int mx = even ? xs * sx : xs * sy;
		int my = even ? ys * sy : ys * sx;
		if (camX + mx == doorX && camY + my == doorY) {
			tx = sx;
			ty = sy;
			slotFound = true;
			break;
		}
	}
	if (!slotFound)
		return false;
	bool inside = false;
	for (const WallsetFile::Tile &tile : wallset.tiles) {
		if ((int)(signed char)tile.loc[0] != tx || (int)(signed char)tile.loc[1] != ty)
			continue;
		if (tile.type != BrushDoor && tile.type != BrushLocked)
			continue;
		if (tile.w == 0 || tile.h == 0)
			continue;
		if (lx >= (float)tile.screen[0] && ly >= (float)tile.screen[1]
			&& lx < (float)tile.screen[0] + (float)tile.w
			&& ly < (float)tile.screen[1] + (float)tile.h)
			inside = true;
	}
	if (!inside)
		return false;
	maze.cells[idx] = (cell == BrushOpen) ? (unsigned char)BrushDoor : (unsigned char)BrushOpen;
	return true;
}

void drawDungeonView(MazeFile &maze, const WallsetFile &wallset, const std::vector<DungeonSheet> &sheets,
	int camX, int camY, int facing, float scale)
{
	static const int kSlot[18][2] = {
		{-3, -3}, {-2, -3}, {-1, -3}, {3, -3}, {2, -3}, {1, -3}, {0, -3},
		{-2, -2}, {-1, -2}, {2, -2}, {1, -2}, {0, -2},
		{-1, -1}, {1, -1}, {0, -1},
		{-1, 0}, {1, 0}, {0, 0},
	};
	static const int kFace[4][2] = {{1, 1}, {-1, 1}, {-1, -1}, {1, -1}};
	const float viewW = 240.0f * scale;
	const float viewH = 180.0f * scale;
	ImVec2 origin = ImGui::GetCursorScreenPos();
	ImDrawList *dl = ImGui::GetWindowDrawList();
	ImU32 clear = IM_COL32(8, 8, 12, 255);
	if (wallset.palette.size() >= 3)
		clear = IM_COL32(wallset.palette[0], wallset.palette[1], wallset.palette[2], 255);
	dl->AddRectFilled(origin, ImVec2(origin.x + viewW, origin.y + viewH), clear);
	dl->PushClipRect(origin, ImVec2(origin.x + viewW, origin.y + viewH), true);
	if (facing < 0 || facing > 3)
		facing = 0;
	int xs = kFace[facing][0];
	int ys = kFace[facing][1];
	bool even = (facing % 2) == 0;
	if (layersMatch(maze)) {
		for (int slot = 0; slot < 18; slot++) {
			int tx = kSlot[slot][0];
			int ty = kSlot[slot][1];
			int mx = even ? xs * tx : xs * ty;
			int my = even ? ys * ty : ys * tx;
			int cx = camX + mx;
			int cy = camY + my;
			if (cx < 0 || cy < 0 || cx >= maze.width || cy >= maze.height)
				continue;
			int cell = maze.cells[(size_t)cy * (size_t)maze.width + (size_t)cx];
			for (const WallsetFile::Tile &tile : wallset.tiles) {
				if ((int)(signed char)tile.loc[0] != tx || (int)(signed char)tile.loc[1] != ty)
					continue;
				if (!tileVisible(cell, tile.type))
					continue;
				const DungeonSheet *sheet = sheetFor(sheets, tile.setIndex);
				if (!sheet || tile.w == 0 || tile.h == 0)
					continue;
				float u0 = (float)tile.x / (float)sheet->w;
				float v0 = (float)tile.y / (float)sheet->h;
				float u1 = (float)(tile.x + tile.w) / (float)sheet->w;
				float v1 = (float)(tile.y + tile.h) / (float)sheet->h;
				ImVec2 p0(origin.x + (float)tile.screen[0] * scale, origin.y + (float)tile.screen[1] * scale);
				ImVec2 p1(p0.x + (float)tile.w * scale, p0.y + (float)tile.h * scale);
				dl->AddImage((ImTextureID)(intptr_t)sheet->tex, p0, p1, ImVec2(u0, v0), ImVec2(u1, v1));
			}
		}
	}
	dl->PopClipRect();
	dl->AddRect(origin, ImVec2(origin.x + viewW, origin.y + viewH), IM_COL32(255, 255, 255, 40));
	ImGui::InvisibleButton("dungeon_view", ImVec2(viewW, viewH));
	if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && scale > 0.0f) {
		ImVec2 m = ImGui::GetIO().MousePos;
		clickDoorAhead(maze, wallset, camX, camY, facing, (m.x - origin.x) / scale, (m.y - origin.y) / scale);
	}
}

void drawMazeEditor(MazeFile &maze, LvlEntities &lvl,
	const std::vector<MonsterRow> &monsters,
	const std::vector<ItemRow> &items,
	const WallsetFile &wallset,
	const std::vector<DungeonSheet> &sheets,
	GameManifest &man, int levelIndex,
	const std::string &root,
	const std::function<void()> &reloadLevel,
	char *err, int errBytes)
{
	if (!ImGui::Begin("Maze (level01.maze)")) {
		ImGui::End();
		return;
	}

	static int editW = 16;
	static int editH = 16;
	static int syncedW = -1;
	static int syncedH = -1;
	static int mode = ModeDraw;
	static int brush = BrushWall;
	static int camX = 1;
	static int camY = 1;
	static int camFace = 0;
	static Hist hist;
	static bool stroke = false;
	static float cell = 22.0f;
	static bool stampIdx = false;
	static int stampWall = 0;
	static int stampFloor = 0;
	static int selX = 0;
	static int selY = 0;
	static bool rectDrag = false;
	static int anchorX = 0;
	static int anchorY = 0;

	if (maze.width != syncedW || maze.height != syncedH) {
		editW = maze.width > 0 ? maze.width : 1;
		editH = maze.height > 0 ? maze.height : 1;
		syncedW = maze.width;
		syncedH = maze.height;
		if (selX >= maze.width)
			selX = 0;
		if (selY >= maze.height)
			selY = 0;
	}

	const GameManifest::Level *lv = nullptr;
	if (levelIndex >= 0 && levelIndex < (int)man.levels.size())
		lv = &man.levels[levelIndex];
	if (!lv) {
		ImGui::TextDisabled("No level in game.smt.");
	} else if (lv->mazePath.empty()) {
		ImGui::TextDisabled("Level %d uses the built-in demo maze. Save writes %s and points game.smt at it.",
			levelIndex, defaultLevelFile(root, levelIndex, "maze").c_str());
	} else {
		ImGui::Text("Level %d maze: %s", levelIndex, lv->mazePath.c_str());
	}
	if (lv)
		ImGui::Text("Entities: %s    Wallset: %s",
			lv->entitiesPath.empty() ? "(none)" : lv->entitiesPath.c_str(),
			lv->wallsetPath.empty() ? "(default factory2)" : lv->wallsetPath.c_str());
	if (ImGui::Button("Reload from game")) {
		if (reloadLevel)
			reloadLevel();
		else {
			std::string er;
			if (!loadLevelFromManifest(man, levelIndex, maze, lvl, er))
				noteErr(err, errBytes, er);
			else
				noteErr(err, errBytes, "");
		}
		hist.have = false;
	}
	ImGui::SameLine();
	if (ImGui::Button("Save maze")) {
		std::string er;
		if (!saveMazeToManifest(man, levelIndex, maze, root, er))
			noteErr(err, errBytes, er);
		else
			noteErr(err, errBytes, "");
	}
	ImGui::SameLine();
	if (ImGui::Button("Save entities")) {
		std::string er;
		if (!saveEntitiesToManifest(man, levelIndex, lvl, root, er))
			noteErr(err, errBytes, er);
		else
			noteErr(err, errBytes, "");
	}

	for (int m = 0; m < 3; m++) {
		if (m)
			ImGui::SameLine();
		bool on = mode == m;
		if (on)
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(129 / 255.0f, 161 / 255.0f, 255 / 255.0f, 0.45f));
		if (ImGui::Button(kMode[m]))
			mode = m;
		if (on)
			ImGui::PopStyleColor();
	}
	ImGui::SameLine();
	ImGui::BeginDisabled(hist.undo.empty());
	if (ImGui::Button("Undo"))
		historyUndo(hist, maze, lvl);
	ImGui::EndDisabled();
	ImGui::SameLine();
	ImGui::BeginDisabled(hist.redo.empty());
	if (ImGui::Button("Redo"))
		historyRedo(hist, maze, lvl);
	ImGui::EndDisabled();

	ImGui::BeginDisabled(mode == ModeView);
	ImGui::SetNextItemWidth(80.0f);
	ImGui::InputInt("W", &editW);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(80.0f);
	ImGui::InputInt("H", &editH);
	ImGui::SameLine();
	if (ImGui::Button("Resize")) {
		resizeMaze(maze, editW, editH);
		syncedW = maze.width;
		syncedH = maze.height;
		editW = maze.width;
		editH = maze.height;
	}
	ImGui::EndDisabled();
	ImGui::SameLine();
	ImGui::SetNextItemWidth(80.0f);
	ImGui::SliderFloat("Zoom", &cell, 12.0f, 40.0f, "%.0f");
	if (cell < 12.0f)
		cell = 12.0f;

	if (brush > BrushTrigger)
		brush = BrushWall;
	if (mode == ModeDraw) {
		for (int b = 0; b <= BrushTrigger; b++) {
			if (b)
				ImGui::SameLine();
			bool on = brush == b;
			if (on)
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(129 / 255.0f, 161 / 255.0f, 255 / 255.0f, 0.45f));
			if (ImGui::Button(kBrush[b]))
				brush = b;
			if (on)
				ImGui::PopStyleColor();
		}
		ImGui::SameLine();
		ImGui::Checkbox("Stamp indexes", &stampIdx);
		if (stampIdx) {
			ImGui::SameLine();
			ImGui::SetNextItemWidth(70.0f);
			ImGui::InputInt("##sw", &stampWall);
			ImGui::SameLine();
			ImGui::SetNextItemWidth(70.0f);
			ImGui::InputInt("##sf", &stampFloor);
			stampWall = clampByte(stampWall);
			stampFloor = clampByte(stampFloor);
		}
	}

	bool keys = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && !ImGui::GetIO().WantTextInput;
	if (keys) {
		ImGuiID moveOwner = ImGui::GetID("dungeon_move");
		const ImGuiKey moveKeys[] = {
			ImGuiKey_W, ImGuiKey_A, ImGuiKey_S, ImGuiKey_D, ImGuiKey_Q, ImGuiKey_E,
			ImGuiKey_UpArrow, ImGuiKey_DownArrow, ImGuiKey_LeftArrow, ImGuiKey_RightArrow,
		};
		for (ImGuiKey key : moveKeys)
			ImGui::SetKeyOwner(key, moveOwner);
		if (ImGui::IsKeyPressed(ImGuiKey_W) || ImGui::IsKeyPressed(ImGuiKey_UpArrow))
			stepFacing(maze, camFace, camX, camY);
		if (ImGui::IsKeyPressed(ImGuiKey_S) || ImGui::IsKeyPressed(ImGuiKey_DownArrow))
			stepFacing(maze, camFace + 2, camX, camY);
		if (ImGui::IsKeyPressed(ImGuiKey_A) || ImGui::IsKeyPressed(ImGuiKey_LeftArrow))
			stepFacing(maze, camFace + 3, camX, camY);
		if (ImGui::IsKeyPressed(ImGuiKey_D) || ImGui::IsKeyPressed(ImGuiKey_RightArrow))
			stepFacing(maze, camFace + 1, camX, camY);
		if (ImGui::IsKeyPressed(ImGuiKey_Q, false))
			camFace = (camFace + 3) & 3;
		if (ImGui::IsKeyPressed(ImGuiKey_E, false))
			camFace = (camFace + 1) & 3;
		if (mode == ModeDraw) {
			for (int k = 0; k <= BrushTrigger; k++) {
				if (ImGui::IsKeyPressed((ImGuiKey)(ImGuiKey_1 + k), false))
					brush = k;
			}
		}
		if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_Z) && !ImGui::IsKeyDown(ImGuiKey_LeftShift) && !ImGui::IsKeyDown(ImGuiKey_RightShift))
			historyUndo(hist, maze, lvl);
		if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_Y) || ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_Z))
			historyRedo(hist, maze, lvl);
	}

	if (mode == ModeView)
		ImGui::TextDisabled("View. Click a floor cell to stand there. W A S D and the arrows walk. Q and E turn.");
	else if (mode == ModeEdit)
		ImGui::TextDisabled("Edit. Click a cell, then change it in the side panel. W A S D and the arrows walk. Q and E turn.");
	else
		ImGui::TextDisabled("Draw. Drag paints. Right-drag lays floor. Shift-drag fills a rectangle. Keys 1-6 pick the brush. W A S D and the arrows walk.");

	if (!layersMatch(maze)) {
		ImGui::TextDisabled("Load a maze, or set a size and press Resize.");
		historySync(hist, maze, lvl, levelIndex, false);
		ImGui::End();
		return;
	}

	drawInspector(maze, lvl, monsters, items, man.levelCount, selX, selY, mode != ModeView);
	ImGui::SameLine();

	ImGui::BeginChild("maze_right", ImVec2(0.0f, -1.0f), ImGuiChildFlags_None);
	{
		static const char *kFaceName[] = {"North", "East", "South", "West"};
		if (camX < 0 || camY < 0 || camX >= maze.width || camY >= maze.height) {
			camX = 1;
			camY = 1;
		}
		if (ImGui::Button("Forward"))
			stepFacing(maze, camFace, camX, camY);
		ImGui::SameLine();
		if (ImGui::Button("Back"))
			stepFacing(maze, camFace + 2, camX, camY);
		ImGui::SameLine();
		if (ImGui::Button("Strafe left"))
			stepFacing(maze, camFace + 3, camX, camY);
		ImGui::SameLine();
		if (ImGui::Button("Strafe right"))
			stepFacing(maze, camFace + 1, camX, camY);
		ImGui::SameLine();
		if (ImGui::Button("Turn left"))
			camFace = (camFace + 3) & 3;
		ImGui::SameLine();
		if (ImGui::Button("Turn right"))
			camFace = (camFace + 1) & 3;
		ImGui::SameLine();
		ImGui::Text("%d, %d  %s", camX, camY, kFaceName[camFace & 3]);
		ImGui::SameLine();
		ImGui::TextDisabled("Click the door ahead to open or close it.");
		float avail = ImGui::GetContentRegionAvail().x;
		float availH = ImGui::GetContentRegionAvail().y;
		float scale = avail > 0.0f ? avail / 240.0f : 1.0f;
		if (availH > 80.0f) {
			float byH = (availH * 0.55f) / 180.0f;
			if (scale > byH)
				scale = byH;
		}
		if (scale > 2.0f)
			scale = 2.0f;
		if (scale < 0.75f)
			scale = 0.75f;
		if (wallset.tiles.empty() || sheets.empty())
			ImGui::TextDisabled("The dungeon view uses the level's wallset planes.");
		drawDungeonView(maze, wallset, sheets, camX, camY, camFace, scale);
	}

	ImGui::BeginChild("maze_canvas", ImVec2(0.0f, -1.0f), ImGuiChildFlags_None);
	ImVec2 origin = ImGui::GetCursorScreenPos();
	float gridW = cell * (float)maze.width;
	float gridH = cell * (float)maze.height;
	ImDrawList *dl = ImGui::GetWindowDrawList();
	float gap = cell >= 16.0f ? 1.0f : 0.0f;

	std::vector<unsigned char> mark(maze.cells.size(), 0);
	for (const MazeEvent &ev : maze.events) {
		if (ev.x >= 0 && ev.y >= 0 && ev.x < maze.width && ev.y < maze.height)
			mark[(size_t)ev.y * (size_t)maze.width + (size_t)ev.x] |= 1;
	}
	for (const auto &m : lvl.monsters) {
		if (m.x >= 0 && m.y >= 0 && m.x < maze.width && m.y < maze.height)
			mark[(size_t)m.y * (size_t)maze.width + (size_t)m.x] |= 2;
	}
	for (const auto &g : lvl.ground) {
		if (g.x >= 0 && g.y >= 0 && g.x < maze.width && g.y < maze.height)
			mark[(size_t)g.y * (size_t)maze.width + (size_t)g.x] |= 4;
	}
	for (const auto &p : lvl.plates) {
		if (p.x >= 0 && p.y >= 0 && p.x < maze.width && p.y < maze.height)
			mark[(size_t)p.y * (size_t)maze.width + (size_t)p.x] |= 8;
	}
	for (const auto &w : lvl.wallBtns) {
		if (w.x >= 0 && w.y >= 0 && w.x < maze.width && w.y < maze.height)
			mark[(size_t)w.y * (size_t)maze.width + (size_t)w.x] |= 16;
	}
	for (const auto &d : lvl.doorBtns) {
		if (d.x >= 0 && d.y >= 0 && d.x < maze.width && d.y < maze.height)
			mark[(size_t)d.y * (size_t)maze.width + (size_t)d.x] |= 16;
	}
	for (const auto &k : lvl.locks) {
		if (k.doorX >= 0 && k.doorY >= 0 && k.doorX < maze.width && k.doorY < maze.height)
			mark[(size_t)k.doorY * (size_t)maze.width + (size_t)k.doorX] |= 16;
	}

	for (int y = 0; y < maze.height; y++) {
		for (int x = 0; x < maze.width; x++) {
			size_t i = (size_t)y * (size_t)maze.width + (size_t)x;
			ImVec2 tl(origin.x + x * cell, origin.y + y * cell);
			ImVec2 br(tl.x + cell - gap, tl.y + cell - gap);
			int type = maze.cells[i];
			dl->AddRectFilled(tl, br, cellFill(type, maze.cols[i], maze.floors[i]));
			drawCellGlyph(dl, type, tl, cell - gap);
			unsigned flags = mark[i];
			float d = (std::min)(5.0f, cell * 0.22f);
			if (flags & 1)
				dl->AddRectFilled(ImVec2(tl.x + 1, tl.y + 1), ImVec2(tl.x + 1 + d, tl.y + 1 + d), IM_COL32(129, 161, 255, 255));
			if (flags & 2)
				dl->AddRectFilled(ImVec2(br.x - d - 1, tl.y + 1), ImVec2(br.x - 1, tl.y + 1 + d), IM_COL32(220, 90, 90, 255));
			if (flags & 4)
				dl->AddRectFilled(ImVec2(tl.x + 1, br.y - d - 1), ImVec2(tl.x + 1 + d, br.y - 1), IM_COL32(230, 180, 70, 255));
			if (flags & 8)
				dl->AddRectFilled(ImVec2(br.x - d - 1, br.y - d - 1), ImVec2(br.x - 1, br.y - 1), IM_COL32(80, 200, 190, 255));
			if (flags & 16)
				dl->AddCircleFilled(ImVec2((tl.x + br.x) * 0.5f, (tl.y + br.y) * 0.5f), (std::max)(1.5f, d * 0.45f), IM_COL32(245, 245, 250, 230));
			if (x == selX && y == selY)
				dl->AddRect(tl, br, IM_COL32(129, 161, 255, 255), 0.0f, 0, 2.0f);
			if (x == camX && y == camY) {
				ImVec2 c((tl.x + br.x) * 0.5f, (tl.y + br.y) * 0.5f);
				float r = (cell - gap) * 0.28f;
				ImVec2 tip = c, a = c, b = c;
				if (camFace == 0) {
					tip = ImVec2(c.x, c.y - r); a = ImVec2(c.x - r, c.y + r); b = ImVec2(c.x + r, c.y + r);
				} else if (camFace == 1) {
					tip = ImVec2(c.x + r, c.y); a = ImVec2(c.x - r, c.y - r); b = ImVec2(c.x - r, c.y + r);
				} else if (camFace == 2) {
					tip = ImVec2(c.x, c.y + r); a = ImVec2(c.x + r, c.y - r); b = ImVec2(c.x - r, c.y - r);
				} else {
					tip = ImVec2(c.x - r, c.y); a = ImVec2(c.x + r, c.y + r); b = ImVec2(c.x + r, c.y - r);
				}
				dl->AddTriangleFilled(tip, a, b, IM_COL32(255, 255, 255, 230));
			}
		}
	}

	ImGui::Dummy(ImVec2(gridW, gridH));
	ImGui::SetCursorScreenPos(origin);
	ImGui::InvisibleButton("maze_grid", ImVec2(gridW, gridH));

	int hoverX = -1;
	int hoverY = -1;
	if (ImGui::IsItemHovered() || ImGui::IsItemActive()) {
		ImVec2 mp = ImGui::GetIO().MousePos;
		hoverX = (int)((mp.x - origin.x) / cell);
		hoverY = (int)((mp.y - origin.y) / cell);
		if (hoverX < 0 || hoverY < 0 || hoverX >= maze.width || hoverY >= maze.height)
			hoverX = hoverY = -1;
	}
	if (ImGui::IsItemHovered() && ImGui::GetIO().KeyCtrl && ImGui::GetIO().MouseWheel != 0.0f) {
		cell += ImGui::GetIO().MouseWheel * 2.0f;
		if (cell < 12.0f)
			cell = 12.0f;
		if (cell > 40.0f)
			cell = 40.0f;
	}

	bool drawing = mode == ModeDraw;
	if (!drawing)
		rectDrag = false;
	if (ImGui::IsItemActivated()) {
		rectDrag = drawing && ImGui::GetIO().KeyShift;
		anchorX = hoverX;
		anchorY = hoverY;
		if (drawing && !rectDrag && hoverX >= 0)
			paintCell(maze, hoverX, hoverY, brush, stampIdx, stampWall, stampFloor, selX, selY);
		else if (hoverX >= 0) {
			selX = hoverX;
			selY = hoverY;
			if (mode == ModeView && canStand(maze, hoverX, hoverY))
				camX = hoverX, camY = hoverY;
		}
	} else if (drawing && ImGui::IsItemActive() && !rectDrag && hoverX >= 0 && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
		paintCell(maze, hoverX, hoverY, brush, stampIdx, stampWall, stampFloor, selX, selY);
	}
	if (drawing && rectDrag && ImGui::IsItemDeactivated() && anchorX >= 0 && hoverX >= 0)
		paintRect(maze, anchorX, anchorY, hoverX, hoverY, brush, stampIdx, stampWall, stampFloor, selX, selY);
	if (!ImGui::IsItemActive())
		rectDrag = false;

	if (drawing && ImGui::IsItemHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Right) && hoverX >= 0)
		paintCell(maze, hoverX, hoverY, BrushFloor, false, 0, 0, selX, selY);
	if (drawing && ImGui::IsItemHovered()
		&& (ImGui::IsMouseDown(ImGuiMouseButton_Left) || ImGui::IsMouseDown(ImGuiMouseButton_Right)))
		stroke = true;
	if (!ImGui::IsMouseDown(ImGuiMouseButton_Left) && !ImGui::IsMouseDown(ImGuiMouseButton_Right))
		stroke = false;

	if (rectDrag && anchorX >= 0 && hoverX >= 0) {
		float x0 = origin.x + (std::min)(anchorX, hoverX) * cell;
		float y0 = origin.y + (std::min)(anchorY, hoverY) * cell;
		float x1 = origin.x + ((std::max)(anchorX, hoverX) + 1) * cell - gap;
		float y1 = origin.y + ((std::max)(anchorY, hoverY) + 1) * cell - gap;
		dl->AddRect(ImVec2(x0, y0), ImVec2(x1, y1), IM_COL32(129, 161, 255, 255), 0.0f, 0, 2.0f);
	} else if (hoverX >= 0 && (hoverX != selX || hoverY != selY)) {
		ImVec2 tl(origin.x + hoverX * cell, origin.y + hoverY * cell);
		ImVec2 br(tl.x + cell - gap, tl.y + cell - gap);
		dl->AddRect(tl, br, IM_COL32(255, 255, 255, 90), 0.0f, 0, 1.0f);
	}

	if (hoverX >= 0)
		ImGui::SetTooltip("%d, %d", hoverX, hoverY);

	ImGui::EndChild();
	ImGui::EndChild();
	historySync(hist, maze, lvl, levelIndex, stroke || ImGui::IsAnyItemActive());
	ImGui::End();
}
