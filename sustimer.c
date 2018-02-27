#include <windows.h>
#include <powrprof.h>
/* #include <shobjidl.h>
  gcc's shobjidl.h:
    STDMETHOD(SetProgressValue)(THIS_ ULONGLONG,ULONGLONG) PURE;
    > ..is incorrect definition
  correct:
    STDMETHOD(SetProgressValue)(THIS_ HWND,ULONGLONG,ULONGLONG) PURE;
*/
#undef INTERFACE
#define INTERFACE ITaskbarList3
DECLARE_INTERFACE(INTERFACE) {
  void *qa[2];
  STDMETHOD_(ULONG, Release)(THIS);
  void *ha[6];
  STDMETHOD(SetProgressValue)(THIS_ HWND, ULONGLONG, ULONGLONG);
};
#undef INTERFACE
#define MS 1000
#define WND_WIDTH 320
#define WND_HEIGHT 240
#define WND_BG RGB(30, 90, 200)
#define TEXT_COLOR RGB(255, 255, 255)
#define PRG_BORDER 2
#define CLS_BORDER 10
#define WTIMER_ID 0
#define WTIMER_OUT 100
#define ATIMEOUT_DEFAULT 60

void __start__() {
  // program will start from here if `gcc -nostartfiles`
  ExitProcess(WinMain(GetModuleHandle(NULL), 0, "", 0));
}

BOOL atimeover;

void startMouseTrack(HWND hwnd) {
  TRACKMOUSEEVENT tme;
  tme.cbSize = sizeof(TRACKMOUSEEVENT);
  tme.dwFlags = TME_LEAVE;
  tme.hwndTrack = hwnd;
  tme.dwHoverTime = 1;
  TrackMouseEvent(&tme);
}

void setTBProgress(HWND hwnd, int now, int max) {
  ITaskbarList3* ppv;
  CLSID tbclsid;
  IID tbiid;
  HRESULT result;
  CLSIDFromString(OLESTR("{56FDF344-FD6D-11d0-958A-006097C9A090}"), &tbclsid);
  IIDFromString(OLESTR("{EA1AFB91-9E28-4B86-90E9-9E9F8A5EEFAF}"), &tbiid);
  CoInitialize(NULL);
  result = CoCreateInstance(&tbclsid, NULL, CLSCTX_ALL, &tbiid, (LPVOID *)&ppv);
  if (result == S_OK) {
    ppv->lpVtbl->SetProgressValue(ppv, hwnd, now, max);
    ppv->lpVtbl->Release(ppv);
  }
  CoUninitialize();
}

int getATimeout() {
  int argc;
  LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &argc);
  if (argc > 1) {
    LPWSTR ts = argv[1];
    int a = 0, b = 0 ,c = 0;
    int *p = NULL;
    while (*ts) { // alt for swscanf(ts,"%d:%d:%d",&a,&b,&c) in msvcrt.dll
      if (*ts >= '0' && *ts <= '9') {
        if (p == NULL) p = &a;
        *p = (*p * 10) + (*ts - '0');
      } else if (*ts == ':') { // hh:mm:ss OR mm:ss OR ss
        p = p == &a ? &b : &c;
      }
      ts++;
    }
    if (p == &c) { // hh:mm:ss
      return (a * 3600 + b * 60 + c) * MS;
    } else if (p == &b) { // mm:ss
      return (a * 60 + b) * MS;
    } else if (p == &a) { // ss
      return a * MS;
    } else ; // p == NULL
  }
  return ATIMEOUT_DEFAULT * MS;
}

