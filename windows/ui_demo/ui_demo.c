// Copyright (C) 2018 David Helkowski

#ifndef UNICODE
#define UNICODE
#endif 

#define HIMETRIC_INCH 2540

#include<windows.h>
#include<wchar.h>
#include<olectl.h>
#include<winhttp.h>
#include"xmlbare/parser.h"

HWND windows[20];
int windowCnt = 0;
LPPICTURE image;

LRESULT CALLBACK WindowProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
void CALLBACK httpCallback( HINTERNET hInternet, DWORD *context, DWORD dwInternetStatus, void * lpvStatusInfo, DWORD dwStatusInfoLength );

static void drawPicture(HWND hWnd, HDC hdc, LPPICTURE picture, int x, int y);
static int utf8_to_ws( char *w, wchar_t **ws, int *len );
static int fetchfile( char *domainz, char *pagez, char *ipz, HWND window );

#define newType(structName) structName *self = ( structName * ) malloc( sizeof( structName ) ); memset( (char *) self, 0, sizeof( structName ) );

#define ELT_IMAGE 1
#define ELT_BUTTON 2
#define ELT_TEXT 3

typedef struct elBase {
    char rawtype;
    elBase *next;
} elBase;

typedef struct elImage {
    char rawtype;
    elBase *next;
    LPPICTURE picture;
    int x;
    int y;
} elImage;

elImage *elImage__new( node *node ) {
    newType( elImage );
    self->rawtype = ELT_IMAGE;
    struct nodec *fileNode = nodec__getnode( node, "file" );
    
    if( !fileNode ) {
        printf("no filenode\n");
        return NULL;
    }
    
    char *file = nodec__getval( fileNode );
    att *x = nodec__getatt( node, "x" );
    if( x ) self->x = atoi( attc__getval( x ) );
    else printf("x is not set\n");
    
    att *y = nodec__getatt( node, "y" );
    if( y ) self->y = atoi( attc__getval( y ) );
    else printf("y is not set\n");
    
    WCHAR *fileW = NULL;
    int len;
    utf8_to_ws( file, &fileW, &len );
    
    HRESULT hr = OleLoadPicturePath( fileW, 0, 0, 0, IID_IPicture, (void**)&self->picture );
    free( fileW );
    if( hr != S_OK ) image = NULL;
    
    printf("Created image\n");
    
    return self;
}

typedef struct elText {
    char rawtype;
    elBase *next;
    int x;
    int y;
    int w;
    int h;
    WCHAR *text;
    char textAlloc;
} elText;

typedef struct elButton {
    char rawtype;
    elBase *next;
    int x;
    int y;
    int w;
    int h;
    WCHAR *text;
    char textAlloc;
} elButton;

elButton *elButton__new( node *node );

elText *elText__new( node *node ) {
    elText *self = (elText *) elButton__new( node );
    self->rawtype = ELT_TEXT;
    return self;
}

elButton *elButton__new( node *node ) {
    newType( elButton );
    self->rawtype = ELT_BUTTON;
    
    int x1=0,y1=0,w1=0,h1=0;
    
    att *x = nodec__getatt( node, "x" );
    if( x ) self->x = x1 = atoi( attc__getval( x ) );

    att *y = nodec__getatt( node, "y" );
    if( y ) self->y = y1 = atoi( attc__getval( y ) );
    
    att *w = nodec__getatt( node, "w" );
    if( w ) self->w = w1 = atoi( attc__getval( w ) );
    
    att *h = nodec__getatt( node, "h" );
    if( h ) self->h = h1 = atoi( attc__getval( h ) );
    
    att *text = nodec__getatt( node, "text" );
    if( text ) {
        int len;
        utf8_to_ws( attc__getval( text ), &self->text, &len );
    }
    else {
        self->text = L"OK";
    }
    
    printf("Created button x:%i,y:%i,w:%i,h:%i\n", x1, y1, w1, h1 );
    
    return self;
}

elButton *elButton__destroy( elButton *self ) {
    if( self->textAlloc ) free( self->text );
}

