/*
  Copyright (C) 2010-2012 Birunthan Mohanathas <http://poiru.net>

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "StdAfx.h"
#include "PluginInterface.h"

const int WM_QUERY_RAINMETER = WM_APP + 1000;
const int RAINMETER_QUERY_ID_SKINS_PATH = 4101;

HWND g_RainmeterWindow = nullptr;
HWND g_NppWindow = nullptr;
std::wstring g_SkinsPath;

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_COPYDATA)
	{
		COPYDATASTRUCT* cds = (COPYDATASTRUCT*)lParam;
		if (cds->dwData == RAINMETER_QUERY_ID_SKINS_PATH)
		{
			g_SkinsPath = (WCHAR*)cds->lpData;
		}

		return TRUE;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

bool GetRainmeter()
{
	if (!g_RainmeterWindow || !IsWindow(g_RainmeterWindow))
	{
		HWND trayWindow = FindWindow(L"RainmeterTrayClass", nullptr);
		HWND meterWindow = FindWindow(L"RainmeterMeterWindow", nullptr);
		if (trayWindow && meterWindow)
		{
			// Create window to recieve WM_COPYDATA from Rainmeter
			HWND wnd = CreateWindow(
				L"STATIC",
				L"RainLexer",
				WS_DISABLED,
				CW_USEDEFAULT, CW_USEDEFAULT,
				CW_USEDEFAULT, CW_USEDEFAULT,
				nullptr,
				nullptr,
				nullptr,
				nullptr);

			if (wnd)
			{
				SetWindowLongPtr(wnd, GWLP_WNDPROC, (LONG_PTR)WndProc);

				SendMessage(trayWindow, WM_QUERY_RAINMETER, RAINMETER_QUERY_ID_SKINS_PATH, (LPARAM)wnd);
				DestroyWindow(wnd);

				if (!g_SkinsPath.empty())
				{
					g_RainmeterWindow = meterWindow;
					return true;
				}
			}
		}
	}
	else
	{
		return true;
	}

	return false;
}

void RefreshSkin()
{
	if (!GetRainmeter()) return;

	WCHAR currentPath[MAX_PATH];
	BOOL ret = SendMessage(g_NppWindow, NPPM_GETFULLCURRENTPATH, MAX_PATH, (LPARAM)&currentPath);

	if (ret)
	{
		size_t skinsPathLen = wcslen(g_SkinsPath.c_str());
		size_t currentPathLen = wcslen(currentPath);

		// Make sure the file is in the skins folder and that extension is .ini
		if (wcsncmp(g_SkinsPath.c_str(), currentPath, skinsPathLen) == 0 &&
			_wcsicmp(&currentPath[currentPathLen - 4], L".ini") == 0)
		{
			WCHAR* relativePath = &currentPath[skinsPathLen];
			WCHAR* pos = wcsrchr(relativePath, L'\\');

			if (pos)
			{
				std::wstring bang = L"!Refresh \"";
				bang.append(relativePath, pos - relativePath);	// TODO FIXME
				bang += L'"';

				COPYDATASTRUCT cds;
				cds.dwData = 1;
				cds.cbData = (DWORD)(bang.length() + 1) * sizeof(WCHAR);
				cds.lpData = (void*)bang.c_str();
				SendMessage(g_RainmeterWindow, WM_COPYDATA, 0, (LPARAM)&cds);
			}
		}
	}
}

void RefreshAll()
{
	if (!GetRainmeter()) return;

	COPYDATASTRUCT cds;
	cds.dwData = 1;
	cds.cbData = sizeof(L"!Refresh *");
	cds.lpData = L"!Refresh *";
	SendMessage(g_RainmeterWindow, WM_COPYDATA, 0, (LPARAM)&cds);
}

void About()
{
	MessageBox(
		g_NppWindow,
		L"By Birunthan Mohanathas.\n"
		L"poiru.github.com/rainlexer",
		L"RainLexer 1.1.1",
		MB_OK);
}

// Notepad++ exports

BOOL isUnicode()
{
	return TRUE;
}

const WCHAR* getName()
{
	return L"&RainLexer";
}

FuncItem* getFuncsArray(int* count)
{
	static FuncItem funcItems[] =
	{
		{ L"Refresh skin", RefreshSkin, 0, false, nullptr },
		{ L"Refresh all", RefreshAll, 0, false, nullptr },
		{ L"&About...", About, 0, false, nullptr }
	};

	*count = _countof(funcItems);

	return funcItems;
}

void setInfo(NppData data)
{
	g_NppWindow = data._nppHandle;
}

void beNotified(SCNotification* scn)
{
}

LRESULT messageProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return TRUE;
}