#include "dungeon_atlas.hpp"

#include "atlas_pack.hpp"
#include "dungeon_gl.hpp"
#include "gl_extra.hpp"

#if defined(_WIN32) && !defined(NOMINMAX)
#define NOMINMAX
#endif
#define GL_SILENCE_DEPRECATION
#include <imgui_impl_opengl3_loader.h>
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#else
#include <unistd.h>
#endif

namespace fs = std::filesystem;

static void (*g_dungeonAtlasFlushDeferredImpl)(int, int) = nullptr;

namespace {

static std::string jsonEscapeSimple(const std::string &s)
{
	std::string o;
	for (unsigned char uc : s) {
		char c = (char)uc;
		switch (c) {
		case '\\':
			o += "\\\\";
			break;
		case '"':
			o += "\\\"";
			break;
		case '\n':
			o += "\\n";
			break;
		case '\r':
			break;
		default:
			o += c;
			break;
		}
	}
	return o;
}

static std::string buildManifestJson(const AtlasPacker &packer, int paddingNote)
{
	std::ostringstream o;
	o << "{\"version\":\"0.0.1-smite_editor\",\"note\":\"Smite dungeon atlas (GL capture + bin-pack).\",";
	o << "\"atlas\":{\"width\":" << packer.width() << ",\"height\":" << packer.height() << "},\"padding\":" << paddingNote << ",\"tiles\":[";
	for (size_t i = 0; i < packer.packed().size(); i++) {
		const PackedEntry &t = packer.packed()[i];
		if (i)
			o << ',';
		o << "{\"name\":\"" << jsonEscapeSimple(t.name) << "\",\"x\":" << t.x << ",\"y\":" << t.y << ",\"w\":" << t.w << ",\"h\":" << t.h << "}";
	}
	o << "]}";
	return o.str();
}

static GLuint g_previewTex = 0;
static int g_previewW = 0, g_previewH = 0;

static void destroyPreviewTex()
{
	if (g_previewTex) {
		glDeleteTextures(1, &g_previewTex);
		g_previewTex = 0;
	}
	g_previewW = g_previewH = 0;
}

static void uploadPreview(const std::vector<unsigned char> &rgba, int w, int h)
{
	destroyPreviewTex();
	if (w < 1 || h < 1 || rgba.size() < (size_t)w * h * 4)
		return;
	glGenTextures(1, &g_previewTex);
	glBindTexture(GL_TEXTURE_2D, g_previewTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
	g_previewW = w;
	g_previewH = h;
}

struct TexEntry {
	std::string name;
	std::string path;
	GLuint gl = 0;
	int w = 0, h = 0;
};

static std::vector<TexEntry> g_texLib;
static bool loadTextureFile(const std::string &path, std::string &err);
static void destroyTexEntry(TexEntry &t)
{
	if (t.gl) {
		glDeleteTextures(1, &t.gl);
		t.gl = 0;
	}
}

struct LoadedImg {
	std::string path;
	std::string label;
	std::vector<unsigned char> rgba;
	int w = 0, h = 0;
};

static std::vector<LoadedImg> g_queue;
static std::vector<char> g_texPathBuf(1024, 0);
static std::vector<char> g_queuePathBuf(1024, 0);
static std::vector<char> g_jsonBuf(1 << 20, 0);
static int g_gutter = 0;
static std::string g_lastErr;
static AtlasPacker g_lastPacker;
static bool g_hasPacked = false;
static std::string g_captureJson;

static DungeonGlSettings g_cap;
static std::vector<DungeonGlLayer> g_layers;
static int g_nextStableIndex = 0;

static std::string nativeExecutableDir()
{
#if defined(_WIN32)
	char buf[MAX_PATH];
	DWORD n = GetModuleFileNameA(nullptr, buf, MAX_PATH);
	if (!n || n >= MAX_PATH)
		return {};
	return fs::path(buf).parent_path().string();
#elif defined(__APPLE__)
	char buf[1024];
	uint32_t sz = sizeof(buf);
	if (_NSGetExecutablePath(buf, &sz) != 0)
		return {};
	std::error_code ec;
	fs::path p = fs::canonical(buf, ec);
	if (ec)
		p = fs::path(buf);
	return p.parent_path().string();
#else
	char buf[4096];
	ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
	if (n <= 0)
		return {};
	buf[n] = '\0';
	return fs::path(buf).parent_path().string();
#endif
}

static bool texLibHasBasename(const char *base)
{
	for (const auto &e : g_texLib) {
		if (e.name == base)
			return true;
	}
	return false;
}

static void tryLoadSampleTexture(const fs::path &pngPath)
{
	if (!fs::is_regular_file(pngPath))
		return;
	std::string base = pngPath.filename().string();
	if (texLibHasBasename(base.c_str()))
		return;
	std::string er;
	loadTextureFile(pngPath.string(), er);
}

static void ensureDefaultSampleTextures()
{
	static const char *kNames[] = {"eob_bricks.png", "eob_keyhole.png"};
	static bool tried = false;
	if (tried)
		return;

	const std::string exe = nativeExecutableDir();
	const fs::path cwd = fs::current_path();

	const fs::path candidates[] = {
#ifdef SMITE_EDITOR_SAMPLE_TEX_DIR
		fs::path(SMITE_EDITOR_SAMPLE_TEX_DIR),
#endif
		exe.empty() ? fs::path() : fs::path(exe) / "sample_textures",
		cwd / "sample_textures",
		cwd / ".." / "sample_textures",
		exe.empty() ? fs::path() : fs::path(exe) / ".." / "sample_textures",
		cwd / ".." / "Release" / "sample_textures",
		cwd / ".." / "Debug" / "sample_textures",
	};

	for (const fs::path &dir : candidates) {
		if (dir.empty())
			continue;
		std::error_code ec;
		fs::path norm = dir.lexically_normal();
		if (!fs::is_directory(norm, ec))
			continue;
		for (const char *name : kNames)
			tryLoadSampleTexture(norm / name);
	}

	tried = true;
}

static void ensureDefaultLayers()
{
	if (!g_layers.empty())
		return;
	g_nextStableIndex = 0;
	auto L = [&](bool on, const char *name, DungeonLayerType t, const char *tex, float sx = 1.f, float sy = 1.f, float oy = 0.f) {
		DungeonGlLayer x;
		x.on = on;
		x.stableIndex = g_nextStableIndex++;
		x.id = (int)g_layers.size() + 1;
		x.name = name;
		x.type = t;
		x.textureName = tex;
		x.scaleX = sx;
		x.scaleY = sy;
		x.offsetY = oy;
		g_layers.push_back(std::move(x));
	};
	L(true, "Grey stone floor", DungeonLayerType::Floor, "eob_bricks.png");
	L(true, "Grey stone ceiling", DungeonLayerType::Ceiling, "eob_bricks.png");
	L(true, "Grey stone walls", DungeonLayerType::Walls, "eob_bricks.png");
	L(true, "Keyhole object", DungeonLayerType::Object, "eob_keyhole.png", 0.25f, 0.5f, -0.15f);
}

static void applyDefaultCaptureSettings()
{
	g_cap.viewportW = 320;
	g_cap.viewportH = 256;
	g_cap.dungeonDepth = 5;
	g_cap.dungeonWidth = 4;
	g_cap.cameraOffsetX = 0.5f;
	g_cap.cameraOffsetY = 0.005f;
	g_cap.cameraOffsetZ = 0.026f;
	g_cap.cameraTiltRad = -0.15f;
	g_cap.cameraFOV = 50.f;
	g_cap.ceilingOffsetY = -0.225f;
	g_cap.tileWidth = 0.f;
	g_cap.cameraShiftZ = -0.115f;
	g_cap.tilePadding = 2;
	g_cap.useFog = false;
	g_cap.fogStart = 0.f;
	g_cap.fogEnd = 3.5f;
	g_cap.fogColor[0] = g_cap.fogColor[1] = g_cap.fogColor[2] = 0.f;
	g_cap.textureFiltering = false;
}

static bool loadTextureFile(const std::string &path, std::string &err)
{
	int nw = 0, nh = 0, nc = 0;
	unsigned char *data = stbi_load(path.c_str(), &nw, &nh, &nc, 4);
	if (!data) {
		err = stbi_failure_reason() ? stbi_failure_reason() : "load failed";
		return false;
	}
	std::string base = fs::path(path).filename().string();
	for (auto &e : g_texLib) {
		if (e.name == base) {
			destroyTexEntry(e);
			GLuint tid = 0;
			glGenTextures(1, &tid);
			glBindTexture(GL_TEXTURE_2D, tid);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, nw, nh, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
			e.path = path;
			e.gl = tid;
			e.w = nw;
			e.h = nh;
			stbi_image_free(data);
			return true;
		}
	}
	GLuint tid = 0;
	glGenTextures(1, &tid);
	glBindTexture(GL_TEXTURE_2D, tid);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, nw, nh, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	stbi_image_free(data);
	TexEntry e;
	e.name = base;
	e.path = path;
	e.gl = tid;
	e.w = nw;
	e.h = nh;
	g_texLib.push_back(std::move(e));
	return true;
}

static void sectionTitle(const char *title)
{
	ImGui::SeparatorText(title);
}

static void drawTexturesTab()
{
	ImGui::TextWrapped(
		"Textures are matched to layers by file name (e.g. walls use eob_bricks.png). "
		"Load each PNG you reference in the Layers tab.");
	ImGui::Spacing();

	ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 88);
	ImGui::InputTextWithHint("##texpath", "Full path to .png file…", g_texPathBuf.data(), g_texPathBuf.size());
	ImGui::SameLine();
	if (ImGui::Button("Add##texadd", ImVec2(80, 0))) {
		std::string p(g_texPathBuf.data());
		g_lastErr.clear();
		if (!p.empty() && fs::exists(p)) {
			std::string er;
			if (!loadTextureFile(p, er))
				g_lastErr = er;
		} else
			g_lastErr = "File not found.";
	}

	if (g_texLib.empty()) {
		ImGui::Spacing();
		ImGui::TextDisabled("No textures loaded yet.");
		return;
	}

	ImGui::Spacing();
	if (ImGui::BeginTable("texlib", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit)) {
		ImGui::TableSetupColumn("Preview", ImGuiTableColumnFlags_WidthFixed, 52.f);
		ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 72.f);
		ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 28.f);
		ImGui::TableHeadersRow();
		for (int i = 0; i < (int)g_texLib.size(); i++) {
			TexEntry &te = g_texLib[i];
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::PushID(i);
			if (te.gl)
				ImGui::Image((ImTextureID)(intptr_t)te.gl, ImVec2(44, 44));
			else
				ImGui::Dummy(ImVec2(44, 44));
			ImGui::TableSetColumnIndex(1);
			ImGui::TextUnformatted(te.name.c_str());
			if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort) && !te.path.empty())
				ImGui::SetTooltip("%s", te.path.c_str());
			ImGui::TableSetColumnIndex(2);
			ImGui::Text("%d×%d", te.w, te.h);
			ImGui::TableSetColumnIndex(3);
			if (ImGui::SmallButton("×")) {
				destroyTexEntry(te);
				g_texLib.erase(g_texLib.begin() + i);
				ImGui::PopID();
				ImGui::EndTable();
				return;
			}
			ImGui::PopID();
		}
		ImGui::EndTable();
	}
}

