#include "formats.hpp"
#include "stb_image.h"
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <system_error>

bool readRawFile(const std::string &path, std::vector<unsigned char> &out, std::string &err)
{
	std::ifstream f(path, std::ios::binary);
	if (!f) { err = "Cannot open: " + path; return false; }
	f.seekg(0, std::ios::end);
	auto sz = f.tellg();
	f.seekg(0);
	out.resize((size_t)sz);
	if (sz > 0)
		f.read(reinterpret_cast<char *>(out.data()), sz);
	return true;
}

static bool readFile(const std::string &path, std::vector<unsigned char> &out, std::string &err)
{
	return readRawFile(path, out, err);
}

static bool writeFile(const std::string &path, const std::vector<unsigned char> &data, std::string &err)
{
	std::ofstream f(path, std::ios::binary);
	if (!f) { err = "Cannot write: " + path; return false; }
	f.write(reinterpret_cast<const char *>(data.data()), data.size());
	return true;
}

std::uint16_t readBe16(const unsigned char *p)
{
	return (std::uint16_t)((unsigned)p[0] << 8 | p[1]);
}

void writeBe16(std::vector<unsigned char> &out, std::uint16_t v)
{
	out.push_back((unsigned char)(v >> 8));
	out.push_back((unsigned char)(v & 0xff));
}

static void padPath(std::vector<unsigned char> &o, const std::string &s)
{
	for (int i = 0; i < 64; i++) {
		unsigned char c = (i < (int)s.size()) ? (unsigned char)s[i] : 0;
		o.push_back(c);
	}
}

static std::string getPath64(const unsigned char *p)
{
	std::string s;
	for (int i = 0; i < 64 && p[i]; i++)
		s.push_back((char)p[i]);
	return s;
}

bool loadGameManifest(const std::string &path, GameManifest &m, std::string &err)
{
	std::vector<unsigned char> d;
	if (!readFile(path, d, err)) return false;
	if (d.size() < 4 + 1 + 1 + 1 + 64 * 3) { err = "game.smt too small"; return false; }
	if (d[0] != 'S' || d[1] != 'M' || d[2] != 'T' || d[3] != 'E') { err = "bad SMTE magic"; return false; }
	m.version = d[4];
	m.startLevel = d[5];
	m.levelCount = d[6];
	size_t off = 7;
	m.itemsPath = getPath64(d.data() + off); off += 64;
	m.monstersPath = getPath64(d.data() + off); off += 64;
	m.uiPalettePath = getPath64(d.data() + off); off += 64;
	m.levels.clear();
	for (int li = 0; li < m.levelCount; li++) {
		if (off + 64 * 3 > d.size()) { err = "truncated levels"; return false; }
		GameManifest::Level lv;
		lv.mazePath = getPath64(d.data() + off); off += 64;
		lv.wallsetPath = getPath64(d.data() + off); off += 64;
		lv.entitiesPath = getPath64(d.data() + off); off += 64;
		m.levels.push_back(lv);
	}
	return true;
}

bool saveGameManifest(const std::string &path, const GameManifest &m, std::string &err)
{
	std::vector<unsigned char> o;
	o.insert(o.end(), {'S','M','T','E'});
	o.push_back((unsigned char)m.version);
	o.push_back((unsigned char)m.startLevel);
	o.push_back((unsigned char)m.levelCount);
	padPath(o, m.itemsPath);
	padPath(o, m.monstersPath);
	padPath(o, m.uiPalettePath);
	for (int i = 0; i < m.levelCount; i++) {
		const auto &lv = m.levels[i];
		padPath(o, lv.mazePath);
		padPath(o, lv.wallsetPath);
		padPath(o, lv.entitiesPath);
	}
	return writeFile(path, o, err);
}

bool loadItemsDat(const std::string &path, std::vector<ItemRow> &rows, std::string &err)
{
	std::vector<unsigned char> d;
	if (!readFile(path, d, err)) return false;
	if (d.empty()) { err = "empty"; return false; }
	int count = d[0];
	size_t off = 1;
	rows.clear();
	for (int i = 0; i < count; i++) {
		if (off + 11 > d.size()) { err = "truncated item"; return false; }
		ItemRow r;
		r.type = d[off++]; r.subType = d[off++]; r.flags = d[off++]; r.modType = d[off++];
		r.mod = d[off++]; r.value = d[off++]; r.weight = d[off++]; r.icon = d[off++];
		r.usage = d[off++]; r.keyId = d[off++]; r.equipSlot = d[off++];
		int nl = d[off++];
		if (off + nl > d.size()) { err = "bad name"; return false; }
		r.name.assign((char *)d.data() + off, (char *)d.data() + off + nl);
		off += nl;
		rows.push_back(std::move(r));
	}
	return true;
}

bool saveItemsDat(const std::string &path, const std::vector<ItemRow> &rows, std::string &err)
{
	std::vector<unsigned char> o;
	o.push_back((unsigned char)rows.size());
	for (const auto &r : rows) {
		o.push_back(r.type);
		o.push_back(r.subType);
		o.push_back(r.flags);
		o.push_back(r.modType);
		o.push_back(r.mod);
		o.push_back(r.value);
		o.push_back(r.weight);
		o.push_back(r.icon);
		o.push_back(r.usage);
		o.push_back(r.keyId);
		o.push_back(r.equipSlot);
		int nl = (int)r.name.size();
		if (nl > 63) nl = 63;
		o.push_back((unsigned char)nl);
		for (int i = 0; i < nl; i++) o.push_back((unsigned char)r.name[i]);
	}
	return writeFile(path, o, err);
}

