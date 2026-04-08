#include "dungeon_gl.hpp"
#include "gl_extra.hpp"

#define GL_SILENCE_DEPRECATION
#include <GLFW/glfw3.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <sstream>
#include <string>
#include <vector>

static std::string g_dungeonGlDebugLog;

void dungeonGlClearDebugLog()
{
	g_dungeonGlDebugLog.clear();
}

const std::string &dungeonGlGetDebugLog()
{
	return g_dungeonGlDebugLog;
}

static void dungeonGlDbg(const char *fmt, ...)
{
	char buf[1024];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf, sizeof buf, fmt, ap);
	va_end(ap);
	g_dungeonGlDebugLog += buf;
	g_dungeonGlDebugLog += '\n';
}

static void dungeonGlDbgGlErrors(const char *where)
{
	for (;;) {
		GLenum e = glGetError();
		if (e == GL_NO_ERROR)
			break;
		dungeonGlDbg("GL error after %s: 0x%x", where, (unsigned)e);
	}
}

static void dungeonGlDbgPassAlpha(const char *label, const std::vector<unsigned char> &px, int w, int h)
{
	if ((int)px.size() < w * h * 4) {
		dungeonGlDbg("  %s: pixel buffer size mismatch (got %zu need %d)", label, px.size(), w * h * 4);
		return;
	}
	long long nz = 0;
	int minA = 256, maxA = 0;
	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			unsigned char a = px[((size_t)y * w + x) * 4 + 3];
			if (a) {
				nz++;
				if (a < minA)
					minA = a;
				if (a > maxA)
					maxA = a;
			}
		}
	}
	if (!nz)
		minA = maxA = 0;
	int cx = w / 2, cy = h / 2;
	size_t o = ((size_t)cy * w + cx) * 4;
	dungeonGlDbg("  %s: alpha>0: %lld / %d px  minA=%d maxA=%d  center(%d,%d) RGBA=%u,%u,%u,%u", label, nz, w * h, minA, maxA, cx, cy,
	    (unsigned)px[o], (unsigned)px[o + 1], (unsigned)px[o + 2], (unsigned)px[o + 3]);
}

namespace {

constexpr int kMagR = 255, kMagG = 0, kMagB = 255;

GLuint g_fbo = 0, g_color = 0, g_depth = 0;
GLuint g_whiteTex = 0;
GLuint g_vao = 0, g_vbo = 0, g_ibo = 0;
GLuint g_prog = 0;
int g_fbW = 0, g_fbH = 0;

static GLuint compile(GLenum type, const char *src)
{
	GLuint s = glCreateShader(type);
	glShaderSource(s, 1, &src, nullptr);
	glCompileShader(s);
	GLint ok = 0;
	glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
	if (!ok) {
		char buf[1024];
		glGetShaderInfoLog(s, sizeof buf, nullptr, buf);
		glDeleteShader(s);
		return 0;
	}
	return s;
}

static GLuint link(GLuint vs, GLuint fs)
{
	GLuint p = glCreateProgram();
	glAttachShader(p, vs);
	glAttachShader(p, fs);
	smiteGlBindAttribLocation(p, 0, "aPos");
	smiteGlBindAttribLocation(p, 1, "aUv");
	glLinkProgram(p);
	glDeleteShader(vs);
	glDeleteShader(fs);
	GLint ok = 0;
	glGetProgramiv(p, GL_LINK_STATUS, &ok);
	if (!ok) {
		char buf[1024];
		glGetProgramInfoLog(p, sizeof buf, nullptr, buf);
		glDeleteProgram(p);
		return 0;
	}
	return p;
}

#if defined(__APPLE__)
static const char *kVs = R"GL(#version 150 core
uniform mat4 uMVP;
uniform mat4 uMV;
in vec3 aPos;
in vec2 aUv;
out vec2 vUv;
out float vEyeZ;
void main() {
	vec4 mv = uMV * vec4(aPos, 1.0);
	vEyeZ = -mv.z;
	vUv = aUv;
	gl_Position = uMVP * vec4(aPos, 1.0);
}
)GL";

static const char *kFs = R"GL(#version 150 core
uniform sampler2D uTex;
uniform vec3 uFogColor;
uniform float uFogStart;
uniform float uFogEnd;
uniform int uUseFog;
in vec2 vUv;
in float vEyeZ;
out vec4 fragColor;
void main() {
	vec4 c = texture(uTex, vUv);
	if (uUseFog == 1) {
		float fogFactor = smoothstep(uFogStart, uFogEnd, vEyeZ);
		c.rgb = mix(c.rgb, uFogColor, fogFactor);
	}
	fragColor = c;
}
)GL";
#else
static const char *kVs = R"GL(#version 420 core
uniform mat4 uMVP;
uniform mat4 uMV;
in vec3 aPos;
in vec2 aUv;
out vec2 vUv;
out float vEyeZ;
void main() {
	vec4 mv = uMV * vec4(aPos, 1.0);
	vEyeZ = -mv.z;
	vUv = aUv;
	gl_Position = uMVP * vec4(aPos, 1.0);
}
)GL";

static const char *kFs = R"GL(#version 420 core
uniform sampler2D uTex;
uniform vec3 uFogColor;
uniform float uFogStart;
uniform float uFogEnd;
uniform int uUseFog;
in vec2 vUv;
in float vEyeZ;
out vec4 fragColor;
void main() {
	vec4 c = texture(uTex, vUv);
	if (uUseFog == 1) {
		float fogFactor = smoothstep(uFogStart, uFogEnd, vEyeZ);
		c.rgb = mix(c.rgb, uFogColor, fogFactor);
	}
	fragColor = c;
}
)GL";
#endif

static bool ensureResources(int w, int h)
{
	smiteGlLoadExtra();
	if (!g_prog) {
		GLuint vs = compile(GL_VERTEX_SHADER, kVs);
		GLuint fs = compile(GL_FRAGMENT_SHADER, kFs);
		if (!vs || !fs)
			return false;
		g_prog = link(vs, fs);
		if (!g_prog)
			return false;
	}

	if (!g_whiteTex) {
		unsigned char px[4] = {255, 255, 255, 255};
		glGenTextures(1, &g_whiteTex);
		glBindTexture(GL_TEXTURE_2D, g_whiteTex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, px);
	}

	if (!g_vao) {
		glGenVertexArrays(1, &g_vao);
		glGenBuffers(1, &g_vbo);
		glGenBuffers(1, &g_ibo);
		unsigned short idx[] = {0, 1, 2, 0, 2, 3};
		glBindVertexArray(g_vao);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_ibo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof idx, idx, GL_STATIC_DRAW);
		glBindVertexArray(0);
	}

	if (w != g_fbW || h != g_fbH) {
		if (g_color) {
			glDeleteTextures(1, &g_color);
			g_color = 0;
		}
		if (g_depth) {
			smiteGlDeleteRenderbuffers(1, &g_depth);
			g_depth = 0;
		}
		if (g_fbo) {
			smiteGlDeleteFramebuffers(1, &g_fbo);
			g_fbo = 0;
		}
		g_fbW = w;
		g_fbH = h;
		if (w < 1 || h < 1)
			return false;

		smiteGlGenFramebuffers(1, &g_fbo);
		smiteGlBindFramebuffer(GL_FRAMEBUFFER, g_fbo);
		glGenTextures(1, &g_color);
		glBindTexture(GL_TEXTURE_2D, g_color);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		smiteGlFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_color, 0);
		smiteGlGenRenderbuffers(1, &g_depth);
		smiteGlBindRenderbuffer(GL_RENDERBUFFER, g_depth);
		smiteGlRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, w, h);
		smiteGlFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, g_depth);
		GLenum st = smiteGlCheckFramebufferStatus(GL_FRAMEBUFFER);
		smiteGlBindFramebuffer(GL_FRAMEBUFFER, 0);
		if (st != GL_FRAMEBUFFER_COMPLETE) {
			dungeonGlDbg("ensureResources: FBO incomplete status=0x%x size=%dx%d", (unsigned)st, w, h);
			return false;
		}
	}
	return true;
}

