/*
** visualinfo.c
**
** Copyright (C) Nate Robins, 1997
**               Michael Wimmer, 1999
**               Milan Ikits, 2002-2008
**               Nigel Stewart, 2008-2013
**
** visualinfo is a small utility that displays all available visuals,
** aka. pixelformats, in an OpenGL system along with renderer version
** information. It shows a table of all the visuals that support OpenGL
** along with their capabilities. The format of the table is similar to
** that of glxinfo on Unix systems:
**
** visual ~= pixel format descriptor
** id       = visual id (integer from 1 - max visuals)
** tp       = type (wn: window, pb: pbuffer, wp: window & pbuffer, bm: bitmap)
** ac	    = acceleration (ge: generic, fu: full, no: none)
** fm	    = format (i: integer, f: float, c: color index)
** db	    = double buffer (y = yes)
** sw       = swap method (x: exchange, c: copy, u: undefined)
** st	    = stereo (y = yes)
** sz       = total # bits
** r        = # bits of red
** g        = # bits of green
** b        = # bits of blue
** a        = # bits of alpha
** axbf     = # aux buffers
** dpth     = # bits of depth
** stcl     = # bits of stencil
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <GL/glew.h>
#if defined(_WIN32)
#include <GL/wglew.h>
#elif defined(__APPLE__) && !defined(GLEW_APPLE_GLX)
#include <OpenGL/OpenGL.h>
#include <OpenGL/CGLTypes.h>
#elif !defined(__HAIKU__)
#include <GL/glxew.h>
#endif

#ifdef GLEW_MX
GLEWContext _glewctx;
#  define glewGetContext() (&_glewctx)
#  ifdef _WIN32
WGLEWContext _wglewctx;
#    define wglewGetContext() (&_wglewctx)
#  elif !defined(__APPLE__) && !defined(__HAIKU__) || defined(GLEW_APPLE_GLX)
GLXEWContext _glxewctx;
#    define glxewGetContext() (&_glxewctx)
#  endif
#endif /* GLEW_MX */

typedef struct GLContextStruct
{
#ifdef _WIN32
  HWND wnd;
  HDC dc;
  HGLRC rc;
#elif defined(__APPLE__) && !defined(GLEW_APPLE_GLX)
  CGLContextObj ctx, octx;
#elif !defined(__HAIKU__)
  Display* dpy;
  XVisualInfo* vi;
  GLXContext ctx;
  Window wnd;
  Colormap cmap;
#endif
} GLContext;

void InitContext (GLContext* ctx);
GLboolean CreateContext (GLContext* ctx);
void DestroyContext (GLContext* ctx);
void VisualInfo (GLContext* ctx);
void PrintExtensions (const char* s);
GLboolean ParseArgs (int argc, char** argv);

int showall = 0;
int displaystdout = 0;
int verbose = 0;
int drawableonly = 0;

char* display = NULL;
int visual = -1;

/* ---------------------------------------------------------------------- */

#if defined(_WIN32)

/* ---------------------------------------------------------------------- */

#elif defined(__APPLE__) && !defined(GLEW_APPLE_GLX)

void
VisualInfo (GLContext* __attribute__((__unused__)) ctx)
{
/*
  int attrib[] = { AGL_RGBA, AGL_NONE };
  AGLPixelFormat pf;
  GLint value;
  pf = aglChoosePixelFormat(NULL, 0, attrib);
  while (pf != NULL)
  {
    aglDescribePixelFormat(pf, GL_RGBA, &value);
    fprintf(stderr, "%d\n", value);
    pf = aglNextPixelFormat(pf);
  }
*/
}

/* ---------------------------------------------------------------------- */

#elif defined(__HAIKU__)

void
VisualInfo (GLContext* ctx)
{
  /* TODO */
}

#else /* GLX */

#endif

/* ------------------------------------------------------------------------ */

#if defined(_WIN32)

void InitContext (GLContext* ctx)
{
  ctx->wnd = NULL;
  ctx->dc = NULL;
  ctx->rc = NULL;
}

