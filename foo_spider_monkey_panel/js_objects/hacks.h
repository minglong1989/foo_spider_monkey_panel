#pragma once

#include <js_objects/object_base.h>

#include <optional>
#include <string>
#include <memory>


namespace mozjs
{

class JsFbMetadbHandle;

class JsHacks
    : public JsObjectBase<JsHacks>
{
public:
    static constexpr bool HasProto = false;
    static constexpr bool HasProxy = false;
    static constexpr bool HasPostCreate = false;

    static const JSClass JsClass;
    static const JSFunctionSpec* JsFunctions;
    static const JSPropertySpec* JsProperties;

public:
    ~JsHacks();

    static std::unique_ptr<JsHacks> CreateNative( JSContext* cx );
    static size_t GetInternalSize();

public:
    std::optional<JSObject*> GetFbWindow();

private:
    JsHacks( JSContext* cx );

private:
    JSContext * pJsCtx_ = nullptr;
};

}

