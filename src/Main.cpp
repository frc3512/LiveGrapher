//=============================================================================
//File Name: Main.cpp
//Description: Displays graph of data coming from remote location
//Author: FRC Team 3512, Spartatroniks
//=============================================================================

#define _WIN32_WINNT 0x0501
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>

#define _WIN32_IE 0x0600
#include <commctrl.h>

#include <cstdlib>
#include <cstring>
#include <cmath>
#include <list>
#include <sstream>
#include <string>
#include <vector>

#include "Resource.h"
#include "WindowVars.hpp"

#include "WinGDI/Graph.hpp"

// Global because the drawing is set up to be continuous in CALLBACK OnEvent
HINSTANCE gInstance = NULL;
HWND gDisplayWindow = NULL;
HWND gTabWindow = NULL;
HICON gMainIcon = NULL;
HMENU gMainMenu = NULL;

std::vector<HWND> gTabs;
// Stores index of last selected tab
unsigned int gLastTab = 0;

// Stores all Drawables to be drawn with WM_PAINT message
std::vector<Drawable*> gDrawables;

template <typename T>
std::string numberToString( T number );

LRESULT CALLBACK MainProc( HWND handle , UINT message , WPARAM wParam , LPARAM lParam );

LRESULT CALLBACK StaticProc( HWND handle , UINT message , WPARAM wParam , LPARAM lParam );

BOOL CALLBACK AboutCbk( HWND hDlg , UINT message , WPARAM wParam , LPARAM lParam );

INT WINAPI WinMain( HINSTANCE Instance , HINSTANCE , LPSTR , INT ) {
    gInstance = Instance;

    INITCOMMONCONTROLSEX icc;

    // Initialize common controls.
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&icc);

    // Initialize menu bar and set the check boxes' initial state
    gMainMenu = LoadMenu( Instance , "mainMenu" );

    gMainIcon = LoadIcon( Instance , "mainIcon" );

    HBRUSH mainBrush = CreateSolidBrush( RGB( 240 , 240 , 240 ) );

    // Define a class for our main window
    WNDCLASSEX WindowClass;
    ZeroMemory( &WindowClass , sizeof(WNDCLASSEX) );
    WindowClass.cbSize        = sizeof(WNDCLASSEX);
    WindowClass.style         = 0;
    WindowClass.lpfnWndProc   = &MainProc;
    WindowClass.cbClsExtra    = 0;
    WindowClass.cbWndExtra    = 0;
    WindowClass.hInstance     = Instance;
    WindowClass.hIcon         = gMainIcon;
    WindowClass.hCursor       = LoadCursor( NULL , IDC_ARROW );
    WindowClass.hbrBackground = mainBrush;
    WindowClass.lpszMenuName  = "mainMenu";
    WindowClass.lpszClassName = "LiveGrapher";
    WindowClass.hIconSm       = gMainIcon;
    RegisterClassEx( &WindowClass );

    WNDCLASSEX StaticClass;
    ZeroMemory( &StaticClass , sizeof(WNDCLASSEX) );
    StaticClass.cbSize        = sizeof(WNDCLASSEX);
    StaticClass.style         = 0;
    StaticClass.lpfnWndProc   = &StaticProc;
    StaticClass.cbClsExtra    = 0;
    StaticClass.cbWndExtra    = 0;
    StaticClass.hInstance     = Instance;
    StaticClass.hIcon         = gMainIcon;
    StaticClass.hCursor       = LoadCursor( NULL , IDC_ARROW );
    StaticClass.hbrBackground = mainBrush;
    StaticClass.lpszMenuName  = "mainMenu";
    StaticClass.lpszClassName = "LiveStatic";
    StaticClass.hIconSm       = gMainIcon;
    RegisterClassEx( &StaticClass );

    MSG Message;
    HACCEL hAccel;

    /* ===== Make two windows to display ===== */
    RECT winSize = { 0 , 0 , static_cast<int>(WindowVars::width) , static_cast<int>(WindowVars::height) }; // set the size, but not the position
    AdjustWindowRect(
            &winSize ,
            WS_SYSMENU | WS_CAPTION | WS_VISIBLE | WS_MINIMIZEBOX | WS_CLIPCHILDREN ,
            TRUE ); // window has menu

    // Create a new window to be used for the lifetime of the application
    gDisplayWindow = CreateWindowEx( 0 ,
            "LiveGrapher" ,
            "LiveGrapher" ,
            WS_SYSMENU | WS_CAPTION | WS_VISIBLE | WS_MINIMIZEBOX | WS_CLIPCHILDREN ,
            ( GetSystemMetrics(SM_CXSCREEN) - ( winSize.right - winSize.left ) ) / 2 ,
            ( GetSystemMetrics(SM_CYSCREEN) - ( winSize.bottom - winSize.top ) ) / 2 ,
            winSize.right - winSize.left , // returns image width (resized as window)
            winSize.bottom - winSize.top , // returns image height (resized as window)
            NULL ,
            gMainMenu ,
            Instance ,
            NULL );
    /* ======================================= */

    // Load keyboard accelerators
    hAccel = LoadAccelerators( gInstance , "KeyAccel" );

    while ( GetMessage( &Message , NULL , 0 , 0 ) > 0 ) {
        if ( !TranslateAccelerator(
                gDisplayWindow,    // Handle to receiving window
                hAccel,            // Handle to active accelerator table
                &Message) )        // Message data
        {
            // If a message was waiting in the message queue, process it
            TranslateMessage( &Message );
            DispatchMessage( &Message );
        }
    }

    // Delete dynamically allocated Drawables
    for ( unsigned int i = 0 ; i < gDrawables.size() ; i++ ) {
        delete gDrawables[i];
    }

    DestroyIcon( gMainIcon );

    UnregisterClass( "LiveGrapher" , Instance );
    UnregisterClass( "LiveStatic" , Instance );

    return Message.wParam;
}