GLboolean CreateContext (GLContext* ctx)
{
  WNDCLASS wc;
  PIXELFORMATDESCRIPTOR pfd;
  /* check for input */
  if (NULL == ctx) return GL_TRUE;
  /* register window class */
  ZeroMemory(&wc, sizeof(WNDCLASS));
  wc.hInstance = GetModuleHandle(NULL);
  wc.lpfnWndProc = DefWindowProc;
  wc.lpszClassName = "GLEW";
  if (0 == RegisterClass(&wc)) return GL_TRUE;
  /* create window */
  ctx->wnd = CreateWindow("GLEW", "GLEW", 0, CW_USEDEFAULT, CW_USEDEFAULT, 
                          CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, 
                          GetModuleHandle(NULL), NULL);
  if (NULL == ctx->wnd) return GL_TRUE;
  /* get the device context */
  ctx->dc = GetDC(ctx->wnd);
  if (NULL == ctx->dc) return GL_TRUE;
  /* find pixel format */
  ZeroMemory(&pfd, sizeof(PIXELFORMATDESCRIPTOR));
  if (visual == -1) /* find default */
  {
    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL;
    visual = ChoosePixelFormat(ctx->dc, &pfd);
    if (0 == visual) return GL_TRUE;
  }
  /* set the pixel format for the dc */
  if (FALSE == SetPixelFormat(ctx->dc, visual, &pfd)) return GL_TRUE;
  /* create rendering context */
  ctx->rc = wglCreateContext(ctx->dc);
  if (NULL == ctx->rc) return GL_TRUE;
  if (FALSE == wglMakeCurrent(ctx->dc, ctx->rc)) return GL_TRUE;
  return GL_FALSE;
}

void DestroyContext (GLContext* ctx)
{
  if (NULL == ctx) return;
  if (NULL != ctx->rc) wglMakeCurrent(NULL, NULL);
  if (NULL != ctx->rc) wglDeleteContext(wglGetCurrentContext());
  if (NULL != ctx->wnd && NULL != ctx->dc) ReleaseDC(ctx->wnd, ctx->dc);
  if (NULL != ctx->wnd) DestroyWindow(ctx->wnd);
  UnregisterClass("GLEW", GetModuleHandle(NULL));
}

/* ------------------------------------------------------------------------ */

#elif defined(__APPLE__) && !defined(GLEW_APPLE_GLX)

void InitContext (GLContext* ctx)
{
  ctx->ctx = NULL;
  ctx->octx = NULL;
}

GLboolean CreateContext (GLContext* ctx)
{
  CGLPixelFormatAttribute attrib[] = { kCGLPFAAccelerated, (CGLPixelFormatAttribute)0 };
  CGLPixelFormatObj pf;
  GLint npix;
  CGLError error;
  /* check input */
  if (NULL == ctx) return GL_TRUE;
  error = CGLChoosePixelFormat(attrib, &pf, &npix);
  if (error) return GL_TRUE;
  error = CGLCreateContext(pf, NULL, &ctx->ctx);
  if (error) return GL_TRUE;
  CGLReleasePixelFormat(pf);
  ctx->octx = CGLGetCurrentContext();
  error = CGLSetCurrentContext(ctx->ctx);
  if (error) return GL_TRUE;
  return GL_FALSE;
}

void DestroyContext (GLContext* ctx)
{
  if (NULL == ctx) return;
  CGLSetCurrentContext(ctx->octx);
  if (NULL != ctx->ctx) CGLReleaseContext(ctx->ctx);
}

/* ------------------------------------------------------------------------ */

#elif defined(__HAIKU__)

void
InitContext (GLContext* ctx)
{
  /* TODO */
}

GLboolean
CreateContext (GLContext* ctx)
{
  /* TODO */
  return GL_FALSE;
}

void
DestroyContext (GLContext* ctx)
{
  /* TODO */
}

/* ------------------------------------------------------------------------ */

#else /* __UNIX || (__APPLE__ && GLEW_APPLE_GLX) */

