#include "pch.h"
#include "Paradise/Globals.h"
#include "Paradise/Public/Redirect.h"
#include "Paradise/Public/Core.h"
#include "Paradise/Public/Finder.h"

LONG WINAPI TamperVeh(EXCEPTION_POINTERS* ExceptionInfo)
{
    if (ExceptionInfo->ExceptionRecord->ExceptionCode != EXCEPTION_ACCESS_VIOLATION)
        return EXCEPTION_CONTINUE_SEARCH;
    DWORD accessType = (DWORD)ExceptionInfo->ExceptionRecord->ExceptionInformation[0];
    uintptr_t exeBase = (uintptr_t)GetModuleHandle(0);
    CONTEXT* ctx = ExceptionInfo->ContextRecord;
    auto isRet = [](uintptr_t addr, uintptr_t textStart, uintptr_t textEnd) -> bool {
        if (addr < textStart + 7 || addr > textEnd) return false;
        const uint8_t* p = reinterpret_cast<const uint8_t*>(addr);
        if (p[-5] == 0xE8) return true;
        if (p[-2] == 0xFF && (p[-1] & 0xF8) == 0xD0) return true;
        if (p[-3] == 0xFF && ((p[-2] & 0xF8) == 0x10 || (p[-2] & 0xF8) == 0x50)) return true;
        if ((p[-3] & 0xF0) == 0x40 && p[-2] == 0xFF && (p[-1] & 0xF8) == 0xD0) return true;
        if (p[-4] == 0xFF && p[-3] == 0x54 && p[-2] == 0x24) return true;
        if ((p[-4] & 0xF0) == 0x40 && p[-3] == 0xFF && (p[-2] & 0xF8) == 0x50) return true;
        if ((p[-5] & 0xF0) == 0x40 && p[-4] == 0xFF && p[-3] == 0x54 && p[-2] == 0x24) return true;
        if (p[-6] == 0xFF && (p[-5] == 0x15 || (p[-5] & 0xF8) == 0x90)) return true;
        if ((p[-7] & 0xF0) == 0x40 && p[-6] == 0xFF && (p[-5] == 0x15 || (p[-5] & 0xF8) == 0x90)) return true;
        return false;
        };
    auto isExecutableCode = [](uintptr_t addr) -> bool {
        MEMORY_BASIC_INFORMATION mbi{};
        if (VirtualQuery(reinterpret_cast<void*>(addr), &mbi, sizeof(mbi)) == 0)
            return false;
        if (mbi.State != MEM_COMMIT)
            return false;
        if (mbi.Protect & (PAGE_GUARD | PAGE_NOACCESS))
            return false;
        constexpr DWORD kExecMask = PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY;
        return (mbi.Protect & kExecMask) != 0;
        };
    auto doStackWalk = [&]() -> LONG {
        constexpr uintptr_t kTextStart = 0x1000, kTextEnd = 0x616B310;
        uintptr_t* sp = reinterpret_cast<uintptr_t*>(ctx->Rsp);
        for (int i = 0; i < 512; i++) {
            uintptr_t candidate = sp[i];
            uintptr_t rva = candidate - exeBase;
            if (rva >= kTextStart && rva < kTextEnd && isExecutableCode(candidate) && isRet(candidate, exeBase + kTextStart, exeBase + kTextEnd)) {
                ctx->Rip = candidate;
                ctx->Rsp = reinterpret_cast<uintptr_t>(&sp[i + 1]);
                return EXCEPTION_CONTINUE_EXECUTION;
            }
        }
        return EXCEPTION_CONTINUE_SEARCH;
        };
    if (accessType == 8) {
        void* faultAddr = ExceptionInfo->ExceptionRecord->ExceptionAddress;
        MEMORY_BASIC_INFORMATION mbi{};
        VirtualQuery(faultAddr, &mbi, sizeof(mbi));
        constexpr DWORD kExecMask = PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY;
        if (!(mbi.Protect & kExecMask))
            return doStackWalk();
        return EXCEPTION_CONTINUE_SEARCH;
    }
    if (accessType == 1) {
        uintptr_t ripRVA = ctx->Rip - exeBase;
        if (ripRVA > 0x799B000) {
            void* writeTarget = (void*)ExceptionInfo->ExceptionRecord->ExceptionInformation[1];
            MEMORY_BASIC_INFORMATION mbi{};
            VirtualQuery(writeTarget, &mbi, sizeof(mbi));
            constexpr DWORD kWriteMask = PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY;
            if (mbi.State == MEM_COMMIT && !(mbi.Protect & kWriteMask))
                return doStackWalk();
        }
        return EXCEPTION_CONTINUE_SEARCH;
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

namespace Paradise
{
    bool Core::Initialize()
    {
        Memcury::Scanner scanner = Memcury::Scanner::FindStringRef(Strings::ProcessRequestStat, Memcury::PE::GetModuleBase(), false);

        if (!scanner.IsValid())
            scanner = Memcury::Scanner::FindStringRef(Strings::ProcessRequest, Memcury::PE::GetModuleBase(), false);

        if (!scanner.IsValid())
        {
            scanner = Memcury::Scanner::FindStringRef(Strings::ProcessRequestStat, Memcury::PE::GetModuleBase(), false);

            if (!scanner.IsValid())
                scanner = Memcury::Scanner::FindStringRef(Strings::ProcessRequest, Memcury::PE::GetModuleBase(), false);
            if (!scanner.IsValid())
                return false;
        }

        uint8_t* stream = scanner.GetAs<uint8_t*>();
        void* FunctionPtr = nullptr;

        for (int i = 0; i < 2048; i++)
        {
            uint8_t* current = stream - i;

            bool isPattern1 = current[0] == 0x4C && current[1] == 0x8B && current[2] == 0xDC;
            bool isPattern2 = current[0] == 0x48 && current[1] == 0x8B && current[2] == 0xC4;

            if (isPattern1)
            {
                FunctionPtr = current;
                break;
            }
            else if (isPattern2)
            {
                FunctionPtr = current;
                break;
            }
            else
            {
                bool isPattern3 = current[0] == 0x48 && current[1] == 0x81 && current[2] == 0xEC;
                bool isPattern4 = current[0] == 0x48 && current[1] == 0x83 && current[2] == 0xEC;

                if (isPattern3 || isPattern4)
                {
                    for (int x = 0; x < 50; x++)
                    {
                        uint8_t* inner = stream - i - x;

                        if (inner[0] == 0x40)
                        {
                            FunctionPtr = inner;
                            goto _found;
                        }
                        else
                        {
                            bool innerPattern1 = inner[0] == 0x4C && inner[1] == 0x8B && inner[2] == 0xDC;
                            bool innerPattern2 = inner[0] == 0x48 && inner[1] == 0x8B && inner[2] == 0xC4;
                            bool innerPattern3 = inner[0] == 0x48 && inner[1] == 0x89 && inner[2] == 0x5C;

                            if (innerPattern1 || innerPattern2 || innerPattern3)
                                break;
                        }
                    }
                }
            }
        }

    _found:
        if (!FunctionPtr)
            return false;

        ProcessRequestOG = reinterpret_cast<decltype(ProcessRequestOG)>(FunctionPtr);
        void** ref = Memcury::Scanner::FindPtrRef(ProcessRequestOG).GetAs<void**>();

        if (!ref)
            return false;

        DWORD oldProtect;

        if (VirtualProtect(ref, sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtect))
        {
            *ref = reinterpret_cast<void*>(Redirect::ProcessRequest);
            VirtualProtect(ref, sizeof(void*), oldProtect, &oldProtect);
        }

        AddVectoredExceptionHandler(1, TamperVeh);

        return true;
    }
}