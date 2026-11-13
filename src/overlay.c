/*
 * ImageOverlay — простая программа: картинка поверх всех окон с настраиваемой
 * непрозрачностью и масштабом, режим "сквозных кликов" для рисования/обводки.
 * Чистый Win32 + GDI+, без CRT и без внешних зависимостей.
 */
#include <stddef.h>
#include <stdint.h>

/* ---------------------------------------------------------------- types */
typedef int BOOL;
typedef unsigned int UINT;
typedef unsigned long DWORD; /* 32-bit on Windows (LLP64) */
typedef unsigned short WORD;
typedef unsigned char BYTE;
typedef long LONG;
typedef long HRESULT;
typedef uint32_t UINT32;
#define TRUE 1
#define FALSE 0
typedef unsigned long ULONG;
typedef intptr_t LONG_PTR;
typedef uintptr_t UINT_PTR, ULONG_PTR, WPARAM;
typedef LONG_PTR LPARAM, LRESULT;
typedef void *HANDLE, *HWND, *HINSTANCE, *HDC, *HBITMAP, *HGDIOBJ, *HICON,
    *HCURSOR, *HBRUSH, *HFONT, *HMENU, *HGLOBAL, *HDROP;
typedef struct { LONG x, y; } POINT;
typedef struct { LONG cx, cy; } SIZE;
typedef struct { LONG left, top, right, bottom; } RECT;
typedef LRESULT (*WNDPROC)(HWND, UINT, WPARAM, LPARAM);

typedef struct {
  UINT cbSize, style;
  WNDPROC lpfnWndProc;
  int cbClsExtra, cbWndExtra;
  HINSTANCE hInstance;
  HICON hIcon;
  HCURSOR hCursor;
  HBRUSH hbrBackground;
  const wchar_t *lpszMenuName, *lpszClassName;
  HICON hIconSm;
} WNDCLASSEXW;

typedef struct {
  HWND hwnd;
  UINT message;
  WPARAM wParam;
  LPARAM lParam;
  DWORD time;
  POINT pt;
  DWORD lPrivate;
} MSG;

typedef struct {
  DWORD biSize;
  LONG biWidth, biHeight;
  WORD biPlanes, biBitCount;
  DWORD biCompression, biSizeImage;
  LONG biXPelsPerMeter, biYPelsPerMeter;
  DWORD biClrUsed, biClrImportant;
} BITMAPINFOHEADER;

typedef struct { BYTE BlendOp, BlendFlags, SourceConstantAlpha, AlphaFormat; } BLENDFUNCTION;

typedef struct {
  DWORD lStructSize;
  HWND hwndOwner;
  HINSTANCE hInstance;
  const wchar_t *lpstrFilter;
  wchar_t *lpstrCustomFilter;
  DWORD nMaxCustFilter, nFilterIndex;
  wchar_t *lpstrFile;
  DWORD nMaxFile;
  wchar_t *lpstrFileTitle;
  DWORD nMaxFileTitle;
  const wchar_t *lpstrInitialDir, *lpstrTitle;
  DWORD Flags;
  WORD nFileOffset, nFileExtension;
  const wchar_t *lpstrDefExt;
  LPARAM lCustData;
  void *lpfnHook;
  const wchar_t *lpTemplateName;
  void *pvReserved;
  DWORD dwReserved, FlagsEx;
} OPENFILENAMEW;

typedef struct { DWORD dwSize, dwICC; } INITCOMMONCONTROLSEX;

_Static_assert(sizeof(WNDCLASSEXW) == 80, "WNDCLASSEXW");
_Static_assert(sizeof(MSG) == 48, "MSG");
_Static_assert(sizeof(BITMAPINFOHEADER) == 40, "BIH");
_Static_assert(sizeof(OPENFILENAMEW) == 152, "OFN");

/* COM IStream – only Release is needed */
typedef struct IStream IStream;
typedef struct {
  void *QueryInterface;
  ULONG (*AddRef)(IStream *);
  ULONG (*Release)(IStream *);
} IStreamVtbl;
struct IStream { IStreamVtbl *lpVtbl; };

/* GDI+ flat API */
typedef void GpImage, GpBitmap, GpGraphics, GpImageAttributes;
typedef struct {
  UINT32 GdiplusVersion;
  void *DebugEventCallback;
  BOOL SuppressBackgroundThread, SuppressExternalCodecs;
} GdiplusStartupInput;
typedef struct {
  DWORD id;
  ULONG length;
  WORD type;
  void *value;
} PropertyItem;