void InitContext (GLContext* ctx)
{
  ctx->dpy = NULL;
  ctx->vi = NULL;
  ctx->ctx = NULL;
  ctx->wnd = 0;
  ctx->cmap = 0;
}

GLboolean CreateContext (GLContext* ctx)
{
  int attrib[] = { GLX_RGBA, GLX_DOUBLEBUFFER, None };
  int erb, evb;
  XSetWindowAttributes swa;
  /* check input */
  if (NULL == ctx) return GL_TRUE;
  /* open display */
  ctx->dpy = XOpenDisplay(display);
  if (NULL == ctx->dpy) return GL_TRUE;
  /* query for glx */
  if (!glXQueryExtension(ctx->dpy, &erb, &evb)) return GL_TRUE;
  /* choose visual */
  ctx->vi = glXChooseVisual(ctx->dpy, DefaultScreen(ctx->dpy), attrib);
  if (NULL == ctx->vi) return GL_TRUE;
  /* create context */
  ctx->ctx = glXCreateContext(ctx->dpy, ctx->vi, None, True);
  if (NULL == ctx->ctx) return GL_TRUE;
  /* create window */
  /*wnd = XCreateSimpleWindow(dpy, RootWindow(dpy, vi->screen), 0, 0, 1, 1, 1, 0, 0);*/
  ctx->cmap = XCreateColormap(ctx->dpy, RootWindow(ctx->dpy, ctx->vi->screen),
                              ctx->vi->visual, AllocNone);
  swa.border_pixel = 0;
  swa.colormap = ctx->cmap;
  ctx->wnd = XCreateWindow(ctx->dpy, RootWindow(ctx->dpy, ctx->vi->screen), 
                           0, 0, 1, 1, 0, ctx->vi->depth, InputOutput, ctx->vi->visual, 
                           CWBorderPixel | CWColormap, &swa);
  /* make context current */
  if (!glXMakeCurrent(ctx->dpy, ctx->wnd, ctx->ctx)) return GL_TRUE;
  return GL_FALSE;
}

void DestroyContext (GLContext* ctx)
{
  if (NULL != ctx->dpy && NULL != ctx->ctx) glXDestroyContext(ctx->dpy, ctx->ctx);
  if (NULL != ctx->dpy && 0 != ctx->wnd) XDestroyWindow(ctx->dpy, ctx->wnd);
  if (NULL != ctx->dpy && 0 != ctx->cmap) XFreeColormap(ctx->dpy, ctx->cmap);
  if (NULL != ctx->vi) XFree(ctx->vi);
  if (NULL != ctx->dpy) XCloseDisplay(ctx->dpy);
}

#endif /* __UNIX || (__APPLE__ && GLEW_APPLE_GLX) */

GLboolean ParseArgs (int argc, char** argv)
{
  int p = 0;
  while (p < argc)
  {
#if defined(_WIN32)
    if (!strcmp(argv[p], "-pf") || !strcmp(argv[p], "-pixelformat"))
    {
      if (++p >= argc) return GL_TRUE;
      display = NULL;
      visual = strtol(argv[p], NULL, 0);
    }
    else if (!strcmp(argv[p], "-a"))
    {
      showall = 1;
    }
    else if (!strcmp(argv[p], "-s"))
    {
      displaystdout = 1;
    }
    else if (!strcmp(argv[p], "-h"))
    {
      return GL_TRUE;
    }
    else
      return GL_TRUE;
#else
    if (!strcmp(argv[p], "-display"))
    {
      if (++p >= argc) return GL_TRUE;
      display = argv[p];
    }
    else if (!strcmp(argv[p], "-visual"))
    {
      if (++p >= argc) return GL_TRUE;
      visual = (int)strtol(argv[p], NULL, 0);
    }
    else if (!strcmp(argv[p], "-h"))
    {
      return GL_TRUE;
    }
    else
      return GL_TRUE;
#endif
    p++;
  }
  return GL_FALSE;
}
