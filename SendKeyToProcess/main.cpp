#include <windows.h>
#include <psapi.h>
#include <commctrl.h>
#include <windowsx.h>

#include <stdlib.h>
#include <string.h>
#include <tchar.h>

#include <iostream>
#include <string>
#include <vector>

#include <memory>
#include <thread>
#include <mutex>

#include "ProcManipulation.h"
#include "threadManager.h"

////	Program setup
void initialize();
int createWindow(WNDPROC proc, HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);

////	Events
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
void wndHandlerOfThreads(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);	//processes interesting stuff and creates separate threads for some tasks
void onListViewNotify(LPARAM lParam);	//listView events, because there's lots of them

void checkBoxClick(HWND ckhBox);

////	Useful "abstractions"
HWND createPidListView(HWND hwndParent, HINSTANCE hInst);
HWND createHwndListView(HWND hwndParent, HINSTANCE hInst);

////	gui """""API"""""
void writeWindowText(HDC descr, const std::string& text, int x, int y);

void clearListView(HWND lView);

void appendText(HWND textHwnd, const std::string& newText);

BOOL insertPidListViewItems(HWND hWndListView, std::vector<ProcessData>& cItems);
BOOL insertHwndListViewItems(HWND hWndListView, std::vector<WindowElData>& cItems);

std::string getTextFieldContent(HWND textfield);
HWND gatherHwndTextFieldData(HWND textfield);
int gatherIntTextFieldData(HWND textfield);
bool isBoxChecked(HWND checkbox);
bool isRadioChecked(HWND radioButton);

////	global data

//Window handlers for all window elemetns
static HWND hWnd;
static HWND hwndButton1;
static HWND hwndButton2;
static HWND hwndButton3;
static HWND hwndButton4;
static HWND hWndEdit1;
static HWND hWndEdit2;
static HWND hWndListView1;
static HWND hWndListView2;

static HWND hWndRadioGroup1;
static HWND hWndRadioBtn1;
static HWND hWndRadioBtn2;

static HWND hWndEditReps;
static HWND hWndEditTIntervals;

static HWND chwndCheckBox1;
static HWND hwndEditMX;
static HWND hwndEditMY;
static HWND hwndEditKeys;

static HWND hwndButtonStop;

static HWND hwndButtonMouseMove;
static HWND hwndButtonMouseLC;
static HWND hwndButtonMouseLDC;

//Windows elemt ids (HMENU), for recognition on events
static int button1Id;
static int button2Id;
static int button3Id;
static int button4Id;
static int listView1Id;
static int listView2Id;

static int radioB1Id;
static int radioB2Id;

static int checkBox1Id;

static int buttonStopId;

static int buttonMouseMoveId;
static int buttonMouseLCId;
static int buttonMouseLDCId;

// Entry point, as required by win32 api
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	initialize();

	createWindow(WndProc, hInstance, hPrevInstance, lpCmdLine, nCmdShow);

	//message loop to listen for the messages that the operating system sends. 
	//When the application receives a message, this loop dispatches it to the WndProc function to be handled. 
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		threadManager::checkCompletedIds();
	}

	threadManager::detachThreads();

	return (int)msg.wParam;
}



////////////////////////////////////////////



void initialize()
{
	//ids for buttons, and othe window elements, to recognize them on command event
	button1Id = 1;
	button2Id = 2;
	button3Id = 3;
	button4Id = 4;

	listView1Id = 5;
	listView2Id = 6;

	radioB2Id = 11;
	radioB1Id = 10;
	checkBox1Id = 12;

	buttonStopId = 20;
	buttonMouseMoveId = 21;
	buttonMouseLCId = 22;
	buttonMouseLDCId = 23;
}