static void flipY(std::vector<unsigned char> &px, int w, int h)
{
	std::vector<unsigned char> row((size_t)w * 4);
	for (int y = 0; y < h / 2; y++) {
		int y2 = h - 1 - y;
		memcpy(row.data(), px.data() + (size_t)y * w * 4, (size_t)w * 4);
		memcpy(px.data() + (size_t)y * w * 4, px.data() + (size_t)y2 * w * 4, (size_t)w * 4);
		memcpy(px.data() + (size_t)y2 * w * 4, row.data(), (size_t)w * 4);
	}
}

static void grabRegion(const std::vector<unsigned char> &src, int sw, int sh, int x, int y, int rw, int rh, std::vector<unsigned char> &out)
{
	out.assign((size_t)rw * rh * 4, 0);
	for (int j = 0; j < rh; j++) {
		for (int i = 0; i < rw; i++) {
			int sx = x + i, sy = y + j;
			if (sx < 0 || sy < 0 || sx >= sw || sy >= sh)
				continue;
			size_t si = ((size_t)sy * sw + sx) * 4;
			size_t di = ((size_t)j * rw + i) * 4;
			memcpy(&out[di], &src[si], 4);
		}
	}
}

static bool boundsByAlpha(const std::vector<unsigned char> &d, int vw, int vh, int &x1, int &y1, int &x2, int &y2)
{
	x1 = y1 = x2 = y2 = -1;
	for (int x = 0; x < vw; x++) {
		for (int y = 0; y < vh; y++) {
			unsigned char a = d[((size_t)y * vw + x) * 4 + 3];
			if (a != 0 && x1 < 0)
				x1 = x;
		}
	}
	for (int x = vw - 1; x >= 0; x--) {
		for (int y = 0; y < vh; y++) {
			unsigned char a = d[((size_t)y * vw + x) * 4 + 3];
			if (a != 0 && x2 < 0)
				x2 = x + 1;
		}
	}
	for (int y = 0; y < vh; y++) {
		for (int x = 0; x < vw; x++) {
			unsigned char a = d[((size_t)y * vw + x) * 4 + 3];
			if (a != 0 && y1 < 0)
				y1 = y;
		}
	}
	for (int y = vh - 1; y >= 0; y--) {
		for (int x = 0; x < vw; x++) {
			unsigned char a = d[((size_t)y * vw + x) * 4 + 3];
			if (a != 0 && y2 < 0)
				y2 = y + 1;
		}
	}
	if (x1 < 0)
		x1 = 0;
	if (y1 < 0)
		y1 = 0;
	if (x2 < 0 || y2 < 0)
		return false;
	return true;
}

static bool cropMagenta(const std::vector<unsigned char> &d, int w, int h, int &x1, int &y1, int &x2, int &y2)
{
	x1 = y1 = x2 = y2 = -1;
	for (int x = 0; x < w; x++) {
		for (int y = 0; y < h; y++) {
			size_t i = ((size_t)y * w + x) * 4;
			if (!(d[i] == kMagR && d[i + 1] == kMagG && d[i + 2] == kMagB)) {
				if (x1 < 0)
					x1 = x;
			}
		}
	}
	for (int x = w - 1; x >= 0; x--) {
		for (int y = 0; y < h; y++) {
			size_t i = ((size_t)y * w + x) * 4;
			if (!(d[i] == kMagR && d[i + 1] == kMagG && d[i + 2] == kMagB)) {
				if (x2 < 0)
					x2 = x + 1;
			}
		}
	}
	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			size_t i = ((size_t)y * w + x) * 4;
			if (!(d[i] == kMagR && d[i + 1] == kMagG && d[i + 2] == kMagB)) {
				if (y1 < 0)
					y1 = y;
			}
		}
	}
	for (int y = h - 1; y >= 0; y--) {
		for (int x = 0; x < w; x++) {
			size_t i = ((size_t)y * w + x) * 4;
			if (!(d[i] == kMagR && d[i + 1] == kMagG && d[i + 2] == kMagB)) {
				if (y2 < 0)
					y2 = y + 1;
			}
		}
	}
	if (x1 < 0)
		x1 = 0;
	if (y1 < 0)
		y1 = 0;
	if (x2 < 0)
		x2 = w;
	if (y2 < 0)
		y2 = h;
	return x2 > x1 && y2 > y1;
}

struct Square {
	int x, z;
};

static void buildSquareList(int dungeonW, int dungeonD, std::vector<Square> &out)
{
	out.clear();
	for (int b = 0; b < dungeonW; b++) {
		for (int a = 0; a <= dungeonD; a++)
			out.push_back({0 - b, 0 - a});
	}
}

static const DungeonGlTexture *findTex(const std::vector<DungeonGlTexture> &lib, const std::string &name)
{
	for (const auto &t : lib) {
		if (t.name == name)
			return &t;
	}
	return nullptr;
}

static glm::mat4 matWallFront(float co, float csz)
{
	/* JS: matrix.set(1,0,0,0, 0,1+co,0,0, 0,csz,1,0, 0,0,0,1)  (row-major)
	   GLM column-major: col0=(1,0,0,0) col1=(0,1+co,csz,0) col2=(0,0,1,0) col3=(0,0,0,1) */
	glm::mat4 G(1.f);
	G[0] = glm::vec4(1, 0, 0, 0);
	G[1] = glm::vec4(0, 1.f + co, csz, 0);
	G[2] = glm::vec4(0, 0, 1, 0);
	G[3] = glm::vec4(0, 0, 0, 1);
	glm::mat4 T = glm::translate(glm::mat4(1.f), glm::vec3(0.5f, 0.5f + co * 0.5f, csz * 0.5f));
	return T * G;
}

