var fontface='';
function splitfonttext(transwithfont){
    if(transwithfont[0]=='\x01'){
        transwithfont=transwithfont.substr(1)
        split=transwithfont.search('\x01')
        if(split==-1)return transwithfont;
        fontface=transwithfont.substr(0,split)
        text=transwithfont.substr(split+1)
        return text;
    }
    else{
        return transwithfont;
    }
}
function NWjshook(){
    function NWjssend(s) {
        const _clipboard = require('nw.gui').Clipboard.get();
        _clipboard.set(s, 'text'); 
        transwithfont=_clipboard.get('text');
        if(transwithfont.length==0)return s;
        return splitfonttext(transwithfont)
    }
    
    if(Window_Message.prototype.originstartMessage)return;
    Window_Message.prototype.originstartMessage=Window_Message.prototype.startMessage;
    Bitmap.prototype.origin_makeFontNameText=Bitmap.prototype._makeFontNameText;
    Bitmap.prototype._makeFontNameText=function(){
        if(fontface=='')return this.origin_makeFontNameText();
        return (this.fontItalic ? 'Italic ' : '') +
            this.fontSize + 'px ' + fontface;
    }
    Window_Message.prototype.startMessage = function() 
    {
        gametext = $gameMessage.allText();
        resp=NWjssend(gametext);
        $gameMessage._texts=[resp]
        this.originstartMessage();
    };
}

function Electronhook() {
        
    function Electronsend(s) {
        const clipboard = require('electron').clipboard;
        clipboard.writeText(s);
        transwithfont= clipboard.readText();
        if(transwithfont.length==0)return s;
        return splitfonttext(transwithfont)
    }
    if(tyrano.plugin.kag.tag.text.originstart)return;
    tyrano.plugin.kag.tag.text.originstart=tyrano.plugin.kag.tag.text.start;
    tyrano.plugin.kag.tag.text.start = function (pm) {
        if (1 != this.kag.stat.is_script && 1 != this.kag.stat.is_html) {
			pm.val=Electronsend(pm.val);
            if(fontface!=''){
                this.kag.stat.font.face=fontface
            }
		}
        return this.originstart(pm)
    }
}
function retryinject(times){
    if(times==0)return;
    try{
        if(window.tyrano && tyrano.plugin){
            Electronhook();
        }
        else if(window.Utils && Utils.RPGMAKER_NAME){
            NWjshook();
        }
        else{ 
            setTimeout(retryinject,3000,times-1);
        }
    }
    catch(err){
        //非主线程，甚至没有window对象，会弹窗报错
    }
}
setTimeout(retryinject,1000,3);