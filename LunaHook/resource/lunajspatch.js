

function NWjshook(){
    function NWjssend(s) {
        const _clipboard = require('nw.gui').Clipboard.get();
        _clipboard.set(s, 'text'); 
        return _clipboard.get('text')
    }
    if(Window_Message.prototype.originstartMessage)return;
    Window_Message.prototype.originstartMessage=Window_Message.prototype.startMessage;
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
        const { clipboard } = require('electron');
        clipboard.writeText(s);
        return clipboard.readText();
    }
    if(tyrano.plugin.kag.tag.text.originshowMessage)return;
    tyrano.plugin.kag.tag.text.originshowMessage=tyrano.plugin.kag.tag.text.showMessage;
    tyrano.plugin.kag.tag.text.showMessage = function () {
        arguments[0]=Electronsend(arguments[0]);
        return tyrano.plugin.kag.tag.text.originshowMessage.apply(this, arguments);
    }
    
}
function retryinject(times){
    if(times==0)return;
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
setTimeout(retryinject,3000,3);