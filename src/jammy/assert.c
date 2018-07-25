#include "assert.h"

#include <stdio.h>

#include <Windows.h>

#if JM_ASSERT_ENABLED
WNDPROC g_oldProc;
HHOOK g_hHook;

static LRESULT CALLBACK HookWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT rc = CallWindowProc(g_oldProc, hWnd, uMsg, wParam, lParam);

	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			// change the Retry button text to "Debug"
			HWND retryButton = GetWindow(hWnd, GW_CHILD);
			retryButton = GetWindow(retryButton, GW_HWNDNEXT);
			SetWindowText(retryButton, "&Debug");
		}

		case WM_NCDESTROY:
		{
			UnhookWindowsHookEx(g_hHook);
		}
	}

	return rc;
}

static LRESULT CALLBACK SetHook(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode == HC_ACTION)
	{
		CWPSTRUCT* pwp = (CWPSTRUCT*)lParam;
		if (pwp->message == WM_INITDIALOG)
		{
			g_oldProc = (WNDPROC)SetWindowLongPtr(pwp->hwnd, GWLP_WNDPROC, (LONG_PTR)HookWndProc);
		}
	}
	return CallNextHookEx(g_hHook, nCode, wParam, lParam);
}

void jm_assert_impl(
	const char* expression,
	const char* file,
	unsigned line)
{
	char message[1024];
	sprintf(message, "Expression: %s\n\nFile: %s\nLine: %u", expression, file, line);
	fprintf(stderr, 
		"------------------------\n"
		"!!! ASSERTION FAILED !!!\n"
		"------------------------\n"
		"%s\n"
		"------------------------\n", message);

	g_hHook = SetWindowsHookEx(WH_CALLWNDPROC, SetHook, NULL, GetCurrentThreadId());
	int result = MessageBox(NULL, message, "Assertion failed", MB_ICONERROR | MB_ABORTRETRYIGNORE);
	switch (result)
	{
		case IDABORT:
			ExitProcess(1);
		case IDRETRY:
			DebugBreak();
		case IDIGNORE:
			break;
	}
}
#endif