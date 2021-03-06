#pragma once

// Spider Monkey ESR60 supports only Win7+
#define _WIN32_WINNT _WIN32_WINNT_WIN7
#define WINVER _WIN32_WINNT_WIN7

// Fix std min max conflicts
#define NOMINMAX
#include <algorithm>
namespace Gdiplus
{
using std::min;
using std::max;
};

#include <WinSock2.h>
#include <Windows.h>
#pragma warning( push, 0 )
#   include <GdiPlus.h>
#pragma warning( pop ) 

// COM objects
#include <ActivScp.h>
#include <activdbg.h>
#include <MsHTML.h>
#include <MsHtmHst.h>
#include <ShlDisp.h>
#include <exdisp.h>
#include <shobjidl_core.h>
// Generates wrappers for COM listed above
#include <ComDef.h>

// ATL/WTL
#include <atlstr.h>
#include <atlapp.h>
#include <atlwin.h>
#include <atlframe.h>
#include <atldlgs.h>
#include <atlctrls.h>
#include <atlddx.h>
#include <atlcrack.h>
#include <atltheme.h>
#include <atltypes.h>

// Scintilla
#include <Scintilla.h>
#include <SciLexer.h>

// foobar2000 SDK
#pragma warning( push, 0 )
#   include <foobar2000/SDK/foobar2000.h>
#pragma warning( pop ) 

// Columns UI SDK
#pragma warning( push, 0 )
#   include <columns_ui-sdk/ui_extension.h>
#pragma warning( pop ) 

// Mozilla SpiderMonkey
#pragma warning( push )  
#pragma warning( disable : 4100 ) // unused variable
#pragma warning( disable : 4251 ) // dll interface warning
#pragma warning( disable : 4324 ) // structure was padded due to alignment specifier
#pragma warning( disable : 4996 ) // C++17 deprecation warning
#   include <jsapi.h>
#   include <jsfriendapi.h>
#pragma warning( pop ) 

// Some macros defined by windowsx.h should be removed
#ifdef _INC_WINDOWSX
#undef SubclassWindow
#endif

#include <component_defines.h>
#include <component_guids.h>
#include <smp_exception.h>
