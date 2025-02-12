#include "stdafx.h"
#include "common.h"

#ifdef HAS_WMSG

#include <winuser.h>
#include "event_wmsg.h"

#define WM_SOCKET_NOTIFY	(WM_USER + 8192)

typedef struct EVENT_WMSG {
	NET_EVENT event;
	UINT  nMsg;
	HWND  hWnd;
	HINSTANCE   hInstance;
	const char *class_name;
	NET_FILE **files;
	int  size;
	int  count;

	HTABLE *tbl;
} EVENT_WMSG;

static EVENT_WMSG *get_hwnd_event(HWND hWnd)
{
	EVENT_WMSG *ev = (EVENT_WMSG*) GetWindowLongPtr(hWnd, GWLP_USERDATA);
	return ev;
}

static void set_hwnd_event(HWND hWnd, EVENT_WMSG *ev)
{
	SetWindowLongPtr(hWnd, GWLP_USERDATA, (ULONG_PTR) ev);
}

static NET_FILE *file_event_find(EVENT_WMSG *ev, SOCKET fd)
{
	char key[64];

	//_snprintf(key, sizeof(key), "%u", fd);
	_i64toa(fd, key, 10);

	return (NET_FILE *) htable_find(ev->tbl, key);
}

static void wnet_msg_free(NET_EVENT *ev)
{
	EVENT_WMSG *ew = (EVENT_WMSG *) ev;

	if (ew->hWnd != NULL) {
		WNDCLASSEX wcx;

		DestroyWindow(ew->hWnd);
		if (ew->class_name && GetClassInfoEx(ew->hInstance,
			ew->class_name, &wcx)) {
			net_msg_info("%s(%d): unregister class: %s",
				__FUNCTION__, __LINE__, ew->class_name);
			UnregisterClass(ew->class_name, ew->hInstance);
		}
	}
	htable_free(ew->tbl, NULL);
	mem_free(ew->files);
	mem_free(ew);
}

static void wnet_msg_fdmap_set(EVENT_WMSG *ev, NET_FILE *fe)
{
	NET_FILE *pfe;
	char key[64];

	//_snprintf(key, sizeof(key), "%u", fe->fd);
	_i64toa(fe->fd, key, 10);

	pfe = (NET_FILE *) htable_find(ev->tbl, key);
	if (pfe == NULL) {
		htable_enter(ev->tbl, key, fe);
		ev->event.fdcount++;
	} else if (pfe != fe) {
		net_msg_error("%s(%d): old fe(%p) exist, fd=%d",
			__FUNCTION__, __LINE__, pfe, (int) fe->fd);
	}
}

static NET_FILE *wnet_msg_fdmap_get(EVENT_WMSG *ev, SOCKET fd)
{
	char key[64];

	//_snprintf(key, sizeof(key), "%u", fd);
	_i64toa(fd, key, 10);

	return (NET_FILE *) htable_find(ev->tbl, key);
}

static void wnet_msg_fdmap_del(EVENT_WMSG *ev, NET_FILE *fe)
{
	char key[64];

	//_snprintf(key, sizeof(key), "%u", fe->fd);
	_i64toa(fe->fd, key, 10);

	if (htable_delete(ev->tbl, key, NULL) == 0) {
		ev->event.fdcount--;
	}
}

static int wnet_msg_add_read(EVENT_WMSG *ev, NET_FILE *fe)
{
	long lEvent = FD_READ | FD_CLOSE;

	if (fe->mask & EVENT_WRITE) {
		lEvent |= FD_WRITE;
	} else if (is_listen_socket(fe->fd)) {
		lEvent |= FD_ACCEPT;
	}

	if (WSAAsyncSelect(fe->fd, ev->hWnd, ev->nMsg, lEvent) != 0) {
		net_msg_error("%s(%d): set read error: %s",
			__FUNCTION__, __LINE__, net_last_serror());
		return -1;
	}

	fe->mask |= EVENT_READ;
	wnet_msg_fdmap_set(ev, fe);
	return 0;
}

static int wnet_msg_add_write(EVENT_WMSG *ev, NET_FILE *fe)
{
	long lEvent =  FD_WRITE | FD_CLOSE;

	if (fe->mask & EVENT_READ) {
		lEvent |= FD_READ;
	}

	if (fe->status & STATUS_CONNECTING) {
		lEvent |= FD_CONNECT;
	}

	if (WSAAsyncSelect(fe->fd, ev->hWnd, ev->nMsg, lEvent) != 0) {
		net_msg_error("%s(%d): set read error: %s",
			__FUNCTION__, __LINE__, net_last_serror());
		return -1;
	}

	fe->mask |= EVENT_WRITE;
	wnet_msg_fdmap_set(ev, fe);
	return 0;
}