int createWindow(WNDPROC proc, HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	TCHAR szWindowClass[] = _T("win32app");
	TCHAR szTitle[] = _T("Input bot");

	//1. create a window class structure of type WNDCLASSEX
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = proc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APPLICATION));
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_APPLICATION));

	//2. Now that you have created a window class, you must register it. Use the RegisterClassEx function and pass the window class structure as an argument.
	if (!RegisterClassEx(&wcex))
	{
		MessageBox(NULL, _T("Call to RegisterClassEx failed!"), _T("Win32 Guided Tour"), NULL);

		return EXIT_FAILURE;
	}

	//3. Now you can create a window. Use the CreateWindow function.
	hWnd = CreateWindow(
		szWindowClass,				   // szWindowClass: the name of the application
		szTitle,					   // szTitle: the text that appears in the title bar
		WS_OVERLAPPEDWINDOW,		   // WS_OVERLAPPEDWINDOW: the type of window to create
		CW_USEDEFAULT, CW_USEDEFAULT,  // CW_USEDEFAULT, CW_USEDEFAULT: initial position (x, y)
		1000, 800,					   // 500, 100: initial size (width, length)
		NULL,						   // NULL: the parent of this window
		NULL,						   // NULL: this application does not have a menu bar
		hInstance,					   // hInstance: the first parameter from WinMain
		NULL						   // NULL: not used in this application
		);
	if (!hWnd)
	{
		MessageBox(NULL, _T("Call to CreateWindow failed!"), _T("Win32 Guided Tour"), NULL);

		return EXIT_FAILURE;
	}

	//3.1 Create  buttons (send to pid, refresh pids, refresh hwnds, send to hwnd)
	hwndButton1 = CreateWindow(
		_T("BUTTON"),  // Predefined class; Unicode assumed 
		_T("Send input event to pid (all hwnds)"),      // Button text 
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Styles 
		20,         // x position 
		380,         // y position 
		260,        // Button width
		20,        // Button height
		hWnd,     // Parent window
		(HMENU)button1Id,//will be the wparam for the click event
		(HINSTANCE)SetWindowLongPtr(hWnd, GWLP_HINSTANCE, DWLP_DLGPROC),
		NULL);      // Pointer not needed.
	hwndButton2 = CreateWindow(_T("BUTTON"), _T("Refresh"), WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
		300, 20, 100, 20, hWnd, (HMENU)button2Id, (HINSTANCE)SetWindowLongPtr(hWnd, GWLP_HINSTANCE, DWLP_DLGPROC), NULL);
	hwndButton3 = CreateWindow(_T("BUTTON"), _T("Refresh"), WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
		800, 20, 100, 20, hWnd, (HMENU)button3Id, (HINSTANCE)SetWindowLongPtr(hWnd, GWLP_HINSTANCE, DWLP_DLGPROC), NULL);
	hwndButton4 = CreateWindow(_T("BUTTON"), _T("Send input event to hwnd"), WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
		460, 380, 200, 20, hWnd, (HMENU)button4Id, (HINSTANCE)SetWindowLongPtr(hWnd, GWLP_HINSTANCE, DWLP_DLGPROC), NULL);
	hwndButtonStop = CreateWindow(_T("BUTTON"), _T("Stop All events"), WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
		590, 530, 200, 20, hWnd, (HMENU)buttonStopId, (HINSTANCE)SetWindowLongPtr(hWnd, GWLP_HINSTANCE, DWLP_DLGPROC), NULL);
	hwndButtonMouseMove = CreateWindow(_T("BUTTON"), _T("Mouse move"), WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
		20, 560, 100, 20, hWnd, (HMENU)buttonMouseMoveId, (HINSTANCE)SetWindowLongPtr(hWnd, GWLP_HINSTANCE, DWLP_DLGPROC), NULL);
	hwndButtonMouseLC = CreateWindow(_T("BUTTON"), _T("Mouse leftClick"), WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
		400, 560, 150, 20, hWnd, (HMENU)buttonMouseLCId, (HINSTANCE)SetWindowLongPtr(hWnd, GWLP_HINSTANCE, DWLP_DLGPROC), NULL);
	hwndButtonMouseLDC = CreateWindow(_T("BUTTON"), _T("Mouse L DoubleClick"), WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
		600, 560, 150, 20, hWnd, (HMENU)buttonMouseLDCId, (HINSTANCE)SetWindowLongPtr(hWnd, GWLP_HINSTANCE, DWLP_DLGPROC), NULL);

	//3.2 Editable textfields (pid, hwnd)
	hWndEdit1 = CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("Edit"), TEXT("0"),	//pid
		WS_CHILD | WS_VISIBLE, 280, 380, 140, 20, hWnd, NULL, NULL, NULL);
	hWndEdit2 = CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("Edit"), TEXT("0"),	//hwnd
		WS_CHILD | WS_VISIBLE, 660, 380, 140, 20, hWnd, NULL, NULL, NULL);
	hWndEditReps = CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("Edit"), TEXT("1"),	//repetitions
		WS_CHILD | WS_VISIBLE, 130, 530, 140, 20, hWnd, NULL, NULL, NULL);
	hWndEditTIntervals = CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("Edit"), TEXT("1500"),	//time intervals
		WS_CHILD | WS_VISIBLE, 440, 530, 140, 20, hWnd, NULL, NULL, NULL);

	hwndEditMX = CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("Edit"), TEXT("200"),		//mouse x
		WS_CHILD | WS_VISIBLE, 150, 560, 90, 20, hWnd, NULL, NULL, NULL);
	hwndEditMY = CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("Edit"), TEXT("100"),		//mouse y
		WS_CHILD | WS_VISIBLE, 270, 560, 90, 20, hWnd, NULL, NULL, NULL);
	hwndEditKeys = CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("Edit"), TEXT(""),	//list of key events
		WS_CHILD | WS_VISIBLE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_MULTILINE, 120, 590, 850, 60, hWnd, NULL, NULL, NULL);

	//3.3 Listview of pids
	std::vector<ProcessData> procEntries = printProcesses();
	hWndListView1 = createPidListView(hWnd, (HINSTANCE)SetWindowLongPtr(hWnd, GWLP_HINSTANCE, DWLP_DLGPROC));
	hWndListView2 = createHwndListView(hWnd, (HINSTANCE)SetWindowLongPtr(hWnd, GWLP_HINSTANCE, DWLP_DLGPROC));
	insertPidListViewItems(hWndListView1, procEntries);

	//3.4 Radio buttons
	hWndRadioGroup1 = CreateWindowEx(WS_EX_WINDOWEDGE,
		"BUTTON", "Select event sending mode:",
		WS_VISIBLE | WS_CHILD | BS_GROUPBOX,  // Styles 
		20, 420,
		380, 100,
		hWnd, NULL,
		hInstance, NULL);
	hWndRadioBtn1 = CreateWindowEx(WS_EX_WINDOWEDGE,
		"BUTTON", "Activation (brings window to the foreground)",
		WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON,  // Styles 
		20, 20,
		350, 20,
		hWndRadioGroup1, (HMENU)radioB1Id,
		hInstance, NULL);
	hWndRadioBtn2 = CreateWindowEx(WS_EX_WINDOWEDGE,
		"BUTTON", "Background mode",
		WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON,  // Styles 
		20, 45,
		350, 20,
		hWndRadioGroup1, (HMENU)radioB2Id,
		hInstance, NULL);

	//3.5 Check boxes
	chwndCheckBox1 = CreateWindowEx(WS_EX_WINDOWEDGE, "BUTTON", "With time intervals", BS_CHECKBOX | WS_CHILD | WS_VISIBLE,
		280, 530, 150, 20, hWnd, (HMENU)checkBox1Id, hInstance, NULL);
	Button_SetCheck(chwndCheckBox1, 1);

	Button_SetCheck(hWndRadioBtn2, 1);

	//4. Now, use the following code to display the window.	
	ShowWindow(hWnd,   // hWnd: the value returned from CreateWindow
		nCmdShow);	   // nCmdShow: the fourth parameter from WinMain
	UpdateWindow(hWnd);
}



