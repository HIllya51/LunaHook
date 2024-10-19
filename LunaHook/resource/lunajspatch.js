var fontface = '';
var magicsend = '\x01LUNAFROMJS\x01'
var magicrecv = '\x01LUNAFROMHOST\x01'
var is_packed = IS_PACKED
var is_useclipboard = IS_USECLIPBOARD
var internal_http_port = INTERNAL_HTTP_PORT
function splitfonttext(transwithfont) {
    if (transwithfont.substr(0, magicsend.length) == magicsend) //not trans
    {
        split = transwithfont.search('\x02')
        return transwithfont.substr(split + 1);
    }
    else if (transwithfont.substr(0, magicrecv.length) == magicrecv) {
        transwithfont = transwithfont.substr(magicrecv.length)
        //magic font \x02 text
        split = transwithfont.search('\x02')
        fontface = transwithfont.substr(0, split)
        text = transwithfont.substr(split + 1)
        return text;
    }
    else {
        return transwithfont;
    }
}
function syncquery(s) {
    if (internal_http_port == 0) { throw new Error('') }
    var xhr = new XMLHttpRequest();
    var url = 'http://127.0.0.1:' + internal_http_port + '/fuck?' + s
    xhr.open('GET', url, false);
    xhr.send();
    if (xhr.status === 200) {
        return xhr.responseText;//解析这个会导致v8::String::Length的v8StringUtf8Length出现错误，但不影响。
    } else {
        throw new Error('')
    }

}
function makecomplexs(name, s_raw, lpsplit) {
    return magicsend + name + '\x03' + lpsplit.toString() + '\x02' + s_raw;
}
function cppjsio(s) {
    try {
        return syncquery(s)
    }
    catch (err) {
        try {
            if (!is_useclipboard) { throw new Error('') }
            const _clipboard = require('nw.gui').Clipboard.get();
            _clipboard.set(s, 'text');
            return _clipboard.get('text');
        }
        catch (err2) {
            try {
                if (!is_useclipboard) { throw new Error('') }
                const clipboard = require('electron').clipboard;
                clipboard.writeText(s);
                return clipboard.readText();
            }
            catch (err3) {
                return s_raw;
            }
        }
    }
}
function clipboardsender(name, s_raw, lpsplit) {
    //magic split \x02 text
    if (!s_raw)
        return s_raw
    transwithfont = cppjsio(makecomplexs(name, s_raw, lpsplit))
    if (transwithfont.length == 0) return s_raw;
    return splitfonttext(transwithfont)
}

function clipboardsender_only_send(name, s_raw, lpsplit) {
    //magic split \x02 text
    if (!s_raw)
        return s_raw
    cppjsio(makecomplexs(name, s_raw, lpsplit))
}
function rpgmakerhook() {

    if (Window_Message.prototype.originstartMessage) { }
    else {
        Window_Base.prototype.drawTextEx_origin = Window_Base.prototype.drawTextEx;
        Window_Base.prototype.drawText_origin = Window_Base.prototype.drawText;
        Window_Message.prototype.originstartMessage = Window_Message.prototype.startMessage;

        Bitmap.prototype.drawText_ori = Bitmap.prototype.drawText;
        Bitmap.prototype.last_y = 0;

        Bitmap.prototype.origin_makeFontNameText = Bitmap.prototype._makeFontNameText;
    }
    Bitmap.prototype._makeFontNameText = function () {
        if (!fontface) return this.origin_makeFontNameText();
        return (this.fontItalic ? 'Italic ' : '') +
            this.fontSize + 'px ' + fontface;
    }
    Bitmap.prototype.collectstring = { 2: '', 5: '', 6: '' };
    setInterval(function () {
        for (lpsplit in Bitmap.prototype.collectstring) {
            if (Bitmap.prototype.collectstring[lpsplit].length) {
                clipboardsender_only_send('rpgmakermv', Bitmap.prototype.collectstring[lpsplit], lpsplit)
                Bitmap.prototype.collectstring[lpsplit] = ''
            }
        }
    }, 100);
    if (!is_packed) {

        Bitmap.prototype.drawText = function (text, x, y, maxWidth, lineHeight, align) {
            //y>100的有重复；慢速是单字符，快速是多字符
            if (text && (y < 100)) {
                extra = 5 + ((text.length == 1) ? 0 : 1);
                if (y != Bitmap.prototype.last_y) {
                    Bitmap.prototype.collectstring[extra] += '\n'
                }
                Bitmap.prototype.collectstring[extra] += text;
                Bitmap.prototype.last_y = y;
            }
            return this.drawText_ori(text, x, y, maxWidth, lineHeight, align);
        }
    }
    Window_Message.prototype.startMessage = function () {
        gametext = $gameMessage.allText();
        resp = clipboardsender('rpgmakermv', gametext, 0);
        $gameMessage._texts = [resp]
        this.originstartMessage();
    };
    Window_Base.prototype.drawText = function (text, x, y, maxWidth, align) {
        text = clipboardsender('rpgmakermv', text, 1)
        return this.drawText_origin(text, x, y, maxWidth, align)
    }
    Window_Base.prototype.lastcalltime = 0
    Window_Base.prototype.drawTextEx = function (text, x, y) {
        __last = Window_Base.prototype.lastcalltime
        __now = new Date().getTime()
        Window_Base.prototype.lastcalltime = __now
        if (__now - __last > 100)
            text = clipboardsender('rpgmakermv', text, 2)
        else {
            Bitmap.prototype.collectstring[2] += text;
        }
        return this.drawTextEx_origin(text, x, y)
    }
}

function tyranohook() {

    if (tyrano.plugin.kag.tag.text.originstart) return;
    tyrano.plugin.kag.tag.text.originstart = tyrano.plugin.kag.tag.text.start;
    tyrano.plugin.kag.tag.chara_ptext.startorigin = tyrano.plugin.kag.tag.chara_ptext.start;
    tyrano.plugin.kag.tag.text.start = function (pm) {
        if (1 != this.kag.stat.is_script && 1 != this.kag.stat.is_html) {
            pm.val = clipboardsender('tyranoscript', pm.val, 0);
            if (fontface) {
                this.kag.stat.font.face = fontface
            }
        }
        return this.originstart(pm)
    }
    tyrano.plugin.kag.tag.chara_ptext.start = function (pm) {
        pm.name = clipboardsender('tyranoscript', pm.name, 1)
        return this.startorigin(pm)
    }
}
function retryinject(times) {
    if (times == 0) return;
    try {
        if (window.tyrano && tyrano.plugin) {
            tyranohook();
        }
        else if (window.Utils && Utils.RPGMAKER_NAME) {
            rpgmakerhook();
        }
        else {
            setTimeout(retryinject, 3000, times - 1);
        }
    }
    catch (err) {
        //非主线程，甚至没有window对象，会弹窗报错
    }
}
retryinject(3)