/* ----------------------------------------------------------- imports */
#define IMP __declspec(dllimport)
/* kernel32 */
IMP HINSTANCE GetModuleHandleW(const wchar_t *);
IMP __declspec(noreturn) void ExitProcess(UINT);
IMP wchar_t *GetCommandLineW(void);
IMP HANDLE LocalFree(HANDLE);
IMP HGLOBAL GlobalAlloc(UINT, size_t);
IMP HGLOBAL GlobalFree(HGLOBAL);
IMP void *GlobalLock(HGLOBAL);
IMP BOOL GlobalUnlock(HGLOBAL);
IMP size_t GlobalSize(HGLOBAL);
IMP int MulDiv(int, int, int);
/* user32 */
IMP unsigned short RegisterClassExW(const WNDCLASSEXW *);
IMP HWND CreateWindowExW(DWORD, const wchar_t *, const wchar_t *, DWORD, int, int, int, int, HWND, HMENU, HINSTANCE, void *);
IMP LRESULT DefWindowProcW(HWND, UINT, WPARAM, LPARAM);
IMP BOOL GetMessageW(MSG *, HWND, UINT, UINT);
IMP BOOL TranslateMessage(const MSG *);
IMP LRESULT DispatchMessageW(const MSG *);
IMP BOOL IsDialogMessageW(HWND, MSG *);
IMP void PostQuitMessage(int);
IMP BOOL ShowWindow(HWND, int);
IMP BOOL DestroyWindow(HWND);
IMP BOOL UpdateLayeredWindow(HWND, HDC, POINT *, SIZE *, HDC, POINT *, DWORD, BLENDFUNCTION *, DWORD);
IMP HDC GetDC(HWND);
IMP int ReleaseDC(HWND, HDC);
IMP BOOL SetWindowPos(HWND, HWND, int, int, int, int, UINT);
IMP LONG_PTR GetWindowLongPtrW(HWND, int);
IMP LONG_PTR SetWindowLongPtrW(HWND, int, LONG_PTR);
IMP LRESULT SendMessageW(HWND, UINT, WPARAM, LPARAM);
IMP HCURSOR LoadCursorW(HINSTANCE, const wchar_t *);
IMP HCURSOR SetCursor(HCURSOR);
IMP HICON LoadIconW(HINSTANCE, const wchar_t *);
IMP BOOL RegisterHotKey(HWND, int, UINT, UINT);
IMP BOOL OpenClipboard(HWND);
IMP BOOL CloseClipboard(void);
IMP HANDLE GetClipboardData(UINT);
IMP UINT RegisterClipboardFormatW(const wchar_t *);
IMP int MessageBoxW(HWND, const wchar_t *, const wchar_t *, UINT);
IMP BOOL GetWindowRect(HWND, RECT *);
IMP BOOL SetWindowTextW(HWND, const wchar_t *);
IMP short GetKeyState(int);
IMP BOOL AdjustWindowRectEx(RECT *, DWORD, BOOL, DWORD);
IMP BOOL SystemParametersInfoW(UINT, UINT, void *, UINT);
IMP BOOL SetForegroundWindow(HWND);
IMP int wsprintfW(wchar_t *, const wchar_t *, ...);
/* gdi32 */
IMP HDC CreateCompatibleDC(HDC);
IMP HBITMAP CreateDIBSection(HDC, const void *, UINT, void **, HANDLE, DWORD);
IMP HGDIOBJ SelectObject(HDC, HGDIOBJ);
IMP BOOL DeleteObject(HGDIOBJ);
IMP int GetDeviceCaps(HDC, int);
IMP HFONT CreateFontW(int, int, int, int, int, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, const wchar_t *);
/* shell32 */
IMP UINT DragQueryFileW(HDROP, UINT, wchar_t *, UINT);
IMP void DragFinish(HDROP);
IMP wchar_t **CommandLineToArgvW(const wchar_t *, int *);
/* comdlg32 */
IMP BOOL GetOpenFileNameW(OPENFILENAMEW *);
/* comctl32 */
IMP BOOL InitCommonControlsEx(const INITCOMMONCONTROLSEX *);
/* ole32 */
IMP HRESULT CreateStreamOnHGlobal(HGLOBAL, BOOL, IStream **);
/* gdiplus */
IMP int GdiplusStartup(ULONG_PTR *, const GdiplusStartupInput *, void *);
IMP void GdiplusShutdown(ULONG_PTR);
IMP int GdipCreateBitmapFromFile(const wchar_t *, GpBitmap **);
IMP int GdipCreateBitmapFromStream(IStream *, GpBitmap **);
IMP int GdipCreateBitmapFromGdiDib(const void *, void *, GpBitmap **);
IMP int GdipCreateBitmapFromScan0(int, int, int, int, BYTE *, GpBitmap **);
IMP int GdipGetImageWidth(GpImage *, UINT *);
IMP int GdipGetImageHeight(GpImage *, UINT *);
IMP int GdipGetImageGraphicsContext(GpImage *, GpGraphics **);
IMP int GdipDeleteGraphics(GpGraphics *);
IMP int GdipDisposeImage(GpImage *);
IMP int GdipSetInterpolationMode(GpGraphics *, int);
IMP int GdipSetPixelOffsetMode(GpGraphics *, int);
IMP int GdipSetCompositingMode(GpGraphics *, int);
IMP int GdipGraphicsClear(GpGraphics *, UINT32);
IMP int GdipDrawImageRectRectI(GpGraphics *, GpImage *, int, int, int, int, int, int, int, int, int, const GpImageAttributes *, void *, void *);
IMP int GdipCreateImageAttributes(GpImageAttributes **);
IMP int GdipSetImageAttributesWrapMode(GpImageAttributes *, int, UINT32, BOOL);
IMP int GdipDisposeImageAttributes(GpImageAttributes *);
IMP int GdipGetPropertyItemSize(GpImage *, DWORD, UINT *);
IMP int GdipGetPropertyItem(GpImage *, DWORD, UINT, PropertyItem *);
IMP int GdipImageRotateFlip(GpImage *, int);