/////////////////////////////////////////////



//windows callback, as required by win32api
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_PAINT:
	case WM_DESTROY:
	case WM_NOTIFY:
	case WM_COMMAND:
		wndHandlerOfThreads(hWnd, message, wParam, lParam);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
		break;
	}

	return 0;
}

//Interesting events will be processed here. threads created, if applicable
void wndHandlerOfThreads(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_PAINT:
		PAINTSTRUCT ps;
		HDC hdc;
		hdc = BeginPaint(hWnd, &ps);

		writeWindowText(hdc, "List of the processes currently running", 20, 20);
		writeWindowText(hdc, "List of window handlers of the pid currently selected", 460, 20);
		writeWindowText(hdc, "Event repetitions", 20, 530);

		writeWindowText(hdc, "mX", 125, 560);
		writeWindowText(hdc, "mY", 245, 560);
		writeWindowText(hdc, "Key events", 20, 590);

		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case  WM_NOTIFY:
		onListViewNotify(lParam);
		break;
	case WM_COMMAND:
		if (LOWORD(wParam) == button1Id)	//button 1 click event, send to pid
		{
			//gather data from the gui
			DWORD tergetPid = gatherIntTextFieldData(hWndEdit1);

			std::string commandsStr = getTextFieldContent(hwndEditKeys);

			int numCalls = gatherIntTextFieldData(hWndEditReps);

			int mX = gatherIntTextFieldData(hwndEditMX);
			int mY = gatherIntTextFieldData(hwndEditMY);

			bool activMode = isRadioChecked(hWndRadioBtn1) && !isRadioChecked(hWndRadioBtn2);

			bool isMouse = isBoxChecked(chwndCheckBox1);

			int interval = gatherIntTextFieldData(hWndEditTIntervals);

			//execute in a thread
			threadManager::addThread(std::shared_ptr<std::thread>(new std::thread{ std::bind(sendKeysToProcess, tergetPid, commandsStr, numCalls, activMode, interval) }));
		}
		else if (LOWORD(wParam) == button2Id)	//button 2 click event, refresh pids
		{
			std::vector<ProcessData> procEntries = printProcesses();

			clearListView(hWndListView1);
			insertPidListViewItems(hWndListView1, procEntries);
		}
		else if (LOWORD(wParam) == button3Id)	//button 3 click event, refresh pids
		{
			DWORD tergetPid = gatherIntTextFieldData(hWndEdit1);

			std::vector<WindowElData> hwnds = getPidHwnds(tergetPid);

			clearListView(hWndListView2);
			insertHwndListViewItems(hWndListView2, hwnds);
		}
		else if (LOWORD(wParam) == button4Id)	//button 4 click event, send to hwnd
		{
			//Gather data from the gui
			HWND tergetHwnd = gatherHwndTextFieldData(hWndEdit2);

			int nCalls = gatherIntTextFieldData(hWndEditReps);

			int mX = gatherIntTextFieldData(hwndEditMX);
			int mY = gatherIntTextFieldData(hwndEditMY);

			bool isMouse = isBoxChecked(chwndCheckBox1);

			std::string commandsStr = getTextFieldContent(hwndEditKeys);

			bool isActivation = isRadioChecked(hWndRadioBtn1) && !isRadioChecked(hWndRadioBtn2);

			int interval = gatherIntTextFieldData(hWndEditTIntervals);

			//execute in a thread
			threadManager::addThread(std::shared_ptr<std::thread>(new std::thread{ std::bind(sendKeysToHwnd, tergetHwnd, commandsStr, nCalls, isActivation, interval) }));
		}
		else if (LOWORD(wParam) == buttonMouseMoveId)	//mouse move btn
		{
			std::string mX = getTextFieldContent(hwndEditMX);
			std::string mY = getTextFieldContent(hwndEditMY);

			appendText(hwndEditKeys, "{MMV," + mX + "," + mY + "}");
		}
		else if (LOWORD(wParam) == buttonMouseLCId)		//Mouse leftClick
		{
			std::string mX = getTextFieldContent(hwndEditMX);
			std::string mY = getTextFieldContent(hwndEditMY);

			appendText(hwndEditKeys, "{MLC," + mX + "," + mY + "}");
		}
		else if (LOWORD(wParam) == buttonMouseLDCId)	//Mouse Left double click
		{
			std::string mX = getTextFieldContent(hwndEditMX);
			std::string mY = getTextFieldContent(hwndEditMY);

			appendText(hwndEditKeys, "{MLDC," + mX + "," + mY + "}");
		}
		else if (LOWORD(wParam) == checkBox1Id)	//checkbox for mouse inout clicked
		{
			checkBoxClick(chwndCheckBox1);
		}
		else if (LOWORD(wParam) == (buttonStopId))	//stopping all processes
		{
			threadManager::sendStopToAllThreads(stopThread);
			threadManager::joinThreads();
		}
		break;
	}
}

