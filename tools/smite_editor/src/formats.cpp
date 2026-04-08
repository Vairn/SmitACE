#include "formats.hpp"
#include <cstring>
#include <fstream>

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
			if (off + 18 > d.size()) { err = "tile"; return false; }
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
	if (fileData.size() < 8) { err = "bm small"; return false; }
	std::uint16_t uw = readBe16(fileData.data());
	std::uint16_t uh = readBe16(fileData.data() + 2);
	int bpp = fileData[4];
	int ver = fileData[5];
	int flags = fileData[6];
	(void)flags;
	if (ver != 0) { err = "bm ver"; return false; }
	size_t hdr = 8;
	if (fileData.size() < hdr) { err = "bm hdr"; return false; }
	outW = uw; outH = uh;
	int stride = (uw + 7) / 8;
	size_t planeSz = (size_t)stride * uh;
	if (bpp < 1 || bpp > 8) { err = "bpp"; return false; }
	if (hdr + planeSz * (size_t)bpp > fileData.size()) { err = "bm planes"; return false; }
	rgbaOut.assign((size_t)uw * uh * 4, 0);
	for (int y = 0; y < uh; y++) {
		for (int x = 0; x < uw; x++) {
			int bit = 7 - (x & 7);
			int byteOff = y * stride + (x >> 3);
			int cidx = 0;
			for (int p = 0; p < bpp; p++) {
				size_t po = hdr + (size_t)p * planeSz + byteOff;
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