bool loadMonstersDat(const std::string &path, std::vector<MonsterRow> &rows, std::string &err)
{
	std::vector<unsigned char> d;
	if (!readFile(path, d, err)) return false;
	if (d.size() < 6) { err = "small"; return false; }
	if (d[0] != 'M' || d[1] != 'O' || d[2] != 'N' || d[3] != 'S') { err = "MONS magic"; return false; }
	int ver = d[4];
	int count = d[5];
	if (ver != 1) { err = "monsters ver"; return false; }
	size_t off = 6;
	rows.clear();
	for (int i = 0; i < count; i++) {
		if (off + 1 + 2 + 1 + 1 + 2 + 1 + 1 + 8 + 8 + 1 > d.size()) { err = "trunc mon"; return false; }
		MonsterRow r;
		r.level = d[off++];
		r.maxHp = readBe16(d.data() + off); off += 2;
		r.attack = d[off++];
		r.defense = d[off++];
		r.experience = readBe16(d.data() + off); off += 2;
		r.aggro = d[off++]; r.flee = d[off++];
		memcpy(r.dropTable, d.data() + off, 8); off += 8;
		memcpy(r.dropChance, d.data() + off, 8); off += 8;
		int nl = d[off++];
		if (off + nl > d.size()) { err = "name"; return false; }
		r.name.assign((char *)d.data() + off, (char *)d.data() + off + nl);
		off += nl;
		rows.push_back(std::move(r));
	}
	return true;
}

bool saveMonstersDat(const std::string &path, const std::vector<MonsterRow> &rows, std::string &err)
{
	std::vector<unsigned char> o;
	o.insert(o.end(), {'M','O','N','S', 1, (unsigned char)rows.size()});
	for (const auto &r : rows) {
		o.push_back(r.level);
		writeBe16(o, r.maxHp);
		o.push_back(r.attack);
		o.push_back(r.defense);
		writeBe16(o, r.experience);
		o.push_back(r.aggro);
		o.push_back(r.flee);
		for (int i = 0; i < 8; i++) o.push_back(r.dropTable[i]);
		for (int i = 0; i < 8; i++) o.push_back(r.dropChance[i]);
		int nl = (int)r.name.size();
		if (nl > 255) nl = 255;
		o.push_back((unsigned char)nl);
		for (int i = 0; i < nl; i++) o.push_back((unsigned char)r.name[i]);
	}
	return writeFile(path, o, err);
}

bool loadMaze(const std::string &path, MazeFile &m, std::string &err)
{
	std::vector<unsigned char> d;
	if (!readFile(path, d, err)) return false;
	if (d.size() < 2) { err = "maze small"; return false; }
	m.width = d[0]; m.height = d[1];
	size_t cells = (size_t)m.width * m.height;
	size_t need = 2 + cells * 3;
	if (d.size() < need + 2) { err = "maze data"; return false; }
	size_t off = 2;
	m.cells.assign(d.begin() + off, d.begin() + off + cells); off += cells;
	m.cols.assign(d.begin() + off, d.begin() + off + cells); off += cells;
	m.floors.assign(d.begin() + off, d.begin() + off + cells); off += cells;
	int ec = (int)readBe16(d.data() + off); off += 2;
	m.events.clear();
	for (int i = 0; i < ec; i++) {
		if (off + 4 > d.size()) { err = "evt hdr"; return false; }
		MazeEvent ev;
		ev.x = d[off++]; ev.y = d[off++]; ev.type = d[off++];
		int esz = d[off++];
		if (off + esz > d.size()) { err = "evt data"; return false; }
		ev.data.assign(d.begin() + off, d.begin() + off + esz);
		off += esz;
		m.events.push_back(std::move(ev));
	}
	if (off + 2 > d.size()) { err = "str cnt"; return false; }
	int sc = (int)readBe16(d.data() + off); off += 2;
	m.strings.clear();
	for (int i = 0; i < sc; i++) {
		if (off + 2 > d.size()) { err = "slen"; return false; }
		int slen = (int)readBe16(d.data() + off); off += 2;
		if (off + slen > d.size()) { err = "sdata"; return false; }
		std::vector<unsigned char> s(d.begin() + off, d.begin() + off + slen);
		off += slen;
		m.strings.push_back(std::move(s));
	}
	return true;
}

