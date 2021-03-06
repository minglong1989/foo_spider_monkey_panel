#include <stdafx.h>

#include "drop_source_action.h"

#include <js_engine/js_to_native_invoker.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/js_object_helper.h>

namespace
{

using namespace mozjs;

JSClassOps jsOps = {
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    JsDropSourceAction::FinalizeJsObject,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "DropSourceAction",
    DefaultClassFlags(),
    &jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE( get_Effect, JsDropSourceAction::get_Effect )
MJS_DEFINE_JS_FN_FROM_NATIVE( put_Base, JsDropSourceAction::put_Base )
MJS_DEFINE_JS_FN_FROM_NATIVE( put_Effect, JsDropSourceAction::put_Effect )
MJS_DEFINE_JS_FN_FROM_NATIVE( put_Playlist, JsDropSourceAction::put_Playlist )
MJS_DEFINE_JS_FN_FROM_NATIVE( put_Text, JsDropSourceAction::put_Text )
MJS_DEFINE_JS_FN_FROM_NATIVE( put_ToSelect, JsDropSourceAction::put_ToSelect )

const JSPropertySpec jsProperties[] = {
    JS_PSGS( "Base", DummyGetter, put_Base, DefaultPropsFlags() ),
    JS_PSGS( "Effect", get_Effect, put_Effect, DefaultPropsFlags() ),
    JS_PSGS( "Playlist", DummyGetter, put_Playlist, DefaultPropsFlags() ),
    JS_PSGS( "Text", DummyGetter, put_Text, DefaultPropsFlags() ),
    JS_PSGS( "ToSelect", DummyGetter, put_ToSelect, DefaultPropsFlags() ),
    JS_PS_END
};

const JSFunctionSpec jsFunctions[] = {
    JS_FS_END
};

} // namespace

namespace mozjs
{

const JSClass JsDropSourceAction::JsClass = jsClass;
const JSFunctionSpec* JsDropSourceAction::JsFunctions = jsFunctions;
const JSPropertySpec* JsDropSourceAction::JsProperties = jsProperties;
const JsPrototypeId JsDropSourceAction::PrototypeId = JsPrototypeId::DropSourceAction;

JsDropSourceAction::JsDropSourceAction( JSContext* cx )
    : pJsCtx_( cx )
{
}

JsDropSourceAction::~JsDropSourceAction()
{
}

std::unique_ptr<mozjs::JsDropSourceAction>
JsDropSourceAction::CreateNative( JSContext* cx )
{
    return std::unique_ptr<JsDropSourceAction>( new JsDropSourceAction( cx ) );
}

size_t JsDropSourceAction::GetInternalSize()
{
    return 0;
}

smp::panel::DropActionParams& JsDropSourceAction::GetDropActionParams()
{
    return actionParams_;
}

uint32_t JsDropSourceAction::get_Effect()
{
    return actionParams_.effect;
}

void JsDropSourceAction::put_Base( uint32_t base )
{
    actionParams_.base = base;
}

void JsDropSourceAction::put_Effect( uint32_t effect )
{
    actionParams_.effect = effect;
}

void JsDropSourceAction::put_Playlist( int32_t id )
{
    actionParams_.playlistIdx = id;
}

void JsDropSourceAction::put_Text( const std::wstring& text )
{
    actionParams_.text = text;
}

void JsDropSourceAction::put_ToSelect( bool toSelect )
{
    actionParams_.toSelect = toSelect;
}

} // namespace mozjs
