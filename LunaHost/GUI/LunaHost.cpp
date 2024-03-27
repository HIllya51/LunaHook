 
#include <CommCtrl.h>
#include <TlHelp32.h>
#include<stdio.h>
#include<fstream>
#include"host.h"
#include"hookcode.h"
#include"textthread.h"
#include"LunaHost.h"
#include"processlistwindow.h"
#include"Lang/Lang.h"
void LunaHost::toclipboard(std::wstring& sentence){ 
     
    for (int loop = 0; loop < 10; loop++) {
        if (OpenClipboard(winId)) {
            HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, (sentence.size() + 2) * sizeof(wchar_t));
            memcpy(GlobalLock(hMem), sentence.c_str(), (sentence.size() + 2) * sizeof(wchar_t));
            EmptyClipboard();
            SetClipboardData(CF_UNICODETEXT, hMem);
            GlobalUnlock(hMem);
            CloseClipboard();
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    } 
} 
void LunaHost::on_close(){
    savesettings();
    delete configs;
    for(auto pid:attachedprocess){
        Host::DetachProcess(pid);
    }
    if(attachedprocess.size())
        Sleep(100);
}
void LunaHost::on_size(int w,int h){
    int height = h-110;
    w-=20;
    auto _w=w-20;
    g_selectprocessbutton->setgeo(10,10,_w/3,30);
    btnshowsettionwindow->setgeo(10+10+_w/3,10,_w/3,30);
    btnplugin->setgeo(10+20+_w*2/3,10,_w/3,30);
    g_hListBox_listtext->setgeo(10, 90, w , height/2);
    g_showtexts->setgeo(10, 100+height/2, w , height/2);
    g_hEdit_userhook->setgeo(10,50,_w*2/3+10,30);
    g_hButton_insert->setgeo(10+20+_w*2/3,50,_w/3,30);
}
void LunaHost::savesettings()
{
    configs->set("ToClipboard",check_toclipboard);
    configs->set("AutoAttach",autoattach);
    configs->set("flushDelay",TextThread::flushDelay);
    configs->set("filterRepetition",TextThread::filterRepetition);
    configs->set("maxBufferSize",TextThread::maxBufferSize);
    configs->set("defaultCodepage",Host::defaultCodepage);
}
void LunaHost::loadsettings(){
    check_toclipboard=configs->get("ToClipboard",false);
    autoattach=configs->get("AutoAttach",false);
    TextThread::flushDelay=configs->get("flushDelay",TextThread::flushDelay);
    TextThread::filterRepetition=configs->get("filterRepetition",TextThread::filterRepetition);
    TextThread::maxBufferSize=configs->get("maxBufferSize",TextThread::maxBufferSize);
    Host::defaultCodepage=configs->get("defaultCodepage",Host::defaultCodepage);
}
LunaHost::LunaHost(){
    loadsettings();
    setfont(25);
    configs=new confighelper;
    settext(WndLunaHostGui);
    btnshowsettionwindow=new button(this, BtnShowSettingWindow,100,100,100,100);
    g_selectprocessbutton =new button(this,BtnSelectProcess,830, 10, 200, 40) ; 
        
    g_hEdit_userhook = new textedit(this,L"",10, 60, 600, 40,ES_AUTOHSCROLL);
    btnplugin=new button(this,BtnPlugin,830,60,200,40);
    
    plugins=new Pluginmanager(this);
    btnplugin->onclick=[&](){
        if(pluginwindow==0) pluginwindow=new Pluginwindow(this,plugins);
        pluginwindow->show();
    };
    g_hButton_insert = new button(this,BtnInsertUserHook,610, 60, 200, 40) ;
    btnshowsettionwindow->onclick=[&](){
        if(settingwindow==0) settingwindow=new Settingwindow(this);
        settingwindow->show();
    };
    g_selectprocessbutton->onclick=[&](){
        if(_processlistwindow==0) _processlistwindow=new processlistwindow(this);
        _processlistwindow->show();
    };
    g_hButton_insert->onclick=[&](){
        auto hp = HookCode::Parse(std::move(g_hEdit_userhook->text()));
        if(hp){
            for(auto _:attachedprocess){
                Host::InsertHook(_,hp.value()); 
            }
        }
        else{
            g_showtexts->appendtext(NotifyInvalidHookCode);
        }
    };

    g_hListBox_listtext = new listbox(this,10, 120, 200, 200);
    g_hListBox_listtext->oncurrentchange=[&](int idx){
        uint64_t handle = g_hListBox_listtext->getdata(idx);
        std::wstring get;
        for(auto& _:savetext.at(handle)){
            get+=_;
            get+=L"\r\n";
        }
        currentselect=handle;
        g_showtexts->settext(get);
        g_showtexts->scrolltoend();
    };
    
#define IDM_REMOVE_HOOK 1001
#define IDM_DETACH_PROCESS 1002
#define IDM_COPY_HOOKCODE 1003
    g_hListBox_listtext->oncontextmenu=[](){
        HMENU hMenu = CreatePopupMenu();
        AppendMenu(hMenu, MF_STRING, IDM_COPY_HOOKCODE, MenuCopyHookCode);
        AppendMenu(hMenu, MF_STRING, IDM_REMOVE_HOOK, MenuRemoveHook);
        AppendMenu(hMenu, MF_STRING, IDM_DETACH_PROCESS, MenuDetachProcess);
        return hMenu;
    };
    g_hListBox_listtext->oncontextmenucallback=[&](WPARAM wparam){

        auto handle = g_hListBox_listtext->getdata(g_hListBox_listtext->currentidx());
        auto tt=Host::GetThread(handle);
        if(tt==0)return;
        switch (LOWORD(wparam)) {

            case IDM_COPY_HOOKCODE:
                toclipboard(std::wstring(tt->hp.hookcode));
                break;
            case IDM_DETACH_PROCESS:
                Host::DetachProcess(tt->tp.processId);
                break;
            case IDM_REMOVE_HOOK:
                Host::RemoveHook(tt->tp.processId,tt->tp.addr);
                break;
        }
    };
    g_showtexts = new textedit(this,L"",10, 330, 200, 200,ES_MULTILINE |ES_AUTOVSCROLL| WS_VSCROLL);
    g_showtexts->setreadonly(true);
             
    Host::Start(
        [&](DWORD pid) {attachedprocess.push_back(pid);}, 
        [&](DWORD pid) { 
            attachedprocess.erase(std::remove(attachedprocess.begin(), attachedprocess.end(), pid), attachedprocess.end());
        }, 
        std::bind(&LunaHost::on_thread_create,this,std::placeholders::_1),
        std::bind(&LunaHost::on_thread_delete,this,std::placeholders::_1),
        std::bind(&LunaHost::on_text_recv,this,std::placeholders::_1,std::placeholders::_2)
    ); 
    
    setcentral(1000,600);
}