void loadBuiltinDemoMaze(MazeFile &m)
{
	static const unsigned char map[1024] = {
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
		1,0,0,3,0,3,0,0,1,0,0,0,0,1,0,1,0,0,0,1,0,6,1,0,0,1,0,2,0,3,0,0,
		1,0,0,1,0,1,0,0,1,0,0,0,0,1,0,1,1,0,0,1,0,1,1,0,0,1,1,1,2,1,1,1,
		1,1,1,1,0,1,1,1,1,0,0,0,0,1,0,0,0,0,0,1,0,0,0,0,0,1,5,0,0,0,0,1,
		1,0,0,2,0,2,0,0,1,1,1,1,0,1,1,1,1,1,2,1,1,1,1,1,2,1,1,1,1,2,1,1,
		1,0,0,1,0,1,0,0,1,0,0,0,0,1,0,0,0,2,0,0,0,1,0,0,0,0,1,0,0,0,0,1,
		1,1,1,1,0,1,1,1,1,5,1,0,0,0,0,0,0,1,0,0,0,0,0,1,0,0,2,0,1,1,0,1,
		1,0,0,2,0,2,0,0,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,0,0,1,1,1,1,1,1,
		1,0,0,1,0,1,0,0,1,0,0,0,0,5,1,0,0,1,0,0,0,0,0,1,0,0,1,0,0,0,0,1,
		1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,0,0,1,0,1,1,1,1,1,0,0,1,0,1,1,0,1,
		1,0,0,2,0,2,0,0,1,0,0,1,0,0,1,0,0,1,0,0,0,0,0,1,0,0,1,0,0,1,0,1,
		1,0,0,1,0,1,0,5,1,0,0,0,0,0,1,0,0,1,0,1,0,1,0,1,0,0,1,0,0,0,0,1,
		1,1,1,1,0,1,1,1,1,1,1,2,1,1,1,0,0,1,1,1,0,1,1,1,0,0,1,0,0,1,0,1,
		1,0,0,2,0,2,0,0,1,0,2,0,0,0,2,0,0,2,0,0,0,0,0,2,0,0,2,0,1,1,0,1,
		1,0,0,1,0,1,0,0,1,1,1,0,1,1,1,0,0,1,1,1,0,1,1,1,0,0,1,0,0,1,0,1,
		1,1,1,1,0,1,1,1,1,0,0,5,0,0,1,0,0,1,0,0,0,0,0,1,0,0,1,0,0,0,0,1,
		1,0,0,2,0,2,0,0,1,0,1,1,1,0,1,0,0,1,0,0,0,0,0,1,0,0,1,0,0,1,0,1,
		1,0,0,1,0,1,0,0,1,0,0,1,0,0,1,0,0,1,0,1,1,1,5,1,0,0,1,0,1,1,0,1,
		1,1,1,1,0,1,1,1,1,0,0,1,0,0,1,5,0,1,0,0,0,0,0,1,0,0,1,0,0,0,0,1,
		1,0,0,2,0,2,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,1,1,1,1,1,1,
		1,5,0,1,0,1,0,0,1,5,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,0,1,1,
		1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,1,5,1,3,1,1,1,1,0,1,0,1,
		1,0,0,2,0,2,0,0,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,1,0,0,0,1,
		1,0,0,1,0,1,0,0,1,0,0,0,2,0,0,0,2,0,0,0,0,1,1,1,1,0,1,1,1,1,2,1,
		1,1,1,1,0,1,1,1,1,1,1,1,1,1,2,1,1,1,2,1,1,1,0,0,0,0,0,0,0,1,0,1,
		1,0,0,2,0,2,0,0,1,5,1,1,0,1,0,1,0,0,0,0,1,1,0,1,1,1,1,1,0,1,0,1,
		1,5,0,1,0,1,0,0,1,0,5,1,0,0,0,1,0,0,1,0,0,1,0,0,0,1,0,0,0,1,0,1,
		1,1,1,1,0,1,1,0,1,0,1,1,0,0,0,1,0,1,0,0,1,1,0,1,1,1,1,1,0,1,0,1,
		1,0,0,2,0,1,1,1,1,0,1,1,0,0,0,1,0,0,1,0,0,1,0,0,0,1,0,0,0,1,0,1,
		1,0,0,1,0,1,0,0,0,0,0,0,0,1,0,1,0,1,0,0,1,1,0,1,1,1,1,1,0,1,0,1,
		1,1,1,1,0,2,0,1,0,0,1,1,0,1,0,1,0,0,1,0,0,1,0,0,5,1,5,0,0,1,5,1,
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	};
	m = MazeFile();
	m.width = 32;
	m.height = 32;
	m.cells.assign(map, map + 1024);
	m.cols.assign(1024, 0);
	m.floors.assign(1024, 0);
	const char *msg = "You trigger a pressure plate.";
	m.strings.push_back(std::vector<unsigned char>(msg, msg + 30));
	auto add = [&](int x, int y, int type, unsigned char a, unsigned char b, int n) {
		MazeEvent ev;
		ev.x = x;
		ev.y = y;
		ev.type = type;
		ev.data.push_back(a);
		if (n > 1)
			ev.data.push_back(b);
		m.events.push_back(std::move(ev));
	};
	add(27, 3, 10, 27, 3, 2);
	add(9, 6, 10, 9, 6, 2);
	add(14, 8, 10, 14, 8, 2);
	add(12, 15, 10, 12, 15, 2);
	for (int y = 0; y < 32; y++) {
		for (int x = 0; x < 32; x++) {
			if (m.cells[(size_t)y * 32 + (size_t)x] != 5)
				continue;
			bool taken = false;
			for (const MazeEvent &ev : m.events) {
				if (ev.x == x && ev.y == y)
					taken = true;
			}
			if (!taken)
				add(x, y, 221, 25, 0, 1);
		}
	}
}

