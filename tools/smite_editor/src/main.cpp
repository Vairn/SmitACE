#include "dungeon_atlas.hpp"
#include "formats.hpp"
#include "maze_editor.hpp"
#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <commdlg.h>
#ifdef _MSC_VER
#pragma comment(lib, "comdlg32.lib")
#endif
#endif
#include <algorithm>
#include <imgui.h>
#include <imgui_internal.h>
#include <misc/cpp/imgui_stdlib.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_opengl3_loader.h>
#include <stdio.h>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>
#define GL_SILENCE_DEPRECATION
#include <GLFW/glfw3.h>

#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

namespace fs = std::filesystem;

static std::string joinPath(const std::string &a, const std::string &b)
{
	if (a.empty()) return b;
	if (!a.empty() && (a.back() == '/' || a.back() == '\\'))
		return a + b;
	return a + "/" + b;
}

struct PlaneView {
	GLuint tex = 0;
	int w = 0;
	int h = 0;
	int index = 0;
	std::string name;
};

static std::vector<PlaneView> g_planes;

static void clearPlanes()
{
	for (PlaneView &p : g_planes) {
		if (p.tex)
			glDeleteTextures(1, &p.tex);
	}
	g_planes.clear();
}

static GLuint uploadRgbaTex(const std::vector<unsigned char> &rgba, int w, int h)
{
	if (w <= 0 || h <= 0 || rgba.size() < (size_t)w * (size_t)h * 4)
		return 0;
	GLuint tex = 0;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
	return tex;
}

static void apply_default_dock_layout(ImGuiID dockspace_id)
{
	ImGui::DockBuilderRemoveNode(dockspace_id);
	ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
	ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetMainViewport()->Size);

	ImGuiID dock_main = dockspace_id;
	ImGuiID dock_left = 0;
	ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Left, 0.22f, &dock_left, &dock_main);
	ImGuiID dock_right = 0;
	ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Right, 0.28f, &dock_right, &dock_main);
	ImGuiID dock_bottom = 0;
	ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Down, 0.50f, &dock_bottom, &dock_main);

	ImGui::DockBuilderDockWindow("Project", dock_left);
	ImGui::DockBuilderDockWindow("Wallset preview", dock_main);
	ImGui::DockBuilderDockWindow("Maze (level01.maze)", dock_bottom);
	ImGui::DockBuilderDockWindow("Items", dock_right);
	ImGui::DockBuilderDockWindow("Monsters", dock_right);
	ImGui::DockBuilderDockWindow("Level entities (.lvl)", dock_right);
	ImGui::DockBuilderDockWindow("Dungeon atlas", dock_right);
	ImGui::DockBuilderFinish(dockspace_id);
}

struct MonShot {
	GLuint tex = 0;
	int w = 0;
	int h = 0;
};

static std::vector<std::vector<MonShot>> g_monShots;
static std::vector<std::string> g_monPng;

static void clearMonShots()
{
	for (auto &row : g_monShots) {
		for (auto &s : row) {
			if (s.tex) {
				glDeleteTextures(1, &s.tex);
				s.tex = 0;
			}
			s.w = 0;
			s.h = 0;
		}
	}
	g_monShots.clear();
}