template <typename T>
std::string numberToString( T number ) {
    std::stringstream ss;
    std::string str;

    ss << number;
    ss >> str;

    return str;
}

LRESULT CALLBACK MainProc( HWND handle , UINT message , WPARAM wParam , LPARAM lParam ) {
    switch ( message ) {
    case WM_CREATE: {
        TCITEM tabInfo;
        char* tempBuf = static_cast<char*>( std::malloc( 8 ) );
        std::strcpy( tempBuf , "Graph" );

        HWND tabWindow = CreateWindowEx( 0,
                WC_TABCONTROL,
                "",
                WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE,
                9,
                9,
                WindowVars::width - 18,
                WindowVars::height - 52,
                handle,
                NULL,
                gInstance,
                NULL);

        SendMessage( tabWindow,
                WM_SETFONT,
                reinterpret_cast<WPARAM>( GetStockObject( DEFAULT_GUI_FONT ) ),
                MAKELPARAM( FALSE , 0 ) );

        gTabWindow = tabWindow;

        // Get size of tabWindow's client area
        RECT tabWinRect;
        GetClientRect( tabWindow , &tabWinRect );

        // Add tabs to tabWindow
        tabInfo.mask = TCIF_TEXT | TCIF_IMAGE;
        tabInfo.iImage = -1;
        tabInfo.pszText = tempBuf;

        HWND tab1 = CreateWindowEx( 0,
                "LiveStatic",
                "",
                WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE,
                1,
                21,
                tabWinRect.right - 9 + 5,
                tabWinRect.bottom - 9 - 14,
                tabWindow,
                reinterpret_cast<HMENU>( IDC_TABSTART ),
                gInstance,
                NULL);

        gTabs.push_back( tab1 );

        HWND tab2 = CreateWindowEx( 0,
                "LiveStatic",
                "",
                WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE,
                1,
                21,
                tabWinRect.right - 9 + 5,
                tabWinRect.bottom - 9 - 14,
                tabWindow,
                reinterpret_cast<HMENU>( IDC_TABSTART + 1 ),
                gInstance,
                NULL);

        gTabs.push_back( tab2 );

        if ( TabCtrl_InsertItem( tabWindow , 0 , &tabInfo ) == -1 ) {
            DestroyWindow( tabWindow );
        }
        else {
            std::strcpy( tabInfo.pszText , "Options" );
            if ( TabCtrl_InsertItem( tabWindow , 1 , &tabInfo ) == -1 ) {
                DestroyWindow( tabWindow );
            }
        }

        std::free( tempBuf );

        // Create controls in each tab
        HWND graphGroup = CreateWindowEx( 0,
                "BUTTON",
                "PID Tuner",
                WS_VISIBLE | WS_CHILD | BS_GROUPBOX | BS_TEXT,
                9,
                9,
                tabWinRect.right - 18 - 4,
                tabWinRect.bottom - 22 - 18,
                tab1,
                reinterpret_cast<HMENU>( IDC_GRAPHGROUP ),
                gInstance,
                NULL);

        SendMessage( graphGroup,
                WM_SETFONT,
                reinterpret_cast<WPARAM>( GetStockObject( DEFAULT_GUI_FONT ) ),
                MAKELPARAM( FALSE , 0 ) );

        // Create PID loop graph
        gDrawables.push_back( new Graph( graphGroup , Vector2i( 9 , 23 ) , Vector2i( 450 , 350 ) ) );
        Graph* pidGraph = static_cast<Graph*>(gDrawables[0]);
        pidGraph->setHistoryLength( 450 * 10 );
        pidGraph->setYMin( 0 );
        pidGraph->setYMax( 5200 );

        HWND Pedit = CreateWindowEx( WS_EX_STATICEDGE,
                "EDIT",
                "Edit P",
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_LEFT,
                469,
                23,
                122,
                20,
                graphGroup,
                reinterpret_cast<HMENU>( IDC_PEDIT ),
                (HINSTANCE)GetWindowLong( graphGroup , GWL_HINSTANCE ) , NULL );

        SendMessage( Pedit,
                WM_SETFONT,
                reinterpret_cast<WPARAM>( GetStockObject( DEFAULT_GUI_FONT ) ),
                MAKELPARAM( FALSE , 0 ) );

        HWND Iedit = CreateWindowEx( WS_EX_STATICEDGE,
                "EDIT",
                "Edit I",
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_LEFT,
                469,
                52,
                122,
                20,
                graphGroup,
                reinterpret_cast<HMENU>( IDC_IEDIT ),
                (HINSTANCE)GetWindowLong( graphGroup , GWL_HINSTANCE ) , NULL );

        SendMessage( Iedit,
                WM_SETFONT,
                reinterpret_cast<WPARAM>( GetStockObject( DEFAULT_GUI_FONT ) ),
                MAKELPARAM( FALSE , 0 ) );

        HWND Dedit = CreateWindowEx( WS_EX_STATICEDGE,
                "EDIT",
                "Edit D",
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_LEFT,
                469,
                81,
                122,
                20,
                graphGroup,
                reinterpret_cast<HMENU>( IDC_DEDIT ),
                (HINSTANCE)GetWindowLong( graphGroup , GWL_HINSTANCE ) , NULL );

        SendMessage( Dedit,
                WM_SETFONT,
                reinterpret_cast<WPARAM>( GetStockObject( DEFAULT_GUI_FONT ) ),
                MAKELPARAM( FALSE , 0 ) );

        HWND Fedit = CreateWindowEx( WS_EX_STATICEDGE,
                "EDIT",
                "Edit F",
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_LEFT,
                469,
                110,
                122,
                20,
                graphGroup,
                reinterpret_cast<HMENU>( IDC_FEDIT ),
                (HINSTANCE)GetWindowLong( graphGroup , GWL_HINSTANCE ) , NULL );

        SendMessage( Fedit,
                WM_SETFONT,
                reinterpret_cast<WPARAM>( GetStockObject( DEFAULT_GUI_FONT ) ),
                MAKELPARAM( FALSE , 0 ) );

        HWND connectButton = CreateWindowEx( 0,
                "BUTTON",
                "Connect Robot",
                WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                9,
                WindowVars::height - 9 - 27,
                100,
                27,
                handle,
                reinterpret_cast<HMENU>( IDM_CONNECT ),
                gInstance,
                NULL);

        SendMessage( connectButton,
                WM_SETFONT,
                reinterpret_cast<WPARAM>( GetStockObject( DEFAULT_GUI_FONT ) ),
                MAKELPARAM( FALSE , 0 ) );

        HWND clearButton = CreateWindowEx( 0,
                "BUTTON",
                "Clear Data",
                WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                WindowVars::width - 2 * ( 9 + 100 ),
                WindowVars::height - 9 - 27,
                100,
                27,
                handle,
                reinterpret_cast<HMENU>( IDM_CLEAR ),
                gInstance,
                NULL);

        SendMessage( clearButton,
                WM_SETFONT,
                reinterpret_cast<WPARAM>( GetStockObject( DEFAULT_GUI_FONT ) ),
                MAKELPARAM( FALSE , 0 ) );

        HWND saveButton = CreateWindowEx( 0,
                "BUTTON",
                "Save",
                WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                WindowVars::width - 9 - 100,
                WindowVars::height - 9 - 27,
                100,
                27,
                handle,
                reinterpret_cast<HMENU>( IDM_SAVE ),
                gInstance,
                NULL);

        SendMessage( saveButton,
                WM_SETFONT,
                reinterpret_cast<WPARAM>( GetStockObject( DEFAULT_GUI_FONT ) ),
                MAKELPARAM( FALSE , 0 ) );

        break;
    }

    case WM_COMMAND: {
        int wmId = LOWORD( wParam );

        switch( wmId ) {
            case IDM_CONNECT: {
                static_cast<Graph*>(gDrawables[0])->reconnect();

                break;
            }

            case IDM_CLEAR: {
                static_cast<Graph*>(gDrawables[0])->clearAllData();

                break;
            }

            case IDM_SAVE: {
                if ( static_cast<Graph*>(gDrawables[0])->saveToCSV( "dataSave.csv" ) ) {
                    MessageBox( handle , "Data has been saved" , "Save To CSV" , MB_ICONINFORMATION | MB_OK );
                }
                else {
                    MessageBox( handle , "Failed to save data" , "Save To CSV" , MB_ICONERROR | MB_OK );
                }

                break;
            }

            case IDM_EXIT: {
                PostQuitMessage( 0 );

                break;
            }

            case IDM_ABOUT: {
                DialogBox( gInstance , MAKEINTRESOURCE(IDD_ABOUTBOX) , handle , AboutCbk );

                break;
            }
        }

        break;
    }

    case WM_NOTIFY: {
        LPNMHDR control = (LPNMHDR)lParam;

        if ( control->code == TCN_SELCHANGE ) {
            unsigned int curSel = TabCtrl_GetCurSel( control->hwndFrom );

            // Hide last selected tab
            ShowWindow( gTabs[gLastTab] , SW_HIDE );

            gLastTab = curSel;

            // Show currently selected tab
            ShowWindow( gTabs[gLastTab] , SW_SHOW );
            if ( GetTopWindow( gDisplayWindow ) != gTabs[gLastTab] ) {
                // Put static control on top in Z-order
                BringWindowToTop( gTabs[gLastTab] );
            }
        }
        else {
            return DefWindowProc( handle , message , wParam , lParam );
        }

        break;
    }

    case WM_DESTROY: {
        // If the display window is being closed, exit the application
        if ( handle == gDisplayWindow ) {
            PostQuitMessage( 0 );
        }

        break;
    }

    default: {
        return DefWindowProc( handle , message , wParam , lParam );
    }
    }

    return 0;
}