static int wnet_msg_del_read(EVENT_WMSG *ev, NET_FILE *fe)
{
	long lEvent;

	if (fe->mask & EVENT_WRITE) {
		lEvent = FD_CLOSE | FD_WRITE;
	} else {
		lEvent = FD_CLOSE;
	}

	fe->mask &= ~EVENT_READ;
	if (fe->mask == 0) {
		wnet_msg_fdmap_del(ev, fe);
	}

	if (WSAAsyncSelect(fe->fd, ev->hWnd, lEvent ? ev->nMsg : 0, lEvent)) {
		net_msg_error("%s(%d): set read error: %s",
			__FUNCTION__, __LINE__, net_last_serror());
		return -1;
	}

	return 0;
}

static int wnet_msg_del_write(EVENT_WMSG *ev, NET_FILE *fe)
{
	long lEvent;

	if (fe->mask & EVENT_READ) {
		lEvent = FD_CLOSE | FD_READ;
	} else {
		lEvent = FD_CLOSE;
	}

	fe->mask &= ~EVENT_WRITE;
	if (fe->mask == 0) {
		wnet_msg_fdmap_del(ev, fe);
	}

	if (WSAAsyncSelect(fe->fd, ev->hWnd, lEvent ? ev->nMsg : 0, lEvent)) {
		net_msg_error("%s(%d): set read error: %s",
			__FUNCTION__, __LINE__, net_last_serror());
		return -1;
	}
	return 0;
}

static int wnet_msg_checkfd(EVENT_WMSG *ev, NET_FILE *fe)
{
	(void) ev;
	return getsocktype(fe->fd) == -1 ? -1 : 0;
}

static int wnet_msg_wait(NET_EVENT *ev, int timeout)
{
	MSG msg;
	UINT_PTR id = SetTimer(NULL, 0, timeout, NULL);
	BOOL res = GetMessage(&msg, NULL, 0, 0);

	KillTimer(NULL, id);
	if (!res) {
		return 0;
	}
	TranslateMessage(&msg);
	DispatchMessage(&msg);
#if 1
	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		if (msg.message == WM_QUIT) {
			PostQuitMessage(0);
			break;
		}
	}
#endif
	return 0;
}

static void onRead(EVENT_WMSG *ev, SOCKET fd)
{
	NET_FILE *fe = wnet_msg_fdmap_get(ev, fd);
	if (fe == NULL) {
		net_msg_error("%s(%d): no NET_FILE, fd=%d",
			__FUNCTION__, __LINE__, fd);
	} else if (fe->r_proc == NULL) {
		net_msg_error("%s(%d): r_proc NULL, fd=%d",
			__FUNCTION__, __LINE__, fd);
	} else {
		//fe->mask &= ~EVENT_READ;
		fe->r_proc(&ev->event, fe);
	}
}

static void onWrite(EVENT_WMSG *ev, SOCKET fd)
{
	NET_FILE *fe = wnet_msg_fdmap_get(ev, fd);
	if (fe == NULL) {
		net_msg_error("%s(%d): no NET_FILE, fd=%d",
			__FUNCTION__, __LINE__, fd);
	} else if (fe->w_proc == NULL) {
		net_msg_error("%s(%d): w_proc NULL, fd=%d",
			__FUNCTION__, __LINE__, fd);
	} else {
		//fe->mask &= ~EVENT_WRITE;
		fe->w_proc(&ev->event, fe);
	}
}

static void onAccept(EVENT_WMSG *ev, SOCKET fd)
{
	NET_FILE *fe = wnet_msg_fdmap_get(ev, fd);
	if (fe == NULL) {
		net_msg_error("%s(%d): no NET_FILE, fd=%d",
			__FUNCTION__, __LINE__, fd);
	} else if (fe->r_proc == NULL) {
		net_msg_error("%s(%d): r_proc NULL, fd=%d",
			__FUNCTION__, __LINE__, fd);
	} else {
		//fe->mask &= ~EVENT_READ;
		fe->r_proc(&ev->event, fe);
	}
}

static void onConnect(EVENT_WMSG *ev, SOCKET fd)
{
	onWrite(ev, fd);
}