static void drawSceneTab()
{
	if (ImGui::Button("Reset all to defaults", ImVec2(-1, 0)))
		applyDefaultCaptureSettings();

	/* Table: row height follows content. (BeginChild with height 0 stretches to fill the
	   tab region’s fixed height, which leaves huge empty space when the preview is open.) */
	if (ImGui::BeginTable("scene_cols", 2,
		    ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
		ImGui::TableSetupColumn("scene_l", ImGuiTableColumnFlags_WidthStretch, 0.5f);
		ImGui::TableSetupColumn("scene_r", ImGuiTableColumnFlags_WidthStretch, 0.5f);
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		sectionTitle("Resolution");
		ImGui::SetNextItemWidth(-1);
		ImGui::InputInt("Viewport width", &g_cap.viewportW);
		if (g_cap.viewportW < 32)
			g_cap.viewportW = 32;
		ImGui::SetNextItemWidth(-1);
		ImGui::InputInt("Viewport height", &g_cap.viewportH);
		if (g_cap.viewportH < 32)
			g_cap.viewportH = 32;
		sectionTitle("Dungeon grid");
		ImGui::SetNextItemWidth(-1);
		ImGui::InputInt("Depth (rows)", &g_cap.dungeonDepth);
		if (g_cap.dungeonDepth < 1)
			g_cap.dungeonDepth = 1;
		ImGui::SetNextItemWidth(-1);
		ImGui::InputInt("Width (columns)", &g_cap.dungeonWidth);
		if (g_cap.dungeonWidth < 1)
			g_cap.dungeonWidth = 1;

		ImGui::TableSetColumnIndex(1);
		sectionTitle("Camera");
		ImGui::SetNextItemWidth(-1);
		ImGui::SliderFloat("Offset X", &g_cap.cameraOffsetX, -1.f, 1.f);
		ImGui::SetNextItemWidth(-1);
		ImGui::SliderFloat("Offset Y", &g_cap.cameraOffsetY, -1.f, 1.f);
		ImGui::SetNextItemWidth(-1);
		ImGui::SliderFloat("Offset Z", &g_cap.cameraOffsetZ, -1.f, 1.f);
		ImGui::SetNextItemWidth(-1);
		ImGui::SliderFloat("Tilt (rad)", &g_cap.cameraTiltRad, -1.f, 1.f);
		ImGui::SetNextItemWidth(-1);
		ImGui::SliderFloat("Field of view", &g_cap.cameraFOV, 10.f, 90.f);
		ImGui::EndTable();
	}

	sectionTitle("Wall / ceiling shaping");
	ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.48f);
	ImGui::SliderFloat("Ceiling Y offset", &g_cap.ceilingOffsetY, -1.f, 1.f);
	ImGui::SameLine();
	ImGui::SliderFloat("Tile width", &g_cap.tileWidth, -0.5f, 0.5f);
	ImGui::PopItemWidth();
	ImGui::SetNextItemWidth(-1);
	ImGui::SliderFloat("Shift Z", &g_cap.cameraShiftZ, -1.f, 1.f);
	ImGui::TextDisabled("Tip: adjust Shift Z so vertical wall edges look straight (same as dungeoncrawlers.org).");

	sectionTitle("Raster output");
	ImGui::SetNextItemWidth(200);
	ImGui::InputInt("Tile padding (px)", &g_cap.tilePadding);
	if (g_cap.tilePadding < 0)
		g_cap.tilePadding = 0;
	ImGui::Checkbox("Linear texture filtering", &g_cap.textureFiltering);

	sectionTitle("Fog");
	ImGui::Checkbox("Enable fog", &g_cap.useFog);
	ImGui::BeginDisabled(!g_cap.useFog);
	ImGui::SetNextItemWidth(-1);
	ImGui::SliderFloat("Fog start", &g_cap.fogStart, -10.f, 10.f);
	ImGui::SetNextItemWidth(-1);
	ImGui::SliderFloat("Fog end", &g_cap.fogEnd, 0.f, 25.f);
	ImGui::ColorEdit3("Fog color", g_cap.fogColor, ImGuiColorEditFlags_Float);
	ImGui::EndDisabled();
}