static glm::mat4 matWallSide(float co, float csz)
{
	/* JS: matrix.set(1,-csz,0,0, 0,1+co,0,0, 0,0,1,0, 0,0,0,1)  (row-major)
	   GLM column-major: col0=(1,0,0,0) col1=(-csz,1+co,0,0) col2=(0,0,1,0) col3=(0,0,0,1) */
	glm::mat4 G(1.f);
	G[0] = glm::vec4(1, 0, 0, 0);
	G[1] = glm::vec4(-csz, 1.f + co, 0, 0);
	G[2] = glm::vec4(0, 0, 1, 0);
	G[3] = glm::vec4(0, 0, 0, 1);
	glm::mat4 R = glm::rotate(glm::mat4(1.f), glm::radians(90.f), glm::vec3(0, 1, 0));
	glm::mat4 T = glm::translate(glm::mat4(1.f), glm::vec3(0.f, 0.5f + co * 0.5f, -0.5f + csz * 0.5f));
	return T * R * G;
}

static glm::mat4 matFloor()
{
	glm::mat4 R = glm::rotate(glm::mat4(1.f), glm::radians(90.f), glm::vec3(1, 0, 0));
	glm::mat4 T = glm::translate(glm::mat4(1.f), glm::vec3(0.5f, 0.f, 0.5f));
	return T * R;
}

static glm::mat4 matCeiling(float co, float csz, float /*tileW*/)
{
	/* JS: no matrix, just position(0.5, 1+co, 0.5+csz) rotation(90°,0,0) */
	glm::mat4 R = glm::rotate(glm::mat4(1.f), glm::radians(90.f), glm::vec3(1, 0, 0));
	glm::mat4 T = glm::translate(glm::mat4(1.f), glm::vec3(0.5f, 1.f + co, 0.5f + csz));
	return T * R;
}

static glm::mat4 matDecalFront(float co, float csz, float sx, float sy, float oy)
{
	/* JS: same matrix as WALL_FRONT but PlaneGeometry(sx, sy) + offset.y */
	glm::mat4 G(1.f);
	G[0] = glm::vec4(1, 0, 0, 0);
	G[1] = glm::vec4(0, 1.f + co, csz, 0);
	G[2] = glm::vec4(0, 0, 1, 0);
	G[3] = glm::vec4(0, 0, 0, 1);
	glm::mat4 S = glm::scale(glm::mat4(1.f), glm::vec3(sx, sy, 1.f));
	glm::mat4 T = glm::translate(glm::mat4(1.f), glm::vec3(0.5f, (0.5f + (co * 0.5f)) + oy, csz * 0.5f));
	return T * S * G;
}

static glm::mat4 matDecalSide(float co, float csz, float sx, float sy, float oy)
{
	/* JS: same matrix as WALL_SIDE but PlaneGeometry(sx, sy) + offset.y
	   JS row-major: (1,-csz,0,0, 0,1+co,0,0, 0,0,1,0, 0,0,0,1)
	   GLM col-major: col0=(1,0,0,0) col1=(-csz,1+co,0,0) col2=(0,0,1,0) col3=(0,0,0,1) */
	glm::mat4 G(1.f);
	G[0] = glm::vec4(1, 0, 0, 0);
	G[1] = glm::vec4(-csz, 1.f + co, 0, 0);
	G[2] = glm::vec4(0, 0, 1, 0);
	G[3] = glm::vec4(0, 0, 0, 1);
	glm::mat4 S = glm::scale(glm::mat4(1.f), glm::vec3(sx, sy, 1.f));
	glm::mat4 R = glm::rotate(glm::mat4(1.f), glm::radians(90.f), glm::vec3(0, 1, 0));
	glm::mat4 T = glm::translate(glm::mat4(1.f), glm::vec3(0.f, (0.5f + co * 0.5f) + oy, -0.5f + csz * 0.5f));
	return T * R * S * G;
}

static glm::mat4 matObj1(float co, float csz, int squareZ)
{
	/* JS: same matrix as WALL_FRONT, but with zz offset in Z */
	float zz = (squareZ == 0) ? -0.6f : -0.15f;
	glm::mat4 G(1.f);
	G[0] = glm::vec4(1, 0, 0, 0);
	G[1] = glm::vec4(0, 1.f + co, csz, 0);
	G[2] = glm::vec4(0, 0, 1, 0);
	G[3] = glm::vec4(0, 0, 0, 1);
	glm::mat4 T = glm::translate(glm::mat4(1.f), glm::vec3(0.5f, 0.5f + co * 0.5f, zz + csz * 0.5f));
	return T * G;
}

static glm::mat4 matObj2(float co, float csz, float sx, float sy, float oy, int squareZ)
{
	float zz = (squareZ == 0) ? -0.6f : -0.15f;
	glm::mat4 G(1.f);
	G[0] = glm::vec4(1, 0, 0, 0);
	G[1] = glm::vec4(0, 1.f + co, csz, 0);
	G[2] = glm::vec4(0, 0, 1, 0);
	G[3] = glm::vec4(0, 0, 0, 1);
	glm::mat4 S = glm::scale(glm::mat4(1.f), glm::vec3(sx, sy, 1.f));
	glm::mat4 T = glm::translate(glm::mat4(1.f), glm::vec3(0.5f, (0.5f + (co * 0.5f)) + oy, zz + (csz * 0.5f)));
	return T * S * G;
}

static void uploadQuad(const glm::mat4 &M)
{
	const float verts[] = {
		-0.5f, -0.5f, 0.f, 0.f, 1.f,
		0.5f, -0.5f, 0.f, 1.f, 1.f,
		0.5f, 0.5f, 0.f, 1.f, 0.f,
		-0.5f, 0.5f, 0.f, 0.f, 0.f,
	};
	float out[4 * 5];
	for (int i = 0; i < 4; i++) {
		glm::vec4 v(verts[i * 5 + 0], verts[i * 5 + 1], verts[i * 5 + 2], 1.f);
		glm::vec4 w = M * v;
		out[i * 5 + 0] = w.x;
		out[i * 5 + 1] = w.y;
		out[i * 5 + 2] = w.z;
		out[i * 5 + 3] = verts[i * 5 + 3];
		out[i * 5 + 4] = verts[i * 5 + 4];
	}
	glBindBuffer(GL_ARRAY_BUFFER, g_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof out, out, GL_STREAM_DRAW);
	glBindVertexArray(g_vao);
	glBindBuffer(GL_ARRAY_BUFFER, g_vbo);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)(3 * sizeof(float)));
}

static void drawQuad()
{
	glBindVertexArray(g_vao);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);
}

static glm::mat4 cameraView(const DungeonGlSettings &st)
{
	glm::vec3 eye(st.cameraOffsetX, st.cameraOffsetY + 0.5f, st.cameraOffsetZ);
	float t = st.cameraTiltRad;
	glm::vec3 dir(0.f, std::sin(t), -std::cos(t));
	glm::vec3 target = eye + dir;
	return glm::lookAt(eye, target, glm::vec3(0.f, 1.f, 0.f));
}

static glm::mat4 cameraProj(const DungeonGlSettings &st)
{
	float aspect = (float)st.viewportW / (float)st.viewportH;
	return glm::perspective(glm::radians(st.cameraFOV), aspect, 0.1f, 1000.f);
}

