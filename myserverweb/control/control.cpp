/*
*myServer
*Copyright (C) Rocky_10_Balboa
*This library is free software; you can redistribute it and/or
*modify it under the terms of the GNU Library General Public
*License as published by the Free Software Foundation; either
*version 2 of the License, or (at your option) any later version.

*This library is distributed in the hope that it will be useful,
*but WITHOUT ANY WARRANTY; without even the implied warranty of
*MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*Library General Public License for more details.

*You should have received a copy of the GNU Library General Public
*License along with this library; if not, write to the
*Free Software Foundation, Inc., 59 Temple Place - Suite 330,
*Boston, MA  02111-1307, USA.
*/

/*
*This is the source code of a GUI application that need for the controls of myServer execution
*/
#include "stdafx.h"
#include "control.h"
#define MAX_LOADSTRING 100
#define Redraw() RedrawWindow(hWnd,NULL,NULL,RDW_INTERNALPAINT)
#define InvalidateRC(x)    InvalidateRect(hWnd, x, FALSE)
#define InvalidateRC2(x)    InvalidateRect(hWnd, x, TRUE)
#define Invalidate()    InvalidateRect(hWnd, NULL, FALSE)
#define Invalidate2()    InvalidateRect(hWnd, NULL, TRUE)
HINSTANCE hInst;
int nCurrentButton;
TCHAR szTitle[MAX_LOADSTRING];
TCHAR szWindowClass[MAX_LOADSTRING];
SERVICE_STATUS          MyServiceStatus; 
SERVICE_STATUS_HANDLE   MyServiceStatusHandle; 
SERVICE_STATUS queryStatus();
void paint(HWND hWnd);
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
HBITMAP toolbar;
HBITMAP buttons[7];
VOID install_service();
VOID remove_service();
VOID register_GuestUser();
HWND hWnd;
void start();
void pause();
void stop();
int WINAPI WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{

	if(!lstrcmpi(lpCmdLine,"UNREGISTER"))
	{
		remove_service();
		return 0;
	}
	if(!lstrcmpi(lpCmdLine,"REGISTER"))
	{
		install_service();
		return 0;
	}

	if(!lstrcmpi(lpCmdLine,"START"))
	{
		start();
		return 0;
	}
	if(!lstrcmpi(lpCmdLine,"PAUSE"))
	{
		pause();
		return 0;
	}
	if(!lstrcmpi(lpCmdLine,"STOP"))
	{
		stop();
		return 0;
	}
	if(!lstrcmpi(lpCmdLine,"REGISTERGUESTUSER"))
	{
		register_GuestUser();
		return 0;
	}

	MSG msg;
	HACCEL hAccelTable;
	nCurrentButton=-1;
	toolbar=LoadBitmap(hInstance,MAKEINTRESOURCE(IDB_TOOLBAR));
	buttons[0]=LoadBitmap(hInstance,MAKEINTRESOURCE(IDB_uSTART));
	buttons[1]=LoadBitmap(hInstance,MAKEINTRESOURCE(IDB_uPAUSE));
	buttons[2]=LoadBitmap(hInstance,MAKEINTRESOURCE(IDB_uSTOP));
	buttons[3]=LoadBitmap(hInstance,MAKEINTRESOURCE(IDB_cSTART));
	buttons[4]=LoadBitmap(hInstance,MAKEINTRESOURCE(IDB_cPAUSE));
	buttons[5]=LoadBitmap(hInstance,MAKEINTRESOURCE(IDB_cSTOP));
	buttons[6]=LoadBitmap(hInstance,MAKEINTRESOURCE(IDB_LOGO));
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_CONTROL, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);
	if (!InitInstance (hInstance, nCmdShow)) 
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)IDC_CONTROL);
	while (GetMessage(&msg, NULL, 0, 0)) 
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) 
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return (int) msg.wParam;
}



ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;
	HBRUSH b=CreateSolidBrush(0);
	wcex.cbSize = sizeof(WNDCLASSEX); 
	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, (LPCTSTR)IDI_CONTROL);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= b;
	wcex.lpszMenuName	= (LPCTSTR)IDC_CONTROL;
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);
	DeleteObject(b);
	return RegisterClassEx(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance;

	hWnd = CreateWindow(szWindowClass, szTitle, WS_POPUP|WS_VISIBLE|WS_BORDER,
		CW_USEDEFAULT, 0,300,120, NULL, NULL, hInstance, NULL);

	if (!hWnd)
	{
		return FALSE;
	}

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return TRUE;
}
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	BOOL invalidate=FALSE;
	static int wmId, wmEvent;
	static int nb;
	static POINT p;
	switch (message) 
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam); 
		wmEvent = HIWORD(wParam); 
		switch (wmId)
		{
		case ID_FILE_START:
			start();
			break;
		case ID_FILE_PAUSE:
			pause();
			break;
		case ID_FILE_STOP:
			stop();
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_MOUSEMOVE:
		GetCursorPos(&p);
		nb=-1;
		ScreenToClient(hWnd, &p);
		RECT brc;
		for(int i=0;i<3;i++)
		{
			brc.bottom = 36;
			brc.left = 7+i*92;
			brc.right = 7+(i+1)*92;
			brc.top = 4;
			if(brc.left   < p.x)
			if(brc.right  > p.x)
			if(brc.top    < p.y)
			if(brc.bottom > p.y)
			{
				invalidate=TRUE;
				nb=i;
				break;
			}
		}
		if(nb!=nCurrentButton)
		{
			if(invalidate)
				InvalidateRC(&brc);
			if(nCurrentButton!= -1)
			{
				brc.left = 7+nCurrentButton*92;
				brc.right = 7+(nCurrentButton+1)*92;
				InvalidateRC(&brc);
			}
			nCurrentButton=nb;
			paint(hWnd);
		}
		break;
	case WM_LBUTTONDOWN:
		Invalidate2();
		if(nCurrentButton==0)
			start();
		if(nCurrentButton==1)
			pause();
		if(nCurrentButton==2)
			stop();
		break;
	case WM_PAINT:
		Invalidate();
		paint(hWnd);
		break;
	case WM_DESTROY:
		DeleteObject(toolbar);
		for(int i=0;i<7;i++)
			DeleteObject(buttons[i]);
		PostQuitMessage(0);
		TerminateProcess(GetCurrentProcess(),0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

void start()
{
	SC_HANDLE service,manager;

	manager = OpenSCManager(NULL,NULL,SC_MANAGER_ALL_ACCESS);
	if (manager)
	{
		service = OpenService (manager, "myServer", SERVICE_ALL_ACCESS);
		if (service)
		{
			StartService(service,0,NULL);
			Invalidate2();
			paint(hWnd);
			while (QueryServiceStatus (service, &MyServiceStatus))
			if (MyServiceStatus.dwCurrentState != SERVICE_START_PENDING)
				break;
			Invalidate2();
			paint(hWnd);
			CloseServiceHandle (service);
			CloseServiceHandle (manager);
		}
	}

}
void pause()
{
	SC_HANDLE service,manager;

	manager = OpenSCManager(NULL,NULL,SC_MANAGER_ALL_ACCESS);
	if (manager)
	{
		service = OpenService (manager, "myServer", SERVICE_ALL_ACCESS);
		if (service)
		{
			ControlService (service, 
SERVICE_CONTROL_PAUSE,&MyServiceStatus);
			Invalidate2();
			paint(hWnd);
			while (QueryServiceStatus (service, &MyServiceStatus))
			if (MyServiceStatus.dwCurrentState != SERVICE_PAUSE_PENDING)
				break;
			Invalidate2();
			paint(hWnd);
			CloseServiceHandle (service);
			CloseServiceHandle (manager);
		}
	}


}
void stop()
{
	SC_HANDLE service,manager;

	manager = OpenSCManager(NULL,NULL,SC_MANAGER_ALL_ACCESS);
	if (manager)
	{
		service = OpenService (manager, "myServer", SERVICE_ALL_ACCESS);
		if (service)
		{
			ControlService (service, 
SERVICE_CONTROL_STOP,&MyServiceStatus);
			Invalidate2();
			paint(hWnd);
			while (QueryServiceStatus (service, &MyServiceStatus))
			if (MyServiceStatus.dwCurrentState != SERVICE_STOP_PENDING)
				break;
			Invalidate2();
			paint(hWnd);
			CloseServiceHandle (service);
			CloseServiceHandle (manager);
		}
	}



}
SERVICE_STATUS queryStatus()
{
	SC_HANDLE service,manager;
	manager = OpenSCManager(NULL,NULL,SC_MANAGER_ALL_ACCESS);
	if (manager)
	{
		service = OpenService (manager, "myServer", SERVICE_ALL_ACCESS);
		if (service)
		{
			QueryServiceStatus (service, &MyServiceStatus);
			CloseServiceHandle (service);
			CloseServiceHandle (manager);
		}
	}
	return MyServiceStatus;
}


VOID install_service()
{
	SC_HANDLE service,manager;
	char path [MAX_PATH];
	GetCurrentDirectory(MAX_PATH,path);
	lstrcat(path,"\\");
	lstrcat(path,"myServer.exe SERVICE");
	
	manager = OpenSCManager(NULL,NULL,SC_MANAGER_ALL_ACCESS);
	if (manager)
	{
		service = 
CreateService(manager,"myServer","myServer",SERVICE_ALL_ACCESS, 
SERVICE_WIN32_OWN_PROCESS,SERVICE_DEMAND_START, SERVICE_ERROR_IGNORE, path,0, 0, 
0, 0, 0);

		if (service)
		{
			CloseServiceHandle (service);
		}

		CloseServiceHandle (manager);
	}
}
VOID remove_service()
{
	SC_HANDLE service,manager;

    manager = OpenSCManager(NULL,NULL,SC_MANAGER_ALL_ACCESS);
	if (manager)
	{
		service = OpenService (manager, "myServer", SERVICE_ALL_ACCESS);
		if (service)
		{
			ControlService (service, 
SERVICE_CONTROL_STOP,&MyServiceStatus);
			while (QueryServiceStatus (service, &MyServiceStatus))
			if (MyServiceStatus.dwCurrentState != SERVICE_STOP_PENDING)
				break;

			DeleteService(service);
			CloseServiceHandle (service);
			CloseServiceHandle (manager);
		}
	}
}
void paint(HWND hWnd)
{
	RECT rc;
	HDC hdc,hdc2;
	PAINTSTRUCT ps;
	char str[260];
	GetClientRect(hWnd,&rc);
	hdc = BeginPaint(hWnd, &ps);
	hdc2=CreateCompatibleDC(hdc);
	SelectObject(hdc2,toolbar);
	BitBlt(hdc,0,0,320,200,hdc2,0,0,SRCCOPY);
	for(int i=0;i<3;i++)
	{
		int nButton=i;
		if(i==nCurrentButton)
			nButton+=3;
		SelectObject(hdc2,buttons[nButton]);
		BitBlt(hdc,7+i*92,4,7+(i+1)*92,36,hdc2,0,0,SRCCOPY);
	}

	switch(queryStatus().dwCurrentState)
	{
		case SERVICE_STOP_PENDING:
			lstrcpy(str,"Service is stopping");
			break;
		case SERVICE_START_PENDING:
			lstrcpy(str,"Service is starting");
			break;
		case SERVICE_STOPPED:
			lstrcpy(str,"Service is stopped");
			break;
		case SERVICE_RUNNING:
			lstrcpy(str,"Service is running");
			break;
		case SERVICE_PAUSED:
			lstrcpy(str,"Service is paused");
			break;
		default:
			lstrcpy(str,"Cannot query service");
			break;
	}
	SelectObject(hdc2,buttons[6]);
	BitBlt(hdc,rc.left,38,rc.right,70,hdc2,0,0,SRCCOPY);
	SetTextColor(hdc,0);
	SetBkMode(hdc,OPAQUE);
	SetBkColor(hdc,0xFFFFFF);
	rc.top=rc.bottom-20;
	DrawText(hdc,str,lstrlen(str),&rc,DT_CENTER|DT_TOP);
	DeleteDC(hdc2);
	EndPaint(hWnd, &ps);
	GetWindowRect(hWnd,&rc);
	ValidateRect(hWnd,&rc);
}
VOID register_GuestUser()
{

} 

