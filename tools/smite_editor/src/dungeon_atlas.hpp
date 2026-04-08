#pragma once

void initDungeonAtlasAfterGl();
void drawDungeonAtlasWindow();
/* Run after ImGui::Render(), before clearing the swapchain — avoids GL state from layout/widgets. */
void dungeonAtlasFlushDeferredGenerate(int framebufferW, int framebufferH);
void shutdownDungeonAtlasWindow();