static void drawLayersTab()
{
	ImGui::TextWrapped(
		"Stack order matches the browser tool (e.g. floor → ceiling → walls). "
		"Texture names must match files in the Textures tab.");
	ImGui::Spacing();

	if (ImGui::Button("Add layer", ImVec2(140, 0))) {
		DungeonGlLayer L;
		L.on = true;
		L.stableIndex = g_nextStableIndex++;
		L.id = (int)g_layers.size() + 1;
		L.name = "New layer";
		L.type = DungeonLayerType::Walls;
		L.textureName.clear();
		g_layers.push_back(L);
	}

	ImGui::Spacing();

	for (int li = 0; li < (int)g_layers.size(); li++) {
		DungeonGlLayer &L = g_layers[li];
		ImGui::PushID(L.stableIndex);

		ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 6.f);
		ImVec4 bg = ImGui::GetStyleColorVec4(ImGuiCol_FrameBg);
		bg.w = 0.55f;
		ImGui::PushStyleColor(ImGuiCol_ChildBg, bg);
		char childId[48];
		snprintf(childId, sizeof childId, "layer_%d", L.stableIndex);
		ImGui::BeginChild(childId, ImVec2(-1, 0),
		    ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeY);

		ImGui::Checkbox("##on", &L.on);
		ImGui::SameLine();
		ImGui::SetNextItemWidth(44);
		ImGui::InputInt("##id", &L.id);
		ImGui::SameLine();
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		ImGui::InputTextWithHint("##nm", "Layer name", &L.name);

		ImGui::SetNextItemWidth(130);
		int lt = (int)L.type;
		const char *items[] = {"Walls", "Floor", "Ceiling", "Decal", "Object"};
		ImGui::Combo("Type##lt", &lt, items, 5);
		L.type = (DungeonLayerType)lt;
		ImGui::SameLine();
		std::string curTex = L.textureName.empty() ? std::string("(no texture)") : L.textureName;
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		if (ImGui::BeginCombo("Texture##lt", curTex.c_str())) {
			if (ImGui::Selectable("(no texture)", L.textureName.empty()))
				L.textureName.clear();
			for (const auto &t : g_texLib) {
				bool sel = (L.textureName == t.name);
				if (ImGui::Selectable(t.name.c_str(), sel))
					L.textureName = t.name;
			}
			ImGui::EndCombo();
		}

		if (L.type == DungeonLayerType::Decal || L.type == DungeonLayerType::Object) {
			ImGui::Separator();
			ImGui::TextUnformatted("Decal / object placement");
			ImGui::SetNextItemWidth(-1);
			ImGui::SliderFloat("Scale X", &L.scaleX, 0.001f, 1.f);
			ImGui::SetNextItemWidth(-1);
			ImGui::SliderFloat("Scale Y", &L.scaleY, 0.001f, 1.f);
			ImGui::SetNextItemWidth(-1);
			ImGui::SliderFloat("Offset Y", &L.offsetY, -1.f, 1.f);
		}

		ImGui::Separator();
		ImGui::TextUnformatted("Order");
		ImGui::BeginDisabled(li <= 0);
		if (ImGui::Button("Move up"))
			std::swap(g_layers[li], g_layers[li - 1]);
		ImGui::EndDisabled();
		ImGui::SameLine();
		ImGui::BeginDisabled(li >= (int)g_layers.size() - 1);
		if (ImGui::Button("Move down"))
			std::swap(g_layers[li], g_layers[li + 1]);
		ImGui::EndDisabled();
		ImGui::SameLine();
		const bool canDel = g_layers.size() > 1;
		ImGui::BeginDisabled(!canDel);
		if (ImGui::Button("Remove layer") && canDel) {
			g_layers.erase(g_layers.begin() + li);
			ImGui::EndDisabled();
			ImGui::EndChild();
			ImGui::PopStyleColor();
			ImGui::PopStyleVar();
			ImGui::PopID();
			break;
		}
		ImGui::EndDisabled();

		ImGui::EndChild();
		ImGui::PopStyleColor();
		ImGui::PopStyleVar();
		ImGui::Spacing();
		ImGui::PopID();
	}
}