typedef struct instPage {
    
} instPage;

elBase *firstEl = NULL;
elBase *lastEl = NULL;
void addEl( elBase *newEl ) {
    if( !newEl ) return;
    if( !firstEl ) {
        firstEl = lastEl = newEl;
        return;
    }
    lastEl->next = newEl;
    lastEl = newEl;
}

void readConf() {
    struct parserc *parser = parserc__new();
    
    if( parser ) {
        struct nodec *root = parserc__file( parser, "data.xml" );
        if( root ) {    
            node *xml = nodec__getnode( root, "xml" );
            if( xml ) {
                node *curNode = xml->firstchild;
                while( curNode ) {
                    if( !curNode->rawtype ) {
                        if( curNode->namelen == 5 && !strncmp( curNode->name, "image", 5 ) ) {
                            elImage *image = elImage__new( curNode );
                            addEl( (elBase *) image );
                        }
                        else if( curNode->namelen == 6 && !strncmp( curNode->name, "button", 6 ) ) {
                            elButton *button = elButton__new( curNode );
                            addEl( (elBase *) button );
                        }
                        else if( curNode->namelen == 4 && !strncmp( curNode->name, "text", 4 ) ) {
                            elText *text = elText__new( curNode );
                            addEl( (elBase *) text );
                        }
                    }
                    curNode = curNode->next;
                }
            }
            else {
                printf("xml has no root xml node\n");
            }
            
            nodec__destroy( root );
        }
        else {
            printf("Could not load xml\n");
        }
        parserc__destroy( parser );
    }
    else {
        printf("Could not create parser\n");
    }
}

static void addButton( HWND, HINSTANCE, elButton * );
static void addText( HWND, HINSTANCE, elText * );

int WINAPI wWinMain( HINSTANCE progInstance, HINSTANCE prevInstance, PWSTR args, int visibilityFlag ) {
    const wchar_t CLASS_NAME[]  = L"My Window Class";
            
    WNDCLASS wc = { };

    wc.lpfnWndProc   = WindowProc;
    wc.hInstance     = progInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass( &wc );

    HWND window = CreateWindowEx(
        0,
        CLASS_NAME,
        L"Main Window", // title bar text
        WS_OVERLAPPEDWINDOW,

        // Size and position
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,

        NULL,         // Parent window    
        NULL,         // Menu
        progInstance,
        NULL          // Application data
    );

    readConf();
    
    if( !window ) return 0;
    
    {
        elBase *curEl = firstEl;
        while( curEl ) {
            if( curEl->rawtype == ELT_BUTTON ) {
                addButton( window, progInstance, ( elButton * ) curEl );
            }
            if( curEl->rawtype == ELT_TEXT ) {
                addText( window, progInstance, ( elText * ) curEl );
            }
            curEl = curEl->next;
        }
    }
        
    //fetchfile( "livxtrm.com", 0, 0, window );
    
    ShowWindow( window, visibilityFlag );

    MSG msg = { };
    while( GetMessage( &msg, NULL, 0, 0 ) ) {
        TranslateMessage( &msg );
        DispatchMessage( &msg );
    }
    
    {
        elBase *curEl = firstEl;
        while( curEl ) {
            elBase *nextEl = curEl->next;
            if( curEl->rawtype == ELT_BUTTON ) {
                elButton__destroy( ( elButton * ) curEl );
            }
            if( curEl->rawtype == ELT_TEXT ) {
                elButton__destroy( ( elButton * ) curEl );
            }
            free( curEl );
            curEl = nextEl;
        }
    }

    return 0;
}

static void addButton( HWND window, HINSTANCE inst, elButton *button2 ) {
    HWND button = CreateWindow( 
        L"BUTTON",  // Predefined class; Unicode assumed 
        button2->text,      // Button text 
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Styles 
        button2->x,         // x position 
        button2->y,         // y position 
        button2->w,        // Button width
        button2->h,        // Button height
        window,     // Parent window
        NULL,       // No menu.
        inst, 
        NULL    // Pointer not needed.
    );      
    windows[ windowCnt++ ] = button;
}