static void uploadMonShot(MonShot &s, const std::vector<unsigned char> &rgba, int w, int h)
{
	if (s.tex) {
		glDeleteTextures(1, &s.tex);
		s.tex = 0;
	}
	s.w = 0;
	s.h = 0;
	if (w <= 0 || h <= 0 || rgba.size() < (size_t)w * (size_t)h * 4)
		return;
	glGenTextures(1, &s.tex);
	glBindTexture(GL_TEXTURE_2D, s.tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
	s.w = w;
	s.h = h;
}

static std::string monsterFramesDir(const std::string &datPath, const std::string &root)
{
	std::string p = datPath.empty() ? joinPath(root, "monsters.dat") : datPath;
	size_t slash = p.find_last_of("/\\");
	std::string dir = (slash == std::string::npos) ? std::string() : p.substr(0, slash + 1);
	return dir + "monsters";
}

/** WinUAE reads the Amiga build's data/, not the editor's. Copy baked frames there. */
static void mirrorMonsterFrames(const std::string &outDir)
{
	std::error_code ec;
	fs::path src(outDir);
	if (!fs::is_directory(src, ec))
		return;
	fs::path dataDir = src.parent_path();
	fs::path buildDir = dataDir.parent_path().parent_path();
	auto copyInto = [&](const fs::path &destData) {
		if (destData.empty() || destData == dataDir)
			return;
		std::error_code e2;
		if (!fs::is_regular_file(destData / "monsters.dat", e2))
			return;
		fs::path dest = destData / "monsters";
		fs::create_directories(dest, e2);
		for (const fs::directory_entry &file : fs::directory_iterator(src, e2)) {
			if (!file.is_regular_file())
				continue;
			fs::copy_file(file.path(), dest / file.path().filename(),
				fs::copy_options::overwrite_existing, e2);
		}
	};
	if (fs::is_directory(buildDir, ec)) {
		for (const fs::directory_entry &ent : fs::directory_iterator(buildDir, ec)) {
			if (ent.is_directory())
				copyInto(ent.path() / "data");
		}
		copyInto(buildDir.parent_path() / "data");
	}
}

static void refreshMonsterPreviews(const std::string &dir, const std::vector<MonsterRow> &mons, const WallsetFile &wst)
{
	clearMonShots();
	g_monShots.resize(mons.size());
	if (g_monPng.size() < mons.size())
		g_monPng.resize(mons.size());
	if (wst.palette.empty())
		return;
	int palN = (int)wst.palette.size() / 3;
	for (size_t i = 0; i < mons.size(); i++) {
		for (int f = 0; f < 16; f++) {
			std::string stem = joinPath(dir, std::to_string(i) + "_f" + std::to_string(f) + "_d0");
			MonsterFrameImage img;
			std::string er;
			if (!loadMonsterFramePreview(stem + ".bm", stem + ".msk", wst.palette.data(), palN, img, er))
				break;
			MonShot shot;
			uploadMonShot(shot, img.rgba, img.w, img.h);
			g_monShots[i].push_back(shot);
		}
		if (!g_monShots[i].empty())
			continue;
		for (int d = 0; d < 4; d++) {
			std::string stem = joinPath(dir, std::to_string(i) + "_d" + std::to_string(d));
			MonsterFrameImage img;
			std::string er;
			if (!loadMonsterFramePreview(stem + ".bm", stem + ".msk", wst.palette.data(), palN, img, er))
				continue;
			MonShot shot;
			uploadMonShot(shot, img.rgba, img.w, img.h);
			g_monShots[i].push_back(shot);
		}
	}
}

static bool browsePng(std::string &out)
{
#if defined(_WIN32)
	char file[MAX_PATH] = "";
	if (!out.empty()) {
		strncpy(file, out.c_str(), MAX_PATH - 1);
		file[MAX_PATH - 1] = '\0';
	}
	OPENFILENAMEA ofn;
	memset(&ofn, 0, sizeof ofn);
	ofn.lStructSize = sizeof ofn;
	ofn.lpstrFilter = "PNG image\0*.png\0All files\0*.*\0";
	ofn.lpstrFile = file;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
	ofn.lpstrDefExt = "png";
	if (!GetOpenFileNameA(&ofn))
		return false;
	out = file;
	return true;
#else
	(void)out;
	return false;
#endif
}

static ImVec4 rgba(int r, int g, int b, int a = 255)
{
	return ImVec4(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
}

static void load_editor_font(ImGuiIO &io)
{
	const char *paths[] = {
		"C:/Windows/Fonts/segoeui.ttf",
		"C:/Windows/Fonts/SegoeUI.ttf",
	};
	for (const char *path : paths) {
		if (!fs::exists(path))
			continue;
		if (io.Fonts->AddFontFromFileTTF(path, 16.0f))
			return;
	}
}

static void apply_editor_style()
{
	ImGui::StyleColorsDark();
	ImGuiStyle &s = ImGui::GetStyle();
	s.FontSizeBase = 16.0f;
	s.WindowRounding = 8.0f;
	s.ChildRounding = 6.0f;
	s.FrameRounding = 6.0f;
	s.PopupRounding = 8.0f;
	s.ScrollbarRounding = 8.0f;
	s.GrabRounding = 6.0f;
	s.TabRounding = 6.0f;
	s.WindowBorderSize = 1.0f;
	s.ChildBorderSize = 1.0f;
	s.PopupBorderSize = 1.0f;
	s.FrameBorderSize = 0.0f;
	s.TabBorderSize = 0.0f;
	s.TabBarBorderSize = 0.0f;
	s.TabBarOverlineSize = 2.0f;
	s.WindowPadding = ImVec2(14.0f, 12.0f);
	s.FramePadding = ImVec2(10.0f, 6.0f);
	s.ItemSpacing = ImVec2(8.0f, 8.0f);
	s.ItemInnerSpacing = ImVec2(6.0f, 4.0f);
	s.CellPadding = ImVec2(8.0f, 6.0f);
	s.ScrollbarSize = 12.0f;
	s.ScrollbarPadding = 3.0f;
	s.GrabMinSize = 10.0f;
	s.DockingSeparatorSize = 1.0f;
	s.SeparatorTextPadding = ImVec2(12.0f, 6.0f);
	s.WindowMenuButtonPosition = ImGuiDir_None;

	const ImVec4 text = rgba(236, 237, 241);
	const ImVec4 muted = rgba(154, 157, 168);
	const ImVec4 bg = rgba(18, 19, 23);
	const ImVec4 surface = rgba(28, 29, 34);
	const ImVec4 raised = rgba(40, 42, 49);
	const ImVec4 hover = rgba(52, 55, 64);
	const ImVec4 active = rgba(62, 66, 78);
	const ImVec4 line = rgba(255, 255, 255, 22);
	const ImVec4 accent = rgba(129, 161, 255);
	const ImVec4 accentSoft = rgba(129, 161, 255, 56);

	ImVec4 *c = s.Colors;
	c[ImGuiCol_Text] = text;
	c[ImGuiCol_TextDisabled] = muted;
	c[ImGuiCol_WindowBg] = bg;
	c[ImGuiCol_ChildBg] = ImVec4(0, 0, 0, 0);
	c[ImGuiCol_PopupBg] = surface;
	c[ImGuiCol_Border] = line;
	c[ImGuiCol_BorderShadow] = ImVec4(0, 0, 0, 0);
	c[ImGuiCol_FrameBg] = raised;
	c[ImGuiCol_FrameBgHovered] = hover;
	c[ImGuiCol_FrameBgActive] = active;
	c[ImGuiCol_TitleBg] = surface;
	c[ImGuiCol_TitleBgActive] = surface;
	c[ImGuiCol_TitleBgCollapsed] = surface;
	c[ImGuiCol_MenuBarBg] = surface;
	c[ImGuiCol_ScrollbarBg] = ImVec4(0, 0, 0, 0);
	c[ImGuiCol_ScrollbarGrab] = rgba(90, 94, 108, 180);
	c[ImGuiCol_ScrollbarGrabHovered] = rgba(120, 126, 144, 220);
	c[ImGuiCol_ScrollbarGrabActive] = accent;
	c[ImGuiCol_CheckMark] = accent;
	c[ImGuiCol_SliderGrab] = accent;
	c[ImGuiCol_SliderGrabActive] = rgba(176, 196, 255);
	c[ImGuiCol_Button] = raised;
	c[ImGuiCol_ButtonHovered] = hover;
	c[ImGuiCol_ButtonActive] = accentSoft;
	c[ImGuiCol_Header] = accentSoft;
	c[ImGuiCol_HeaderHovered] = rgba(129, 161, 255, 80);
	c[ImGuiCol_HeaderActive] = rgba(129, 161, 255, 110);
	c[ImGuiCol_Separator] = line;
	c[ImGuiCol_SeparatorHovered] = accent;
	c[ImGuiCol_SeparatorActive] = accent;
	c[ImGuiCol_ResizeGrip] = rgba(255, 255, 255, 16);
	c[ImGuiCol_ResizeGripHovered] = accentSoft;
	c[ImGuiCol_ResizeGripActive] = accent;
	c[ImGuiCol_Tab] = bg;
	c[ImGuiCol_TabHovered] = hover;
	c[ImGuiCol_TabSelected] = surface;
	c[ImGuiCol_TabSelectedOverline] = accent;
	c[ImGuiCol_TabDimmed] = bg;
	c[ImGuiCol_TabDimmedSelected] = surface;
	c[ImGuiCol_TabDimmedSelectedOverline] = rgba(129, 161, 255, 120);
	c[ImGuiCol_DockingPreview] = rgba(129, 161, 255, 90);
	c[ImGuiCol_DockingEmptyBg] = bg;
	c[ImGuiCol_TableHeaderBg] = raised;
	c[ImGuiCol_TableBorderStrong] = line;
	c[ImGuiCol_TableBorderLight] = rgba(255, 255, 255, 12);
	c[ImGuiCol_TableRowBg] = ImVec4(0, 0, 0, 0);
	c[ImGuiCol_TableRowBgAlt] = rgba(255, 255, 255, 8);
	c[ImGuiCol_TextSelectedBg] = accentSoft;
	c[ImGuiCol_NavHighlight] = accent;
	c[ImGuiCol_NavWindowingHighlight] = rgba(255, 255, 255, 40);
	c[ImGuiCol_ModalWindowDimBg] = rgba(0, 0, 0, 140);
}

int main(int, char **)
{
	const bool need_default_dock = !fs::exists("smite_editor.ini");

	glfwSetErrorCallback([](int code, const char *desc) {
		fprintf(stderr, "GLFW error %d: %s\n", code, desc);
	});
	if (!glfwInit())
		return 1;

#if GLFW_VERSION_MAJOR > 3 || (GLFW_VERSION_MAJOR == 3 && GLFW_VERSION_MINOR >= 3)
	glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);
#endif

#if defined(IMGUI_IMPL_OPENGL_ES2)
	const char *glsl_version = "#version 100";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
	const char *glsl_version = "#version 150";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#else
	/* Windows/Linux: OpenGL 4.2 core (dungeon capture + ImGui). */
	const char *glsl_version = "#version 420";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif

	GLFWwindow *window = glfwCreateWindow(1280, 720, "Smite Authoring Tool", nullptr, nullptr);
	if (!window) {
		glfwTerminate();
		return 1;
	}
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigDpiScaleFonts = true;
	io.IniFilename = "smite_editor.ini";

	load_editor_font(io);
	apply_editor_style();

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(glsl_version);
	initDungeonAtlasAfterGl();

	static char s_root[512] = "data";
	static char s_err[512] = "";
	GameManifest gMan;
	std::vector<ItemRow> gItems;
	std::vector<MonsterRow> gMons;
	MazeFile gMaze;
	LvlEntities gLvl;
	WallsetFile gWst;
	std::string gWallPath;
	bool dirty = false;
	bool default_dock_pending = need_default_dock;
	int editLevel = 0;
	bool editLevelSet = false;

	auto note = [&](const std::string &er) {
		if (s_err[0] == 0 && !er.empty())
			snprintf(s_err, sizeof s_err, "%s", er.c_str());
	};

	auto loadLevelAssets = [&]() {
		std::string er;
		std::string root(s_root);
		if (gMan.levels.empty()) {
			gMaze = MazeFile();
			gLvl = LvlEntities();
			return;
		}
		if (editLevel < 0)
			editLevel = 0;
		if (editLevel >= (int)gMan.levels.size())
			editLevel = (int)gMan.levels.size() - 1;
		if (!loadLevelFromManifest(gMan, editLevel, gMaze, gLvl, er))
			note(er);
		const GameManifest::Level &lv = gMan.levels[editLevel];
		std::string wpath = lv.wallsetPath.empty() ? joinPath(root, "factory2/factory2.wll") : lv.wallsetPath;
		gWallPath = wpath;
		gWst = WallsetFile();
		clearPlanes();
		if (!loadWallsetMain(wpath, gWst, er)) {
			note(er);
			return;
		}
		std::string base = wpath;
		size_t dot = base.find_last_of('.');
		if (dot != std::string::npos)
			base.resize(dot);
		int palN = (int)gWst.palette.size() / 3;
		static const unsigned char maskPal[] = {0, 0, 0, 255, 255, 255};
		for (int pi = 0; pi < gWst.gfxCount; pi++) {
			std::string pln = base + "_" + std::to_string(pi) + ".pln";
			std::vector<unsigned char> plnData;
			std::string perr;
			if (!readRawFile(pln, plnData, perr) || plnData.empty() || palN < 1)
				continue;
			std::vector<unsigned char> rgba;
			int rw = 0, rh = 0;
			if (!decodeAcePlanar(plnData, gWst.palette.data(), palN, rgba, rw, rh, perr)) {
				note(pln + ": " + perr);
				continue;
			}
			std::string msk = base + "_" + std::to_string(pi) + ".msk";
			std::vector<unsigned char> mskData;
			std::string merr;
			std::vector<unsigned char> maskRgba;
			int mw = 0, mh = 0;
			if (readRawFile(msk, mskData, merr)
				&& decodeAcePlanar(mskData, maskPal, 2, maskRgba, mw, mh, merr)
				&& mw == rw && mh == rh) {
				for (int px = 0; px < rw * rh; px++)
					rgba[(size_t)px * 4 + 3] = maskRgba[(size_t)px * 4] ? 255 : 0;
			}
			PlaneView view;
			view.tex = uploadRgbaTex(rgba, rw, rh);
			view.w = rw;
			view.h = rh;
			view.index = pi;
			view.name = pln;
			if (view.tex)
				g_planes.push_back(std::move(view));
		}
		if (!g_planes.empty())
			gWallPath = g_planes.front().name;
	};

	auto reloadAll = [&]() {
		s_err[0] = 0;
		std::string er;
		std::string root(s_root);
		if (!loadGameManifest(joinPath(root, "game.smt"), gMan, er)) {
			gMan = GameManifest{};
			gMan.levelCount = 1;
			gMan.levels.resize(1);
			gMan.itemsPath = "data/items.dat";
			gMan.monstersPath = "data/monsters.dat";
			gMan.uiPalettePath = "data/playfield.plt";
			gMan.levels[0].wallsetPath = "data/factory2/factory2.wll";
			gMan.levels[0].entitiesPath = "data/level00.lvl";
			note(er.empty() ? "manifest" : er);
		}
		if (!editLevelSet) {
			editLevel = gMan.startLevel;
			editLevelSet = true;
		}
		if (gMan.itemsPath.empty())
			gMan.itemsPath = joinPath(root, "items.dat");
		if (gMan.monstersPath.empty())
			gMan.monstersPath = joinPath(root, "monsters.dat");
		if (!loadItemsDat(gMan.itemsPath, gItems, er))
			note(er);
		if (!loadMonstersDat(gMan.monstersPath, gMons, er))
			note(er);
		loadLevelAssets();
		dirty = false;
		refreshMonsterPreviews(monsterFramesDir(gMan.monstersPath, root), gMons, gWst);
	};

	reloadAll();

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		int fbW, fbH, winW, winH;
		glfwGetFramebufferSize(window, &fbW, &fbH);
		glfwGetWindowSize(window, &winW, &winH);
		if (winW > 0 && winH > 0)
			io.DisplayFramebufferScale = ImVec2((float)fbW / (float)winW, (float)fbH / (float)winH);

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		const ImGuiID dockspace_id = ImGui::GetID("SmiteEditorDock");
		if (default_dock_pending) {
			default_dock_pending = false;
			apply_default_dock_layout(dockspace_id);
		}

		if (ImGui::BeginMainMenuBar()) {
			if (ImGui::BeginMenu("File")) {
				if (ImGui::MenuItem("Reload all from folder"))
					reloadAll();
				if (ImGui::MenuItem("Quit"))
					glfwSetWindowShouldClose(window, 1);
				ImGui::EndMenu();
			}
			ImGui::EndMainMenuBar();
		}

		ImGui::DockSpaceOverViewport(dockspace_id, nullptr, ImGuiDockNodeFlags_None);

		if (ImGui::Begin("Project")) {
			ImGui::InputText("Data root", s_root, sizeof s_root);
			if (ImGui::Button("Reload"))
				reloadAll();
			if (s_err[0])
				ImGui::TextColored(ImVec4(1, 0.4f, 0.4f, 1), "%s", s_err);
			ImGui::Separator();
			ImGui::InputInt("Start level", &gMan.startLevel);
			ImGui::InputInt("Level count", &gMan.levelCount);
			if (gMan.levelCount < 1) gMan.levelCount = 1;
			if ((int)gMan.levels.size() < gMan.levelCount)
				gMan.levels.resize(gMan.levelCount);
			if ((int)gMan.levels.size() > gMan.levelCount)
				gMan.levels.resize(gMan.levelCount);
			if (ImGui::InputInt("Editing level", &editLevel))
				loadLevelAssets();
			ImGui::SameLine();
			if (ImGui::Button("Load this level")) {
				s_err[0] = 0;
				loadLevelAssets();
				refreshMonsterPreviews(monsterFramesDir(gMan.monstersPath, s_root), gMons, gWst);
			}
			ImGui::InputText("Items", &gMan.itemsPath);
			ImGui::InputText("Monsters", &gMan.monstersPath);
			ImGui::InputText("UI palette", &gMan.uiPalettePath);
			for (int i = 0; i < gMan.levelCount; i++) {
				ImGui::PushID(i);
				ImGui::Text("Level %d", i);
				ImGui::InputText("Maze", &gMan.levels[i].mazePath);
				ImGui::InputText("Wallset", &gMan.levels[i].wallsetPath);
				ImGui::InputText("Entities .lvl", &gMan.levels[i].entitiesPath);
				ImGui::PopID();
			}
			if (ImGui::Button("Save game.smt")) {
				std::string er;
				if (saveGameManifest(joinPath(s_root, "game.smt"), gMan, er))
					dirty = false;
				else
					snprintf(s_err, sizeof s_err, "%s", er.c_str());
			}
		}
		ImGui::End();

		if (ImGui::Begin("Items")) {
			if (ImGui::Button("Add row")) {
				gItems.push_back(ItemRow{});
				dirty = true;
			}
			ImGui::SameLine();
			if (ImGui::Button("Save items.dat")) {
				std::string er;
				if (gMan.itemsPath.empty())
					gMan.itemsPath = joinPath(s_root, "items.dat");
				if (!saveItemsDat(gMan.itemsPath, gItems, er))
					snprintf(s_err, sizeof s_err, "%s", er.c_str());
			}
			for (size_t i = 0; i < gItems.size(); i++) {
				ImGui::PushID((int)i);
				ItemRow &r = gItems[i];
				ImGui::InputScalar("type", ImGuiDataType_U8, &r.type);
				ImGui::InputScalar("flags", ImGuiDataType_U8, &r.flags);
				ImGui::InputText("name", &r.name);
				ImGui::PopID();
			}
		}
		ImGui::End();

		if (ImGui::Begin("Monsters")) {
			if (ImGui::Button("Add")) {
				gMons.push_back(MonsterRow{});
				g_monPng.emplace_back();
				g_monShots.emplace_back();
				dirty = true;
			}
			ImGui::SameLine();
			if (ImGui::Button("Save monsters.dat")) {
				std::string er;
				if (gMan.monstersPath.empty())
					gMan.monstersPath = joinPath(s_root, "monsters.dat");
				if (g_monPng.size() < gMons.size())
					g_monPng.resize(gMons.size());
				if (g_monShots.size() < gMons.size())
					g_monShots.resize(gMons.size());
				std::string bakeErr;
				bool anyPicture = false;
				for (const std::string &png : g_monPng) {
					if (!png.empty())
						anyPicture = true;
				}
				if (anyPicture && gWst.palette.empty())
					bakeErr = "Load a wallset (Reload) before saving creature pictures.";
				else if (anyPicture) {
					std::string outDir = monsterFramesDir(gMan.monstersPath, s_root);
					for (size_t i = 0; i < gMons.size(); i++) {
						if (g_monPng[i].empty())
							continue;
						std::vector<MonsterFrameImage> frames;
						std::string one;
						if (!exportMonsterFrames(g_monPng[i], outDir, (int)i, gWst, one, &frames)) {
							if (bakeErr.empty())
								bakeErr = "Monster " + std::to_string(i) + ": " + one;
							continue;
						}
						for (MonShot &old : g_monShots[i]) {
							if (old.tex)
								glDeleteTextures(1, &old.tex);
						}
						g_monShots[i].clear();
						for (const MonsterFrameImage &fr : frames) {
							MonShot shot;
							uploadMonShot(shot, fr.rgba, fr.w, fr.h);
							g_monShots[i].push_back(shot);
						}
					}
					if (bakeErr.empty())
						mirrorMonsterFrames(outDir);
				}
				if (!saveMonstersDat(gMan.monstersPath, gMons, er))
					snprintf(s_err, sizeof s_err, "%s", er.c_str());
				else if (!bakeErr.empty())
					snprintf(s_err, sizeof s_err, "%s", bakeErr.c_str());
				else
					s_err[0] = 0;
			}
			bool canImport = !gWst.palette.empty();
			if (!canImport)
				ImGui::TextDisabled("Load a wallset (Reload) before importing a creature.");
			else
				ImGui::TextDisabled("One row: Front, Back, Left1, Left2, Attack1, Attack2. Save bakes them and mirrors Left into Right.");
			if (g_monPng.size() < gMons.size())
				g_monPng.resize(gMons.size());
			if (g_monShots.size() < gMons.size())
				g_monShots.resize(gMons.size());
			for (size_t i = 0; i < gMons.size(); i++) {
				ImGui::PushID((int)i);
				ImGui::Separator();
				{
					std::string frames = monsterFramesDir(gMan.monstersPath, s_root);
					ImGui::Text("Index %zu  ->  %s/%zu_f0_d0.bm ...", i, frames.c_str(), i);
				}
				MonsterRow &r = gMons[i];
				ImGui::InputInt("level", (int *)&r.level);
				ImGui::InputInt("maxHp", (int *)&r.maxHp);
				ImGui::InputInt("atk", (int *)&r.attack);
				ImGui::InputInt("def", (int *)&r.defense);
				ImGui::InputInt("xp", (int *)&r.experience);
				ImGui::InputText("name", &r.name);
				ImGui::InputText("image", &g_monPng[i]);
				ImGui::SameLine();
				if (ImGui::Button("Browse"))
					browsePng(g_monPng[i]);
				ImGui::BeginDisabled(!canImport || g_monPng[i].empty());
				if (ImGui::Button("Import")) {
					std::vector<MonsterFrameImage> frames;
					std::string er;
					std::string outDir = monsterFramesDir(gMan.monstersPath, s_root);
					if (exportMonsterFrames(g_monPng[i], outDir, (int)i, gWst, er, &frames)) {
						for (MonShot &old : g_monShots[i]) {
							if (old.tex)
								glDeleteTextures(1, &old.tex);
						}
						g_monShots[i].clear();
						for (const MonsterFrameImage &fr : frames) {
							MonShot shot;
							uploadMonShot(shot, fr.rgba, fr.w, fr.h);
							g_monShots[i].push_back(shot);
						}
						s_err[0] = 0;
						mirrorMonsterFrames(outDir);
					} else {
						snprintf(s_err, sizeof s_err, "%s", er.c_str());
					}
				}
				ImGui::EndDisabled();
				static const char *kPoseName[] = {
					"Front", "Back", "Left1", "Left2", "Right1", "Right2", "Attack1", "Attack2"
				};
				for (int f = 0; f < (int)g_monShots[i].size(); f++) {
					if (!g_monShots[i][f].tex)
						continue;
					if (f < 8)
						ImGui::Text("%s", kPoseName[f]);
					else
						ImGui::Text("f%d", f);
					ImGui::SameLine();
					float scale = 1.0f;
					if (g_monShots[i][f].h > 96)
						scale = 96.0f / (float)g_monShots[i][f].h;
					ImGui::Image((ImTextureID)(intptr_t)g_monShots[i][f].tex,
						ImVec2((float)g_monShots[i][f].w * scale, (float)g_monShots[i][f].h * scale));
				}
				ImGui::PopID();
			}
		}
		ImGui::End();

		std::vector<DungeonSheet> sheets;
		sheets.reserve(g_planes.size());
		for (const PlaneView &p : g_planes) {
			DungeonSheet s;
			s.tex = p.tex;
			s.w = p.w;
			s.h = p.h;
			s.index = p.index;
			sheets.push_back(s);
		}
		drawMazeEditor(gMaze, gLvl, gMons, gItems, gWst, sheets, gMan, editLevel, s_root, [&]() {
			s_err[0] = 0;
			loadLevelAssets();
			refreshMonsterPreviews(monsterFramesDir(gMan.monstersPath, s_root), gMons, gWst);
		}, s_err, (int)sizeof s_err);

		if (ImGui::Begin("Level entities (.lvl)")) {
			const char *entPath = "(none)";
			if (editLevel >= 0 && editLevel < (int)gMan.levels.size() && !gMan.levels[editLevel].entitiesPath.empty())
				entPath = gMan.levels[editLevel].entitiesPath.c_str();
			ImGui::Text("File: %s", entPath);
			if (ImGui::Button("Reload .lvl")) {
				s_err[0] = 0;
				loadLevelAssets();
			}
			ImGui::SameLine();
			if (ImGui::Button("Save .lvl")) {
				std::string er;
				if (!saveEntitiesToManifest(gMan, editLevel, gLvl, s_root, er))
					snprintf(s_err, sizeof s_err, "%s", er.c_str());
				else
					s_err[0] = 0;
			}
			ImGui::Text("Wall btns %zu | Door %zu | Plates %zu | Ground %zu | Mon %zu",
				gLvl.wallBtns.size(), gLvl.doorBtns.size(), gLvl.plates.size(),
				gLvl.ground.size(), gLvl.monsters.size());
			if (ImGui::Button("+Wall btn")) {
				LvlEntities::WB w{};
				gLvl.wallBtns.push_back(w);
			}
			if (ImGui::Button("+Monster spawn")) {
				LvlEntities::MS m{};
				gLvl.monsters.push_back(m);
			}
		}
		ImGui::End();

		if (ImGui::Begin("Wallset preview")) {
			ImGui::Text("Tiles: %d  Planes: %d  Shown: %d", gWst.totalTiles, gWst.gfxCount, (int)g_planes.size());
			ImGui::TextWrapped("%s", gWallPath.empty() ? "No wallset loaded." : gWallPath.c_str());
			for (const PlaneView &p : g_planes) {
				ImGui::Separator();
				ImGui::TextUnformatted(p.name.c_str());
				float maxW = ImGui::GetContentRegionAvail().x;
				float scale = p.w > 0 ? (std::min)(1.0f, maxW / (float)p.w) : 1.0f;
				ImGui::Image((ImTextureID)(intptr_t)p.tex, ImVec2((float)p.w * scale, (float)p.h * scale));
			}
		}
		ImGui::End();

		drawDungeonAtlasWindow();

		ImGui::Render();
		glfwGetFramebufferSize(window, &fbW, &fbH);
		glViewport(0, 0, fbW, fbH);
		glDisable(GL_SCISSOR_TEST);
		dungeonAtlasFlushDeferredGenerate(fbW, fbH);
		glViewport(0, 0, fbW, fbH);
		glClearColor(18.0f / 255.0f, 19.0f / 255.0f, 23.0f / 255.0f, 1);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		glfwSwapBuffers(window);
	}

	clearPlanes();
	clearMonShots();
	shutdownDungeonAtlasWindow();
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