/* ---------------------------------------------- minimal CRT pieces */
/* The program is linked without the C runtime, so it provides the few
 * functions the compiler may call implicitly. */
int _fltused = 0;
#if defined(_MSC_VER) && !defined(__clang__)
#include <intrin.h>
#pragma function(memset, memcpy)
void *memset(void *d, int c, size_t n) {
  __stosb((unsigned char *)d, (unsigned char)c, n);
  return d;
}
void *memcpy(void *d, const void *s, size_t n) {
  __movsb((unsigned char *)d, (const unsigned char *)s, n);
  return d;
}
#else
void *memset(void *d, int c, size_t n) {
  void *r = d;
  __asm__ volatile("rep stosb" : "+D"(d), "+c"(n) : "a"(c) : "memory");
  return r;
}
void *memcpy(void *d, const void *s, size_t n) {
  void *r = d;
  __asm__ volatile("rep movsb" : "+D"(d), "+S"(s), "+c"(n) : : "memory");
  return r;
}
#endif

/* ------------------------------------------------------- constants */
#define WM_CREATE 0x0001
#define WM_DESTROY 0x0002
#define WM_CLOSE 0x0010
#define WM_SETFONT 0x0030
#define WM_NCHITTEST 0x0084
#define WM_NCLBUTTONDBLCLK 0x00A3
#define WM_NCRBUTTONDOWN 0x00A4
#define WM_NCRBUTTONUP 0x00A5
#define WM_KEYDOWN 0x0100
#define WM_COMMAND 0x0111
#define WM_SYSCOMMAND 0x0112
#define WM_HSCROLL 0x0114
#define WM_SETCURSOR 0x0020
#define WM_MOUSEWHEEL 0x020A
#define WM_DROPFILES 0x0233
#define WM_HOTKEY 0x0312
#define WM_USER 0x0400
#define SC_MAXIMIZE 0xF030
#define HTCAPTION 2
#define BM_GETCHECK 0x00F0
#define BM_SETCHECK 0x00F1
#define TBM_GETPOS (WM_USER)
#define TBM_SETPOS (WM_USER + 5)
#define TBM_SETRANGE (WM_USER + 6)
#define TBM_SETPAGESIZE (WM_USER + 21)
#define TBS_NOTICKS 0x0010
#define TB_THUMBTRACK 5
#define WS_POPUP 0x80000000u
#define WS_CHILD 0x40000000u
#define WS_VISIBLE 0x10000000u
#define WS_TABSTOP 0x00010000u
#define WS_CAPTION 0x00C00000u
#define WS_SYSMENU 0x00080000u
#define WS_MINIMIZEBOX 0x00020000u
#define WS_EX_TOPMOST 0x00000008u
#define WS_EX_ACCEPTFILES 0x00000010u
#define WS_EX_TRANSPARENT 0x00000020u
#define WS_EX_TOOLWINDOW 0x00000080u
#define WS_EX_LAYERED 0x00080000u
#define BS_PUSHBUTTON 0
#define BS_AUTOCHECKBOX 3
#define SW_HIDE 0
#define SW_SHOWNOACTIVATE 4
#define SW_SHOW 5
#define SWP_NOSIZE 1
#define SWP_NOMOVE 2
#define SWP_NOZORDER 4
#define SWP_NOACTIVATE 0x10
#define SWP_FRAMECHANGED 0x20
#define HWND_TOPMOST ((HWND)(LONG_PTR)-1)
#define GWL_EXSTYLE (-20)
#define MOD_ALT 1
#define MOD_CONTROL 2
#define MOD_NOREPEAT 0x4000
#define VK_CONTROL 0x11
#define VK_ADD 0x6B
#define VK_SUBTRACT 0x6D
#define VK_OEM_PLUS 0xBB
#define VK_OEM_MINUS 0xBD
#define CF_DIB 8
#define CF_HDROP 15
#define SPI_GETWORKAREA 0x30
#define LOGPIXELSX 88
#define MB_ICONWARNING 0x30
#define COLOR_BTNFACE 15
#define CW_USEDEFAULT ((int)0x80000000)
#define OFN_HIDEREADONLY 0x4
#define OFN_PATHMUSTEXIST 0x800
#define OFN_FILEMUSTEXIST 0x1000
#define OFN_EXPLORER 0x80000
#define GMEM_MOVEABLE 2
#define IDC_ARROW ((const wchar_t *)32512)
#define IDC_SIZEALL ((const wchar_t *)32646)
#define PixelFormat32bppPARGB 0x000E200B
#define UnitPixel 2
#define WrapModeTileFlipXY 3
#define InterpBilinear 3
#define InterpNearest 5
#define InterpHQBicubic 7
#define PixelOffsetHalf 4
#define CompositingSourceCopy 1

