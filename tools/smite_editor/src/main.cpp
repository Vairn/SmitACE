#include "formats.hpp"
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

static unsigned int g_tex = 0;
static int g_texW = 0, g_texH = 0;

static void uploadRgba(const std::vector<unsigned char> &rgba, int w, int h)
{
	if (g_tex) {
		glDeleteTextures(1, &g_tex);
		g_tex = 0;
	}
	if (w <= 0 || h <= 0 || rgba.size() < (size_t)w * h * 4)
		return;
	glGenTextures(1, &g_tex);
	glBindTexture(GL_TEXTURE_2D, g_tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
	g_texW = w;
	g_texH = h;
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
	ImGui::DockBuilderFinish(dockspace_id);
}

static void apply_editor_style()
{
	ImGui::StyleColorsDark();
	ImGuiStyle &s = ImGui::GetStyle();
	s.WindowRounding = 5.0f;
	s.ChildRounding = 4.0f;
	s.FrameRounding = 4.0f;
	s.PopupRounding = 4.0f;
	s.ScrollbarRounding = 9.0f;
	s.GrabRounding = 3.0f;
	s.TabRounding = 4.0f;
	s.WindowBorderSize = 1.0f;
	s.ChildBorderSize = 1.0f;
	s.FrameBorderSize = 0.0f;
	s.WindowPadding = ImVec2(10.0f, 10.0f);
	s.ItemSpacing = ImVec2(10.0f, 6.0f);
	ImVec4 accent(0.15f, 0.55f, 0.52f, 1.0f);
	s.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.15f, 0.45f, 0.42f, 0.86f);
	s.Colors[ImGuiCol_HeaderActive] = accent;
	s.Colors[ImGuiCol_CheckMark] = accent;
	s.Colors[ImGuiCol_TabActive] = ImVec4(0.12f, 0.22f, 0.22f, 1.0f);
	s.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.10f, 0.30f, 0.28f, 1.0f);
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
	const char *glsl_version = "#version 130";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
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

	apply_editor_style();

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(glsl_version);

	static char s_root[512] = "data";
	static char s_err[512] = "";
	GameManifest gMan;
	std::vector<ItemRow> gItems;
	std::vector<MonsterRow> gMons;
	MazeFile gMaze;
	LvlEntities gLvl;
	WallsetFile gWst;
	std::vector<unsigned char> gPlnData;
	bool dirty = false;
	bool default_dock_pending = need_default_dock;

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
			snprintf(s_err, sizeof s_err, "manifest: %s", er.c_str());
		}
		loadItemsDat(joinPath(root, "items.dat"), gItems, er);
		loadMonstersDat(joinPath(root, "monsters.dat"), gMons, er);
		gMaze = MazeFile();
		loadMaze(joinPath(root, "level01.maze"), gMaze, er);
		loadLvl(joinPath(root, "level00.lvl"), gLvl, er);
		gWst = WallsetFile();
		gPlnData.clear();
		std::string wpath = joinPath(root, "factory2/factory2.wll");
		if (loadWallsetMain(wpath, gWst, er)) {
			std::string base = wpath;
			size_t dot = base.find_last_of('.');
			if (dot != std::string::npos)
				base.resize(dot);
			std::string pln = base + "_0.pln";
			readRawFile(pln, gPlnData, er);
			if (!gPlnData.empty() && !gWst.palette.empty()) {
				std::vector<unsigned char> rgba;
				int rw, rh;
				int palN = (int)gWst.palette.size() / 3;
				if (decodeAcePlanar(gPlnData, gWst.palette.data(), palN, rgba, rw, rh, er))
					uploadRgba(rgba, rw, rh);
			}
		}
		dirty = false;
	};

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
				saveItemsDat(joinPath(s_root, "items.dat"), gItems, er);
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
				dirty = true;
			}
			ImGui::SameLine();
			if (ImGui::Button("Save monsters.dat")) {
				std::string er;
				saveMonstersDat(joinPath(s_root, "monsters.dat"), gMons, er);
			}
			for (size_t i = 0; i < gMons.size(); i++) {
				ImGui::PushID((int)i);
				MonsterRow &r = gMons[i];
				ImGui::InputInt("level", (int *)&r.level);
				ImGui::InputInt("maxHp", (int *)&r.maxHp);
				ImGui::InputInt("atk", (int *)&r.attack);
				ImGui::InputInt("def", (int *)&r.defense);
				ImGui::InputInt("xp", (int *)&r.experience);
				ImGui::InputText("name", &r.name);
				ImGui::PopID();
			}
		}
		ImGui::End();

		if (ImGui::Begin("Maze (level01.maze)")) {
			static char mazePath[256] = "level01.maze";
			ImGui::InputText("Relative path", mazePath, sizeof mazePath);
			if (ImGui::Button("Load")) {
				std::string er;
				loadMaze(joinPath(s_root, mazePath), gMaze, er);
			}
			ImGui::SameLine();
			if (ImGui::Button("Save")) {
				std::string er;
				saveMaze(joinPath(s_root, mazePath), gMaze, er);
			}
			ImGui::InputInt("W", &gMaze.width);
			ImGui::InputInt("H", &gMaze.height);
			int cells = gMaze.width * gMaze.height;
			if (cells > 0) {
				if ((int)gMaze.cells.size() != cells) {
					gMaze.cells.assign(cells, 1);
					gMaze.cols.assign(cells, 0);
					gMaze.floors.assign(cells, 0);
				}
				ImGui::Text("Cell grid (click to cycle 0=floor 1=wall 2=door):");
				for (int y = 0; y < gMaze.height && y < 64; y++) {
					for (int x = 0; x < gMaze.width && x < 64; x++) {
						ImGui::PushID(y * 256 + x);
						int idx = y * gMaze.width + x;
						unsigned char v = gMaze.cells[idx];
						char lbl[8];
						snprintf(lbl, sizeof lbl, "%u", (unsigned)v);
						if (ImGui::SmallButton(lbl)) {
							gMaze.cells[idx] = (unsigned char)((v + 1) % 6);
							dirty = true;
						}
						if (x + 1 < gMaze.width)
							ImGui::SameLine();
						ImGui::PopID();
					}
				}
			}
			ImGui::Text("Events: %d", (int)gMaze.events.size());
			if (ImGui::Button("Add event")) {
				gMaze.events.push_back(MazeEvent{});
				dirty = true;
			}
			for (size_t i = 0; i < gMaze.events.size(); i++) {
				ImGui::PushID((int)i);
				MazeEvent &e = gMaze.events[i];
				ImGui::InputInt("x", &e.x);
				ImGui::InputInt("y", &e.y);
				ImGui::InputInt("type", &e.type);
				static char hex[256];
				snprintf(hex, sizeof hex, "%zu bytes", e.data.size());
				ImGui::TextUnformatted(hex);
				ImGui::PopID();
			}
		}
		ImGui::End();

		if (ImGui::Begin("Level entities (.lvl)")) {
			static char lvlPath[256] = "level00.lvl";
			ImGui::InputText("Path", lvlPath, sizeof lvlPath);
			if (ImGui::Button("Load .lvl")) {
				std::string er;
				loadLvl(joinPath(s_root, lvlPath), gLvl, er);
			}
			ImGui::SameLine();
			if (ImGui::Button("Save .lvl")) {
				std::string er;
				saveLvl(joinPath(s_root, lvlPath), gLvl, er);
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
			ImGui::Text("Tiles: %d  Planes: %d", gWst.totalTiles, gWst.gfxCount);
			if (g_tex && g_texW > 0)
				ImGui::Image((ImTextureID)(intptr_t)g_tex, ImVec2((float)g_texW, (float)g_texH));
			else
				ImGui::TextUnformatted("Load project + factory2 wallset for preview.");
		}
		ImGui::End();

		ImGui::Render();
		glfwGetFramebufferSize(window, &fbW, &fbH);
		glViewport(0, 0, fbW, fbH);
		glClearColor(0.08f, 0.08f, 0.10f, 1);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		glfwSwapBuffers(window);
	}

	if (g_tex)
		glDeleteTextures(1, &g_tex);
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
