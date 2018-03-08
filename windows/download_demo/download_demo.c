// Copyright (C) 2018 David Helkowski

#include <windows.h>
#include <winhttp.h>
#include <stdio.h>
#include <string>

/*
// Assume only ascii bytes in; and just force into a wide string. Won't handle utf8 correctly
void ascii2ws( const std::string &s, std::wstring &ws ) {
    std::wstring wsTmp(s.begin(), s.end());
    ws = wsTmp;
}
*/

#include <vector>
// Convert utf8 to utf16/wchar
int ascii2ws( const std::string &s, std::wstring &ws ) {
   if( ws.empty() ) ws = L"";

   size_t len = ::MultiByteToWideChar( CP_UTF8, 0, s.data(), (int)s.size(), NULL, 0 );
   if( !len ) return 1;
   
   std::vector<wchar_t> buffer( len );
   int lenDone = ::MultiByteToWideChar( CP_UTF8, 0, s.data(), (int)s.size(), &buffer[0], buffer.size() );
   if( !lenDone ) return 2;
   
   ws = std::wstring( &buffer[0], lenDone );
   return 0;
}

int fetchfile( char *domainz, char *pagez, char *ipz ) {
    if( !pagez ) pagez = "/";
    char domainWithIp = 0;
    if( !ipz ) {
        ipz = domainz;
        domainWithIp = 1;
    }
    std::wstring ip;
    std::wstring page;
    std::wstring domain;
    ascii2ws( ipz, ip );
    ascii2ws( pagez, page );
    ascii2ws( domainz, domain );
    
    HINTERNET session = WinHttpOpen(
        L"XJ Installer", 
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME, 
        WINHTTP_NO_PROXY_BYPASS,
        0
    );
    if( !session ) {
        printf("Could not open session\n");
        return 1;
    }
    
    int port = 80;
    HINTERNET conn = WinHttpConnect( session, ip.c_str(), port, 0 );
    if( !conn ) {
        printf("Could not open connection\n");
        return 1;
    }
    
    HINTERNET req = WinHttpOpenRequest(
        conn,
        L"GET",
        page.c_str(),
        NULL,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        0
    );
    if( !req ) {
        printf("Could not create request\n");
        return 1;
    }
    
    if( domainWithIp ) {
        WinHttpAddRequestHeaders(
            req, 
            domain.c_str(), 
            (DWORD) domain.length(), 
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
        0
    );
    if( !send_ok ) {
        printf("Send failed\n");
        return 1;
    }
    
    BOOL recv_ok = WinHttpReceiveResponse( req, NULL );
    if( !recv_ok ) {
        printf("Recieve failed\n");
        return 1;
    }
    
    DWORD size = 0;
    if( !WinHttpQueryDataAvailable( req, &size ) ) {
        DWORD err = GetLastError();
        if( err == ERROR_WINHTTP_CONNECTION_ERROR ) printf( "Connection error\n");
        if( err == ERROR_WINHTTP_INCORRECT_HANDLE_STATE ) printf( "Incorrect handle state\n");
        //printf( "Error %u from WinHttpQueryDataAvailable\n",  );
        return 1;
    }
    
    // Buffer size should be at least 8kb; MSDN says internal buffer used is that size
    LPSTR buffer = new char[ size + 1 ];
    if( !buffer ) {
        printf( "Out of memory\n" );
        return 1;
    }
    ZeroMemory( buffer, size + 1 );
    
    DWORD size_fetched;
    if( !WinHttpReadData( req, (LPVOID) buffer, size, &size_fetched ) ) {
        printf( "Error %u from WinHttpReadData\n", GetLastError() );
        return 1;
    }
    
    printf( "%.*s\n", size_fetched, buffer );
    
    return 0;
}

void showpath() {
    char buffer[1025];
    DWORD err = GetModuleFileNameA( 0, &buffer[0], 1024 );
    if( err < 0 ) {
        DWORD lastError = GetLastError();
        printf("GetModuleFileNameA error %u\n", lastError );
    }
    printf("File: %s\n", buffer );
    
    // Based on:
    // - https://msdn.microsoft.com/en-us/library/ms809762.aspx
    // - https://stackoverflow.com/questions/8782771/loading-pe-headers
    HANDLE fh = CreateFile( buffer, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if( fh == INVALID_HANDLE_VALUE ) return;
    
    HANDLE fmap = CreateFileMapping( fh, NULL, PAGE_READONLY, 0, 0, NULL );
    if( !fmap ) {
        CloseHandle( fh );
        return;
    }
    
    LPVOID fileBase = MapViewOfFile( fmap, FILE_MAP_READ, 0, 0, 0);
    if( !fileBase ) {
        CloseHandle( fmap );
        CloseHandle( fh );
    }
    
    PIMAGE_DOS_HEADER dosHeader      = ( PIMAGE_DOS_HEADER      ) fileBase;
    PIMAGE_NT_HEADERS ntHeader       = ( PIMAGE_NT_HEADERS      ) ( (BYTE*) dosHeader + dosHeader->e_lfanew );
    PIMAGE_FILE_HEADER fileHeader    = ( PIMAGE_FILE_HEADER     ) &ntHeader->FileHeader;
    PIMAGE_OPTIONAL_HEADER optHeader = ( PIMAGE_OPTIONAL_HEADER ) &ntHeader->OptionalHeader;
    PIMAGE_SECTION_HEADER secHeader  = ( PIMAGE_SECTION_HEADER  )
        ( (BYTE*) ntHeader + sizeof( IMAGE_NT_HEADERS ) + ( fileHeader->NumberOfSections - 1 ) * sizeof( IMAGE_SECTION_HEADER ) );
    DWORD peSize = secHeader->PointerToRawData + secHeader->SizeOfRawData;
    printf("Pesize: %u\n", peSize );    
    CloseHandle( fmap );
    CloseHandle( fh );
    
    FILE *fh2 = fopen( buffer, "r" );
    fseek( fh2, 0, SEEK_END );
    long int fileLen = ftell( fh2 );
    if( fileLen <= peSize ) {
        printf("No extra data\n");
        fclose( fh2 );
        return;
    }
    fseek( fh2, peSize, SEEK_SET );
    
    char data[100];
    size_t dataLen = fread( data, 1, 100, fh2 );
    if( dataLen == 0 ) {
        printf("No data could be read\n");
        fclose( fh2 );
        return;
    }
    
    printf("Data: %.*s\n", dataLen, data );
}

int main( int argc, char *argv[] ) {
    showpath();
    //return fetchfile( "livxtrm.com", 0, 0 );
}