static void renderPass(const DungeonGlSettings &st, const glm::mat4 &model, GLuint texId, bool whitePass, bool useFog)
{
	glm::mat4 V = cameraView(st);
	glm::mat4 P = cameraProj(st);
	glm::mat4 MV = V;
	glm::mat4 MVP = P * V;

	glUseProgram(g_prog);
	dungeonGlDbgGlErrors("glUseProgram");
	GLint locMvp = glGetUniformLocation(g_prog, "uMVP");
	GLint locMv = glGetUniformLocation(g_prog, "uMV");
	glUniformMatrix4fv(locMvp, 1, GL_FALSE, glm::value_ptr(MVP));
	glUniformMatrix4fv(locMv, 1, GL_FALSE, glm::value_ptr(MV));

	GLuint bind = whitePass ? g_whiteTex : (texId ? texId : g_whiteTex);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, bind);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, st.textureFiltering ? GL_LINEAR : GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, st.textureFiltering ? GL_LINEAR : GL_NEAREST);
	glUniform1i(glGetUniformLocation(g_prog, "uTex"), 0);

	glUniform1i(glGetUniformLocation(g_prog, "uUseFog"), (whitePass || !useFog) ? 0 : 1);
	smiteGlUniform3f(glGetUniformLocation(g_prog, "uFogColor"), st.fogColor[0], st.fogColor[1], st.fogColor[2]);
	smiteGlUniform1f(glGetUniformLocation(g_prog, "uFogStart"), st.fogStart);
	smiteGlUniform1f(glGetUniformLocation(g_prog, "uFogEnd"), st.fogEnd);

	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	dungeonGlDbg("  renderPass: prog=%u locMVP=%d locMV=%d tex=%u white=%d", (unsigned)g_prog, locMvp, locMv, (unsigned)bind, whitePass ? 1 : 0);

	/* Log world-space vertex positions and clip-space positions */
	const float baseVerts[] = {-0.5f,-0.5f,0.f, 0.5f,-0.5f,0.f, 0.5f,0.5f,0.f, -0.5f,0.5f,0.f};
	for (int i = 0; i < 4; i++) {
		glm::vec4 v(baseVerts[i * 3], baseVerts[i * 3 + 1], baseVerts[i * 3 + 2], 1.f);
		glm::vec4 world = model * v;
		glm::vec4 clip = MVP * world;
		float ndcX = clip.w != 0.f ? clip.x / clip.w : 0.f;
		float ndcY = clip.w != 0.f ? clip.y / clip.w : 0.f;
		float ndcZ = clip.w != 0.f ? clip.z / clip.w : 0.f;
		dungeonGlDbg("    v%d world=(%.3f,%.3f,%.3f) clip=(%.3f,%.3f,%.3f,%.3f) ndc=(%.3f,%.3f,%.3f)",
		    i, world.x, world.y, world.z, clip.x, clip.y, clip.z, clip.w, ndcX, ndcY, ndcZ);
	}

	uploadQuad(model);
	drawQuad();
	dungeonGlDbgGlErrors("glDrawElements");
}

static bool readPass(std::vector<unsigned char> &px, int w, int h)
{
	px.resize((size_t)w * h * 4);
	smiteGlLoadExtra();
	/* Keep draw+read on the capture FBO (some stacks desync GL_DRAW vs GL_READ after UI). */
	smiteGlBindFramebuffer(GL_FRAMEBUFFER, g_fbo);
	smiteGlReadBuffer(GL_COLOR_ATTACHMENT0);
	glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
	return true;
}

#ifndef GL_COLOR_WRITEMASK
#define GL_COLOR_WRITEMASK 0x0C23
#endif
#ifndef GL_DEPTH_WRITEMASK
#define GL_DEPTH_WRITEMASK 0x0B72
#endif
#ifndef GL_SAMPLER_BINDING
#define GL_SAMPLER_BINDING 0x8919
#endif
#ifndef GL_DEPTH_FUNC
#define GL_DEPTH_FUNC 0x0B74
#endif

static int queryGlVersion100()
{
	GLint major = 0, minor = 0;
	glGetIntegerv(GL_MAJOR_VERSION, &major);
	glGetIntegerv(GL_MINOR_VERSION, &minor);
	if (major == 0 && minor == 0)
		return 320;
	return major * 100 + minor * 10;
}

/* Save/restore GL state around offscreen capture so ImGui/engine state cannot suppress draws or readback. */
struct CaptureGlStack {
	GLenum last_active_texture = GL_TEXTURE0;
	GLuint last_program = 0;
	GLuint last_texture = 0;
	GLuint last_sampler = 0;
	GLuint last_array_buffer = 0;
	GLuint last_vertex_array_object = 0;
	GLuint last_element_array_buffer = 0;
	GLint last_viewport[4] = {};
	GLint last_scissor_box[4] = {};
	GLenum last_blend_src_rgb = 0, last_blend_dst_rgb = 0, last_blend_src_alpha = 0, last_blend_dst_alpha = 0;
	GLenum last_blend_equation_rgb = 0, last_blend_equation_alpha = 0;
	GLboolean last_enable_blend = GL_FALSE, last_enable_cull_face = GL_FALSE, last_enable_depth_test = GL_FALSE;
	GLboolean last_enable_stencil_test = GL_FALSE, last_enable_scissor_test = GL_FALSE;
	GLboolean last_enable_primitive_restart = GL_FALSE;
	GLint last_depth_func = GL_LESS;
	GLboolean last_color_mask[4] = {GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE};
	GLboolean last_depth_mask = GL_TRUE;
	GLint last_polygon_mode[2] = {GL_FILL, GL_FILL};
	GLint saved_draw_fbo = 0;
	int gl_ver = 320;
	bool has_bind_sampler = false;
	bool has_polygon_mode = true;
	bool has_primitive_restart = false;
	bool has_profile_mask = false;
	GLint gl_profile_mask = 0;

	void push()
	{
		gl_ver = queryGlVersion100();
		has_bind_sampler = gl_ver >= 330;
		has_primitive_restart = gl_ver >= 310;
		has_polygon_mode = true;
#if defined(GL_CONTEXT_PROFILE_MASK)
		has_profile_mask = true;
		glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &gl_profile_mask);
#else
		has_profile_mask = false;
		gl_profile_mask = 0;
#endif
		glGetIntegerv(GL_FRAMEBUFFER_BINDING, &saved_draw_fbo);
		glGetIntegerv(GL_VIEWPORT, last_viewport);
		last_enable_scissor_test = glIsEnabled(GL_SCISSOR_TEST);
		glGetIntegerv(GL_SCISSOR_BOX, last_scissor_box);