//Yeah, you actually have to set that in your code YOURSELF for checkbox, it doesn't
//get checkd on click by itself
void checkBoxClick(HWND ckhBox)
{
	BOOL checked = Button_GetCheck(ckhBox);
	if (checked)
		Button_SetCheck(ckhBox, 0);
	else
		Button_SetCheck(ckhBox, 1);
}

//  - Handles the LVN_GETDISPINFO notification code that is 
//         sent  to the list view parent window. The function 
//        provides display strings for list view items and subitems.
// lParam - The LPARAM parameter passed with the WM_NOTIFY message.
void onListViewNotify(LPARAM lParam)
{
	int itemnum;
	int colnum;

	NMLVDISPINFO* plvdi;

	LPNMLISTVIEW lvData;
	LPNMHDR eHead = (LPNMHDR)lParam;
	switch (eHead->code)
	{
	case LVN_GETDISPINFO:
		plvdi = (NMLVDISPINFO*)lParam;
		plvdi->item.pszText = (LPSTR)std::to_string(plvdi->item.iItem).c_str();
		break;
	case NM_DBLCLK: //add item
		/*sprintf(af, "Item %d", index);
		list.additem(index, 0, af);
		list.additem(index++, 1, "Subitem 1");*/
		//++ puts it at the index location and then increments
		break;
	case NM_RCLICK: //todo
		if (eHead->idFrom == listView1Id)	//list view of the process data for all processes
		{
			lvData = (LPNMLISTVIEW)lParam;
			itemnum = lvData->iItem;
		}
		break;
	case LVN_COLUMNCLICK: //user clicked a column - get the column number
		if (eHead->idFrom == listView1Id)	//list view of the process data for all processes
		{
			lvData = (LPNMLISTVIEW)lParam;
			colnum = lvData->iSubItem;
		}
		break;
	case LVN_ITEMCHANGED: //user clicked a different item
		if (eHead->idFrom == listView1Id)	//list view of the process data for all processes
		{
			lvData = (LPNMLISTVIEW)lParam;

			//Get data from the second column(pid), adn send it to the input textField
			TCHAR szWindowClass[1024];
			LV_ITEM colData;
			colData.pszText = szWindowClass;	//LPSTR_TEXTCALLBACK; // Sends an LVN_GETDISPINFO message.
			colData.cchTextMax = 1024;
			colData.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
			colData.iSubItem = 1;
			colData.iItem = lvData->iItem;
			if (!ListView_GetItem(hWndListView1, &colData))
				MessageBox(hWnd, (LPCSTR)"Unable to get the column", (LPCSTR)"Unable to get the column", MB_ICONWARNING | MB_CANCELTRYCONTINUE | MB_DEFBUTTON2);
			SetWindowText(hWndEdit1, colData.pszText);
			std::vector<WindowElData> hwnds = getPidHwnds(atoi(colData.pszText));
			ListView_DeleteAllItems(hWndListView2);
			insertHwndListViewItems(hWndListView2, hwnds);
		}
		else if (eHead->idFrom == listView2Id)
		{
			lvData = (LPNMLISTVIEW)lParam;

			//Get data from the second column(pid), adn send it to the input textField
			TCHAR szWindowClass[1024];
			LV_ITEM colData;
			colData.pszText = szWindowClass;	//LPSTR_TEXTCALLBACK; // Sends an LVN_GETDISPINFO message.
			colData.cchTextMax = 1024;
			colData.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
			colData.iSubItem = 0;
			colData.iItem = lvData->iItem;
			if (!ListView_GetItem(hWndListView2, &colData))
				MessageBox(hWnd, (LPCSTR)"Unable to get the column", (LPCSTR)"Unable to get the column", MB_ICONWARNING | MB_CANCELTRYCONTINUE | MB_DEFBUTTON2);
			SetWindowText(hWndEdit2, colData.pszText);
		}
		break;
	}

	return;
}



