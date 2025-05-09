#include <iostream>
#include <windows.h>
#include <patch.hpp>
#include <console.hpp>
#include <fontmanager.hpp>
#include <fortissimo_exs.hpp>

namespace FORTISSIMO_EXS
{
    static Utils::FontManager FontManager{};

    static auto CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) -> LRESULT
    {
        if (uMsg == WM_CREATE)
        {
            ::SetWindowTextW(hWnd, FORTISSIMO_EXS::TitleName);
            HMENU SystemMenu{ ::GetSystemMenu(hWnd, FALSE) };
            if (SystemMenu != nullptr)
            {
                ::AppendMenuW(SystemMenu, 0, 0x114514, L"更改字体");
            }

            if (FORTISSIMO_EXS::FontManager.GUI() == nullptr)
            {
                FORTISSIMO_EXS::FontManager.Init(hWnd);
            }
        }
        else if (uMsg == WM_SIZE && FontManager.GUI() != nullptr)
        {
            FontManager.GUIUpdateDisplayState();
        }
        else if (uMsg == WM_SYSCOMMAND && wParam == 0x114514)
        {
            if (FORTISSIMO_EXS::FontManager.GUI() != nullptr)
            {
                FORTISSIMO_EXS::FontManager.GUIChooseFont();
            }
            return TRUE;
        }
        return Patch::Hooker::Call<FORTISSIMO_EXS::WndProc>(hWnd, uMsg, wParam, lParam);
    }

    static auto ReplacePathW(std::wstring_view path) -> std::wstring_view
    {
        static std::wstring new_path{};
        size_t pos{ path.find_last_of(L"\\/") };
        if (pos != std::wstring_view::npos)
        {

            new_path = std::wstring{ L".\\cn_Data" }.append(path.substr(pos));
            DWORD attr{ ::GetFileAttributesW(new_path.c_str()) };
            if (attr != INVALID_FILE_ATTRIBUTES)
            {
                return { new_path };
            }
        }
        return {};
    }

    static auto ReplacePathA(std::string_view path) -> std::string_view
    {
        static std::string new_path{};
        size_t pos{ path.find_last_of("\\/") };
        if (pos != std::wstring_view::npos)
        {
            new_path = std::string{ ".\\cn_Data" }.append(path.substr(pos));
            DWORD attr{ ::GetFileAttributesA(new_path.c_str()) };
            if (attr != INVALID_FILE_ATTRIBUTES)
            {
                return { new_path };
            }
        }
        return {};
    }

    static auto WINAPI CreateFileA(LPCSTR lpFN, DWORD dwDA, DWORD dwSM, LPSECURITY_ATTRIBUTES lpSA, DWORD dwCD, DWORD dwFAA, HANDLE hTF) -> HANDLE
    {
        std::string_view new_path{ FORTISSIMO_EXS::ReplacePathA(lpFN) };
        return Patch::Hooker::Call<FORTISSIMO_EXS::CreateFileA>(new_path.empty() ? lpFN : new_path.data(), dwDA, dwSM, lpSA, dwCD, dwFAA, hTF);
    }

    static auto WINAPI CreateFileW(LPCWSTR lpFN, DWORD dwDA, DWORD dwSM, LPSECURITY_ATTRIBUTES lpSA, DWORD dwCD, DWORD dwFAA, HANDLE hTF) -> HANDLE
    {
        std::wstring_view new_path{ FORTISSIMO_EXS::ReplacePathW(lpFN) };
        return Patch::Hooker::Call<FORTISSIMO_EXS::CreateFileW>(new_path.empty() ? lpFN : new_path.data(), dwDA, dwSM, lpSA, dwCD, dwFAA, hTF);
    }

    static auto WINAPI FindFirstFileA(LPCSTR lpFileName, LPWIN32_FIND_DATAA lpFindFileData) -> HANDLE
    {
        std::string_view new_path{ FORTISSIMO_EXS::ReplacePathA(lpFileName) };
        return Patch::Hooker::Call<FORTISSIMO_EXS::FindFirstFileA>(new_path.empty() ? lpFileName : new_path.data(), lpFindFileData);
    }

    static auto WINAPI GetGlyphOutlineA(HDC hdc, UINT uChar, UINT fuf, LPGLYPHMETRICS lpgm, DWORD cjbf, LPVOID pvbf, MAT2* lpmat) -> DWORD
    {
        if (tagTEXTMETRICA lptm{}; ::GetTextMetricsA(hdc, &lptm))
        {
            if (0xA1EC == uChar) // § -> ♪
            {
                HFONT font{ FORTISSIMO_EXS::FontManager.GetJISFont(lptm.tmHeight) };
                if (font != nullptr)
                {
                    font = { reinterpret_cast<HFONT>(::SelectObject(hdc, font)) };
                    DWORD result{ ::GetGlyphOutlineW(hdc, L'♪', fuf, lpgm, cjbf, pvbf, lpmat) };
                    static_cast<void>(::SelectObject(hdc, font));
                    return result;
                }
            }

            if (uChar == 0x23) { uChar = 0x20; }

            HFONT font{ FORTISSIMO_EXS::FontManager.GetGBKFont(lptm.tmHeight) };
            if (font != nullptr)
            {
                font = { reinterpret_cast<HFONT>(::SelectObject(hdc, font)) };
                DWORD result{ Patch::Hooker::Call<FORTISSIMO_EXS::GetGlyphOutlineA>(hdc, uChar, fuf, lpgm, cjbf, pvbf, lpmat) };
                static_cast<void>(::SelectObject(hdc, font));
                return result;
            }
        }

        return Patch::Hooker::Call<FORTISSIMO_EXS::GetGlyphOutlineA>(hdc, uChar, fuf, lpgm, cjbf, pvbf, lpmat);
    }

    auto FORTISSIMO_EXS::INIT_ALL_PATCH(void) -> void
    {
        //console::make();
        Patch::Hooker::Begin();
        Patch::Hooker::Add<FORTISSIMO_EXS::CreateFileA>(::CreateFileA);
        Patch::Hooker::Add<FORTISSIMO_EXS::CreateFileW>(::CreateFileW);
        Patch::Hooker::Add<FORTISSIMO_EXS::FindFirstFileA>(::FindFirstFileA);
        Patch::Hooker::Add<FORTISSIMO_EXS::GetGlyphOutlineA>(::GetGlyphOutlineA);
        Patch::Hooker::Add<FORTISSIMO_EXS::WndProc>(reinterpret_cast<void*>(0x40C390));
        Patch::Hooker::Commit();
    }
}