static bool g_deferredGenerate = false;
static std::string g_glCaptureDebug;
static bool g_playDirty = true; /* forward declaration; also set in Play tab code */

static void runGenerate()
{
	g_lastErr.clear();
	g_hasPacked = false;
	g_captureJson.clear();
	std::vector<DungeonGlTexture> refs;
	for (const auto &t : g_texLib) {
		if (t.gl)
			refs.push_back({t.name, t.gl, t.w, t.h});
	}
	AtlasPacker packer;
	std::string er;
	if (!dungeonGlGenerate(g_cap, g_layers, refs, packer, 0, g_captureJson, er))
		g_lastErr = er;
	else {
		g_lastPacker = std::move(packer);
		g_hasPacked = true;
		uploadPreview(g_lastPacker.pixels(), g_lastPacker.width(), g_lastPacker.height());
		std::string man = buildManifestJson(g_lastPacker, g_cap.tilePadding);
		std::string combo = g_captureJson + "\n\n--- manifest (tile rects) ---\n" + man;
		if (combo.size() + 1 > g_jsonBuf.size())
			g_jsonBuf.resize(combo.size() + 4096);
		memcpy(g_jsonBuf.data(), combo.c_str(), combo.size() + 1);
	}
	g_glCaptureDebug = dungeonGlGetDebugLog();
	g_playDirty = true;
}

static void runPackQueue()
{
	g_lastErr.clear();
	g_hasPacked = false;
	if (g_queue.empty()) {
		g_lastErr = "Queue is empty.";
		return;
	}
	AtlasPacker P;
	P.reset(16, 16);
	for (const LoadedImg &L : g_queue) {
		std::string er;
		if (!P.addImage(L.rgba, L.w, L.h, L.label, g_gutter, er)) {
			g_lastErr = er;
			return;
		}
	}
	g_lastPacker = std::move(P);
	g_hasPacked = true;
	uploadPreview(g_lastPacker.pixels(), g_lastPacker.width(), g_lastPacker.height());
	std::string js = buildManifestJson(g_lastPacker, g_gutter);
	if (js.size() + 1 > g_jsonBuf.size())
		g_jsonBuf.resize(js.size() + 4096);
	memcpy(g_jsonBuf.data(), js.c_str(), js.size() + 1);
}

