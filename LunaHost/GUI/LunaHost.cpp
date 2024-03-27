 
#include <CommCtrl.h>
#include <TlHelp32.h>
#include<stdio.h>
#include<thread>
#include<fstream>
#include"host.h"
#include"hookcode.h"
#include"textthread.h"
#include"LunaHost.h"
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
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
}
void LunaHost::on_size(int w,int h){
    int height = h-110;
    w-=20;
    auto _w=w-20;
    
    g_selectprocessbutton->setgeo(10,10,_w/4,30);
    btndetachall->setgeo(10+10+_w/4,10,_w/4,30);
    //btnsavehook->setgeo(10+20+_w*2/4,10,_w/4,30);
    btnshowsettionwindow->setgeo(10+30+_w*3/4,10,_w/4,30);
    g_hListBox_listtext->setgeo(10, 90, w , height/2);
    g_showtexts->setgeo(10, 100+height/2, w , height/2);
    g_hEdit_userhook->setgeo(10,50,_w*2/4+10,30);
    g_hButton_insert->setgeo(10+20+_w*2/4,50,_w/4,30);
    btnplugin->setgeo(10+30+_w*3/4,50,_w/4,30);
}
void LunaHost::savesettings()
{
    configs->set("ToClipboard",check_toclipboard);
    configs->set("AutoAttach",autoattach);
    configs->set("AutoAttach_SavedOnly",autoattach_savedonly);
    configs->set("flushDelay",TextThread::flushDelay);
    configs->set("filterRepetition",TextThread::filterRepetition);
    configs->set("maxBufferSize",TextThread::maxBufferSize);
    configs->set("defaultCodepage",Host::defaultCodepage);
    configs->set("autoattachexes",autoattachexes);
    configs->set("savedhookcontext",savedhookcontext);
}
void LunaHost::loadsettings(){
    check_toclipboard=configs->get("ToClipboard",false);
    autoattach=configs->get("AutoAttach",false);
    autoattach_savedonly=configs->get("AutoAttach_SavedOnly",true);
    TextThread::flushDelay=configs->get("flushDelay",TextThread::flushDelay);
    TextThread::filterRepetition=configs->get("filterRepetition",TextThread::filterRepetition);
    TextThread::maxBufferSize=configs->get("maxBufferSize",TextThread::maxBufferSize);
    Host::defaultCodepage=configs->get("defaultCodepage",Host::defaultCodepage);
    autoattachexes=configs->get("autoattachexes",std::set<std::string>{});
    savedhookcontext=configs->get("savedhookcontext",decltype(savedhookcontext){});
}

std::unordered_map<std::wstring,std::vector<int>> getprocesslist();
void LunaHost::doautoattach()
{
    
    if(autoattach==false&&autoattach_savedonly==false)return;
    
    if(autoattachexes.empty())return;
    
    for(auto [pexe,pids ]:getprocesslist())
    {
        auto&& u8procname=WideStringToString(pexe);
        if(autoattachexes.find(u8procname)==autoattachexes.end())continue;
        if(autoattach_savedonly && savedhookcontext.find(u8procname)==savedhookcontext.end())continue;
        for(auto pid:pids){
            if(userdetachedpids.find(pid)!=userdetachedpids.end())continue;
            
            if(attachedprocess.find(pid)==attachedprocess.end())
                Host::InjectProcess(pid);
        }
            
        break; 
    } 
    
}

void LunaHost::on_proc_connect(DWORD pid)
{
    attachedprocess.insert(pid);
    
    if(auto pexe=GetModuleFilename(pid))
    {
        autoattachexes.insert(WideStringToString(pexe.value()));
    }
}

