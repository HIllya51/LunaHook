#include "livecaptions.h"

bool livecaptions::attach_function()
{

    //         .text:0000000180001C98                 push    rbx
    // .text:0000000180001C9A                 sub     rsp, 20h
    // .text:0000000180001C9E                 mov     rbx, rcx
    // .text:0000000180001CA1                 call    memmove_0
    // .text:0000000180001CA6                 mov     rax, rbx
    // .text:0000000180001CA9                 add     rsp, 20h
    // .text:0000000180001CAD                 pop     rbx
    // .text:0000000180001CAE                 retn
    HookParam hp;
    hp.address = (uintptr_t)GetProcAddress(GetModuleHandle(L"vcruntime140_app.dll"), "memmove");
    hp.text_fun = [](hook_stack *stack, HookParam *hp, uintptr_t *data, uintptr_t *split, size_t *len)
    {
        BYTE sig[] = {
            0x40, 0x53, 0x48, 0x83, 0xEC, 0x20, 0x48, 0x8B, 0xD9,
            0xE8, XX4};
        auto a1 = stack->retaddr - sizeof(sig);
        if ((stack->retaddr > (uintptr_t)GetModuleHandle(L"Microsoft.CognitiveServices.Speech.extension.embedded.sr.dll")))
            if (memcmp((void *)a1, &sig, sizeof(sig) - 4) == 0)
            {
                static std::set<uintptr_t> once;
                if (once.find(stack->retaddr) != once.end())
                    return;
                once.insert(stack->retaddr);
                // hp->text_fun=nullptr;
                // hp->type=HOOK_EMPTY;

                HookParam hpinternal;
                hpinternal.address = a1; // 0xE551+(uintptr_t)GetModuleHandle(L"Microsoft.CognitiveServices.Speech.extension.embedded.sr.dll");
                hpinternal.type = USING_STRING | CODEC_UTF8 | FULL_STRING;
                hpinternal.text_fun = [](hook_stack *stack, HookParam *hp, uintptr_t *data, uintptr_t *split, size_t *len)
                {
                    auto ptr = stack->rdx;
                    auto size = stack->r8;
                    if (size == strnlen((char *)ptr, TEXT_BUFFER_SIZE)) // 否则有短acsii
                    {
                        *data = ptr;
                        *len = size;
                    }
                };
                NewHook(hpinternal, "std::_Char_traits<char,int>::copy(void *, const void *, size_t)");
            }
    };

    return NewHook(hp, "vcruntime140_app:memmove");
}