bool saveMaze(const std::string &path, const MazeFile &m, std::string &err)
{
	std::vector<unsigned char> o;
	o.push_back((unsigned char)m.width);
	o.push_back((unsigned char)m.height);
	for (auto v : m.cells) o.push_back(v);
	for (auto v : m.cols) o.push_back(v);
	for (auto v : m.floors) o.push_back(v);
	writeBe16(o, (std::uint16_t)m.events.size());
	for (const auto &ev : m.events) {
		o.push_back((unsigned char)ev.x);
		o.push_back((unsigned char)ev.y);
		o.push_back((unsigned char)ev.type);
		int esz = (int)ev.data.size();
		if (esz > 255) esz = 255;
		o.push_back((unsigned char)esz);
		for (int i = 0; i < esz; i++) o.push_back(ev.data[i]);
	}
	writeBe16(o, (std::uint16_t)m.strings.size());
	for (const auto &s : m.strings) {
		int sl = (int)s.size();
		if (sl > 65535) sl = 65535;
		writeBe16(o, (std::uint16_t)sl);
		for (int i = 0; i < sl; i++) o.push_back(s[i]);
	}
	return writeFile(path, o, err);
}

bool loadLvl(const std::string &path, LvlEntities &e, std::string &err)
{
	std::vector<unsigned char> d;
	if (!readFile(path, d, err)) return false;
	if (d.size() < 6) { err = "lvl small"; return false; }
	if (d[0] != 'L' || d[1] != 'V' || d[2] != 'L' || d[3] != 'E') { err = "LVLE"; return false; }
	e.version = d[4];
	size_t off = 5;
	e.wallBtns.clear();
	int nwb = d[off++];
	for (int i = 0; i < nwb; i++) {
		if (off + 7 > d.size()) { err = "wb"; return false; }
		LvlEntities::WB w;
		w.x = d[off++]; w.y = d[off++]; w.wallSide = d[off++]; w.type = d[off++];
		w.gfx = d[off++]; w.evt = d[off++]; w.esz = d[off++];
		if (off + w.esz > d.size()) { err = "wb data"; return false; }
		w.ed.assign(d.begin() + off, d.begin() + off + w.esz);
		off += w.esz;
		e.wallBtns.push_back(std::move(w));
	}
	if (off >= d.size()) { err = "door cnt"; return false; }
	int ndb = d[off++];
	e.doorBtns.clear();
	for (int i = 0; i < ndb; i++) {
		if (off + 7 > d.size()) { err = "db"; return false; }
		LvlEntities::DB x;
		x.x = d[off++]; x.y = d[off++]; x.wallSide = d[off++]; x.type = d[off++];
		x.gfx = d[off++]; x.tx = d[off++]; x.ty = d[off++];
		e.doorBtns.push_back(x);
	}
	if (off >= d.size()) { err = "lk cnt"; return false; }
	int nlk = d[off++];
	e.locks.clear();
	for (int i = 0; i < nlk; i++) {
		if (off + 5 > d.size()) { err = "lk"; return false; }
		LvlEntities::Lk x;
		x.doorX = d[off++]; x.doorY = d[off++]; x.type = d[off++]; x.keyId = d[off++]; x.code = d[off++];
		e.locks.push_back(x);
	}
	if (off >= d.size()) { err = "pl cnt"; return false; }
	int np = d[off++];
	e.plates.clear();
	for (int i = 0; i < np; i++) {
		if (off + 4 > d.size()) { err = "pl"; return false; }
		LvlEntities::Pl x;
		x.x = d[off++]; x.y = d[off++]; x.evt = d[off++]; x.dsz = d[off++];
		if (x.dsz > 8) {
			off += x.dsz;
			continue;
		}
		if (off + x.dsz > d.size()) { err = "pl d"; return false; }
		memcpy(x.d, d.data() + off, x.dsz);
		off += x.dsz;
		e.plates.push_back(x);
	}
	if (off >= d.size()) { err = "g cnt"; return false; }
	int ng = d[off++];
	e.ground.clear();
	for (int i = 0; i < ng; i++) {
		if (off + 4 > d.size()) { err = "g"; return false; }
		LvlEntities::GI x;
		x.x = d[off++]; x.y = d[off++]; x.item = d[off++]; x.qty = d[off++];
		e.ground.push_back(x);
	}
	if (off >= d.size()) { err = "m cnt"; return false; }
	int nm = d[off++];
	e.monsters.clear();
	for (int i = 0; i < nm; i++) {
		if (off + 3 > d.size()) { err = "m"; return false; }
		LvlEntities::MS x;
		x.type = d[off++]; x.x = d[off++]; x.y = d[off++];
		e.monsters.push_back(x);
	}
	return true;
}