static void addText( HWND window, HINSTANCE inst, elText *button2 ) {
    HWND button = CreateWindow( 
        L"STATIC",  // Predefined class; Unicode assumed 
        L"Blah",      // Button text 
        WS_VISIBLE | WS_CHILD,  // Styles 
        button2->x,         // x position 
        button2->y,         // y position 
        button2->w,        // Button width
        button2->h,        // Button height
        window,     // Parent window
        NULL,       // No menu.
        inst, 
        NULL    // Pointer not needed.
    );   
    SetWindowTextW( button, button2->text );
    windows[ windowCnt++ ] = button;
}

LRESULT CALLBACK WindowProc( HWND window, UINT msg, WPARAM wParam, LPARAM lParam ) {
    switch( msg ) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    
        case WM_PAINT:
            {
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint( window, &ps );
    
                FillRect( hdc, &ps.rcPaint, (HBRUSH) (COLOR_WINDOW+1) );
                
                elBase *curEl = firstEl;
                while( curEl ) {
                    if( curEl->rawtype == ELT_IMAGE ) {
                        elImage *image = ( elImage * ) curEl;
                        drawPicture( window, hdc, image->picture, image->x, image->y );
                    }
                    curEl = curEl->next;
                }
                //drawPicture( window, hdc, image );
                                
                EndPaint( window, &ps );
            }
            return 0;
    
        case WM_COMMAND:
            if( lParam && wParam == BN_CLICKED ) {
                HWND button = (HWND) lParam;
                
                wchar_t buffer[100] = L"";
                for( int i=0;i<windowCnt;i++ ) {
                    if( windows[i] == button ) {
                        swprintf( buffer, 100, L"window %i", i );
                    }
                }
                /*int res = */MessageBox(
                    NULL,
                    buffer, // window text
                    L"test2", // window title
                    MB_OK );
            }
            break;
    }
    return DefWindowProc( window, msg, wParam, lParam );
}

static void drawPicture( HWND hWnd, HDC hdc, LPPICTURE picture, int x, int y ) {
    if( !picture ) return;
    
    long hmWidth;
    long hmHeight;
    picture->get_Width(&hmWidth);
    picture->get_Height(&hmHeight);

    int nWidth  = MulDiv(hmWidth,  GetDeviceCaps(hdc, LOGPIXELSX), HIMETRIC_INCH);
    int nHeight = MulDiv(hmHeight, GetDeviceCaps(hdc, LOGPIXELSY), HIMETRIC_INCH);

    RECT rc;
    GetClientRect(hWnd, &rc); // I have tried GetWindowRect() also

    //int x = 100, y = 100;
    
    //https://msdn.microsoft.com/en-us/library/windows/desktop/ms682202(v=vs.85).aspx - IPicture render
    picture->Render(hdc, x, y, nHeight, nWidth, 0, 0, hmWidth, hmHeight, &rc);
}