		glGetIntegerv(GL_ACTIVE_TEXTURE, (GLint *)&last_active_texture);
		glActiveTexture(GL_TEXTURE0);
		glGetIntegerv(GL_CURRENT_PROGRAM, (GLint *)&last_program);
		glGetIntegerv(GL_TEXTURE_BINDING_2D, (GLint *)&last_texture);
		if (has_bind_sampler)
			glGetIntegerv(GL_SAMPLER_BINDING, (GLint *)&last_sampler);
		glGetIntegerv(GL_VERTEX_ARRAY_BINDING, (GLint *)&last_vertex_array_object);
		glGetIntegerv(GL_ARRAY_BUFFER_BINDING, (GLint *)&last_array_buffer);
		glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, (GLint *)&last_element_array_buffer);

		glGetIntegerv(GL_BLEND_SRC_RGB, (GLint *)&last_blend_src_rgb);
		glGetIntegerv(GL_BLEND_DST_RGB, (GLint *)&last_blend_dst_rgb);
		glGetIntegerv(GL_BLEND_SRC_ALPHA, (GLint *)&last_blend_src_alpha);
		glGetIntegerv(GL_BLEND_DST_ALPHA, (GLint *)&last_blend_dst_alpha);
		glGetIntegerv(GL_BLEND_EQUATION_RGB, (GLint *)&last_blend_equation_rgb);
		glGetIntegerv(GL_BLEND_EQUATION_ALPHA, (GLint *)&last_blend_equation_alpha);
		last_enable_blend = glIsEnabled(GL_BLEND);
		last_enable_cull_face = glIsEnabled(GL_CULL_FACE);
		last_enable_depth_test = glIsEnabled(GL_DEPTH_TEST);
		last_enable_stencil_test = glIsEnabled(GL_STENCIL_TEST);
		glGetIntegerv(GL_DEPTH_FUNC, &last_depth_func);
		smiteGlGetBooleanv(GL_COLOR_WRITEMASK, last_color_mask);
		smiteGlGetBooleanv(GL_DEPTH_WRITEMASK, &last_depth_mask);
		if (has_polygon_mode)
			glGetIntegerv(GL_POLYGON_MODE, last_polygon_mode);
		if (has_primitive_restart)
			last_enable_primitive_restart = glIsEnabled(GL_PRIMITIVE_RESTART);
	}

	void applyCleanDrawState()
	{
		if (has_bind_sampler)
			glBindSampler(0, 0);
		smiteGlColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		smiteGlDepthMask(GL_TRUE);
		glDisable(GL_BLEND);
		glDisable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_STENCIL_TEST);
		glDisable(GL_SCISSOR_TEST);
		if (has_primitive_restart)
			glDisable(GL_PRIMITIVE_RESTART);
		if (has_polygon_mode)
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	void restore()
	{
		smiteGlBindFramebuffer(GL_FRAMEBUFFER, (GLuint)saved_draw_fbo);
		if (last_program == 0 || glIsProgram(last_program))
			glUseProgram(last_program);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, last_texture);
		if (has_bind_sampler)
			glBindSampler(0, last_sampler);
		glActiveTexture(last_active_texture);
		glBindVertexArray(last_vertex_array_object);
		glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, last_element_array_buffer);
		glBlendEquationSeparate(last_blend_equation_rgb, last_blend_equation_alpha);
		glBlendFuncSeparate(last_blend_src_rgb, last_blend_dst_rgb, last_blend_src_alpha, last_blend_dst_alpha);
		if (last_enable_blend)
			glEnable(GL_BLEND);
		else
			glDisable(GL_BLEND);
		if (last_enable_cull_face)
			glEnable(GL_CULL_FACE);
		else
			glDisable(GL_CULL_FACE);
		if (last_enable_depth_test)
			glEnable(GL_DEPTH_TEST);
		else
			glDisable(GL_DEPTH_TEST);
		smiteGlDepthFunc((GLenum)last_depth_func);
		if (last_enable_stencil_test)
			glEnable(GL_STENCIL_TEST);
		else
			glDisable(GL_STENCIL_TEST);
		if (last_enable_scissor_test)
			glEnable(GL_SCISSOR_TEST);
		else
			glDisable(GL_SCISSOR_TEST);
		if (has_primitive_restart) {
			if (last_enable_primitive_restart)
				glEnable(GL_PRIMITIVE_RESTART);
			else
				glDisable(GL_PRIMITIVE_RESTART);
		}
		if (has_polygon_mode) {
			bool compat = has_profile_mask && (gl_profile_mask & GL_CONTEXT_COMPATIBILITY_PROFILE_BIT) != 0;
			if (gl_ver <= 310 || compat) {
				glPolygonMode(GL_FRONT, (GLenum)last_polygon_mode[0]);
				glPolygonMode(GL_BACK, (GLenum)last_polygon_mode[1]);
			} else {
				glPolygonMode(GL_FRONT_AND_BACK, (GLenum)last_polygon_mode[0]);
			}
		}
		smiteGlColorMask(last_color_mask[0], last_color_mask[1], last_color_mask[2], last_color_mask[3]);
		smiteGlDepthMask(last_depth_mask);
		glViewport(last_viewport[0], last_viewport[1], last_viewport[2], last_viewport[3]);
		glScissor(last_scissor_box[0], last_scissor_box[1], last_scissor_box[2], last_scissor_box[3]);
	}
};