bool saveLvl(const std::string &path, const LvlEntities &e, std::string &err)
{
	std::vector<unsigned char> o;
	o.insert(o.end(), {'L','V','L','E', (unsigned char)e.version});
	o.push_back((unsigned char)e.wallBtns.size());
	for (const auto &w : e.wallBtns) {
		o.push_back((unsigned char)w.x);
		o.push_back((unsigned char)w.y);
		o.push_back((unsigned char)w.wallSide);
		o.push_back((unsigned char)w.type);
		o.push_back((unsigned char)w.gfx);
		o.push_back((unsigned char)w.evt);
		int esz = (int)w.ed.size();
		if (esz > 255) esz = 255;
		o.push_back((unsigned char)esz);
		for (int i = 0; i < esz; i++) o.push_back(w.ed[i]);
	}
	o.push_back((unsigned char)e.doorBtns.size());
	for (const auto &x : e.doorBtns) {
		o.push_back((unsigned char)x.x); o.push_back((unsigned char)x.y);
		o.push_back((unsigned char)x.wallSide); o.push_back((unsigned char)x.type);
		o.push_back((unsigned char)x.gfx); o.push_back((unsigned char)x.tx); o.push_back((unsigned char)x.ty);
	}
	o.push_back((unsigned char)e.locks.size());
	for (const auto &x : e.locks) {
		o.push_back((unsigned char)x.doorX); o.push_back((unsigned char)x.doorY);
		o.push_back((unsigned char)x.type); o.push_back((unsigned char)x.keyId); o.push_back((unsigned char)x.code);
	}
	o.push_back((unsigned char)e.plates.size());
	for (const auto &x : e.plates) {
		o.push_back((unsigned char)x.x); o.push_back((unsigned char)x.y);
		o.push_back((unsigned char)x.evt);
		int dsz = x.dsz;
		if (dsz > 8) dsz = 8;
		o.push_back((unsigned char)dsz);
		for (int i = 0; i < dsz; i++) o.push_back(x.d[i]);
	}
	o.push_back((unsigned char)e.ground.size());
	for (const auto &x : e.ground) {
		o.push_back((unsigned char)x.x); o.push_back((unsigned char)x.y);
		o.push_back((unsigned char)x.item); o.push_back((unsigned char)x.qty);
	}
	o.push_back((unsigned char)e.monsters.size());
	for (const auto &x : e.monsters) {
		o.push_back((unsigned char)x.type); o.push_back((unsigned char)x.x); o.push_back((unsigned char)x.y);
	}
	return writeFile(path, o, err);
}

bool loadWallsetMain(const std::string &path, WallsetFile &w, std::string &err)
{
	std::vector<unsigned char> d;
	if (!readFile(path, d, err)) return false;
	if (d.size() < 3 + 1 + 2 + 1) { err = "wst small"; return false; }
	size_t off = 0;
	w.header[0] = d[off++]; w.header[1] = d[off++]; w.header[2] = d[off++];
	int ps = d[off++];
	if (off + ps * 3 > d.size()) { err = "pal"; return false; }
	w.palette.assign(d.begin() + off, d.begin() + off + ps * 3);
	off += ps * 3;
	w.totalTiles = readBe16(d.data() + off); off += 2;
	w.gfxCount = d[off++];
	w.tiles.clear();
	w.tilesPerGroup.clear();
	for (int ts = 0; ts < w.gfxCount; ts++) {
		if (off + 1 > d.size()) { err = "wcnt"; return false; }
		int wc = d[off++];
		w.tilesPerGroup.push_back(wc);
		for (int k = 0; k < wc; k++) {
			if (off + 16 > d.size()) { err = "tile"; return false; }
			WallsetFile::Tile t;
			t.type = d[off++]; t.setIndex = d[off++];
			t.loc[0] = (char)d[off++]; t.loc[1] = (char)d[off++];
			t.screen[0] = (std::int16_t)readBe16(d.data() + off); off += 2;
			t.screen[1] = (std::int16_t)readBe16(d.data() + off); off += 2;
			t.x = readBe16(d.data() + off); off += 2;
			t.y = readBe16(d.data() + off); off += 2;
			t.w = readBe16(d.data() + off); off += 2;
			t.h = readBe16(d.data() + off); off += 2;
			w.tiles.push_back(t);
		}
	}
	return true;
}

bool decodeAcePlanar(const std::vector<unsigned char> &fileData, const unsigned char *palRgb, int palColors,
	std::vector<unsigned char> &rgbaOut, int &outW, int &outH, std::string &err)
{
	if (fileData.size() < 9) { err = "bm small"; return false; }
	std::uint16_t uw = readBe16(fileData.data());
	std::uint16_t uh = readBe16(fileData.data() + 2);
	int bpp = fileData[4];
	int ver = fileData[5];
	int flags = fileData[6];
	if (ver != 0) { err = "bm ver"; return false; }
	/* ACE bitmapCreateFromFd: width, height, depth, version, flags, then 2 unused bytes. */
	size_t hdr = 9;
	outW = uw; outH = uh;
	int stride = (uw + 7) / 8;
	size_t planeSz = (size_t)stride * uh;
	bool interleaved = (flags & 1) != 0;
	if (bpp < 1 || bpp > 8) { err = "bpp"; return false; }
	if (hdr + planeSz * (size_t)bpp > fileData.size()) { err = "bm planes"; return false; }
	rgbaOut.assign((size_t)uw * uh * 4, 0);
	for (int y = 0; y < uh; y++) {
		for (int x = 0; x < uw; x++) {
			int bit = 7 - (x & 7);
			int byteOff = y * stride + (x >> 3);
			int cidx = 0;
			for (int p = 0; p < bpp; p++) {
				size_t po = interleaved
					? hdr + ((size_t)y * (size_t)bpp + (size_t)p) * (size_t)stride + (size_t)byteOff
					: hdr + (size_t)p * planeSz + byteOff;
				int bitv = (fileData[po] >> bit) & 1;
				cidx |= bitv << p;
			}
			if (cidx < palColors && palRgb) {
				size_t pi = (size_t)cidx * 3;
				unsigned char *px = &rgbaOut[((size_t)y * uw + x) * 4];
				px[0] = palRgb[pi];
				px[1] = palRgb[pi + 1];
				px[2] = palRgb[pi + 2];
				px[3] = 255;
			}
		}
	}
	return true;
}

