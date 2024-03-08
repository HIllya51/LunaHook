try:
    import os
    import renpy
    def hook_renpy_text_font_get_font_init(original):
        def new_init(*args, **kwargs):
            #ctypes.windll.user32.MessageBoxW(None, str(kwargs), str(args), 0)
            if os.path.exists(r'C:\Windows\Fonts\msyh.ttc'):
                font='msyh.ttc'
            elif os.path.exists(r'C:\Windows\Fonts\arial.ttf'):
                font='arial.ttf'
            else:
                font=None
            if font:
                args=(font,)+args[1:]
                if 'fn' in kwargs:
                    kwargs['fn']=font
            return original(*args, **kwargs)

        return new_init
    if "original_renpy_text_font_get_font" not in globals():
        original_renpy_text_font_get_font = renpy.text.font.get_font
    renpy.text.font.get_font = hook_renpy_text_font_get_font_init(original_renpy_text_font_get_font)

except:
    pass