/////////////////////////////////



void writeWindowText(HDC descr, const std::string& text, int x, int y)
{
	TextOut(descr, x, y, (LPCSTR)text.c_str(), text.size());
}

std::string getTextFieldContent(HWND textfield)
{
	char inputContents[512];
	GetWindowText(textfield, inputContents, sizeof(inputContents));

	std::string retVal(inputContents);

	return retVal;
}

HWND gatherHwndTextFieldData(HWND textfield)
{
	std::string data = getTextFieldContent(textfield);
	HWND hwnd = (HWND)atoi(data.c_str());
	return hwnd;
}

int gatherIntTextFieldData(HWND textfield)
{
	std::string data = getTextFieldContent(textfield);
	int retval = atoi(data.c_str());
	return retval;
}

bool isBoxChecked(HWND checkbox)
{
	return Button_GetCheck(checkbox);
}

bool isRadioChecked(HWND radioButton)
{
	return Button_GetCheck(radioButton);
}

void clearListView(HWND lView)
{
	ListView_DeleteAllItems(lView);
}

void appendText(HWND textHwnd, const std::string& newText)
{
	// get the current selection
	DWORD startPos, endPos;
	SendMessage(textHwnd, EM_GETSEL, reinterpret_cast<WPARAM>(&startPos), reinterpret_cast<WPARAM>(&endPos));

	// move the caret to the end of the text
	int outLength = GetWindowTextLength(textHwnd);
	SendMessage(textHwnd, EM_SETSEL, outLength, outLength);

	// insert the text at the new caret position
	SendMessage(textHwnd, EM_REPLACESEL, TRUE, reinterpret_cast<LPARAM>(newText.c_str()));

	// restore the previous selection
	SendMessage(textHwnd, EM_SETSEL, startPos, endPos);
}