#define LOWORD(x) ((WORD)((UINT_PTR)(x) & 0xFFFF))
#define HIWORD(x) ((WORD)(((UINT_PTR)(x) >> 16) & 0xFFFF))

enum { ID_OPEN = 101, ID_PASTE, ID_LOCK, ID_HIDE };
enum { HK_LOCK = 1, HK_HIDE, HK_PLUS, HK_MINUS, HK_PLUS2, HK_MINUS2 };

/* --------------------------------------------------------- state */
static HINSTANCE g_inst;
static HWND g_ctrl, g_over;
static HWND g_btnOpen, g_btnPaste, g_lblOp, g_tbOp, g_lblSc, g_tbSc, g_chkLock, g_chkHide, g_hint;
static HFONT g_font;
static int g_dpi = 96;
static GpBitmap *g_img;
static int g_imgW, g_imgH;
static int g_opacity = 50; /* 5..100 */
static int g_scale = 100;  /* 10..400 */
static int g_maxScale = 400;
static BOOL g_locked, g_hidden;
static HDC g_memDC;
static HBITMAP g_dib;
static int g_rw, g_rh;
static wchar_t g_path[1024];

static int S(int v) { return MulDiv(v, g_dpi, 96); }
static int clampi(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }

static const wchar_t APP_TITLE[] = L"Оверлей картинки";

/* ------------------------------------------------------ rendering */
static void push_layered(POINT *pos) {
  if (!g_dib) return;
  SIZE sz = {g_rw, g_rh};
  POINT src = {0, 0};
  BLENDFUNCTION bf = {0, 0, (BYTE)(g_opacity * 255 / 100), 1};
  UpdateLayeredWindow(g_over, NULL, pos, &sz, g_memDC, &src, 0, &bf, 2);
}

/* Re-render the image at the current scale. keepCenter: keep window centre. */
static void render(BOOL hq, BOOL keepCenter) {
  if (!g_img) return;
  int w = (int)((long long)g_imgW * g_scale / 100);
  int h = (int)((long long)g_imgH * g_scale / 100);
  if (w < 1) w = 1;
  if (h < 1) h = 1;

  BITMAPINFOHEADER bi;
  memset(&bi, 0, sizeof bi);
  bi.biSize = sizeof bi;
  bi.biWidth = w;
  bi.biHeight = -h; /* top-down */
  bi.biPlanes = 1;
  bi.biBitCount = 32;
  void *bits = NULL;
  HBITMAP dib = CreateDIBSection(NULL, &bi, 0, &bits, NULL, 0);
  if (!dib || !bits) {
    if (dib) DeleteObject(dib);
    return;
  }

  GpBitmap *target = NULL;
  if (GdipCreateBitmapFromScan0(w, h, w * 4, PixelFormat32bppPARGB, (BYTE *)bits, &target) == 0) {
    GpGraphics *g = NULL;
    if (GdipGetImageGraphicsContext(target, &g) == 0) {
      GdipSetCompositingMode(g, CompositingSourceCopy);
      GdipSetPixelOffsetMode(g, PixelOffsetHalf);
      GdipSetInterpolationMode(g, g_scale == 100 ? InterpNearest : (hq ? InterpHQBicubic : InterpBilinear));
      GpImageAttributes *ia = NULL;
      GdipCreateImageAttributes(&ia);
      if (ia) GdipSetImageAttributesWrapMode(ia, WrapModeTileFlipXY, 0, 0);
      GdipDrawImageRectRectI(g, g_img, 0, 0, w, h, 0, 0, g_imgW, g_imgH, UnitPixel, ia, NULL, NULL);
      if (ia) GdipDisposeImageAttributes(ia);
      GdipDeleteGraphics(g);
    }
    GdipDisposeImage(target);
  }

  POINT pos, *ppos = NULL;
  if (keepCenter && g_dib) {
    RECT r;
    GetWindowRect(g_over, &r);
    pos.x = (r.left + r.right) / 2 - w / 2;
    pos.y = (r.top + r.bottom) / 2 - h / 2;
    ppos = &pos;
  } else if (!keepCenter) {
    RECT wa;
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &wa, 0);
    pos.x = (wa.left + wa.right) / 2 - w / 2;
    pos.y = (wa.top + wa.bottom) / 2 - h / 2;
    ppos = &pos;
  }

  HGDIOBJ old = SelectObject(g_memDC, dib);
  if (g_dib && old == g_dib) DeleteObject(g_dib);
  g_dib = dib;
  g_rw = w;
  g_rh = h;
  push_layered(ppos);
}