LunaHost::LunaHost(){
    
    configs=new confighelper;
    loadsettings();

    std::thread([&]{
        while(1){
            doautoattach();
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    }).detach();

    setfont(25);
    settext(WndLunaHostGui);
    btnshowsettionwindow=new button(this, BtnShowSettingWindow,100,100,100,100);
    g_selectprocessbutton =new button(this,BtnSelectProcess,830, 10, 200, 40) ; 
    
    // btnsavehook=new button(this,BtnSaveHook,10,10,10,10);
    // btnsavehook->onclick=std::bind(&LunaHost::btnsavehookscallback,this);
    btndetachall=new button(this,BtnDetach,10,10,10,10);
    btndetachall->onclick=[&](){
        for(auto pid:attachedprocess){
            Host::DetachProcess(pid);
            userdetachedpids.insert(pid);
        }
    };

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
#define IDM_REMEMBER_SELECTION 1004
#define IDM_FORGET_SELECTION 1005
    g_hListBox_listtext->oncontextmenu=[](){
        HMENU hMenu = CreatePopupMenu();
        AppendMenu(hMenu, MF_STRING, IDM_COPY_HOOKCODE, MenuCopyHookCode);
        AppendMenu(hMenu, MF_STRING, IDM_REMOVE_HOOK, MenuRemoveHook);
        AppendMenu(hMenu, MF_STRING, IDM_DETACH_PROCESS, MenuDetachProcess);
        AppendMenu(hMenu, MF_STRING, IDM_REMEMBER_SELECTION, MenuRemeberSelect);
        AppendMenu(hMenu, MF_STRING, IDM_FORGET_SELECTION, MenuForgetSelect);
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
                userdetachedpids.insert(tt->tp.processId);
                break;
            case IDM_REMOVE_HOOK:
                Host::RemoveHook(tt->tp.processId,tt->tp.addr);
                break;
            case IDM_FORGET_SELECTION:
            case IDM_REMEMBER_SELECTION:
            {
                            
                std::vector<wchar_t> buffer(MAX_PATH);
                auto gmf=[&](DWORD processId)->std::optional<std::wstring>{
                    std::vector<wchar_t> buffer(MAX_PATH);
                    if (AutoHandle<> process = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, processId))
                        if (GetModuleFileNameExW(process, 0, buffer.data(), MAX_PATH)) return buffer.data();
                    return {};
                };
                //见鬼了，GetModuleFileName找不到标识符
                
                if(auto pexe=gmf(tt->tp.processId))
                {
                    auto pexev=WideStringToString(pexe.value());
                    if(LOWORD(wparam)==IDM_REMEMBER_SELECTION)
                    {
                            
                        savedhookcontext[pexev]={
                            {"hookcode",WideStringToString(tt->hp.hookcode)},
                            {"ctx1",tt->tp.ctx},
                            {"ctx2",tt->tp.ctx2},
                        };
                    }
                    else if(LOWORD(wparam)==IDM_FORGET_SELECTION)
                    {
                        savedhookcontext.erase(pexev);
                    }
                }
            }
            break;
        }
    };
    g_showtexts = new textedit(this,L"",10, 330, 200, 200,ES_MULTILINE |ES_AUTOVSCROLL| WS_VSCROLL);
    g_showtexts->setreadonly(true);
             
    Host::Start(
        std::bind(&LunaHost::on_proc_connect,this,std::placeholders::_1),
        [&](DWORD pid) { 
            attachedprocess.erase(pid);
        }, 
        std::bind(&LunaHost::on_thread_create,this,std::placeholders::_1),
        std::bind(&LunaHost::on_thread_delete,this,std::placeholders::_1),
        std::bind(&LunaHost::on_text_recv,this,std::placeholders::_1,std::placeholders::_2)
    ); 
    
    setcentral(1200,800);
}
void LunaHost::on_text_recv_checkissaved(TextThread& thread)
{

    if(onceautoselectthread.find(thread.handle)!=onceautoselectthread.end())return;
    
    onceautoselectthread.insert(thread.handle);
        
    if(auto exe=GetModuleFilename(thread.tp.processId))
    {
        auto exea=WideStringToString(exe.value());
        if(savedhookcontext.find(exea)==savedhookcontext.end())return;
        
        std::string hc= savedhookcontext[exea]["hookcode"];
        uint64_t ctx1=savedhookcontext[exea]["ctx1"];
        uint64_t ctx2=savedhookcontext[exea]["ctx2"];
        if(((ctx1&0xffff)==(thread.tp.ctx&0xffff) )&& (ctx2==thread.tp.ctx2) && (hc==WideStringToString(thread.hp.hookcode)))
        {
            for(int i=0;i<g_hListBox_listtext->count();i++)
            {
                auto handle=g_hListBox_listtext->getdata(i);
                if(handle==thread.handle)
                {
                    g_hListBox_listtext->setcurrent(i);
                    break;
                }
            }
            
        }

    }
            
}
bool LunaHost::on_text_recv(TextThread& thread, std::wstring& output){
    std::lock_guard _(settextmutex);
    on_text_recv_checkissaved(thread);
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
    int labelwidth=300;
    g_timeout = new spinbox(this,std::to_wstring(TextThread::flushDelay),space+labelwidth, curry, 100, height) ;
    new label(this,LblFlushDelay,10, curry, labelwidth, height);curry+=height+space;
    
    g_codepage = new spinbox(this,std::to_wstring(Host::defaultCodepage),space+labelwidth, curry, 100, height) ;
    new label(this,LblCodePage,10, curry, labelwidth, height);curry+=height+space;
    
    spinmaxbuffsize = new spinbox(this,std::to_wstring(TextThread::maxBufferSize),space+labelwidth, curry, 100, height) ;
    new label(this,LblMaxBuff,10, curry, labelwidth, height);curry+=height+space;

    spinmaxbuffsize->onvaluechange=[=](int v){
        TextThread::maxBufferSize=v;
    };
    spinmaxbuffsize->setminmax(0,0x7fffffff);

    ckbfilterrepeat=new checkbox(this,LblFilterRepeat,10, curry, labelwidth, height);curry+=height+space;
    ckbfilterrepeat->onclick=[=](){
        TextThread::filterRepetition=ckbfilterrepeat->ischecked();
    };
    ckbfilterrepeat->setcheck(TextThread::filterRepetition);

    g_check_clipboard =new checkbox(this,BtnToClipboard,10, curry, labelwidth, height) ;curry+=height+space;
    g_check_clipboard->onclick=[=](){
        host->check_toclipboard=g_check_clipboard->ischecked();
    }; 
    g_check_clipboard->setcheck(host->check_toclipboard);

    autoattach =new checkbox(this,LblAutoAttach,10, curry, labelwidth, height) ;curry+=height+space;
    autoattach->onclick=[=](){
        host->autoattach=autoattach->ischecked();
    }; 
    autoattach->setcheck(host->autoattach);

    autoattach_so =new checkbox(this,LblAutoAttach_savedonly,10, curry, labelwidth, height) ;curry+=height+space;
    autoattach_so->onclick=[=](){
        host->autoattach_savedonly=autoattach_so->ischecked();
    }; 
    autoattach_so->setcheck(host->autoattach_savedonly);


    readonlycheck=new checkbox(this,BtnReadOnly,10,curry,labelwidth,height);curry+=height+space;
    readonlycheck->onclick=[=](){
        host->g_showtexts->setreadonly(readonlycheck->ischecked());
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
    setcentral(500,500);
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