LRESULT CALLBACK MainWindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
  static BOOL hover;
  static RECT canvas;
  static RECT progvas;

  static struct {
    int out;
    int rest;
    int fixed;
    int fixedPrev;
  } atimer;

  static struct {
    HFONT font;
    HPEN pen;
    LOGBRUSH logbrush;
    HBRUSH brush;
    TCHAR text[8];
  } counter, closer, logo, progress, progbar, bg;

  static DWORD stime = 0;
  static BOOL counting = FALSE;
  static BOOL timeset = FALSE;

  if (!timeset) {
    atimer.out = getATimeout();
    atimer.rest = atimer.out;
    timeset = TRUE;
  }

  if (counting) {
    atimer.rest = atimer.out - (GetTickCount() - stime);
    if (atimer.rest <= 0) {
      atimeover = TRUE;
      PostMessage(hwnd, WM_CLOSE, 0, 0);
      atimer.rest = 0;
      KillTimer(hwnd, WTIMER_ID);
      counting = FALSE;
    }
  }

  atimer.fixed = atimer.rest / 1000 + counting;

  switch (msg) {
  case WM_CREATE:
    // window timer
    SetTimer(hwnd, WTIMER_ID, WTIMER_OUT, NULL);
    // counter
    counter.font = CreateFont(90, 0, 0, 0, FW_SEMIBOLD, 0, 0, 0,
      DEFAULT_CHARSET, 0, 0, 0, 0, NULL);
    // closer
    closer.pen = CreatePen(PS_SOLID, CLS_BORDER, TEXT_COLOR);
    closer.brush = (HBRUSH)GetStockObject(NULL_BRUSH);
    closer.font = CreateFont(90, 0, 0, 0, FW_SEMIBOLD, 0, 0, 0,
      SYMBOL_CHARSET, 0, 0, 0, 0, NULL);
    // logo
    logo.font = counter.font;
    // progress frame
    progress.pen = CreatePen(PS_SOLID, PRG_BORDER, TEXT_COLOR);
    progress.brush = (HBRUSH)GetStockObject(NULL_BRUSH);
    // progress bar
    progbar.pen = progress.pen;
    progbar.brush = (HBRUSH)CreateSolidBrush(TEXT_COLOR);
    // Set: client area
    GetClientRect(hwnd, &canvas);
    // Set: progressbar area
    progvas.left = canvas.left + 20;
    progvas.top = canvas.bottom - 40;
    progvas.right = canvas.right - 20;
    progvas.bottom = canvas.bottom - 20;
    // background
    bg.brush = (HBRUSH)CreateSolidBrush(WND_BG);
    // repaint
    InvalidateRect(hwnd, NULL, TRUE);
    return 0;
  case WM_TIMER:
    // repaint
    if (atimer.fixed != atimer.fixedPrev) {
      InvalidateRect(hwnd, NULL, TRUE);
      atimer.fixedPrev = atimer.fixed;
    } else {
      InvalidateRect(hwnd, &progvas, FALSE);
    }
    return 0;
  case WM_ERASEBKGND: {
    return 1;
  }
  case WM_PAINT: {
    PAINTSTRUCT ps;
    HDC odc = BeginPaint(hwnd, &ps);
    HDC hdc = CreateCompatibleDC(odc);
    HBITMAP bmp = CreateCompatibleBitmap(odc, WND_WIDTH, WND_HEIGHT);
    int progpos;
    SelectObject(hdc, bmp);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, TEXT_COLOR);
    // background
    FillRect(hdc, &canvas, bg.brush);
    // logo
    SelectObject(hdc, logo.font);
    DrawTextW(hdc, L"\u263e", -1, &canvas,
      DT_LEFT);
    // count down
    SelectObject(hdc, counter.font);
    if (atimer.fixed >= 3600) {
      wsprintf(counter.text, TEXT("%d:%02d:%02d"),
        atimer.fixed / 3600, (atimer.fixed % 3600) / 60, atimer.fixed % 60);
    } else if (atimer.fixed >= 60) {
      wsprintf(counter.text, TEXT("%d:%02d"),
        (atimer.fixed % 3600) / 60, atimer.fixed % 60);
    } else {
      wsprintf(counter.text, TEXT("%d"), atimer.fixed);
    }
    DrawText(hdc, counter.text, -1, &canvas,
      DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    // progress bar
    progpos = (progvas.right - progvas.left) /
      ((float)atimer.out / atimer.rest);
    // progress frame
    SelectObject(hdc, progress.pen);
    SelectObject(hdc, progress.brush);
    Rectangle(hdc,
      progvas.left,
      progvas.top,
      progvas.right,
      progvas.bottom);
    // progress content
    SelectObject(hdc, progbar.pen);
    SelectObject(hdc, progbar.brush);
    Rectangle(hdc,
      progvas.left,
      progvas.top,
      progvas.left + progpos,
      progvas.bottom);
    // close charm
    if (hover) {
      SelectObject(hdc, closer.pen);
      SelectObject(hdc, closer.brush);
      SelectObject(hdc, closer.font);
      Rectangle(hdc, canvas.left, canvas.top, canvas.right, canvas.bottom);
      DrawText(hdc, TEXT("x"), -1, &canvas, DT_RIGHT);
    }
    BitBlt(odc, 0, 0, WND_WIDTH, WND_HEIGHT, hdc, 0, 0, SRCCOPY);
    DeleteDC(hdc);
    DeleteObject(bmp);
    EndPaint(hwnd, &ps);
    setTBProgress(hwnd, atimer.rest, atimer.out);
    if (!stime) {
      stime = GetTickCount();
      counting = TRUE;
    }
    return 0;
  }
  case WM_CLOSE:
    counting = FALSE;
    DestroyWindow(hwnd);
    return 0;
  case WM_DESTROY:
    PostQuitMessage(0);
    return 0;
  case WM_MOUSELEAVE:
    hover = FALSE;
    InvalidateRect(hwnd, NULL, TRUE);
    return 0;
  case WM_MOUSEMOVE:
    if (!hover) {
      hover = TRUE;
      startMouseTrack(hwnd);
      InvalidateRect(hwnd, NULL, TRUE);
    }
    return 0;
  case WM_LBUTTONUP:
    PostMessage(hwnd, WM_CLOSE, 0, 0);
    return 0;
  case WM_CHAR:
    switch (wp) {
    case VK_ESCAPE:
    case VK_SPACE:
    case VK_RETURN:
      PostMessage(hwnd, WM_CLOSE, 0, 0);
      break;
    }
    return 0;
  case WM_POWERBROADCAST:
    if (wp == PBT_APMSUSPEND) {
      PostMessage(hwnd, WM_CLOSE, 0, 0);
      return 0;
    }
    break;
  }
  return DefWindowProc(hwnd, msg, wp, lp);
}

