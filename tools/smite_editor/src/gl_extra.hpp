#pragma once

#if defined(_WIN32) && !defined(NOMINMAX)
#define NOMINMAX
#endif

#include <imgui_impl_opengl3_loader.h>

// Framebuffer / extra GL 3.0 entry points (imgui loader omits these). Call after GL context exists.

void smiteGlLoadExtra();

#ifndef GL_NO_ERROR
#define GL_NO_ERROR 0
#endif
/* 0x8CA8 is GL_READ_FRAMEBUFFER (bind target), not the glGet pname — wrong value caused GL_INVALID_ENUM. */
#ifndef GL_READ_FRAMEBUFFER
#define GL_READ_FRAMEBUFFER 0x8CA8
#endif
#ifndef GL_DRAW_FRAMEBUFFER
#define GL_DRAW_FRAMEBUFFER 0x8CA9
#endif
#ifndef GL_READ_FRAMEBUFFER_BINDING
#define GL_READ_FRAMEBUFFER_BINDING 0x8CAA
#endif
#ifndef GL_DRAW_FRAMEBUFFER_BINDING
#define GL_DRAW_FRAMEBUFFER_BINDING 0x8CA6
#endif
#ifndef GL_FRAMEBUFFER
#define GL_FRAMEBUFFER 0x8D40
#define GL_FRAMEBUFFER_BINDING 0x8CA6
#define GL_COLOR_ATTACHMENT0 0x8CE0
#define GL_DEPTH_ATTACHMENT 0x8D00
#define GL_DEPTH_COMPONENT16 0x81A5
#define GL_RENDERBUFFER 0x8D41
#define GL_FRAMEBUFFER_COMPLETE 0x8CD5
#define GL_DEPTH_BUFFER_BIT 0x00000100
#define GL_LESS 0x0201
#define GL_STATIC_DRAW 0x88E4
#endif
#ifndef GL_STATIC_DRAW
#define GL_STATIC_DRAW 0x88E4
#endif

void smiteGlGenFramebuffers(GLsizei n, GLuint *ids);
void smiteGlDeleteFramebuffers(GLsizei n, const GLuint *ids);
void smiteGlBindFramebuffer(GLenum target, GLuint fb);
GLenum smiteGlCheckFramebufferStatus(GLenum target);
void smiteGlFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint tex, GLint level);
void smiteGlGenRenderbuffers(GLsizei n, GLuint *ids);
void smiteGlDeleteRenderbuffers(GLsizei n, const GLuint *ids);
void smiteGlBindRenderbuffer(GLenum target, GLuint rb);
void smiteGlRenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
void smiteGlFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint rb);
void smiteGlBindAttribLocation(GLuint program, GLuint index, const GLchar *name);
void smiteGlUniform3f(GLint loc, GLfloat v0, GLfloat v1, GLfloat v2);
void smiteGlUniform1f(GLint loc, GLfloat v0);
void smiteGlDepthFunc(GLenum func);
void smiteGlReadBuffer(GLenum src);
void smiteGlColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
void smiteGlDepthMask(GLboolean flag);
void smiteGlGetBooleanv(GLenum pname, GLboolean *data);