static void onClose(EVENT_WMSG *ev, SOCKET fd)
{
	NET_FILE *fe = wnet_msg_fdmap_get(ev, fd);
	if (fe == NULL) {
		/* don nothing */
	} else if (fe->mask & EVENT_READ) {
		if (fe->r_proc) {
			//fe->mask &= ~EVENT_READ;
			fe->r_proc(&ev->event, fe);
		}
	} else if (fe->mask & EVENT_WRITE) {
		if (fe->w_proc) {
			//fe->mask &= ~EVENT_WRITE;
			fe->w_proc(&ev->event, fe);
		}
	}
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	EVENT_WMSG *ev = get_hwnd_event(hWnd);

	if (ev == NULL) {
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}

	if (msg == WM_SOCKET_NOTIFY) {
		SOCKET fd = wParam;
		switch (WSAGETSELECTEVENT(lParam)) {
		case FD_ACCEPT:
			onAccept(ev, fd);
			break;
		case FD_CONNECT:
			onConnect(ev, fd);
			break;
		case FD_READ:
			onRead(ev, fd);
			break;
		case FD_WRITE:
			onWrite(ev, fd);
			break;
		case FD_CLOSE:
			onClose(ev, fd);
			break;
		default:
			break;
		}
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

static BOOL InitApplication(const char *class_name, HINSTANCE hInstance) 
{
	WNDCLASSEX wcx;

	if (GetClassInfoEx(hInstance, class_name, &wcx)) {
		/* class already registered */
		net_msg_info("%s(%d): class(%s) already registered",
			__FUNCTION__, __LINE__, class_name);
		return TRUE;
	}

	/* Fill in the window class structure with parameters
	 * that describe the main window.
	 */

	memset(&wcx, 0, sizeof(wcx));

	wcx.cbSize      = sizeof(wcx);       /* size of structure */
	wcx.style       = CS_HREDRAW | CS_VREDRAW;
	wcx.lpfnWndProc = WndProc;           /* points to window procedure */
	wcx.cbClsExtra  = 0;                 /* no extra class memory */
	wcx.cbWndExtra  = 0;                 /* no extra window memory */
	wcx.hInstance   = hInstance;         /* handle to instance */

	wcx.hIcon = LoadIcon(NULL, IDI_APPLICATION);     /* predefined app. icon */
	wcx.hCursor = LoadCursor(NULL, IDC_ARROW);       /* predefined arrow */
	wcx.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH); /* white background brush */
	wcx.lpszMenuName  =  NULL;          /* name of menu resource */
	wcx.lpszClassName = class_name;     /* name of window class */
	wcx.hIconSm = (HICON) LoadImage(hInstance,  /* small class icon */
		MAKEINTRESOURCE(5),
		IMAGE_ICON,
		GetSystemMetrics(SM_CXSMICON),
		GetSystemMetrics(SM_CYSMICON),
		LR_DEFAULTCOLOR);

	/* Register the window class. */
	if (RegisterClassEx(&wcx) == 0) {
		net_msg_error("%s(%d): RegisterClassEx error(%d, %s)", __FUNCTION__,
			__LINE__, net_last_error(), net_last_serror());
		return FALSE;
	} else {
		return TRUE;
	}
}

static HWND InitInstance(const char *class_name, HINSTANCE hInstance)
{
	HWND hWnd;
	CREATESTRUCT cs;

	cs.dwExStyle = 0;
	cs.lpszClass = class_name;
	cs.lpszName  = "Event Socket Notification Sink";
	cs.style = WS_OVERLAPPED;
	cs.x  = 0;
	cs.y  = 0;
	cs.cx = 0;
	cs.cy = 0;
	cs.hwndParent     = NULL;
	cs.hMenu          = NULL;
	cs.hInstance      = hInstance;
	cs.lpCreateParams = NULL;

	hWnd = CreateWindowEx(cs.dwExStyle, cs.lpszClass,
		cs.lpszName, cs.style, cs.x, cs.y, cs.cx, cs.cy,
		cs.hwndParent, cs.hMenu, cs.hInstance, cs.lpCreateParams);
	if (hWnd == NULL) {
		net_msg_error("%s(%d): create windows error: %s",
			__FUNCTION__, __LINE__, net_last_serror());
	}
	return hWnd;
}

static HWND CreateSockWindow(const char *class_name, HINSTANCE hInstance)
{
	if (InitApplication(class_name, hInstance) == FALSE) {
		return FALSE;
	}
	return InitInstance(class_name, hInstance);
}

static acl_handle_t wnet_msg_handle(NET_EVENT *ev)
{
	EVENT_WMSG *ew = (EVENT_WMSG *) ev;
	return (acl_handle_t) ew->hInstance;
}

static const char *wnet_msg_name(void)
{
	return "wmsg";
}

static const char *__class_name = "__EventEventsMainWClass";

NET_EVENT *event_wnet_msg_create(int size)
{
	EVENT_WMSG *ew = (EVENT_WMSG *) mem_calloc(1, sizeof(EVENT_WMSG));
	HINSTANCE hInstance = GetModuleHandle(NULL);
	HWND hWnd = CreateSockWindow(__class_name, hInstance);

	ew->files = (NET_FILE**) mem_calloc(size, sizeof(NET_FILE*));
	ew->size  = size;
	ew->count = 0;

	ew->nMsg         = WM_SOCKET_NOTIFY;
	ew->hWnd         = hWnd;
	ew->hInstance    = hInstance;
	ew->class_name   = __class_name;
	ew->tbl          = htable_create(10);

	ew->event.name   = wnet_msg_name;
	ew->event.handle = wnet_msg_handle;
	ew->event.free   = wnet_msg_free;
	ew->event.event_wait = wnet_msg_wait;
	ew->event.checkfd    = (net_event_oper *) wnet_msg_checkfd;
	ew->event.add_read   = (net_event_oper *) wnet_msg_add_read;
	ew->event.add_write  = (net_event_oper *) wnet_msg_add_write;
	ew->event.del_read   = (net_event_oper *) wnet_msg_del_read;
	ew->event.del_write  = (net_event_oper *) wnet_msg_del_write;

	set_hwnd_event(hWnd, ew);

	return (NET_EVENT*) ew;
}

#endif
