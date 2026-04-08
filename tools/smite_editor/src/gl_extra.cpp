#include "gl_extra.hpp"

#include <GLFW/glfw3.h>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

typedef void(APIENTRYP PFNGLGENFRAMEBUFFERSPROC)(GLsizei, GLuint *);
typedef void(APIENTRYP PFNGLDELETEFRAMEBUFFERSPROC)(GLsizei, const GLuint *);
typedef void(APIENTRYP PFNGLBINDFRAMEBUFFERPROC)(GLenum, GLuint);
typedef GLenum(APIENTRYP PFNGLCHECKFRAMEBUFFERSTATUSPROC)(GLenum);
typedef void(APIENTRYP PFNGLFRAMEBUFFERTEXTURE2DPROC)(GLenum, GLenum, GLenum, GLuint, GLint);
typedef void(APIENTRYP PFNGLGENRENDERBUFFERSPROC)(GLsizei, GLuint *);
typedef void(APIENTRYP PFNGLDELETERENDERBUFFERSPROC)(GLsizei, const GLuint *);
typedef void(APIENTRYP PFNGLBINDRENDERBUFFERPROC)(GLenum, GLuint);
typedef void(APIENTRYP PFNGLRENDERBUFFERSTORAGEPROC)(GLenum, GLenum, GLsizei, GLsizei);
typedef void(APIENTRYP PFNGLFRAMEBUFFERRENDERBUFFERPROC)(GLenum, GLenum, GLenum, GLuint);
typedef void(APIENTRYP PFNGLBINDATTRIBLOCATIONPROC)(GLuint, GLuint, const GLchar *);
typedef void(APIENTRYP PFNGLUNIFORM3FPROC)(GLint, GLfloat, GLfloat, GLfloat);
typedef void(APIENTRYP PFNGLUNIFORM1FPROC)(GLint, GLfloat);
typedef void(APIENTRYP PFNGLDEPTHFUNCPROC)(GLenum);
typedef void(APIENTRYP PFNGLREADBUFFERPROC)(GLenum);
typedef void(APIENTRYP PFNGLCOLORMASKPROC)(GLboolean, GLboolean, GLboolean, GLboolean);
typedef void(APIENTRYP PFNGLDEPTHMASKPROC)(GLboolean);
typedef void(APIENTRYP PFNGLGETBOOLEANVPROC)(GLenum, GLboolean *);

static PFNGLGENFRAMEBUFFERSPROC pGenFramebuffers;
static PFNGLDELETEFRAMEBUFFERSPROC pDeleteFramebuffers;
static PFNGLBINDFRAMEBUFFERPROC pBindFramebuffer;
static PFNGLCHECKFRAMEBUFFERSTATUSPROC pCheckFramebufferStatus;
static PFNGLFRAMEBUFFERTEXTURE2DPROC pFramebufferTexture2D;
static PFNGLGENRENDERBUFFERSPROC pGenRenderbuffers;
static PFNGLDELETERENDERBUFFERSPROC pDeleteRenderbuffers;
static PFNGLBINDRENDERBUFFERPROC pBindRenderbuffer;
static PFNGLRENDERBUFFERSTORAGEPROC pRenderbufferStorage;
static PFNGLFRAMEBUFFERRENDERBUFFERPROC pFramebufferRenderbuffer;
static PFNGLBINDATTRIBLOCATIONPROC pBindAttribLocation;
static PFNGLUNIFORM3FPROC pUniform3f;
static PFNGLUNIFORM1FPROC pUniform1f;
static PFNGLDEPTHFUNCPROC pDepthFunc;
static PFNGLREADBUFFERPROC pReadBuffer;
static PFNGLCOLORMASKPROC pColorMask;
static PFNGLDEPTHMASKPROC pDepthMask;
static PFNGLGETBOOLEANVPROC pGetBooleanv;

