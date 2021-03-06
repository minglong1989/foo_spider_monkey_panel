#include <stdafx.h>
#include "file_helpers.h"

#include <utils/scope_helpers.h>
#include <utils/string_helpers.h>

#include <helpers.h>

#include <filesystem>


namespace smp::file
{

std::wstring CleanPath( const std::wstring& path )
{ // TODO: replace with `ReplaceChar` or ReplaceString
    std::wstring cleanedPath = path;
    for ( auto& curChar : cleanedPath )
    {
        if ( L'/' == curChar )
        {
            curChar = L'\\';
        }
    }

    return cleanedPath;
}

std::wstring ReadFromFile( JSContext* cx, const pfc::string8_fast& path, uint32_t codepage )
{
    const auto cleanPath = [&] {
        pfc::string8_fast tmpPath = path;
        tmpPath.replace_string( "/", "\\", 0 );
        return tmpPath;
    }();

    namespace fs = std::filesystem;

    fs::path fsPath = fs::u8path( cleanPath.c_str() );
    std::error_code dummyErr;
    if ( !fs::exists( fsPath ) || !fs::is_regular_file( fsPath, dummyErr ) )
    {
        throw SmpException( smp::string::Formatter() << "Path does not point to a valid file: " << cleanPath.c_str() );
    }

    if ( !fs::file_size( fsPath ) )
    {// CreateFileMapping fails on file with zero length, so we need to bail out early
        return std::wstring{};
    }

    // Prepare file

    HANDLE hFile = CreateFile( fsPath.wstring().c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr );
    if ( !hFile )
    {
        throw SmpException( smp::string::Formatter() << "Failed to open script file: " << cleanPath.c_str() );
    }
    utils::final_action autoFile( [hFile]() {
        CloseHandle( hFile );
    } );

    HANDLE hFileMapping = CreateFileMapping( hFile, nullptr, PAGE_READONLY, 0, 0, nullptr );
    if ( !hFileMapping )
    {
        throw SmpException( smp::string::Formatter() << "Internal error: CreateFileMapping failed for `" << cleanPath.c_str() << "`" );
    }
    utils::final_action autoMapping( [hFileMapping]() {
        CloseHandle( hFileMapping );
    } );

    DWORD dwFileSize = GetFileSize( hFile, nullptr );
    LPCBYTE pAddr = (LPCBYTE)MapViewOfFile( hFileMapping, FILE_MAP_READ, 0, 0, 0 );
    if ( !pAddr )
    {
        throw SmpException( smp::string::Formatter() << "Internal error: MapViewOfFile failed for `" << cleanPath.c_str() << "`" );
    }
    utils::final_action autoAddress( [pAddr]() {
        UnmapViewOfFile( pAddr );
    } );

    if ( dwFileSize == INVALID_FILE_SIZE )
    {
        throw SmpException( smp::string::Formatter() << "Internal error: failed to read file size of `" << cleanPath.c_str() << "`" );
    }

    // Read file

    std::wstring fileContent;

    constexpr unsigned char bom32Be[] = { 0x00, 0x00, 0xfe, 0xff };
    constexpr unsigned char bom32Le[] = { 0xff, 0xfe, 0x00, 0x00 };
    constexpr unsigned char bom16Be[] = { 0xfe, 0xff }; // must be 4byte size
    constexpr unsigned char bom16Le[] = { 0xff, 0xfe }; // must be 4byte size, but not 0xff, 0xfe, 0x00, 0x00
    constexpr unsigned char bom8[] = { 0xef, 0xbb, 0xbf };

    if ( dwFileSize >= 4
         && !memcmp( bom16Le, pAddr, sizeof( bom16Le ) ) )
    {
        pAddr += sizeof( bom16Le );
        dwFileSize -= sizeof( bom16Le );

        const size_t outputSize = dwFileSize >> 1;
        fileContent.resize( outputSize );

        // Can't use wstring.assign(), because of potential aliasing issues
        memcpy( fileContent.data(), (const char*)pAddr, dwFileSize );
    }
    else if ( dwFileSize >= sizeof( bom8 )
              && !memcmp( bom8, pAddr, sizeof( bom8 ) ) )
    {
        pAddr += sizeof( bom8 );
        dwFileSize -= sizeof( bom8 );

        size_t outputSize = pfc::stringcvt::estimate_codepage_to_wide( CP_UTF8, (const char*)pAddr, dwFileSize );
        fileContent.resize( outputSize );

        outputSize = pfc::stringcvt::convert_codepage_to_wide( CP_UTF8, fileContent.data(), outputSize, (const char*)pAddr, dwFileSize );
        fileContent.resize( outputSize );
    }
    else
    {
        uint32_t detectedCodepage;
        if ( codepage )
        {
            detectedCodepage = codepage;
        }
        else
        { // TODO: dirty hack! remove
            const std::wstring wpath = fs::absolute( fsPath ).wstring();
            static std::unordered_map<std::wstring, uint32_t> codepageMap;

            if ( codepageMap.count( wpath ) )
            {
                detectedCodepage = codepageMap[wpath];
            }
            else
            {
                detectedCodepage = helpers::detect_text_charset( (const char*)pAddr, dwFileSize );
                codepageMap[wpath] = detectedCodepage;
            }
        }

        size_t outputSize = pfc::stringcvt::estimate_codepage_to_wide( detectedCodepage, (const char*)pAddr, dwFileSize );
        fileContent.resize( outputSize );

        outputSize = pfc::stringcvt::convert_codepage_to_wide( detectedCodepage, fileContent.data(), outputSize, (const char*)pAddr, dwFileSize );
        fileContent.resize( outputSize );
    }

    return fileContent;
}

} // namespace smp::file
