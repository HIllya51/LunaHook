 
#include <CommCtrl.h>
#include <TlHelp32.h>
#include<stdio.h>

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
    for(auto pid:attachedprocess){
        Host::DetachProcess(pid);
    }
}
void LunaHost::on_size(int w,int h){
    int height = h-140;
    g_hListBox_listtext->setgeo(10, 110, w - 20, height/2);
    g_showtexts->setgeo(10, 120+height/2, w - 20, height/2);
}

LunaHost::LunaHost(){
    settext(WndLunaHostGui);
    g_selectprocessbutton =new button(this,BtnSelectProcess,780, 10, 200, 40) ; 
        
    g_hEdit_userhook = new textedit(this,L"",10, 60, 600, 40,ES_AUTOHSCROLL);

    g_hButton_insert = new button(this,BtnInsertUserHook,610, 60, 200, 40) ;

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
    g_check_clipboard =new checkbox(this,BtnToClipboard,550, 10, 200, 40) ;
    g_check_clipboard->onclick=[&](){
        check_toclipboard=g_check_clipboard->ischecked();
    };
    new label(this,LblFlushDelay,10, 10, 150, 40);
    new label(this,LblCodePage,270, 10, 150, 40);

    g_timeout = new textedit(this,std::to_wstring(TextThread::flushDelay).c_str(),160, 10, 100, 40) ;
    g_codepage = new textedit(this,L"932",420, 10, 100, 40) ;
    
    g_timeout->ontextchange=[&](const std::wstring &text){
        TextThread::flushDelay=std::stoi(text);
    };
    g_codepage->ontextchange=[&](const std::wstring &text){
        try {
            auto cp=std::stoi(text);
            if(IsValidCodePage(cp))
                Host::defaultCodepage= cp; 
        }
        catch (const std::invalid_argument& e) { 
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

        int handle = g_hListBox_listtext->getdata(g_hListBox_listtext->currentidx());
        switch (LOWORD(wparam)) {

            case IDM_COPY_HOOKCODE:
                toclipboard(std::wstring(savehooks[handle]->hp.hookcode));
                break;
            case IDM_DETACH_PROCESS:
                Host::DetachProcess(savehooks[handle]->tp.processId);
                break;
            case IDM_REMOVE_HOOK:
                Host::RemoveHook(savehooks[handle]->tp.processId,savehooks[handle]->tp.addr);
                break;
        }
    };
    g_showtexts = new textedit(this,L"",10, 330, 200, 200,ES_READONLY|ES_MULTILINE |ES_AUTOVSCROLL| WS_VSCROLL);
        
    Host::Start(
        [&](DWORD pid) {attachedprocess.push_back(pid);}, 
        [&](DWORD pid) { 
            attachedprocess.erase(std::remove(attachedprocess.begin(), attachedprocess.end(), pid), attachedprocess.end());
        }, 
        [&](TextThread& thread) {
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
            savehooks.insert(std::make_pair(thread.handle,&thread));
        }, 
        [&](TextThread& thread) {
            int count = g_hListBox_listtext->count();
            for (int i = 0; i < count; i++) {
                uint64_t handle = g_hListBox_listtext->getdata(i);
                
                if (handle== thread.handle) { 
                    g_hListBox_listtext->deleteitem(i);
                    break;
                }
            } 
        }, 
        [&](TextThread& thread, std::wstring& output)
            { 
                std::lock_guard _(settextmutex);
                std::wstring lfoutput=output;
                strReplace(lfoutput,L"\n",L"\r\n");
                savetext.at(thread.handle).push_back(lfoutput);
                if(currentselect==thread.handle){ 
                    g_showtexts->scrolltoend();
                    g_showtexts->appendtext(lfoutput);
                    if(check_toclipboard)
                        toclipboard(output);
                }
                return false;
        }
    ); 
}