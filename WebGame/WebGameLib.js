mergeInto(LibraryManager.library,
{
    WG_IsWebGL2Supported: function ()
    {
        var c = document.createElement('canvas');
        var gl2 = c.getContext("webgl2", null);
        var result = gl2 != null;
        delete c;
        return result;
    },
    WG_LoadAndCompileShader: function (handle, type, id)
    {
        var LoadShaderImpl = GetFunc_LoadAndCompileShader();
        return LoadShaderImpl(handle, type, id);
    },
    WG_PrintTest: function (a)
    {
        console.log(UTF8ToString(a));
    },
})