static void drawBuildTab()
{
	ImGui::TextUnformatted("Render the scene with current settings and pack all tiles into one atlas.");
	ImGui::Spacing();

	ImVec2 bigBtn(ImGui::GetContentRegionAvail().x, 36);
	if (ImGui::Button("Generate atlas (OpenGL capture + pack)", bigBtn))
		g_deferredGenerate = true;

	if (!g_glCaptureDebug.empty()) {
		ImGui::Spacing();
		if (ImGui::TreeNodeEx("OpenGL capture debug log", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::InputTextMultiline("##glcapdbg", &g_glCaptureDebug, ImVec2(-1, 220), ImGuiInputTextFlags_ReadOnly);
			ImGui::TreePop();
		}
	}

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	ImGui::TextUnformatted("Manual pack");
	ImGui::TextWrapped("Skip the 3D pass: add PNG files to a queue and bin-pack them only.");
	ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 100);
	ImGui::InputTextWithHint("##qpath", "Path to PNG…", g_queuePathBuf.data(), g_queuePathBuf.size());
	ImGui::SameLine();
	if (ImGui::Button("Queue", ImVec2(92, 0))) {
		std::string p(g_queuePathBuf.data());
		if (!p.empty() && fs::exists(p)) {
			int nw = 0, nh = 0, nc = 0;
			unsigned char *data = stbi_load(p.c_str(), &nw, &nh, &nc, 4);
			if (data) {
				LoadedImg Ld;
				Ld.path = p;
				Ld.label = fs::path(p).filename().string();
				Ld.w = nw;
				Ld.h = nh;
				Ld.rgba.assign(data, data + (size_t)nw * nh * 4);
				stbi_image_free(data);
				g_queue.push_back(std::move(Ld));
				g_lastErr.clear();
			} else
				g_lastErr = stbi_failure_reason() ? stbi_failure_reason() : "Load failed.";
		} else
			g_lastErr = "File not found.";
	}

	ImGui::SetNextItemWidth(160);
	ImGui::InputInt("Gutter (px)", &g_gutter);
	if (g_gutter < 0)
		g_gutter = 0;

	if (!g_queue.empty()) {
		ImGui::Text("Queued: %zu image(s)", g_queue.size());
		ImGui::BeginChild("qlist", ImVec2(-1, (std::min)(120.f, 24.f + 18.f * (float)g_queue.size())), true);
		for (size_t i = 0; i < g_queue.size(); i++) {
			ImGui::PushID((int)i);
			ImGui::BulletText("%s (%d×%d)", g_queue[i].label.c_str(), g_queue[i].w, g_queue[i].h);
			ImGui::SameLine(ImGui::GetContentRegionAvail().x - 32);
			if (ImGui::SmallButton("×")) {
				g_queue.erase(g_queue.begin() + (ptrdiff_t)i);
				ImGui::PopID();
				break;
			}
			ImGui::PopID();
		}
		ImGui::EndChild();
	}

	if (ImGui::Button("Clear queue", ImVec2(120, 0)))
		g_queue.clear();
	ImGui::SameLine();
	if (ImGui::Button("Pack queue", ImVec2(120, 0)))
		runPackQueue();
}

/* ─── Dungeon Play/Preview (matches JS DungeonRenderer) ─────────────────────── */

static const int kPlayMapSize = 16;
static int g_playMap[kPlayMapSize][kPlayMapSize];
static int g_playerX = 1, g_playerY = 2, g_playerDir = 0;
static bool g_playMapInited = false;
static GLuint g_playTex = 0;
static std::vector<unsigned char> g_playBuf;

static void initPlayMap()
{
	for (int y = 0; y < kPlayMapSize; y++)
		for (int x = 0; x < kPlayMapSize; x++)
			g_playMap[y][x] = (x == 0 || y == 0 || x == kPlayMapSize - 1 || y == kPlayMapSize - 1) ? 1 : 0;

	/* Small maze for testing */
	static const int walls[][2] = {
		{2, 2}, {3, 2}, {4, 2}, {5, 2},
		{7, 1}, {7, 2}, {7, 3}, {7, 4}, {7, 5}, {7, 6},
		{2, 4}, {3, 4}, {4, 4},
		{5, 6}, {5, 7}, {5, 8},
		{9, 2}, {10, 2}, {10, 3}, {10, 4},
		{9, 6}, {10, 6}, {11, 6}, {12, 6},
		{12, 7}, {12, 8}, {12, 9},
		{3, 8}, {3, 9}, {3, 10},
		{5, 10}, {6, 10}, {7, 10},
		{9, 9}, {9, 10}, {9, 11},
		{2, 12}, {3, 12}, {4, 12}, {5, 12}, {6, 12},
		{8, 12}, {9, 12},
		{11, 11}, {12, 11}, {13, 11},
		{14, 3}, {14, 4}, {14, 5},
		{11, 3}, {12, 3}, {13, 3},
	};
	for (const auto &w : walls)
		g_playMap[w[1]][w[0]] = 1;

	g_playerX = 1;
	g_playerY = 2;
	g_playerDir = 0;
	g_playMapInited = true;
	g_playDirty = true;
}

static void destroyPlayTex()
{
	if (g_playTex) {
		glDeleteTextures(1, &g_playTex);
		g_playTex = 0;
	}
}

static bool playCanMove(int x, int y)
{
	if (x < 0 || y < 0 || x >= kPlayMapSize || y >= kPlayMapSize)
		return false;
	return g_playMap[y][x] == 0;
}

static void playMoveForward()
{
	static const int dx[] = {0, 1, 0, -1};
	static const int dy[] = {-1, 0, 1, 0};
	int nx = g_playerX + dx[g_playerDir];
	int ny = g_playerY + dy[g_playerDir];
	if (playCanMove(nx, ny)) {
		g_playerX = nx;
		g_playerY = ny;
		g_playDirty = true;
	}
}

static void playMoveBackward()
{
	static const int dx[] = {0, 1, 0, -1};
	static const int dy[] = {-1, 0, 1, 0};
	int nx = g_playerX - dx[g_playerDir];
	int ny = g_playerY - dy[g_playerDir];
	if (playCanMove(nx, ny)) {
		g_playerX = nx;
		g_playerY = ny;
		g_playDirty = true;
	}
}

static void playTurnLeft()
{
	g_playerDir = (g_playerDir + 3) % 4;
	g_playDirty = true;
}

static void playTurnRight()
{
	g_playerDir = (g_playerDir + 1) % 4;
	g_playDirty = true;
}