LRESULT CALLBACK StaticProc( HWND handle , UINT message , WPARAM wParam , LPARAM lParam ) {
    switch ( message ) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint( handle , &ps );

        RECT rect;
        GetClientRect( handle , &rect );

        // Fill DC with a background color
        HBRUSH backgroundBrush = CreateSolidBrush( RGB( 255 , 255 , 255 ) );
        HRGN region = CreateRectRgn( 0 , 0 , rect.right , rect.bottom );
        FillRgn( hdc , region , backgroundBrush );
        DeleteObject( region );
        DeleteObject( backgroundBrush );

        EndPaint( handle , &ps );

        break;
    }

    case WM_CTLCOLORSTATIC: {
        return (INT_PTR)GetStockObject( WHITE_BRUSH );

        break;
    }

    default: {
        return DefWindowProc( handle , message , wParam , lParam );
    }
    }

    return 0;
}

// Message handler for "About" box
BOOL CALLBACK AboutCbk( HWND hDlg , UINT message , WPARAM wParam , LPARAM lParam ) {
    UNREFERENCED_PARAMETER(lParam);
    switch ( message ) {
    case WM_INITDIALOG: {
        return TRUE;
    }

    case WM_COMMAND: {
        if ( LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL ) {
            EndDialog( hDlg , LOWORD(wParam) );
            return TRUE;
        }

        break;
    }
    }

    return FALSE;
}