void smiteGlLoadExtra()
{
	static bool done = false;
	if (done)
		return;
	done = true;
#define LOAD(n, s) n = (decltype(n))glfwGetProcAddress(#s)
	LOAD(pGenFramebuffers, glGenFramebuffers);
	LOAD(pDeleteFramebuffers, glDeleteFramebuffers);
	LOAD(pBindFramebuffer, glBindFramebuffer);
	LOAD(pCheckFramebufferStatus, glCheckFramebufferStatus);
	LOAD(pFramebufferTexture2D, glFramebufferTexture2D);
	LOAD(pGenRenderbuffers, glGenRenderbuffers);
	LOAD(pDeleteRenderbuffers, glDeleteRenderbuffers);
	LOAD(pBindRenderbuffer, glBindRenderbuffer);
	LOAD(pRenderbufferStorage, glRenderbufferStorage);
	LOAD(pFramebufferRenderbuffer, glFramebufferRenderbuffer);
	LOAD(pBindAttribLocation, glBindAttribLocation);
	LOAD(pUniform3f, glUniform3f);
	LOAD(pUniform1f, glUniform1f);
	LOAD(pDepthFunc, glDepthFunc);
	LOAD(pReadBuffer, glReadBuffer);
	LOAD(pColorMask, glColorMask);
	LOAD(pDepthMask, glDepthMask);
	LOAD(pGetBooleanv, glGetBooleanv);
#undef LOAD
#if defined(_WIN32)
	/* 1.0 entry points: glfwGetProcAddress may return null; opengl32 always exports them. */
	if (!pColorMask)
		pColorMask = (PFNGLCOLORMASKPROC)GetProcAddress(GetModuleHandleA("opengl32.dll"), "glColorMask");
	if (!pDepthMask)
		pDepthMask = (PFNGLDEPTHMASKPROC)GetProcAddress(GetModuleHandleA("opengl32.dll"), "glDepthMask");
	if (!pGetBooleanv)
		pGetBooleanv = (PFNGLGETBOOLEANVPROC)GetProcAddress(GetModuleHandleA("opengl32.dll"), "glGetBooleanv");
#endif
}

void smiteGlGenFramebuffers(GLsizei n, GLuint *ids)
{
	pGenFramebuffers(n, ids);
}
void smiteGlDeleteFramebuffers(GLsizei n, const GLuint *ids)
{
	pDeleteFramebuffers(n, ids);
}
void smiteGlBindFramebuffer(GLenum target, GLuint fb)
{
	pBindFramebuffer(target, fb);
}
GLenum smiteGlCheckFramebufferStatus(GLenum target)
{
	return pCheckFramebufferStatus(target);
}
void smiteGlFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint tex, GLint level)
{
	pFramebufferTexture2D(target, attachment, textarget, tex, level);
}
void smiteGlGenRenderbuffers(GLsizei n, GLuint *ids)
{
	pGenRenderbuffers(n, ids);
}
void smiteGlDeleteRenderbuffers(GLsizei n, const GLuint *ids)
{
	pDeleteRenderbuffers(n, ids);
}
void smiteGlBindRenderbuffer(GLenum target, GLuint rb)
{
	pBindRenderbuffer(target, rb);
}
void smiteGlRenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height)
{
	pRenderbufferStorage(target, internalformat, width, height);
}
void smiteGlFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint rb)
{
	pFramebufferRenderbuffer(target, attachment, renderbuffertarget, rb);
}
void smiteGlBindAttribLocation(GLuint program, GLuint index, const GLchar *name)
{
	pBindAttribLocation(program, index, name);
}
void smiteGlUniform3f(GLint loc, GLfloat v0, GLfloat v1, GLfloat v2)
{
	pUniform3f(loc, v0, v1, v2);
}
void smiteGlUniform1f(GLint loc, GLfloat v0)
{
	pUniform1f(loc, v0);
}
void smiteGlDepthFunc(GLenum func)
{
	pDepthFunc(func);
}

void smiteGlReadBuffer(GLenum src)
{
	if (pReadBuffer)
		pReadBuffer(src);
}

void smiteGlColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{
	if (pColorMask)
		pColorMask(red, green, blue, alpha);
}

void smiteGlDepthMask(GLboolean flag)
{
	if (pDepthMask)
		pDepthMask(flag);
}

void smiteGlGetBooleanv(GLenum pname, GLboolean *data)
{
	if (pGetBooleanv) {
		pGetBooleanv(pname, data);
		return;
	}
#ifndef GL_COLOR_WRITEMASK
#define GL_COLOR_WRITEMASK 0x0C23
#endif
#ifndef GL_DEPTH_WRITEMASK
#define GL_DEPTH_WRITEMASK 0x0B72
#endif
	if (pname == GL_COLOR_WRITEMASK && data) {
		data[0] = data[1] = data[2] = data[3] = GL_TRUE;
		return;
	}
	if (pname == GL_DEPTH_WRITEMASK && data)
		*data = GL_TRUE;
}
