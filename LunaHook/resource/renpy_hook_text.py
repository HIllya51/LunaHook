def callLunaHost(text,split):
    try:
        import ctypes
        try:
            internal_renpy_call_host=ctypes.CDLL('LunaHook64').internal_renpy_call_host
        except:
            internal_renpy_call_host=ctypes.CDLL('LunaHook32').internal_renpy_call_host
        internal_renpy_call_host.argstype=ctypes.c_wchar_p,ctypes.c_int
        internal_renpy_call_host.restype=ctypes.c_wchar_p
        if isinstance(text,str):
            try:
                _text=text.decode('utf8')
            except:
                _text=text
            text=internal_renpy_call_host(_text,split)
    except:
        pass
    return text
try:
    #6.1.0
    import renpy
    def hook_initT0(original_init):

        def new_init(self, *args, **kwargs):
            
            if isinstance(args[0], list):
                trs = callLunaHost((args[0][0]),1)
            else:
                trs = callLunaHost((args[0]),1)
            
            nargs = (trs,) + args[1:]
            if 'text' in kwargs:
                kwargs['text'] = nargs[0]
            self.mtoolHooked = True

            original_init(self, *nargs, **kwargs)

        return new_init

    if "original_Text_init_hook" not in globals():
        original_Text_init_hook = renpy.text.text.Text.__init__

    renpy.text.text.Text.__init__ = hook_initT0(original_Text_init_hook)

    def hook_init_renderT0(original):
        def new_init(self, *args, **kwargs):
            if not hasattr(self, "LunaHooked"):
                if isinstance(self.text, list):
                    trs = callLunaHost(str(self.text[0]),2)
                else:
                    trs = callLunaHost(str(self.text),2)
                self.set_text(trs)
                self.LunaHooked = True
            return original(self, *args, **kwargs)

        return new_init


    if "original_hook_init_renderT0" not in globals():
        original_hook_init_renderT0 = renpy.text.text.Text.render

    renpy.text.text.Text.render = hook_init_renderT0(original_hook_init_renderT0)
except:
    pass
try:
    #4.0
    import renpy
    def hook_initT3(original_init):
        def new_init(self, *args, **kwargs):
            trs = callLunaHost(str(args[0]),3)
            
            nargs = (trs,) + args[1:]
            original_init(self, *nargs, **kwargs)

        return new_init


    if "original_Text_init_hookT3" not in globals():
        original_Text_init_hookT3 = renpy.exports.Text.__init__

    renpy.exports.Text.__init__ = hook_initT3(original_Text_init_hookT3)
except:
    pass