int WINAPI WinMain(HINSTANCE hi, HINSTANCE hp, LPSTR cl, int cs) {
  WNDCLASSEX wc;
  HWND hwnd;
  MSG msg;

  // Main Window: Settings
  wc.cbSize = sizeof(WNDCLASSEX);
  wc.style = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc = MainWindowProc;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  wc.hInstance = hi;
  wc.hIcon =
    (HICON)LoadImage(hi, TEXT("APPICON"), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
    //(HICON)LoadImage(NULL, IDI_APPLICATION, IMAGE_ICON, 0, 0, LR_SHARED);
  wc.hCursor =
    (HCURSOR)LoadImage(NULL, IDC_HAND, IMAGE_CURSOR, 0, 0, LR_SHARED);
  wc.hbrBackground = CreateSolidBrush(WND_BG);
  wc.lpszMenuName = NULL;
  wc.lpszClassName = TEXT("Suspend PC Timer Window Class");
  wc.hIconSm =
    (HICON)LoadImage(hi, TEXT("APPICON"), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
    //(HICON)LoadImage(NULL, IDI_APPLICATION, IMAGE_ICON, 16, 16, LR_SHARED);
  // Main Window: Create, Show
  hwnd = CreateWindowEx(
    WS_EX_TOPMOST,
    (LPCTSTR)MAKELONG(RegisterClassEx(&wc), 0), TEXT("Suspend PC Timer"),
    WS_VISIBLE | WS_SYSMENU | WS_MINIMIZEBOX,
    GetSystemMetrics(SM_CXSCREEN) / 2 - WND_WIDTH / 2,
    GetSystemMetrics(SM_CYSCREEN) / 2 - WND_HEIGHT / 2,
    WND_WIDTH,
    WND_HEIGHT,
    NULL, NULL, hi, NULL
  );
  if (hwnd == NULL) return 1;

  // While msg.message != WM_QUIT
  while (GetMessage(&msg, NULL, 0, 0) > 0) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
  // Suspend if time over
  //if (1||atimeover) {TCHAR s[99];wsprintf(s,TEXT("exit code: %d\nsuspend: %d"),msg.wParam,atimeover);MessageBox(NULL,s,s,0);return msg.wParam;}
  if (atimeover) SetSuspendState(FALSE, FALSE, FALSE);
  return msg.wParam;
}