static void blitTile(unsigned char *dst, int dstW, int dstH,
	const unsigned char *atlas, int atlasW, int atlasH,
	int sx, int sy, int sw, int sh,
	int dx, int dy, bool flip)
{
	for (int row = 0; row < sh; row++) {
		int dY = dy + row;
		if (dY < 0 || dY >= dstH)
			continue;
		for (int col = 0; col < sw; col++) {
			int dX = dx + col;
			if (dX < 0 || dX >= dstW)
				continue;
			int srcCol = flip ? (sw - 1 - col) : col;
			int srcX = sx + srcCol;
			int srcY = sy + row;
			if (srcX < 0 || srcX >= atlasW || srcY < 0 || srcY >= atlasH)
				continue;
			const unsigned char *sp = &atlas[((size_t)srcY * atlasW + srcX) * 4];
			unsigned char a = sp[3];
			if (a == 0)
				continue;
			unsigned char *dp = &dst[((size_t)dY * dstW + dX) * 4];
			if (a >= 255) {
				dp[0] = sp[0];
				dp[1] = sp[1];
				dp[2] = sp[2];
				dp[3] = 255;
			} else {
				int ia = 255 - a;
				dp[0] = (unsigned char)((sp[0] * a + dp[0] * ia) / 255);
				dp[1] = (unsigned char)((sp[1] * a + dp[1] * ia) / 255);
				dp[2] = (unsigned char)((sp[2] * a + dp[2] * ia) / 255);
				dp[3] = 255;
			}
		}
	}
}

static const DungeonGlLayer::TileOut *findTile(const char *tileType, int tileX, int tileZ,
	const char *layerType)
{
	for (const auto &L : g_layers) {
		if (!L.on)
			continue;
		const char *lt = nullptr;
		switch (L.type) {
		case DungeonLayerType::Walls:
			lt = "Walls";
			break;
		case DungeonLayerType::Floor:
			lt = "Floor";
			break;
		case DungeonLayerType::Ceiling:
			lt = "Ceiling";
			break;
		case DungeonLayerType::Decal:
			lt = "Decal";
			break;
		case DungeonLayerType::Object:
			lt = "Object";
			break;
		}
		if (!lt || strcmp(lt, layerType) != 0)
			continue;
		for (const auto &t : L.tilesOut) {
			if (t.type == tileType && t.tileX == tileX && t.tileZ == tileZ)
				return &t;
		}
	}
	return nullptr;
}

static void worldPos(int relX, int relZ, int dir, int &outX, int &outY)
{
	switch (dir) {
	case 0:
		outX = g_playerX + relX;
		outY = g_playerY + relZ;
		break;
	case 1:
		outX = g_playerX - relZ;
		outY = g_playerY + relX;
		break;
	case 2:
		outX = g_playerX - relX;
		outY = g_playerY - relZ;
		break;
	case 3:
		outX = g_playerX + relZ;
		outY = g_playerY - relX;
		break;
	}
}

static bool isWall(int wx, int wy)
{
	if (wx < 0 || wy < 0 || wx >= kPlayMapSize || wy >= kPlayMapSize)
		return true;
	return g_playMap[wy][wx] == 1;
}

static void renderDungeonView()
{
	if (!g_hasPacked || g_lastPacker.pixels().empty())
		return;

	int vw = g_cap.viewportW, vh = g_cap.viewportH;
	int depth = g_cap.dungeonDepth;
	int width = g_cap.dungeonWidth;
	const unsigned char *atlas = g_lastPacker.pixels().data();
	int aw = g_lastPacker.width(), ah = g_lastPacker.height();

	g_playBuf.assign((size_t)vw * vh * 4, 0);
	unsigned char *dst = g_playBuf.data();

	/* Floor background — draw all floor tiles back-to-front.
	   Atlas already stores pre-flipped pixel data, so never flip in blitTile.
	   For flipped tiles, screenX is the right edge — subtract width to get draw X. */
	for (const auto &L : g_layers) {
		if (!L.on || L.type != DungeonLayerType::Floor)
			continue;
		for (auto it = L.tilesOut.rbegin(); it != L.tilesOut.rend(); ++it) {
			const auto &t = *it;
			if (t.type != "floor")
				continue;
			int dx = t.flipped ? (t.screenX - t.coordW) : t.screenX;
			blitTile(dst, vw, vh, atlas, aw, ah,
				t.coordX, t.coordY, t.coordW, t.coordH,
				dx, t.screenY, false);
		}
		break;
	}

	/* Ceiling background — same approach */
	for (const auto &L : g_layers) {
		if (!L.on || L.type != DungeonLayerType::Ceiling)
			continue;
		for (auto it = L.tilesOut.rbegin(); it != L.tilesOut.rend(); ++it) {
			const auto &t = *it;
			if (t.type != "ceiling")
				continue;
			int dx = t.flipped ? (t.screenX - t.coordW) : t.screenX;
			blitTile(dst, vw, vh, atlas, aw, ah,
				t.coordX, t.coordY, t.coordW, t.coordH,
				dx, t.screenY, false);
		}
		break;
	}

	/* Walls: far to near, sides then fronts */
	for (int z = -depth; z <= 0; z++) {

		/* Side walls */
		for (int x = -(width - 1); x <= (width - 1); x++) {
			int wx, wy;
			worldPos(x, z, g_playerDir, wx, wy);
			if (!isWall(wx, wy))
				continue;
			const auto *t = findTile("side", x, z, "Walls");
			if (!t)
				continue;
			int ddx = t->flipped ? (t->screenX - t->coordW) : t->screenX;
			blitTile(dst, vw, vh, atlas, aw, ah,
				t->coordX, t->coordY, t->coordW, t->coordH,
				ddx, t->screenY, false);
		}

		/* Front walls */
		for (int x = -(width - 1); x <= (width - 1); x++) {
			int wx, wy;
			worldPos(x, z, g_playerDir, wx, wy);
			if (!isWall(wx, wy))
				continue;
			const auto *t = findTile("front", 0, z, "Walls");
			if (!t)
				continue;
			int ddx = t->screenX + (x * t->fullWidth);
			blitTile(dst, vw, vh, atlas, aw, ah,
				t->coordX, t->coordY, t->coordW, t->coordH,
				ddx, t->screenY, false);
		}
	}

	/* Upload to GL texture */
	if (!g_playTex)
		glGenTextures(1, &g_playTex);
	glBindTexture(GL_TEXTURE_2D, g_playTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, vw, vh, 0, GL_RGBA, GL_UNSIGNED_BYTE, dst);
	g_playDirty = false;
}