static bool renderFirstSecond(const DungeonGlSettings &st, const glm::mat4 &model, const DungeonGlTexture *tex,
	int &outFullW, std::vector<unsigned char> &outRgba, int &outW, int &outH, int &outScreenX, int &outScreenY,
	const char *dbgCtx, std::string &err)
{
	outFullW = 0;
	outW = outH = outScreenX = outScreenY = 0;
	int vw = st.viewportW, vh = st.viewportH;
	if (!ensureResources(vw, vh)) {
		dungeonGlDbg("renderFirstSecond: FBO init failed ctx=%s", dbgCtx ? dbgCtx : "?");
		err = "FBO init failed";
		return false;
	}

	dungeonGlDbg("[tile] %s", dbgCtx ? dbgCtx : "?");

	CaptureGlStack cap;
	cap.push();

	auto restoreGlState = [&]() { cap.restore(); };

	smiteGlBindFramebuffer(GL_FRAMEBUFFER, g_fbo);
	glViewport(0, 0, vw, vh);
	cap.applyCleanDrawState();

	GLenum fbStatus = smiteGlCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (fbStatus != GL_FRAMEBUFFER_COMPLETE)
		dungeonGlDbg("  WARNING: FBO %u incomplete status=0x%x", (unsigned)g_fbo, (unsigned)fbStatus);

	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	dungeonGlDbgGlErrors("glClear pass1");
	renderPass(st, model, tex ? tex->texId : 0, true, false);

	std::vector<unsigned char> pass1;
	readPass(pass1, vw, vh);
	dungeonGlDbgGlErrors("glReadPixels pass1");
	flipY(pass1, vw, vh);
	dungeonGlDbgPassAlpha("pass1", pass1, vw, vh);

	int ax1, ay1, ax2, ay2;
	if (!boundsByAlpha(pass1, vw, vh, ax1, ay1, ax2, ay2)) {
		dungeonGlDbg("  -> skipped (off-screen)");
		restoreGlState();
		err = "empty pass1";
		return false;
	}
	dungeonGlDbg("  pass1 bounds: (%d,%d)-(%d,%d) fullW=%d", ax1, ay1, ax2, ay2, ax2 - ax1);
	outFullW = ax2 - ax1;

	glClearColor(1, 0, 1, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	renderPass(st, model, tex ? tex->texId : 0, false, st.useFog);
	std::vector<unsigned char> pass2;
	readPass(pass2, vw, vh);
	flipY(pass2, vw, vh);
	dungeonGlDbgPassAlpha("pass2", pass2, vw, vh);

	restoreGlState();

	int bx1, by1, bx2, by2;
	if (!boundsByAlpha(pass2, vw, vh, bx1, by1, bx2, by2)) {
		dungeonGlDbg("  -> pass2 empty (off-screen)");
		err = "empty pass2";
		return false;
	}
	dungeonGlDbg("  pass2 bounds: (%d,%d)-(%d,%d)", bx1, by1, bx2, by2);
	std::vector<unsigned char> sub;
	grabRegion(pass2, vw, vh, bx1, by1, bx2 - bx1, by2 - by1, sub);
	int cw = bx2 - bx1, ch = by2 - by1;
	int cx1, cy1, cx2, cy2;
	if (!cropMagenta(sub, cw, ch, cx1, cy1, cx2, cy2)) {
		dungeonGlDbg("  -> magenta crop failed");
		err = "magenta crop failed";
		return false;
	}
	std::vector<unsigned char> cropped;
	grabRegion(sub, cw, ch, cx1, cy1, cx2 - cx1, cy2 - cy1, cropped);

	/* Replace magenta key-color pixels with transparent. */
	int cropW = cx2 - cx1, cropH = cy2 - cy1;
	for (int i = 0; i < cropW * cropH; i++) {
		unsigned char *p = cropped.data() + (size_t)i * 4;
		if (p[0] == kMagR && p[1] == kMagG && p[2] == kMagB) {
			p[0] = p[1] = p[2] = p[3] = 0;
		}
	}

	int fw = cropW + st.tilePadding;
	int fh = cropH + st.tilePadding;
	outRgba.assign((size_t)fw * fh * 4, 0);
	for (int j = 0; j < cropH; j++) {
		memcpy(outRgba.data() + (size_t)j * fw * 4, cropped.data() + (size_t)j * cropW * 4, (size_t)cropW * 4);
	}
	outW = fw;
	outH = fh;
	outScreenX = bx1 + cx1;
	outScreenY = by1 + cy1;
	dungeonGlDbg("  -> output %dx%d screen=(%d,%d)", fw, fh, outScreenX, outScreenY);
	return true;
}

static const char *layerTypeStr(DungeonLayerType t)
{
	switch (t) {
	case DungeonLayerType::Walls:
		return "Walls";
	case DungeonLayerType::Floor:
		return "Floor";
	case DungeonLayerType::Ceiling:
		return "Ceiling";
	case DungeonLayerType::Decal:
		return "Decal";
	case DungeonLayerType::Object:
		return "Object";
	default:
		return "Walls";
	}
}

static void flipHorizontal(const std::vector<unsigned char> &src, int w, int h, std::vector<unsigned char> &dst)
{
	dst.resize((size_t)w * h * 4);
	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			size_t si = ((size_t)y * w + x) * 4;
			size_t di = ((size_t)y * w + (w - 1 - x)) * 4;
			memcpy(&dst[di], &src[si], 4);
		}
	}
}

static void packAndRecord(DungeonGlSettings &st, DungeonGlLayer &layer, AtlasPacker &packer, int gutter, const std::string &typeStr,
	int tpx, int tpz, int sx, int sy, const std::vector<unsigned char> &rgba, int rw, int rh, int pass1FullW,
	const std::string &packName, std::string &err)
{
	if (!packer.addImage(rgba, rw, rh, packName, gutter, err))
		return;
	const PackedEntry &pk = packer.packed().back();
	int cw = std::max(1, pk.w - st.tilePadding);
	int ch = std::max(1, pk.h - st.tilePadding);

	auto push = [&](bool flipped, int coordX, int coordY, int coordW, int coordH) {
		DungeonGlLayer::TileOut o;
		o.type = typeStr;
		o.flipped = flipped;
		o.tileX = tpx;
		o.tileZ = tpz;
		o.screenX = sx;
		o.screenY = sy;
		o.coordX = coordX;
		o.coordY = coordY;
		o.coordW = coordW;
		o.coordH = coordH;
		o.fullWidth = pass1FullW;
		if (flipped) {
			o.screenX = st.viewportW - sx;
			o.tileX = std::abs(tpx);
		}
		layer.tilesOut.push_back(std::move(o));
	};

	push(false, pk.x, pk.y, cw, ch);

	bool needFlip = (typeStr == "side") || ((typeStr == "floor" || typeStr == "ceiling") && tpx < 0);
	if (needFlip) {
		std::vector<unsigned char> flipped;
		flipHorizontal(rgba, rw, rh, flipped);
		std::string flipName = packName + "_flip";
		if (!packer.addImage(flipped, rw, rh, flipName, gutter, err))
			return;
		const PackedEntry &fpk = packer.packed().back();
		int fcw = std::max(1, fpk.w - st.tilePadding);
		int fch = std::max(1, fpk.h - st.tilePadding);
		push(true, fpk.x, fpk.y, fcw, fch);
	}
}

static std::string todayStr()
{
	time_t t = time(nullptr);
	struct tm tmv;
#ifdef _WIN32
	localtime_s(&tmv, &t);
#else
	localtime_r(&t, &tmv);
#endif
	char b[32];
	snprintf(b, sizeof b, "%04d-%02d-%02d", tmv.tm_year + 1900, tmv.tm_mon + 1, tmv.tm_mday);
	return b;
}

static void jsonEsc(std::ostringstream &o, const std::string &s)
{
	for (char c : s) {
		if (c == '\\')
			o << "\\\\";
		else if (c == '"')
			o << "\\\"";
		else if (c == '\n')
			o << "\\n";
		else
			o << c;
	}
}

static std::string buildJson(const DungeonGlSettings &st, const std::vector<DungeonGlLayer> &layers)
{
	std::ostringstream j;
	j << "{\"version\":\"0.0.1\",\"generated\":\"" << todayStr() << "\",\"resolution\":{\"width\":" << st.viewportW << ",\"height\":" << st.viewportH
	  << "},\"depth\":" << st.dungeonDepth << ",\"width\":" << st.dungeonWidth << ",\"layers\":[";
	bool first = true;
	for (const auto &L : layers) {
		if (!L.on)
			continue;
		if (!first)
			j << ',';
		first = false;
		j << "{\"on\":true,\"index\":" << L.stableIndex << ",\"name\":\"";
		jsonEsc(j, L.name);
		j << "\",\"type\":\"" << layerTypeStr(L.type) << "\",\"id\":" << L.id << ",\"tiles\":[";
		for (size_t i = 0; i < L.tilesOut.size(); i++) {
			const auto &t = L.tilesOut[i];
			if (i)
				j << ',';
			j << "{\"type\":\"" << t.type << "\",\"flipped\":" << (t.flipped ? "true" : "false") << ",\"tile\":{\"x\":" << t.tileX << ",\"z\":" << t.tileZ
			  << "},\"screen\":{\"x\":" << t.screenX << ",\"y\":" << t.screenY << "},\"coords\":{\"x\":" << t.coordX << ",\"y\":" << t.coordY << ",\"w\":" << t.coordW
			  << ",\"h\":" << t.coordH << ",\"fullWidth\":" << t.fullWidth << "}}";
		}
		j << "]}";
	}
	j << "]}";
	std::string s = j.str();
	std::string out;
	out.reserve(s.size());
	for (size_t i = 0; i < s.size(); i++) {
		char c = s[i];
		if (c == ' ' || c == '\n' || c == '\r' || c == '\t')
			continue;
		out.push_back(c);
	}
	return out;
}

} // namespace