bool LunaHost::on_text_recv(TextThread& thread, std::wstring& output){
    std::lock_guard _(settextmutex);
    if(!plugins->dispatch(thread,output))return false;
    strReplace(output,L"\n",L"\r\n");
    savetext.at(thread.handle).push_back(output);
    if(currentselect==thread.handle){ 
        g_showtexts->scrolltoend();
        g_showtexts->appendtext(output);
    }
    return true;
}
void LunaHost::on_thread_create(TextThread& thread){
    wchar_t buff[65535];
    swprintf_s(buff,L"[%I64X:%I32X:%I64X:%I64X:%I64X:%s:%s]",
        thread.handle,
        thread.tp.processId,
        thread.tp.addr,
        thread.tp.ctx,
        thread.tp.ctx2,
        thread.name.c_str(),
        thread.hp.hookcode 
    );
    savetext.insert({thread.handle,{}});
    int index=g_hListBox_listtext->additem(buff); 
    g_hListBox_listtext->setdata(index,thread.handle);
}
void LunaHost::on_thread_delete(TextThread& thread){
    int count = g_hListBox_listtext->count();
    for (int i = 0; i < count; i++) {
        uint64_t handle = g_hListBox_listtext->getdata(i);
        
        if (handle== thread.handle) { 
            g_hListBox_listtext->deleteitem(i);
            break;
        }
    } 
}