BOOL insertPidListViewItems(HWND hWndListView, std::vector<ProcessData>& cItems)
{
	// Initialize LVITEM members that are different for each item.
	for (int index = 0; index < cItems.size(); index++)
	{
		// Initialize LVITEM members that are common to all items.
		std::string& pName = cItems[index].procNAme;
		std::string& pid = cItems[index].pid;

		LVITEM c0Item;
		c0Item.pszText = (LPSTR)pName.c_str();	//LPSTR_TEXTCALLBACK; // Sends an LVN_GETDISPINFO message.
		c0Item.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
		c0Item.stateMask = 0;
		c0Item.iSubItem = 0;
		c0Item.state = 0;
		c0Item.iItem = index;
		c0Item.iImage = index;

		// Insert items into the list.
		if (ListView_InsertItem(hWndListView, &c0Item) == -1)
			return FALSE;

		//second column (and the following ones) is set like this
		ListView_SetItemText(hWndListView, index, 1, (LPSTR)pid.c_str());
	}

	return TRUE;
}

BOOL insertHwndListViewItems(HWND hWndListView, std::vector<WindowElData>& cItems)
{
	// Initialize LVITEM members that are different for each item.
	for (int index = 0; index < cItems.size(); index++)
	{
		// Initialize LVITEM members that are common to all items.
		std::string pName = std::to_string((int)cItems[index].eHwnd);

		LVITEM c0Item;
		c0Item.pszText = (LPSTR)pName.c_str();	//LPSTR_TEXTCALLBACK; // Sends an LVN_GETDISPINFO message.
		c0Item.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
		c0Item.stateMask = 0;
		c0Item.iSubItem = 0;
		c0Item.state = 0;
		c0Item.iItem = index;
		c0Item.iImage = index;

		// Insert items into the list.
		if (ListView_InsertItem(hWndListView, &c0Item) == -1)
			return FALSE;

		//second column (and the following ones) is set like this
		ListView_SetItemText(hWndListView, index, 1, (LPSTR)cItems[index].eTitle);
		ListView_SetItemText(hWndListView, index, 2, (LPSTR)cItems[index].eClass);
	}

	return TRUE;
}