static void drawPlayTab()
{
	if (!g_playMapInited)
		initPlayMap();

	if (!g_hasPacked) {
		ImGui::TextDisabled("Generate an atlas first (Build tab), then come back here to walk around.");
		return;
	}

	/* Controls */
	ImGui::TextUnformatted("WASD / Arrow keys to move when this tab is focused.");
	ImGui::SameLine();
	static const char *dirNames[] = {"N", "E", "S", "W"};
	ImGui::Text("Pos: (%d,%d) Dir: %s", g_playerX, g_playerY, dirNames[g_playerDir]);

	bool focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
	if (focused) {
		if (ImGui::IsKeyPressed(ImGuiKey_W) || ImGui::IsKeyPressed(ImGuiKey_UpArrow))
			playMoveForward();
		if (ImGui::IsKeyPressed(ImGuiKey_S) || ImGui::IsKeyPressed(ImGuiKey_DownArrow))
			playMoveBackward();
		if (ImGui::IsKeyPressed(ImGuiKey_Q) || ImGui::IsKeyPressed(ImGuiKey_LeftArrow))
			playTurnLeft();
		if (ImGui::IsKeyPressed(ImGuiKey_E) || ImGui::IsKeyPressed(ImGuiKey_RightArrow))
			playTurnRight();
	}

	{
		float bw = 60.f;
		ImGui::Dummy(ImVec2(bw, 0));
		ImGui::SameLine();
		if (ImGui::Button("W", ImVec2(bw, 0)))
			playMoveForward();
		ImGui::SameLine();
		ImGui::Dummy(ImVec2(bw, 0));
		ImGui::SameLine();
		ImGui::Dummy(ImVec2(8, 0));
		ImGui::SameLine();
		if (ImGui::Button("Reset map", ImVec2(0, 0)))
			initPlayMap();

		if (ImGui::Button("Q", ImVec2(bw, 0)))
			playTurnLeft();
		ImGui::SameLine();
		if (ImGui::Button("S", ImVec2(bw, 0)))
			playMoveBackward();
		ImGui::SameLine();
		if (ImGui::Button("E", ImVec2(bw, 0)))
			playTurnRight();
	}

	ImGui::Spacing();

	/* Side-by-side: dungeon view + minimap */
	float avail = ImGui::GetContentRegionAvail().x;
	int vw = g_cap.viewportW, vh = g_cap.viewportH;
	float viewScale = (std::min)(2.0f, (avail * 0.65f) / (float)vw);
	ImVec2 viewSz((float)vw * viewScale, (float)vh * viewScale);

	if (g_playDirty)
		renderDungeonView();

	ImGui::BeginChild("play_layout", ImVec2(-1, viewSz.y + 8), ImGuiChildFlags_None);

	/* Dungeon view */
	if (g_playTex) {
		ImGui::Image((ImTextureID)(intptr_t)g_playTex, viewSz);
	} else {
		ImGui::Dummy(viewSz);
	}

	ImGui::SameLine();

	/* Minimap */
	ImGui::BeginChild("minimap", ImVec2(0, viewSz.y), ImGuiChildFlags_Borders);
	{
		float cellSz = (std::min)(12.f, ImGui::GetContentRegionAvail().x / (float)kPlayMapSize);
		ImVec2 origin = ImGui::GetCursorScreenPos();
		ImDrawList *dl = ImGui::GetWindowDrawList();

		for (int y = 0; y < kPlayMapSize; y++) {
			for (int x = 0; x < kPlayMapSize; x++) {
				ImVec2 tl(origin.x + x * cellSz, origin.y + y * cellSz);
				ImVec2 br(tl.x + cellSz - 1, tl.y + cellSz - 1);
				if (g_playMap[y][x] == 1)
					dl->AddRectFilled(tl, br, IM_COL32(160, 160, 160, 255));
				else
					dl->AddRectFilled(tl, br, IM_COL32(30, 30, 30, 255));
			}
		}

		/* Player */
		float px = origin.x + g_playerX * cellSz + cellSz * 0.5f;
		float py = origin.y + g_playerY * cellSz + cellSz * 0.5f;
		dl->AddCircleFilled(ImVec2(px, py), cellSz * 0.35f, IM_COL32(0, 220, 100, 255));
		static const float arrowDx[] = {0, 1, 0, -1};
		static const float arrowDy[] = {-1, 0, 1, 0};
		float ax = px + arrowDx[g_playerDir] * cellSz * 0.45f;
		float ay = py + arrowDy[g_playerDir] * cellSz * 0.45f;
		dl->AddLine(ImVec2(px, py), ImVec2(ax, ay), IM_COL32(255, 255, 0, 255), 2.f);

		ImGui::Dummy(ImVec2(kPlayMapSize * cellSz, kPlayMapSize * cellSz));

		ImGui::Spacing();
		ImGui::TextWrapped("Click cells to toggle walls:");
		ImGui::SetCursorScreenPos(origin);
		ImGui::InvisibleButton("mapclick", ImVec2(kPlayMapSize * cellSz, kPlayMapSize * cellSz));
		if (ImGui::IsItemClicked()) {
			ImVec2 mp = ImGui::GetMousePos();
			int cx = (int)((mp.x - origin.x) / cellSz);
			int cy = (int)((mp.y - origin.y) / cellSz);
			if (cx >= 0 && cy >= 0 && cx < kPlayMapSize && cy < kPlayMapSize) {
				if (!(cx == g_playerX && cy == g_playerY)) {
					g_playMap[cy][cx] = g_playMap[cy][cx] ? 0 : 1;
					g_playDirty = true;
				}
			}
		}
	}
	ImGui::EndChild();

	ImGui::EndChild();
}