Settingwindow::Settingwindow(LunaHost* host):mainwindow(host){
    int height=30;int curry=10;int space=10;
    int labelwidth=250;
    g_timeout = new spinbox(this,std::to_wstring(TextThread::flushDelay),space+labelwidth, curry, 100, height) ;
    new label(this,LblFlushDelay,10, curry, labelwidth, height);curry+=height+space;
    
    g_codepage = new spinbox(this,std::to_wstring(Host::defaultCodepage),space+labelwidth, curry, 100, height) ;
    new label(this,LblCodePage,10, curry, labelwidth, height);curry+=height+space;
    
    spinmaxbuffsize = new spinbox(this,std::to_wstring(TextThread::maxBufferSize),space+labelwidth, curry, 100, height) ;
    new label(this,LblMaxBuff,10, curry, labelwidth, height);curry+=height+space;

    spinmaxbuffsize->onvaluechange=[=](int v){
        TextThread::flushDelay=v;
    };
    spinmaxbuffsize->setminmax(0,0x7fffffff);

    ckbfilterrepeat=new checkbox(this,LblFilterRepeat,10, curry, labelwidth, height);curry+=height+space;
    ckbfilterrepeat->onclick=[=](){
        TextThread::filterRepetition=ckbfilterrepeat->ischecked();
    };
    ckbfilterrepeat->setcheck(TextThread::filterRepetition);

    g_check_clipboard =new checkbox(this,BtnToClipboard,10, curry, labelwidth, height) ;curry+=height+space;
    g_check_clipboard->onclick=[=](){
        ((LunaHost*)parent)->check_toclipboard=g_check_clipboard->ischecked();
    }; 
    g_check_clipboard->setcheck(host->check_toclipboard);

    autoattach =new checkbox(this,LblAutoAttach,10, curry, labelwidth, height) ;curry+=height+space;
    autoattach->onclick=[=](){
        ((LunaHost*)parent)->autoattach=autoattach->ischecked();
    }; 
    autoattach->setcheck(host->autoattach);


    readonlycheck=new checkbox(this,BtnReadOnly,10,curry,labelwidth,height);curry+=height+space;
    readonlycheck->onclick=[=](){
        ((LunaHost*)parent)->g_showtexts->setreadonly(readonlycheck->ischecked());
    };
    readonlycheck->setcheck(true);
    
    g_timeout->onvaluechange=[=](int v){
        TextThread::flushDelay=v;
    };
    g_timeout->setminmax(0,9999);
    g_codepage->onvaluechange=[=](int v){ 
            if(IsValidCodePage(v)){
                Host::defaultCodepage= v; 
            }
    }; 
    g_codepage->setminmax(0,CP_UTF8);
    setcentral(300,300);
    settext(WndSetting);
}
void Pluginwindow::on_size(int w,int h){
    listplugins->setgeo(10,80,w-20,h-90);
}
Pluginwindow::Pluginwindow(mainwindow*p,Pluginmanager* pl):mainwindow(p){
    pluginmanager=pl;
    new label(this,LblPluginNotify, 10,10,500,30);
    new label(this,LblPluginRemove, 10,40,500,30);
    static auto listadd=[&](const std::wstring& s){
        auto idx=listplugins->additem(std::filesystem::path(s).stem());
        auto _s=new wchar_t[s.size()+1];wcscpy(_s,s.c_str());
        listplugins->setdata(idx,(LONG_PTR)_s);
    };
    listplugins = new listbox(this,10, 10,360,340);
#define IDM_ADD_PLUGIN 1004
#define IDM_ADD_QT_PLUGIN 1005
#define IDM_REMOVE_PLUGIN 1006
#define IDM_RANK_UP 1007
#define IDM_RANK_DOWN 1008
    listplugins->oncontextmenu=[](){
        HMENU hMenu = CreatePopupMenu();
        AppendMenu(hMenu, MF_STRING, IDM_ADD_PLUGIN, MenuAddPlugin);
        AppendMenu(hMenu, MF_STRING, IDM_ADD_QT_PLUGIN, MenuAddQtPlugin);
        AppendMenu(hMenu, MF_STRING, IDM_REMOVE_PLUGIN, MenuRemovePlugin);
        AppendMenu(hMenu, MF_STRING, IDM_RANK_UP, MenuPluginRankUp);
        AppendMenu(hMenu, MF_STRING, IDM_RANK_DOWN, MenuPluginRankDown);
        return hMenu;
    };
    listplugins->oncontextmenucallback=[&](WPARAM wparam){

        switch (LOWORD(wparam)) {
            case IDM_RANK_DOWN:
            case IDM_RANK_UP:
            {
                auto idx=listplugins->currentidx();
                if(idx==-1)break;
                auto idx2=idx+(
                    (LOWORD(wparam)==IDM_RANK_UP)?-1:1
                );
                auto a=min(idx,idx2),b=max(idx,idx2);
                if(a<0||b>=listplugins->count())break;
                pluginmanager->swaprank(a,b);
                
                auto pa=((LPCWSTR)listplugins->getdata(a));
                auto pb=((LPCWSTR)listplugins->getdata(b));
                
                listplugins->deleteitem(a);
                listplugins->insertitem(b,std::filesystem::path(pa).stem());
                listplugins->setdata(b,(LONG_PTR)pa);
                break;
            }
            case IDM_ADD_PLUGIN:
            case IDM_ADD_QT_PLUGIN:
            {
                auto f=pluginmanager->selectpluginfile();
                if(f.has_value()){
                    if(pluginmanager->addplugin(f.value(),LOWORD(wparam)==IDM_ADD_QT_PLUGIN)){
                        listadd(f.value());
                    }
                }
                break;
            }
            case IDM_REMOVE_PLUGIN:
            {
                auto idx=listplugins->currentidx();
                if(idx==-1)break;
                pluginmanager->remove((LPCWSTR)listplugins->getdata(idx));
                listplugins->deleteitem(idx);
                break;
            }
        }
    };
    for(int i=0;i<pluginmanager->PluginRank.size();i++){
        listadd(pluginmanager->PluginRank[i]);
    }
    settext(WndPlugins);
    setcentral(500,400);
}