/** Height of the front wall or door at this slot. The floor strip is too short to stand a creature on. */
static int frontOpeningHeight(const WallsetFile &w, int locY)
{
	int best = 0;
	for (const auto &t : w.tiles) {
		if ((int)t.loc[0] != 0 || (int)t.loc[1] != locY)
			continue;
		if (t.type != 1 && t.type != 2 && t.type != 3)
			continue;
		if ((int)t.h > best)
			best = (int)t.h;
	}
	return best;
}

static int nearestPalette(int r, int g, int b, const unsigned char *pal, int palN)
{
	int n = palN < 32 ? palN : 32;
	int best = 0;
	int bestD = 1 << 30;
	if (n < 1)
		return 0;
	for (int i = 0; i < n; i++) {
		int dr = r - (int)pal[i * 3];
		int dg = g - (int)pal[i * 3 + 1];
		int db = b - (int)pal[i * 3 + 2];
		int d = dr * dr + dg * dg + db * db;
		if (d < bestD) {
			bestD = d;
			best = i;
		}
	}
	return best;
}

static void appendAceBitmap(std::vector<unsigned char> &out, int w, int h, int bpp,
	const std::vector<unsigned char> &indices)
{
	writeBe16(out, (std::uint16_t)w);
	writeBe16(out, (std::uint16_t)h);
	out.push_back((unsigned char)bpp);
	out.push_back(0);
	out.push_back(0);
	out.push_back(0);
	out.push_back(0);
	int stride = w / 8;
	for (int plane = 0; plane < bpp; plane++) {
		for (int y = 0; y < h; y++) {
			for (int xb = 0; xb < stride; xb++) {
				unsigned char byte = 0;
				for (int bit = 0; bit < 8; bit++) {
					int x = xb * 8 + bit;
					unsigned char idx = indices[(size_t)y * (size_t)w + (size_t)x];
					if (idx & (unsigned char)(1u << plane))
						byte |= (unsigned char)(0x80u >> bit);
				}
				out.push_back(byte);
			}
		}
	}
}

static bool readAceIndices(const std::vector<unsigned char> &file, int &w, int &h, int &bpp,
	std::vector<unsigned char> &indices, std::string &err)
{
	if (file.size() < 9) { err = "bm small"; return false; }
	w = readBe16(file.data());
	h = readBe16(file.data() + 2);
	bpp = file[4];
	if (file[5] != 0) { err = "bm ver"; return false; }
	if (w <= 0 || h <= 0 || bpp < 1 || bpp > 8) { err = "bm size"; return false; }
	int stride = (w + 7) / 8;
	size_t planeSz = (size_t)stride * (size_t)h;
	bool interleaved = (file[6] & 1) != 0;
	size_t hdr = 9;
	if (hdr + planeSz * (size_t)bpp > file.size()) { err = "bm planes"; return false; }
	indices.assign((size_t)w * (size_t)h, 0);
	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			int bit = 7 - (x & 7);
			int byteOff = y * stride + (x >> 3);
			int cidx = 0;
			for (int p = 0; p < bpp; p++) {
				size_t po = interleaved
					? hdr + ((size_t)y * (size_t)bpp + (size_t)p) * (size_t)stride + (size_t)byteOff
					: hdr + (size_t)p * planeSz + (size_t)byteOff;
				if ((file[po] >> bit) & 1)
					cidx |= 1 << p;
			}
			indices[(size_t)y * (size_t)w + (size_t)x] = (unsigned char)cidx;
		}
	}
	return true;
}

bool loadMonsterFramePreview(const std::string &bmPath, const std::string &mskPath,
	const unsigned char *palRgb, int palColors, MonsterFrameImage &out, std::string &err)
{
	out = MonsterFrameImage{};
	std::vector<unsigned char> bm;
	if (!readRawFile(bmPath, bm, err))
		return false;
	int w = 0, h = 0, bpp = 0;
	std::vector<unsigned char> idx;
	if (!readAceIndices(bm, w, h, bpp, idx, err))
		return false;
	std::vector<unsigned char> maskIdx;
	int mw = 0, mh = 0, mbpp = 0;
	std::vector<unsigned char> msk;
	std::string merr;
	bool haveMask = readRawFile(mskPath, msk, merr) && readAceIndices(msk, mw, mh, mbpp, maskIdx, merr)
		&& mw == w && mh == h;
	int n = palColors < 32 ? palColors : 32;
	if (!palRgb || n < 1) { err = "no palette"; return false; }
	out.w = w;
	out.h = h;
	out.rgba.assign((size_t)w * (size_t)h * 4, 0);
	for (int i = 0; i < w * h; i++) {
		unsigned char c = idx[(size_t)i];
		if ((int)c >= n)
			c = 0;
		unsigned char *px = &out.rgba[(size_t)i * 4];
		px[0] = palRgb[c * 3];
		px[1] = palRgb[c * 3 + 1];
		px[2] = palRgb[c * 3 + 2];
		px[3] = (!haveMask || maskIdx[(size_t)i]) ? 255 : 0;
	}
	return true;
}