////////////////////////////////////////////////////



// createListView: Creates a list-view control in report view.
// Returns the handle to the new control
// TO DO:  The calling procedure should determine whether the handle is NULL, in case 
// of an error in creation.
// HINST hInst: The global handle to the applicadtion instance.
// HWND  hWndParent: The handle to the control's parent window. 
HWND createPidListView(HWND hwndParent, HINSTANCE hInst)
{
	INITCOMMONCONTROLSEX icex;           // Structure for control initialization.
	icex.dwICC = ICC_LISTVIEW_CLASSES;
	InitCommonControlsEx(&icex);

	RECT rcClient;                       // The parent window's client area.

	GetClientRect(hwndParent, &rcClient);

	// Create the list-view window in report view with label editing enabled.
	HWND hWndListView = CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEW,
		_T("ListView1"),
		WS_CHILD | LVS_REPORT | LVS_EDITLABELS| WS_VISIBLE,
		20, 50,
		400,//rcClient.right - rcClient.left,
		300,//rcClient.bottom - rcClient.top,
		hwndParent,
		(HMENU)listView1Id,
		hInst,
		NULL);

	LV_COLUMN col1;
	col1.mask = LVCF_WIDTH | LVCF_TEXT;
	col1.cx = 200;
	col1.pszText = "Process name";
	ListView_InsertColumn(hWndListView,  0, (LPARAM)&col1);
	//SendMessage(hWndListView, LVM_INSERTCOLUMN, 0, (LPARAM)&col2);

	LV_COLUMN col2;
	col2.mask = LVCF_WIDTH | LVCF_TEXT;
	col2.cx = 150;
	col2.pszText = "Process id";
	ListView_InsertColumn(hWndListView, 1, (LPARAM)&col2);

	return hWndListView;
}

HWND createHwndListView(HWND hwndParent, HINSTANCE hInst)
{
	INITCOMMONCONTROLSEX icex;           // Structure for control initialization.
	icex.dwICC = ICC_LISTVIEW_CLASSES;
	InitCommonControlsEx(&icex);

	// Create the list-view window in report view with label editing enabled.
	HWND hWndListView = CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEW, _T("ListView1"),
		WS_CHILD | LVS_REPORT | LVS_EDITLABELS | WS_VISIBLE,
		460, 50, 510, 300, hwndParent, (HMENU)listView2Id, hInst, NULL);

	LV_COLUMN col1;
	col1.mask = LVCF_WIDTH | LVCF_TEXT;
	col1.cx = 100;
	col1.pszText = "Hwnd id";
	ListView_InsertColumn(hWndListView, 0, (LPARAM)&col1);
	//SendMessage(hWndListView, LVM_INSERTCOLUMN, 0, (LPARAM)&col2);

	LV_COLUMN col2;
	col2.mask = LVCF_WIDTH | LVCF_TEXT;
	col2.cx = 200;
	col2.pszText = "Hwnd title";
	ListView_InsertColumn(hWndListView, 1, (LPARAM)&col2);

	LV_COLUMN col3;
	col3.mask = LVCF_WIDTH | LVCF_TEXT;
	col3.cx = 200;
	col3.pszText = "Hwnd class";
	ListView_InsertColumn(hWndListView, 2, (LPARAM)&col3);

	return hWndListView;
}