/* ------------------------------------------------------- UI state */
static void set_opacity(int v) {
  g_opacity = clampi(v, 5, 100);
  SendMessageW(g_tbOp, TBM_SETPOS, 1, g_opacity);
  wchar_t buf[64];
  wsprintfW(buf, L"Непрозрачность: %d%%", g_opacity);
  SetWindowTextW(g_lblOp, buf);
  push_layered(NULL);
}

static void set_scale(int v, BOOL hq) {
  int nv = clampi(v, 10, g_maxScale);
  BOOL changed = nv != g_scale;
  g_scale = nv;
  SendMessageW(g_tbSc, TBM_SETPOS, 1, g_scale);
  wchar_t buf[64];
  wsprintfW(buf, L"Масштаб: %d%%", g_scale);
  SetWindowTextW(g_lblSc, buf);
  if (changed || hq) render(hq, TRUE);
}

static void update_title(void) {
  wchar_t buf[128];
  wsprintfW(buf, L"%s%s", APP_TITLE, g_locked ? L" — закреплено" : L"");
  SetWindowTextW(g_ctrl, buf);
}

static void set_locked(BOOL on) {
  g_locked = on;
  LONG_PTR ex = GetWindowLongPtrW(g_over, GWL_EXSTYLE);
  ex = on ? (ex | WS_EX_TRANSPARENT) : (ex & ~(LONG_PTR)WS_EX_TRANSPARENT);
  SetWindowLongPtrW(g_over, GWL_EXSTYLE, ex);
  SetWindowPos(g_over, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_FRAMECHANGED);
  SendMessageW(g_chkLock, BM_SETCHECK, on ? 1 : 0, 0);
  update_title();
}

static void set_hidden(BOOL on) {
  g_hidden = on;
  SendMessageW(g_chkHide, BM_SETCHECK, on ? 1 : 0, 0);
  if (g_img) ShowWindow(g_over, on ? SW_HIDE : SW_SHOWNOACTIVATE);
}

/* ---------------------------------------------------- image loading */
static void apply_exif_orientation(GpBitmap *b) {
  static union { PropertyItem item; BYTE raw[256]; } buf;
  UINT size = 0;
  if (GdipGetPropertyItemSize(b, 0x0112, &size) != 0 || size == 0 || size > sizeof buf) return;
  PropertyItem *pi = &buf.item;
  if (GdipGetPropertyItem(b, 0x0112, size, pi) != 0 || !pi->value || pi->length < 2) return;
  int o = *(WORD *)pi->value;
  static const int map[9] = {0, 0, 4, 2, 6, 5, 1, 7, 3};
  if (o >= 2 && o <= 8) GdipImageRotateFlip(b, map[o]);
}