static const int kMonsterSourcePoses = 6;
static const int kMonsterBakedFrames = 8;

struct MonsterSlice {
	int x = 0;
	int y = 0;
	int w = 0;
	int h = 0;
};

static bool pixelIsBackground(const unsigned char *p, bool useKey, int keyR, int keyG, int keyB)
{
	if (p[3] < 128)
		return true;
	if (!useKey)
		return false;
	int dr = (int)p[0] - keyR;
	int dg = (int)p[1] - keyG;
	int db = (int)p[2] - keyB;
	if (dr < 0)
		dr = -dr;
	if (dg < 0)
		dg = -dg;
	if (db < 0)
		db = -db;
	return dr <= 28 && dg <= 28 && db <= 28;
}

static bool sliceMonsterStrip(const unsigned char *src, int srcW, int srcH, std::vector<MonsterSlice> &out, std::string &err)
{
	out.clear();
	if (srcW < 1 || srcH < 1) {
		err = "Empty image";
		return false;
	}
	const unsigned char *corner = src;
	bool useKey = corner[3] >= 128;
	int keyR = corner[0];
	int keyG = corner[1];
	int keyB = corner[2];
	auto bgAt = [&](int x, int y) {
		return pixelIsBackground(src + ((size_t)y * (size_t)srcW + (size_t)x) * 4, useKey, keyR, keyG, keyB);
	};
	std::vector<int> rowUse(srcH, 0);
	for (int y = 0; y < srcH; y++) {
		for (int x = 0; x < srcW; x++) {
			if (!bgAt(x, y)) {
				rowUse[y] = 1;
				break;
			}
		}
	}
	int rowBands = 0;
	int row0 = 0;
	int row1 = 0;
	for (int y = 0; y < srcH;) {
		while (y < srcH && !rowUse[y])
			y++;
		if (y >= srcH)
			break;
		int y0 = y;
		while (y < srcH && rowUse[y])
			y++;
		rowBands++;
		row0 = y0;
		row1 = y;
	}
	if (rowBands == 0) {
		err = "No creature pixels in the picture";
		return false;
	}
	if (rowBands > 1) {
		err = "This picture has more than one row. Import one creature, poses in a single row.";
		return false;
	}
	std::vector<char> seen((size_t)srcW * (size_t)srcH, 0);
	std::vector<int> qx;
	std::vector<int> qy;
	qx.reserve(256);
	qy.reserve(256);
	for (int y = row0; y < row1; y++) {
		for (int x = 0; x < srcW; x++) {
			size_t start = (size_t)y * (size_t)srcW + (size_t)x;
			if (seen[start] || bgAt(x, y))
				continue;
			int x0 = x, x1 = x, y0 = y, y1 = y;
			qx.clear();
			qy.clear();
			qx.push_back(x);
			qy.push_back(y);
			seen[start] = 1;
			for (size_t q = 0; q < qx.size(); q++) {
				int cx = qx[q];
				int cy = qy[q];
				if (cx < x0) x0 = cx;
				if (cx > x1) x1 = cx;
				if (cy < y0) y0 = cy;
				if (cy > y1) y1 = cy;
				for (int dy = -1; dy <= 1; dy++) {
					for (int dx = -1; dx <= 1; dx++) {
						int nx = cx + dx;
						int ny = cy + dy;
						if (nx < 0 || ny < row0 || nx >= srcW || ny >= row1)
							continue;
						size_t ni = (size_t)ny * (size_t)srcW + (size_t)nx;
						if (seen[ni] || bgAt(nx, ny))
							continue;
						seen[ni] = 1;
						qx.push_back(nx);
						qy.push_back(ny);
					}
				}
			}
			MonsterSlice s;
			s.x = x0;
			s.y = y0;
			s.w = x1 - x0 + 1;
			s.h = y1 - y0 + 1;
			out.push_back(s);
		}
	}
	std::sort(out.begin(), out.end(), [](const MonsterSlice &a, const MonsterSlice &b) {
		return a.x < b.x;
	});
	if ((int)out.size() != kMonsterSourcePoses) {
		err = "Found " + std::to_string(out.size()) + " separate shapes, need 6 poses in one row: Front, Back, Left1, Left2, Attack1, Attack2. Each pose has to be one connected figure. Right is mirrored from the left poses.";
		return false;
	}
	return true;
}

