#define _CRT_NON_CONFORMING_WCSTOK
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <TCHAR.h>
#include <string>

LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow)
{
	HWND     hwnd;
	MSG		 msg;
	WNDCLASS WndClass;

	WndClass.style = CS_HREDRAW | CS_VREDRAW;
	WndClass.lpfnWndProc = WndProc;
	WndClass.cbClsExtra = 0;
	WndClass.cbWndExtra = 0;
	WndClass.hInstance = hInstance;
	WndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	WndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	WndClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	WndClass.lpszMenuName = NULL;
	WndClass.lpszClassName = _T("WHS");
	RegisterClass(&WndClass);


	hwnd = CreateWindow(_T("WHS"),
		_T("[WHS]파일 포렌식 과제"),
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		NULL,
		NULL,
		hInstance,
		NULL
	);
	ShowWindow(hwnd, nCmdShow);
	UpdateWindow(hwnd);

	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return (int)msg.wParam;
}



LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	HDC          dc;
	PAINTSTRUCT  ps;

	static wchar_t   str[0x1000][150];
	static int       str_len;
	static int       str_row;
	static SIZE      str_size;

	static int    start_y;
	static int    end_y;
	static BOOL   isSelecting = FALSE;
	static int    line_start = -1;
	static int	  line_end = -1;

	switch (iMsg)
	{
	case WM_CREATE:
		CreateCaret(hwnd, NULL, 2, 15);
		ShowCaret(hwnd);
		str_len = 0;
		str_row = 0;
		break;


	case WM_LBUTTONDOWN://-----------------------------------------|(드래그 시작) 시작 좌표 초기화
		start_y = HIWORD(lParam);
		line_start = -1;
		isSelecting = TRUE;
		return 0;


	case WM_MOUSEMOVE://-------------------------------------------| (드래그 중) 종료 좌표 초기화
		if (isSelecting) {
			end_y = HIWORD(lParam);
			InvalidateRect(hwnd, NULL, TRUE);
		}
		return 0;


	case WM_LBUTTONUP://-------------------------------------------| (드래그 종료) 선택 범위 확정 및 다양한 상황 처리
		isSelecting = FALSE;

		if (abs(end_y - start_y) < 7) {
			line_start = -1;
			line_end   = -1;
			InvalidateRect(hwnd, NULL, TRUE);
			return 0;
		}

		line_start = (start_y / 20) - 1;
		line_end   = (end_y / 20)   - 1;

		if (line_start > line_end) {
			int exchage = line_start;
			line_start  = line_end;
			line_end    = exchage;
		}
		if (line_start < 0)     line_start = 0;
		if (line_end > str_row) line_end = str_row;


		InvalidateRect(hwnd, NULL, TRUE);
		return 0;


	case WM_CHAR:

		if (wParam == VK_BACK) { //---------------------------------| (back space) 글자 삭제 로직
			if (str_row == 0 && str_len <= 0) break;
			str_len--;

			if (str_len < 0) {
				str_row--;
				str_len = _tcslen(str[str_row]);
			}
			str[str_row][str_len] = NULL;
		}

		else if (wParam == VK_RETURN) { //---------------------------| (enter) 줄바꿈 로직
			if (str_row >= 0x1000) break;
			str[str_row][str_len] = NULL;
			str_len = 0;
			str_row++;
		}

		else {//-----------------------------------------------------| (타이핑) 글자 입력 및 자동 줄바꿈 로직 
			if (str_len >= 149) {
				str[str_row][str_len] = NULL;
				str_len = 0;
				str_row++;
				if (str_row >= 0x1000) break;
			}
			str[str_row][str_len++] = (TCHAR)wParam;
			str[str_row][str_len] = NULL;
		}
		InvalidateRect(hwnd, NULL, TRUE);
		return 0;


	case WM_PAINT:
		dc = BeginPaint(hwnd, &ps);

		for (int i = 0; i <= str_row; i++) {
			int len = _tcslen(str[i]);
			GetTextExtentPointW(dc, str[i], len, &str_size);

			int x = 10;
			int y = (i + 1) * 20;

			// 드레그 선택 영역 색상 변경 (배경 = 검정색,  글자 = 하얀색)
			if (line_start >= 0 && line_end >= 0 && i >= line_start && i <= line_end) {
				RECT bgRect = { x, y, x + str_size.cx, y + 20 };
				SetBkColor(dc, RGB(0, 0, 0));
				SetTextColor(dc, RGB(255, 255, 255));
				ExtTextOutW(dc, x, y, ETO_OPAQUE, &bgRect, str[i], len, NULL);
			}

			else {
				SetBkMode(dc, TRANSPARENT);
				SetTextColor(dc, RGB(0, 0, 0));
				TextOutW(dc, x, y, str[i], len);
			}

			if (i == str_row)
				SetCaretPos(x + str_size.cx, y);
		}

		EndPaint(hwnd, &ps);
		return 0;



	case WM_KEYDOWN:
		if (GetKeyState(VK_CONTROL) & 0x8000) {
			if (wParam == 'C') {
				SendMessage(hwnd, WM_COPY, 0, 0);
				return 0;
			}
			else if (wParam == 'V') {
				SendMessage(hwnd, WM_PASTE, 0, 0);
				return 0;
			}
		}
		break;


	case WM_COPY: {
		if (line_start >= 0 && line_end >= 0) {
			std::wstring sentence;
			for (int i = line_start; i <= line_end; ++i) {
				sentence += str[i];
				sentence += L"\r\n";
			}

			if (OpenClipboard(hwnd)) {
				EmptyClipboard();
				HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, (sentence.size() + 1) * sizeof(wchar_t));
				if (hMem) {
					memcpy(GlobalLock(hMem), sentence.c_str(), (sentence.size() + 1) * sizeof(wchar_t));
					GlobalUnlock(hMem);
					SetClipboardData(CF_UNICODETEXT, hMem);
				}
				CloseClipboard();
			}
		}
		return 0;
	}


	case WM_PASTE: {
		if (OpenClipboard(hwnd)) {
			HANDLE hData = GetClipboardData(CF_UNICODETEXT);
			if (hData) {
				wchar_t* clipText = (wchar_t*)GlobalLock(hData);
				if (clipText) {
					wchar_t* context = NULL;
					wchar_t* line = wcstok(clipText, L"\r\n", &context);
					while (line && str_row < 0x1000) {
						wcsncpy_s(str[str_row], line, _TRUNCATE);
						str_row++;
						line = wcstok(NULL, L"\r\n", &context);
					}
					str_len = _tcslen(str[str_row - 1]);
					GlobalUnlock(hData);
				}
			}
			CloseClipboard();
			InvalidateRect(hwnd, NULL, TRUE);
		}
		return 0;
	}


	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	}
	return DefWindowProc(hwnd, iMsg, wParam, lParam);
}