/* Takes ownership of src: copies it into an independent PARGB bitmap. */
static BOOL set_image(GpBitmap *src) {
  if (!src) return 0;
  apply_exif_orientation(src);
  UINT w = 0, h = 0;
  GdipGetImageWidth(src, &w);
  GdipGetImageHeight(src, &h);
  if (w == 0 || h == 0 || w > 30000 || h > 30000) {
    GdipDisposeImage(src);
    return 0;
  }
  GpBitmap *dst = NULL;
  if (GdipCreateBitmapFromScan0((int)w, (int)h, 0, PixelFormat32bppPARGB, NULL, &dst) != 0) {
    GdipDisposeImage(src);
    return 0;
  }
  GpGraphics *g = NULL;
  if (GdipGetImageGraphicsContext(dst, &g) == 0) {
    GdipGraphicsClear(g, 0);
    GdipSetCompositingMode(g, CompositingSourceCopy);
    GdipSetPixelOffsetMode(g, PixelOffsetHalf);
    GdipSetInterpolationMode(g, InterpNearest);
    GdipDrawImageRectRectI(g, src, 0, 0, (int)w, (int)h, 0, 0, (int)w, (int)h, UnitPixel, NULL, NULL, NULL);
    GdipDeleteGraphics(g);
  }
  GdipDisposeImage(src);

  if (g_img) GdipDisposeImage(g_img);
  g_img = dst;
  g_imgW = (int)w;
  g_imgH = (int)h;

  /* limit rendered size to 8192 px per side */
  int big = g_imgW > g_imgH ? g_imgW : g_imgH;
  g_maxScale = clampi(819200 / big, 10, 400);
  SendMessageW(g_tbSc, TBM_SETRANGE, 1, (LPARAM)((g_maxScale << 16) | 10));

  /* fit to ~85% of the work area */
  RECT wa;
  SystemParametersInfoW(SPI_GETWORKAREA, 0, &wa, 0);
  int fw = (int)((long long)(wa.right - wa.left) * 85 / g_imgW);
  int fh = (int)((long long)(wa.bottom - wa.top) * 85 / g_imgH);
  int fit = fw < fh ? fw : fh;
  g_scale = clampi(fit < 100 ? fit : 100, 10, g_maxScale);
  set_scale(g_scale, FALSE);
  render(TRUE, FALSE);

  g_hidden = 0;
  SendMessageW(g_chkHide, BM_SETCHECK, 0, 0);
  ShowWindow(g_over, SW_SHOWNOACTIVATE);
  SetWindowPos(g_over, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
  return 1;
}

static void load_file(const wchar_t *path) {
  GpBitmap *b = NULL;
  if (GdipCreateBitmapFromFile(path, &b) != 0 || !b || !set_image(b))
    MessageBoxW(g_ctrl,
                L"Не удалось открыть изображение.\n\nПоддерживаются PNG, JPG, BMP, GIF, TIFF.\n"
                L"Для WebP и других форматов: открой картинку в браузере, "
                L"нажми «Копировать изображение» и вставь сюда (Ctrl+V).",
                APP_TITLE, MB_ICONWARNING);
}

static void open_dialog(void) {
  OPENFILENAMEW ofn;
  memset(&ofn, 0, sizeof ofn);
  g_path[0] = 0;
  ofn.lStructSize = sizeof ofn;
  ofn.hwndOwner = g_ctrl;
  ofn.lpstrFilter = L"Изображения\0*.png;*.jpg;*.jpeg;*.bmp;*.gif;*.tif;*.tiff;*.jfif\0Все файлы\0*.*\0";
  ofn.nFilterIndex = 1;
  ofn.lpstrFile = g_path;
  ofn.nMaxFile = 1024;
  ofn.lpstrTitle = L"Выбери картинку";
  ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
  if (GetOpenFileNameW(&ofn)) load_file(g_path);
}

static void paste_clipboard(void) {
  BOOL ok = 0;
  if (!OpenClipboard(g_ctrl)) return;

  /* 1) PNG (keeps transparency; browsers and many editors provide it) */
  UINT fmtPng = RegisterClipboardFormatW(L"PNG");
  HANDLE h = fmtPng ? GetClipboardData(fmtPng) : NULL;
  if (h) {
    size_t n = GlobalSize(h);
    void *p = GlobalLock(h);
    if (p && n) {
      HGLOBAL copy = GlobalAlloc(GMEM_MOVEABLE, n);
      void *cp = copy ? GlobalLock(copy) : NULL;
      if (cp) {
        memcpy(cp, p, n);
        GlobalUnlock(copy);
        IStream *st = NULL;
        if (CreateStreamOnHGlobal(copy, 1, &st) == 0 && st) {
          GpBitmap *b = NULL;
          if (GdipCreateBitmapFromStream(st, &b) == 0 && b) ok = set_image(b);
          st->lpVtbl->Release(st);
        } else {
          GlobalFree(copy);
        }
      } else if (copy) {
        GlobalFree(copy);
      }
    }
    if (p) GlobalUnlock(h);
  }

  /* 2) a copied file in Explorer */
  if (!ok && (h = GetClipboardData(CF_HDROP)) != NULL) {
    if (DragQueryFileW((HDROP)h, 0, g_path, 1024)) {
      GpBitmap *b = NULL;
      if (GdipCreateBitmapFromFile(g_path, &b) == 0 && b) ok = set_image(b);
    }
  }

  /* 3) plain bitmap (screenshots, Paint, etc.) */
  if (!ok && (h = GetClipboardData(CF_DIB)) != NULL) {
    BITMAPINFOHEADER *bih = (BITMAPINFOHEADER *)GlobalLock(h);
    if (bih) {
      size_t off = bih->biSize;
      if (bih->biCompression == 3 /*BI_BITFIELDS*/ && bih->biSize == 40) off += 12;
      DWORD colors = bih->biClrUsed;
      if (!colors && bih->biBitCount <= 8) colors = 1u << bih->biBitCount;
      off += (size_t)colors * 4;
      GpBitmap *b = NULL;
      if (GdipCreateBitmapFromGdiDib(bih, (BYTE *)bih + off, &b) == 0 && b) ok = set_image(b);
      GlobalUnlock(h);
    }
  }

  CloseClipboard();
  if (!ok) MessageBoxW(g_ctrl, L"В буфере обмена нет картинки.", APP_TITLE, MB_ICONWARNING);
}

static void handle_drop(HDROP d) {
  wchar_t path[1024];
  UINT ok = DragQueryFileW(d, 0, path, 1024);
  DragFinish(d);
  if (ok) load_file(path);
}

/* ------------------------------------------------------ overlay window */
static LRESULT overlay_proc(HWND h, UINT m, WPARAM w, LPARAM l) {
  switch (m) {
  case WM_NCHITTEST:
    return HTCAPTION; /* drag anywhere */
  case WM_SETCURSOR:
    SetCursor(LoadCursorW(NULL, IDC_SIZEALL));
    return 1;
  case WM_NCLBUTTONDBLCLK:
  case WM_NCRBUTTONDOWN:
  case WM_NCRBUTTONUP:
    return 0;
  case WM_SYSCOMMAND:
    if ((w & 0xFFF0) == SC_MAXIMIZE) return 0;
    break;
  case WM_MOUSEWHEEL: {
    static int acc; /* touchpads send small deltas — accumulate them */
    acc += (short)HIWORD(w);
    int steps = acc / 120;
    acc -= steps * 120;
    if (!steps) return 0;
    if (GetKeyState(VK_CONTROL) < 0)
      set_scale(g_scale + steps * 5, TRUE);
    else
      set_opacity(g_opacity + steps * 5);
    return 0;
  }
  case WM_DROPFILES:
    handle_drop((HDROP)w);
    return 0;
  case WM_CLOSE:
    set_hidden(1);
    return 0;
  }
  return DefWindowProcW(h, m, w, l);
}

/* ------------------------------------------------------ control window */
static HWND mk(const wchar_t *cls, const wchar_t *text, DWORD style, int x, int y, int w, int h, int id) {
  HWND c = CreateWindowExW(0, cls, text, WS_CHILD | WS_VISIBLE | style, S(x), S(y), S(w), S(h), g_ctrl,
                           (HMENU)(UINT_PTR)id, g_inst, NULL);
  SendMessageW(c, WM_SETFONT, (WPARAM)g_font, 0);
  return c;
}

#define CLIENT_W 340
#define CLIENT_H 330

static void build_controls(void) {
  g_font = CreateFontW(-MulDiv(9, g_dpi, 72), 0, 0, 0, 400, 0, 0, 0, 1, 0, 0, 5, 0, L"Segoe UI");
  g_btnOpen = mk(L"BUTTON", L"Открыть файл…", BS_PUSHBUTTON | WS_TABSTOP, 12, 12, 154, 30, ID_OPEN);
  g_btnPaste = mk(L"BUTTON", L"Вставить (Ctrl+V)", BS_PUSHBUTTON | WS_TABSTOP, 174, 12, 154, 30, ID_PASTE);

  g_lblOp = mk(L"STATIC", L"", 0, 14, 56, 300, 20, 0);
  g_tbOp = mk(L"msctls_trackbar32", L"", TBS_NOTICKS | WS_TABSTOP, 6, 76, 328, 30, 0);
  SendMessageW(g_tbOp, TBM_SETRANGE, 1, (LPARAM)((100 << 16) | 5));
  SendMessageW(g_tbOp, TBM_SETPAGESIZE, 0, 10);

  g_lblSc = mk(L"STATIC", L"", 0, 14, 114, 300, 20, 0);
  g_tbSc = mk(L"msctls_trackbar32", L"", TBS_NOTICKS | WS_TABSTOP, 6, 134, 328, 30, 0);
  SendMessageW(g_tbSc, TBM_SETRANGE, 1, (LPARAM)((400 << 16) | 10));
  SendMessageW(g_tbSc, TBM_SETPAGESIZE, 0, 10);

  g_chkLock = mk(L"BUTTON", L"Закрепить — клики сквозь картинку (Ctrl+Alt+L)", BS_AUTOCHECKBOX | WS_TABSTOP, 14,
                 176, 320, 24, ID_LOCK);
  g_chkHide = mk(L"BUTTON", L"Скрыть картинку (Ctrl+Alt+H)", BS_AUTOCHECKBOX | WS_TABSTOP, 14, 202, 320, 24, ID_HIDE);

  g_hint = mk(L"STATIC",
              L"• Картинку можно перетаскивать мышью.\r\n"
              L"• Колесо мыши над картинкой — непрозрачность,\r\n   Ctrl + колесо — масштаб.\r\n"
              L"• Ctrl+Alt+Плюс / Минус — непрозрачность в любой\r\n   момент, даже когда картинка закреплена.\r\n"
              L"• Файл можно просто перетащить в это окно.",
              0, 14, 236, 318, 92, 0);
}

static LRESULT ctrl_proc(HWND h, UINT m, WPARAM w, LPARAM l) {
  switch (m) {
  case WM_COMMAND:
    if (HIWORD(w) == 0) {
      switch (LOWORD(w)) {
      case ID_OPEN: open_dialog(); return 0;
      case ID_PASTE: paste_clipboard(); return 0;
      case ID_LOCK: set_locked(SendMessageW(g_chkLock, BM_GETCHECK, 0, 0) == 1); return 0;
      case ID_HIDE: set_hidden(SendMessageW(g_chkHide, BM_GETCHECK, 0, 0) == 1); return 0;
      }
    }
    break;
  case WM_HSCROLL:
    if ((HWND)l == g_tbOp) {
      set_opacity((int)SendMessageW(g_tbOp, TBM_GETPOS, 0, 0));
      return 0;
    }
    if ((HWND)l == g_tbSc) {
      /* fast preview while dragging the thumb, high quality afterwards */
      BOOL dragging = LOWORD(w) == TB_THUMBTRACK;
      set_scale((int)SendMessageW(g_tbSc, TBM_GETPOS, 0, 0), !dragging);
      return 0;
    }
    break;
  case WM_HOTKEY:
    switch ((int)w) {
    case HK_LOCK: set_locked(!g_locked); break;
    case HK_HIDE: set_hidden(!g_hidden); break;
    case HK_PLUS: case HK_PLUS2: set_opacity(g_opacity + 5); break;
    case HK_MINUS: case HK_MINUS2: set_opacity(g_opacity - 5); break;
    }
    return 0;
  case WM_DROPFILES:
    handle_drop((HDROP)w);
    return 0;
  case WM_DESTROY:
    PostQuitMessage(0);
    return 0;
  }
  return DefWindowProcW(h, m, w, l);
}

/* ------------------------------------------------------------- entry */
void WinMainCRTStartup(void) {
  g_inst = GetModuleHandleW(NULL);

  INITCOMMONCONTROLSEX icc = {sizeof icc, 0x4 /*ICC_BAR_CLASSES*/ | 0x4000 /*ICC_STANDARD_CLASSES*/};
  InitCommonControlsEx(&icc);

  ULONG_PTR gdipToken = 0;
  GdiplusStartupInput gsi = {1, NULL, 0, 0};
  if (GdiplusStartup(&gdipToken, &gsi, NULL) != 0) {
    MessageBoxW(NULL, L"Не удалось запустить GDI+.", APP_TITLE, MB_ICONWARNING);
    ExitProcess(1);
  }

  HDC sdc = GetDC(NULL);
  g_dpi = GetDeviceCaps(sdc, LOGPIXELSX);
  ReleaseDC(NULL, sdc);
  if (g_dpi < 96) g_dpi = 96;
  g_memDC = CreateCompatibleDC(NULL);

  HICON icon = LoadIconW(g_inst, (const wchar_t *)1);

  WNDCLASSEXW wc;
  memset(&wc, 0, sizeof wc);
  wc.cbSize = sizeof wc;
  wc.lpfnWndProc = ctrl_proc;
  wc.hInstance = g_inst;
  wc.hIcon = icon;
  wc.hIconSm = icon;
  wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
  wc.hbrBackground = (HBRUSH)(UINT_PTR)(COLOR_BTNFACE + 1);
  wc.lpszClassName = L"ImageOverlayControl";
  RegisterClassExW(&wc);

  wc.lpfnWndProc = overlay_proc;
  wc.hbrBackground = NULL;
  wc.hCursor = LoadCursorW(NULL, IDC_SIZEALL);
  wc.lpszClassName = L"ImageOverlayImage";
  RegisterClassExW(&wc);

  DWORD style = WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
  RECT r = {0, 0, S(CLIENT_W), S(CLIENT_H)};
  AdjustWindowRectEx(&r, style, 0, WS_EX_ACCEPTFILES);
  g_ctrl = CreateWindowExW(WS_EX_ACCEPTFILES, L"ImageOverlayControl", APP_TITLE, style, CW_USEDEFAULT, CW_USEDEFAULT,
                           r.right - r.left, r.bottom - r.top, NULL, NULL, g_inst, NULL);

  g_over = CreateWindowExW(WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_ACCEPTFILES, L"ImageOverlayImage",
                           L"Оверлей", WS_POPUP, 0, 0, 1, 1, NULL, NULL, g_inst, NULL);

  build_controls();
  set_opacity(g_opacity);
  set_scale(100, FALSE);
  update_title();

  RegisterHotKey(g_ctrl, HK_LOCK, MOD_CONTROL | MOD_ALT | MOD_NOREPEAT, 'L');
  RegisterHotKey(g_ctrl, HK_HIDE, MOD_CONTROL | MOD_ALT | MOD_NOREPEAT, 'H');
  RegisterHotKey(g_ctrl, HK_PLUS, MOD_CONTROL | MOD_ALT, VK_OEM_PLUS);
  RegisterHotKey(g_ctrl, HK_MINUS, MOD_CONTROL | MOD_ALT, VK_OEM_MINUS);
  RegisterHotKey(g_ctrl, HK_PLUS2, MOD_CONTROL | MOD_ALT, VK_ADD);
  RegisterHotKey(g_ctrl, HK_MINUS2, MOD_CONTROL | MOD_ALT, VK_SUBTRACT);

  ShowWindow(g_ctrl, SW_SHOW);

  /* image passed on the command line ("Open with…" or dropped on the .exe) */
  int argc = 0;
  wchar_t **argv = CommandLineToArgvW(GetCommandLineW(), &argc);
  if (argv) {
    if (argc > 1) load_file(argv[1]);
    LocalFree(argv);
  }

  MSG msg;
  while (GetMessageW(&msg, NULL, 0, 0) > 0) {
    if (msg.message == WM_KEYDOWN && GetKeyState(VK_CONTROL) < 0) {
      if (msg.wParam == 'V') { paste_clipboard(); continue; }
      if (msg.wParam == 'O') { open_dialog(); continue; }
    }
    if (IsDialogMessageW(g_ctrl, &msg)) continue;
    TranslateMessage(&msg);
    DispatchMessageW(&msg);
  }

  if (g_img) GdipDisposeImage(g_img);
  GdiplusShutdown(gdipToken);
  ExitProcess(0);
}