bool exportMonsterFrames(const std::string &pngPath, const std::string &outDir, int typeIndex,
	const WallsetFile &wallset, std::string &err, std::vector<MonsterFrameImage> *outFrames)
{
	int palN = (int)wallset.palette.size() / 3;
	if (palN < 1) { err = "Load a wallset before importing a creature"; return false; }
	if (typeIndex < 0 || typeIndex > 63) { err = "monster index out of range"; return false; }
	int srcW = 0, srcH = 0, srcComp = 0;
	unsigned char *src = stbi_load(pngPath.c_str(), &srcW, &srcH, &srcComp, 4);
	(void)srcComp;
	if (!src || srcW < 1 || srcH < 1) {
		if (src)
			stbi_image_free(src);
		err = "Cannot read image: " + pngPath;
		return false;
	}
	std::vector<MonsterSlice> slices;
	if (!sliceMonsterStrip(src, srcW, srcH, slices, err)) {
		stbi_image_free(src);
		return false;
	}
	std::error_code ec;
	std::filesystem::create_directories(outDir, ec);
	if (ec) {
		stbi_image_free(src);
		err = "Cannot create " + outDir;
		return false;
	}
	for (int f = 0; f < 16; f++) {
		for (int d = 0; d < 4; d++) {
			std::string stem = outDir + "/" + std::to_string(typeIndex) + "_f" + std::to_string(f) + "_d" + std::to_string(d);
			std::filesystem::remove(stem + ".bm", ec);
			std::filesystem::remove(stem + ".msk", ec);
		}
	}
	for (int d = 0; d < 4; d++) {
		std::string stem = outDir + "/" + std::to_string(typeIndex) + "_d" + std::to_string(d);
		std::filesystem::remove(stem + ".bm", ec);
		std::filesystem::remove(stem + ".msk", ec);
	}
	int maxH = 1;
	for (const MonsterSlice &s : slices) {
		if (s.h > maxH)
			maxH = s.h;
	}
	const unsigned char *pal = wallset.palette.data();
	const int yDelta[4] = {0, -1, -2, -3};
	const int fallbackH[4] = {128, 106, 64, 48};
	if (outFrames)
		outFrames->clear();
	const int poseSlice[8] = {0, 1, 2, 3, 2, 3, 4, 5};
	const int poseMirror[8] = {0, 0, 0, 0, 1, 1, 0, 0};
	for (int depth = 0; depth < 4; depth++) {
		int opening = frontOpeningHeight(wallset, yDelta[depth]);
		int dstH = fallbackH[depth];
		if (opening > 0)
			dstH = opening * 2 / 3;
		else if (depth == 0) {
			int ahead = frontOpeningHeight(wallset, -1);
			if (ahead > 0)
				dstH = ahead * 4 / 5;
		}
		if (dstH < 1)
			dstH = 1;
		if (dstH > 128)
			dstH = 128;
		for (int frame = 0; frame < kMonsterBakedFrames; frame++) {
			const MonsterSlice &slice = slices[(size_t)poseSlice[frame]];
			bool mirror = poseMirror[frame] != 0;
			int contentH = (int)((long)slice.h * dstH / maxH);
			int contentW = (int)((long)slice.w * dstH / maxH);
			if (contentH < 1)
				contentH = 1;
			if (contentW < 1)
				contentW = 1;
			if (contentH > dstH)
				contentH = dstH;
			int pad = (16 - (contentW & 15)) & 15;
			int totalW = contentW + pad;
			int left = pad / 2;
			int top = dstH - contentH;
			std::vector<unsigned char> indices((size_t)totalW * (size_t)dstH, 0);
			std::vector<unsigned char> opaque((size_t)totalW * (size_t)dstH, 0);
			for (int y = 0; y < contentH; y++) {
				int sy = slice.y + (int)((long)y * slice.h / contentH);
				if (sy >= slice.y + slice.h)
					sy = slice.y + slice.h - 1;
				for (int x = 0; x < contentW; x++) {
					int sampleX = mirror ? (contentW - 1 - x) : x;
					int sx = slice.x + (int)((long)sampleX * slice.w / contentW);
					if (sx >= slice.x + slice.w)
						sx = slice.x + slice.w - 1;
					const unsigned char *p = src + ((size_t)sy * (size_t)srcW + (size_t)sx) * 4;
					int dx = left + x;
					int dy = top + y;
					size_t di = (size_t)dy * (size_t)totalW + (size_t)dx;
					if (pixelIsBackground(p, src[3] >= 128, src[0], src[1], src[2]))
						continue;
					opaque[di] = 1;
					indices[di] = (unsigned char)nearestPalette(p[0], p[1], p[2], pal, palN);
				}
			}
			std::vector<unsigned char> bm;
			std::vector<unsigned char> msk;
			appendAceBitmap(bm, totalW, dstH, 5, indices);
			appendAceBitmap(msk, totalW, dstH, 1, opaque);
			std::string stem = outDir + "/" + std::to_string(typeIndex) + "_f" + std::to_string(frame) + "_d" + std::to_string(depth);
			if (!writeFile(stem + ".bm", bm, err) || !writeFile(stem + ".msk", msk, err)) {
				stbi_image_free(src);
				return false;
			}
			if (outFrames && depth == 0) {
				MonsterFrameImage fr;
				fr.w = totalW;
				fr.h = dstH;
				fr.rgba.assign((size_t)totalW * (size_t)dstH * 4, 0);
				int n = palN < 32 ? palN : 32;
				for (int i = 0; i < totalW * dstH; i++) {
					unsigned char *px = &fr.rgba[(size_t)i * 4];
					if (!opaque[(size_t)i])
						continue;
					unsigned char c = indices[(size_t)i];
					if ((int)c >= n)
						c = 0;
					px[0] = pal[c * 3];
					px[1] = pal[c * 3 + 1];
					px[2] = pal[c * 3 + 2];
					px[3] = 255;
				}
				outFrames->push_back(std::move(fr));
			}
		}
	}
	stbi_image_free(src);
	return true;
}