void dungeonGlShutdown()
{
	smiteGlLoadExtra();
	if (g_color)
		glDeleteTextures(1, &g_color);
	if (g_depth)
		smiteGlDeleteRenderbuffers(1, &g_depth);
	if (g_fbo)
		smiteGlDeleteFramebuffers(1, &g_fbo);
	if (g_whiteTex)
		glDeleteTextures(1, &g_whiteTex);
	if (g_vbo)
		glDeleteBuffers(1, &g_vbo);
	if (g_ibo)
		glDeleteBuffers(1, &g_ibo);
	if (g_vao)
		glDeleteVertexArrays(1, &g_vao);
	if (g_prog)
		glDeleteProgram(g_prog);
	g_color = g_depth = g_fbo = g_whiteTex = g_vbo = g_ibo = g_vao = g_prog = 0;
	g_fbW = g_fbH = 0;
}

bool dungeonGlGenerate(const DungeonGlSettings &stIn, std::vector<DungeonGlLayer> &layers, const std::vector<DungeonGlTexture> &texLib,
	AtlasPacker &packer, int gutterPad, std::string &jsonMinified, std::string &err)
{
	DungeonGlSettings st = stIn;
	err.clear();
	jsonMinified.clear();
	dungeonGlClearDebugLog();
	dungeonGlDbg("--- dungeonGlGenerate ---");
	dungeonGlDbg("viewport %dx%d  dungeon grid %dx%d (depth x width)", st.viewportW, st.viewportH, st.dungeonDepth, st.dungeonWidth);
	dungeonGlDbg("camera eye=(%.3f,%.3f,%.3f) tilt=%.4f FOV=%.1f ceilOff=%.3f shiftZ=%.3f",
	    st.cameraOffsetX, st.cameraOffsetY + 0.5f, st.cameraOffsetZ, st.cameraTiltRad, st.cameraFOV, st.ceilingOffsetY, st.cameraShiftZ);
	dungeonGlDbg("fog=%s start=%.2f end=%.2f  filtering=%s  padding=%d",
	    st.useFog ? "ON" : "OFF", st.fogStart, st.fogEnd, st.textureFiltering ? "linear" : "nearest", st.tilePadding);
	dungeonGlDbg("texture refs: %zu", texLib.size());
	for (const auto &t : texLib)
		dungeonGlDbg("  id=%u \"%s\" %dx%d", (unsigned)t.texId, t.name.c_str(), t.w, t.h);
	dungeonGlDbg("GL resources: prog=%u fbo=%u color=%u depth=%u whiteTex=%u vao=%u vbo=%u ibo=%u fbSize=%dx%d",
	    (unsigned)g_prog, (unsigned)g_fbo, (unsigned)g_color, (unsigned)g_depth,
	    (unsigned)g_whiteTex, (unsigned)g_vao, (unsigned)g_vbo, (unsigned)g_ibo, g_fbW, g_fbH);

	if (st.viewportW < 32)
		st.viewportW = 32;
	if (st.viewportH < 32)
		st.viewportH = 32;
	if (st.dungeonDepth < 1)
		st.dungeonDepth = 1;
	if (st.dungeonWidth < 1)
		st.dungeonWidth = 1;

	for (auto &L : layers)
		L.clearTiles();

	packer.reset(16, 16);

	std::vector<Square> squares;
	buildSquareList(st.dungeonWidth, st.dungeonDepth, squares);
	dungeonGlDbg("squares: %zu entries", squares.size());
	for (size_t i = 0; i < squares.size(); i++)
		dungeonGlDbg("  sq[%zu] = (%d,%d)", i, squares[i].x, squares[i].z);
	dungeonGlDbg("layers: %zu", layers.size());
	for (const auto &L : layers)
		dungeonGlDbg("  L[%d] \"%s\" type=%d on=%d tex=\"%s\"", L.stableIndex, L.name.c_str(), (int)L.type, L.on ? 1 : 0, L.textureName.c_str());

	int packSeq = 0;
	auto runOne = [&](DungeonGlLayer &layer, const glm::mat4 &modelLocal, int sqx, int sqz,
		    int tileX, int tileZ, const std::string &typeStr, std::string &e) -> bool {
		const DungeonGlTexture *tex = findTex(texLib, layer.textureName);
		if (!tex && !layer.textureName.empty() && layer.textureName != "-1") {
			e = "missing texture: " + layer.textureName;
			return false;
		}
		glm::mat4 Tg = glm::translate(glm::mat4(1.f), glm::vec3((float)sqx, 0.f, (float)sqz));
		glm::mat4 M = Tg * modelLocal;
		int fullW = 0, scrX = 0, scrY = 0, tw = 0, th = 0;
		std::vector<unsigned char> rgba;
		char dbgBuf[256];
		snprintf(dbgBuf, sizeof dbgBuf, "layer \"%s\" id=%d %s sq=(%d,%d) tile=(%d,%d) tex=\"%s\"",
			layer.name.c_str(), layer.id, typeStr.c_str(), sqx, sqz, tileX, tileZ,
		    layer.textureName.c_str());
		if (!renderFirstSecond(st, M, tex, fullW, rgba, tw, th, scrX, scrY, dbgBuf, e)) {
			if (e == "empty pass1" || e == "empty pass2") {
				e.clear();
				return true;
			}
			return false;
		}
		char nameBuf[128];
		snprintf(nameBuf, sizeof nameBuf, "L%d_%s_%d", layer.stableIndex, typeStr.c_str(), packSeq++);
		packAndRecord(st, layer, packer, gutterPad, typeStr, tileX, tileZ, scrX, scrY, rgba, tw, th, fullW, nameBuf, e);
		return e.empty();
	};

	for (auto &layer : layers) {
		if (!layer.on)
			continue;
		const DungeonGlTexture *tex = findTex(texLib, layer.textureName);
		(void)tex;

		switch (layer.type) {
		case DungeonLayerType::Walls: {
			int d = st.dungeonDepth;
			for (size_t i = 0; i < squares.size(); i++) {
				if ((int)i > d)
					break;
				const Square &sq = squares[i];
				glm::mat4 m = matWallFront(st.ceilingOffsetY, st.cameraShiftZ);
				if (!runOne(layer, m, sq.x, sq.z, sq.x, sq.z, "front", err))
					return false;
			}
			for (const Square &sq : squares) {
				glm::mat4 m = matWallSide(st.ceilingOffsetY, st.cameraShiftZ);
				/* JS: packImage(layer.index, square.x-1, square.z, "side", ...) */
				if (!runOne(layer, m, sq.x, sq.z, sq.x - 1, sq.z, "side", err))
					return false;
			}
		} break;

		case DungeonLayerType::Floor: {
			for (const Square &sq : squares) {
				glm::mat4 m = matFloor();
				if (!runOne(layer, m, sq.x, sq.z - 1, sq.x, sq.z, "floor", err))
					return false;
			}
		} break;

		case DungeonLayerType::Ceiling: {
			for (const Square &sq : squares) {
				glm::mat4 m = matCeiling(st.ceilingOffsetY, st.cameraShiftZ, st.tileWidth);
				if (!runOne(layer, m, sq.x, sq.z - 1, sq.x, sq.z, "ceiling", err))
					return false;
			}
		} break;

		case DungeonLayerType::Decal: {
			const DungeonGlTexture *tex = findTex(texLib, layer.textureName);
			if (!tex && !layer.textureName.empty() && layer.textureName != "-1") {
				err = "missing texture: " + layer.textureName;
				return false;
			}
			int d = st.dungeonDepth;
			for (size_t i = 0; i < squares.size(); i++) {
				if ((int)i > d)
					break;
				const Square &sq = squares[i];
				glm::mat4 Tg = glm::translate(glm::mat4(1.f), glm::vec3((float)sq.x, 0.f, (float)sq.z));
				glm::mat4 Mw = Tg * matWallFront(st.ceilingOffsetY, st.cameraShiftZ);
				int fullW1 = 0, scr0 = 0, sy0 = 0, t0 = 0, th0 = 0;
				std::vector<unsigned char> tmp;
				char d1[192];
				snprintf(d1, sizeof d1, "decal front ref wall L\"%s\" sq(%d,%d)", layer.name.c_str(), sq.x, sq.z);
				if (!renderFirstSecond(st, Mw, tex, fullW1, tmp, t0, th0, scr0, sy0, d1, err)) {
					if (err == "empty pass1" || err == "empty pass2") { err.clear(); continue; }
					return false;
				}
				glm::mat4 Md = Tg * matDecalFront(st.ceilingOffsetY, st.cameraShiftZ, layer.scaleX, layer.scaleY, layer.offsetY);
				int fw2 = 0, scrX = 0, scrY = 0, tw = 0, th = 0;
				std::vector<unsigned char> rgba;
				char d2[192];
				snprintf(d2, sizeof d2, "decal front L\"%s\" sq(%d,%d)", layer.name.c_str(), sq.x, sq.z);
				if (!renderFirstSecond(st, Md, tex, fw2, rgba, tw, th, scrX, scrY, d2, err)) {
					if (err == "empty pass1" || err == "empty pass2") { err.clear(); continue; }
					return false;
				}
				char nameBuf[128];
				snprintf(nameBuf, sizeof nameBuf, "L%d_decal_front_%d", layer.stableIndex, packSeq++);
				packAndRecord(st, layer, packer, gutterPad, "front", sq.x, sq.z, scrX, scrY, rgba, tw, th, fullW1, nameBuf, err);
				if (!err.empty())
					return false;
			}
			for (const Square &sq : squares) {
				glm::mat4 Tg = glm::translate(glm::mat4(1.f), glm::vec3((float)sq.x, 0.f, (float)sq.z));
				glm::mat4 Mw = Tg * matWallSide(st.ceilingOffsetY, st.cameraShiftZ);
				int fullW1 = 0, scr0 = 0, sy0 = 0, t0 = 0, th0 = 0;
				std::vector<unsigned char> tmp;
				char d3[192];
				snprintf(d3, sizeof d3, "decal side ref wall L\"%s\" sq(%d,%d)", layer.name.c_str(), sq.x, sq.z);
				if (!renderFirstSecond(st, Mw, tex, fullW1, tmp, t0, th0, scr0, sy0, d3, err)) {
					if (err == "empty pass1" || err == "empty pass2") { err.clear(); continue; }
					return false;
				}
				glm::mat4 Md = Tg * matDecalSide(st.ceilingOffsetY, st.cameraShiftZ, layer.scaleX, layer.scaleY, layer.offsetY);
				int fw2 = 0, scrX = 0, scrY = 0, tw = 0, th = 0;
				std::vector<unsigned char> rgba;
				char d4[192];
				snprintf(d4, sizeof d4, "decal side L\"%s\" sq(%d,%d)", layer.name.c_str(), sq.x, sq.z);
				if (!renderFirstSecond(st, Md, tex, fw2, rgba, tw, th, scrX, scrY, d4, err)) {
					if (err == "empty pass1" || err == "empty pass2") { err.clear(); continue; }
					return false;
				}
				char nameBuf[128];
				snprintf(nameBuf, sizeof nameBuf, "L%d_decal_side_%d", layer.stableIndex, packSeq++);
				packAndRecord(st, layer, packer, gutterPad, "side", sq.x - 1, sq.z, scrX, scrY, rgba, tw, th, fullW1, nameBuf, err);
				if (!err.empty())
					return false;
			}
		} break;

		case DungeonLayerType::Object: {
			const DungeonGlTexture *tex = findTex(texLib, layer.textureName);
			if (!tex && !layer.textureName.empty() && layer.textureName != "-1") {
				err = "missing texture: " + layer.textureName;
				return false;
			}
			int d = st.dungeonDepth;
			for (size_t i = 0; i < squares.size(); i++) {
				if ((int)i >= d)
					break;
				const Square &sq = squares[i];
				glm::mat4 Tg = glm::translate(glm::mat4(1.f), glm::vec3((float)sq.x, 0.f, (float)sq.z));
				glm::mat4 M1 = Tg * matObj1(st.ceilingOffsetY, st.cameraShiftZ, sq.z);
				int fullW1 = 0, scr0 = 0, sy0 = 0, t0 = 0, th0 = 0;
				std::vector<unsigned char> tmp;
				char o1[192];
				snprintf(o1, sizeof o1, "object ref L\"%s\" sq(%d,%d)", layer.name.c_str(), sq.x, sq.z);
				if (!renderFirstSecond(st, M1, tex, fullW1, tmp, t0, th0, scr0, sy0, o1, err)) {
					if (err == "empty pass1" || err == "empty pass2") { err.clear(); continue; }
					return false;
				}
				glm::mat4 M2 = Tg * matObj2(st.ceilingOffsetY, st.cameraShiftZ, layer.scaleX, layer.scaleY, layer.offsetY, sq.z);
				int fw2 = 0, scrX = 0, scrY = 0, tw = 0, th = 0;
				std::vector<unsigned char> rgba;
				char o2[192];
				snprintf(o2, sizeof o2, "object L\"%s\" sq(%d,%d) z=%d", layer.name.c_str(), sq.x, sq.z, sq.z);
				if (!renderFirstSecond(st, M2, tex, fw2, rgba, tw, th, scrX, scrY, o2, err)) {
					if (err == "empty pass1" || err == "empty pass2") { err.clear(); continue; }
					return false;
				}
				char nameBuf[128];
				snprintf(nameBuf, sizeof nameBuf, "L%d_object_%d", layer.stableIndex, packSeq++);
				packAndRecord(st, layer, packer, gutterPad, "object", sq.x, sq.z, scrX, scrY, rgba, tw, th, fullW1, nameBuf, err);
				if (!err.empty())
					return false;
			}
		} break;

		default:
			break;
		}
	}

	jsonMinified = buildJson(st, layers);
	return true;
}