/* ─── Atlas preview panel (build output) ───────────────────────────────────── */

static void drawPreviewPanel()
{
	if (!g_hasPacked || !g_previewTex) {
		ImGui::TextDisabled("No atlas yet. Use Build → Generate or Pack queue.");
		return;
	}

	ImGui::Text("Atlas %d × %d   ·   %zu tiles", g_lastPacker.width(), g_lastPacker.height(), g_lastPacker.packed().size());

	float maxW = ImGui::GetContentRegionAvail().x;
	float scale = (std::min)(1.0f, maxW / (float)g_previewW);
	ImVec2 sz((float)g_previewW * scale, (float)g_previewH * scale);
	ImGui::Image((ImTextureID)(intptr_t)g_previewTex, sz);

	static char pngOut[512] = "dungeon_atlas.png";
	static char jsonOut[512] = "dungeon_atlas.json";
	ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.55f - 8);
	ImGui::InputText("##png", pngOut, sizeof pngOut);
	ImGui::SameLine();
	if (ImGui::Button("Save PNG", ImVec2(100, 0)))
		stbi_write_png(pngOut, g_lastPacker.width(), g_lastPacker.height(), 4, g_lastPacker.pixels().data(), g_lastPacker.width() * 4);

	ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.55f - 8);
	ImGui::InputText("##json", jsonOut, sizeof jsonOut);
	ImGui::SameLine();
	if (ImGui::Button("Save JSON", ImVec2(100, 0))) {
		std::ofstream f(jsonOut, std::ios::binary);
		f << g_jsonBuf.data();
	}

	ImGui::Spacing();
	ImGui::TextUnformatted("Output preview");
	ImGui::InputTextMultiline("##jsonml", g_jsonBuf.data(), g_jsonBuf.size(), ImVec2(-1, ImGui::GetTextLineHeightWithSpacing() * 10), ImGuiInputTextFlags_ReadOnly);
}

static void atlasFlushDeferredGenerateImpl(int framebufferW, int framebufferH)
{
	if (!g_deferredGenerate)
		return;
	g_deferredGenerate = false;
	smiteGlLoadExtra();
	smiteGlBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, framebufferW, framebufferH);
	glDisable(GL_SCISSOR_TEST);
	runGenerate();
	smiteGlBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, framebufferW, framebufferH);
}

struct AtlasDeferredGenerateRegistrar {
	AtlasDeferredGenerateRegistrar()
	{
		g_dungeonAtlasFlushDeferredImpl = atlasFlushDeferredGenerateImpl;
	}
};
static AtlasDeferredGenerateRegistrar s_atlasDeferredGenerateRegistrar;

} // namespace

void dungeonAtlasFlushDeferredGenerate(int framebufferW, int framebufferH)
{
	if (g_dungeonAtlasFlushDeferredImpl)
		g_dungeonAtlasFlushDeferredImpl(framebufferW, framebufferH);
}

void initDungeonAtlasAfterGl()
{
	ensureDefaultSampleTextures();
}

void shutdownDungeonAtlasWindow()
{
	destroyPreviewTex();
	destroyPlayTex();
	for (auto &e : g_texLib)
		destroyTexEntry(e);
	g_texLib.clear();
	dungeonGlShutdown();
}

void drawDungeonAtlasWindow()
{
	static bool capInit = false;
	if (!capInit) {
		capInit = true;
		applyDefaultCaptureSettings();
	}
	ensureDefaultLayers();
	ensureDefaultSampleTextures();

	ImGuiWindowFlags wflags = ImGuiWindowFlags_None;
	if (!ImGui::Begin("Dungeon atlas", nullptr, wflags)) {
		ImGui::End();
		return;
	}

	ImGui::TextWrapped(
		"Build a wall/floor atlas like dungeoncrawlers.org: add textures, tune the virtual camera, stack layers, then generate. "
		"You can also bin-pack arbitrary PNGs without rendering.");
	ImGui::Spacing();

	if (!g_lastErr.empty()) {
		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.45f, 0.12f, 0.12f, 0.35f));
		ImGui::BeginChild("atlas_err", ImVec2(-1, ImGui::GetTextLineHeightWithSpacing() + 12), ImGuiChildFlags_Borders);
		ImGui::TextWrapped("%s", g_lastErr.c_str());
		ImGui::EndChild();
		ImGui::PopStyleColor();
		ImGui::Spacing();
	}

	/* Let the tab block shrink to its content; a fixed-height child here stretched short
	   forms (e.g. Scene) with empty space down to the preview. */
	const float previewMin =
		g_hasPacked && g_previewTex ? (std::min)(280.f, ImGui::GetContentRegionAvail().y * 0.42f) : 0.f;

	ImGui::BeginChild("atlas_tabs_region", ImVec2(-1, 0),
	    ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeY);

	if (ImGui::BeginTabBar("AtlasTabBar", ImGuiTabBarFlags_None)) {
		if (ImGui::BeginTabItem("Textures")) {
			drawTexturesTab();
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Scene")) {
			drawSceneTab();
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Layers")) {
			drawLayersTab();
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Build")) {
			drawBuildTab();
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Play")) {
			drawPlayTab();
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}

	ImGui::EndChild();

	if (previewMin > 0.f) {
		ImGui::Separator();
		ImGui::BeginChild("atlas_preview_region", ImVec2(-1, previewMin), ImGuiChildFlags_Borders);
		drawPreviewPanel();
		ImGui::EndChild();
	}

	ImGui::End();
}