static int fetchfile( char *domainz, char *pagez, char *ipz, HWND window ) {
    if( !pagez ) pagez = (char *) "/";
    char domainWithIp = 0;
    if( !ipz ) {
        ipz = domainz;
        domainWithIp = 1;
    }
    wchar_t *ip = NULL;
    wchar_t *page = NULL;
    wchar_t *domain = NULL;
    int ipLen = 0;
    int pageLen = 0;
    int domainLen = 0;
    utf8_to_ws( ipz, &ip, &ipLen );
    utf8_to_ws( pagez, &page, &pageLen );
    utf8_to_ws( domainz, &domain, &domainLen );
    
    HINTERNET session = WinHttpOpen(
        L"XJ Installer", 
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME, 
        WINHTTP_NO_PROXY_BYPASS,
        WINHTTP_FLAG_ASYNC
    );
    if( !session ) {
        //printf("Could not open session\n");
        return 1;
    }
    
    /*WINHTTP_STATUS_CALLBACK callback = */WinHttpSetStatusCallback(
        session, 
        (WINHTTP_STATUS_CALLBACK) httpCallback,
        WINHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS,
        0
    );
    
    int port = 80;
    HINTERNET conn = WinHttpConnect( session, ip, port, 0 );
    if( !conn ) {
        //printf("Could not open connection\n");
        return 1;
    }
    
    HINTERNET req = WinHttpOpenRequest(
        conn,
        L"GET",
        page,
        NULL,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        0
    );
    if( !req ) {
        //printf("Could not create request\n");
        return 1;
    }
    
    if( domainWithIp ) {
        WinHttpAddRequestHeaders(
            req, 
            domain, 
            (DWORD) domainLen, 
            WINHTTP_ADDREQ_FLAG_REPLACE | WINHTTP_ADDREQ_FLAG_ADD
        );
    }
    
    BOOL send_ok = WinHttpSendRequest(
        req,
        WINHTTP_NO_ADDITIONAL_HEADERS,
        0,
        WINHTTP_NO_REQUEST_DATA,
        0, 
        0,
        (DWORD_PTR) req // the context
    );
    if( !send_ok ) {
        //printf("Send failed\n");
        return 1;
    }
    
    return 0;
}

void CALLBACK httpCallback( HINTERNET hInternet, DWORD *context, DWORD dwInternetStatus, void * lpvStatusInfo, DWORD dwStatusInfoLength ) {
    switch( dwInternetStatus ) {
        case WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE:
            {
                HINTERNET *req = (HINTERNET *) context;
                
                BOOL recv_ok = WinHttpReceiveResponse( req, NULL );
                if( !recv_ok ) {
                    //printf("Recieve failed\n");
                }
            }
            break;
        
        case WINHTTP_CALLBACK_STATUS_DATA_AVAILABLE:
            {
                HINTERNET *req = (HINTERNET *) context;
                
                DWORD *byteCountPtr = (DWORD*) lpvStatusInfo;
                DWORD byteCount = *byteCountPtr;   
                
                /*DWORD size = 0;
                if( !WinHttpQueryDataAvailable( req, &size ) ) {
                    DWORD err = GetLastError();
                    if( err == ERROR_WINHTTP_CONNECTION_ERROR ) printf( "Connection error\n");
                    if( err == ERROR_WINHTTP_INCORRECT_HANDLE_STATE ) printf( "Incorrect handle state\n");
                    //printf( "Error %u from WinHttpQueryDataAvailable\n",  );
                    return 1;
                }*/
        
                // Buffer size should be at least 8kb; MSDN says internal buffer used is that size
                LPSTR buffer = (LPSTR) malloc( byteCount + 1 );//new char[ byteCount + 1 ];
                if( !buffer ) {
                    //printf( "Out of memory\n" );
                    //delete buffer;
                    //free( buffer );
                    return;
                }
                ZeroMemory( buffer, byteCount + 1 );
                
                DWORD size_fetched;
                if( !WinHttpReadData( req, (LPVOID) buffer, byteCount, &size_fetched ) ) {
                    //printf( "Error %u from WinHttpReadData\n", GetLastError() );
                    return;
                }
                
                //printf( "%.*s\n", size_fetched, buffer );
            }
            break;
        default:
            break;
    }
}

static int utf8_to_ws( char *w, wchar_t **ws, int *len ) {
   if( !w ) {
       *ws = ( wchar_t * ) L"";
       *len = 0;
       return 0;
   }   

   //int byteLen = strlen( w );
   size_t charLen = MultiByteToWideChar( CP_UTF8, 0, w, -1, NULL, 0 );
   if( !charLen ) return 1;
   if( len ) *len = charLen;
   
   //std::vector<wchar_t> buffer( len );
   *ws = (wchar_t *) malloc( sizeof( wchar_t ) * charLen ); // new wchar_t[ charLen ];
   int lenDone = MultiByteToWideChar( CP_UTF8, 0, w, -1, *ws, charLen );
   if( !lenDone ) return 2;
   
   return 0;
}