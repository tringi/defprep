#include <Windows.h>

#include <cstring>
#include <cstdint>
#include <cwchar>
#include <cstdio>

#include <string>
#include <string_view>

// options

int argc = 0;
LPWSTR * argw = NULL;

template <typename F>
std::size_t option_nth (std::size_t i, unsigned long argc, wchar_t ** argw, std::wstring_view name, F f) {
    auto skip = 0;
    switch (argw [i][0]) {
        case L'/':
            skip = 1;
            break;
        case L'-':
            while (argw [i][skip] == '-') {
                ++skip;
            }
            break;
    }

    if (std::wcsncmp (&argw [i][skip], name.data (), name.length ()) == 0) {
        switch (argw [i][name.length () + skip]) {
            case L':':
            case L'=':
                f (&argw [i][name.length () + skip + 1]);
                return true;

            case L'\0':
                f (&argw [i][name.length () + skip]);
                return true;
        }
    }
    return false;
}

template <typename F>
std::size_t options (unsigned long argc, wchar_t ** argw, std::wstring_view name, F f) {
    std::size_t n = 0;
    for (auto i = 1uL; i != argc; ++i) {
        n += option_nth (i, argc, argw, name, f);
    }
    return n;
}

const wchar_t * option (unsigned long argc, wchar_t ** argw, std::wstring_view name, const wchar_t * default_ = nullptr) {
    options (argc, argw, name,
             [&default_] (const wchar_t * result) { default_ = result;  });
    return default_;
}

// ACP/UTF-8 <-> UTF-16

std::wstring a2w (UINT codepage, const char * string, std::size_t length) {
    std::wstring s;
    if (length > INT_MAX) {
        length = INT_MAX;
    }
    auto n = MultiByteToWideChar (codepage, 0, string, (int) length, NULL, 0);
    if (n > 0) {
        s.resize (n);
        MultiByteToWideChar (codepage, 0, string, (int) length, &s [0], (int) n);
    }
    return s;
}
std::string w2a (UINT codepage, const wchar_t * string, std::size_t length) {
    std::string s;
    if (length > INT_MAX) {
        length = INT_MAX;
    }
    auto n = WideCharToMultiByte (codepage, 0, string, (int) length, NULL, 0, NULL, NULL);
    if (n > 0) {
        s.resize (n);
        WideCharToMultiByte (codepage, 0, string, (int) length, reinterpret_cast <LPSTR> (&s [0]), (int) n, NULL, NULL);
    }
    return s;
}

template <typename C>
struct traits;

template <>
struct traits <char> {
    static inline const char whitespace [] = " \t\r\n\v\f";
    static inline const char separators [] = "\r\n\v\f";
    static inline const char crlf [] = "\r\n";
};
template <>
struct traits <wchar_t> {
    static inline const wchar_t whitespace [] = L" \t\r\n\v\f\xFEFF"; // "\x1680\x2002\x2003\x2004\x2005\x2006\x2007\x2008\x2009\x200A\x200B\x202F\x205F\x2060\x3000\xFEFF";
    static inline const wchar_t separators [] = L"\r\n\v\f"; // "\x0085\x2028\x2029";
    static inline const wchar_t crlf [] = L"\r\n";
};

// .info file

struct info {
    std::uint16_t major = 0;
    std::uint16_t minor = 0;
    wchar_t       name [256];

    info () {
        name [0] = L'\0';
    }

    bool load (const wchar_t * argument) {

        // fix .info path

        wchar_t path [32768];
        wchar_t * infofile = path;
        if (!GetFullPathName (argument, 32768, path, &infofile)) {
            std::wcscpy (path, argument);
        }

        // load

        this->major = GetPrivateProfileInt (L"description", L"major", 0, path);
        this->minor = GetPrivateProfileInt (L"description", L"minor", 0, path);
        
        GetPrivateProfileString (L"description", L"internalname", L"", this->name, 256, path);
        return true;
    }
} info;

std::wstring_view ltrim (std::wstring_view value) {
    auto begin = value.find_first_not_of (traits <wchar_t> ::whitespace);
    if (begin != std::wstring_view::npos) {
        return value.substr (begin);
    } else
        return std::wstring_view ();
}

bool replace (std::wstring_view line, std::wstring * result) {
    if (line == L"LIBRARY #") {
        result->assign (L"LIBRARY \"");
        result->append (info.name);
        result->append (L"\"");
        return true;
    }
    if (line == L"VERSION #.#") {
        result->assign (L"VERSION ");
        result->append (std::to_wstring (info.major));
        result->append (L".");
        result->append (std::to_wstring (info.minor));
        return true;
    }
    return false;
}

bool replace (std::string_view line, std::string * result, UINT codepage) {
    std::wstring wresult;
    if (replace (a2w (codepage, line.data (), line.size ()), &wresult)) {
        *result = w2a (codepage, wresult.data (), wresult.size ());
        return true;
    } else
        return false;
}

bool filter (std::wstring_view line) {

    auto hash = line.find (L'#');
    while (hash != std::wstring_view::npos) {
        auto end = line.find_first_of (traits <wchar_t> ::whitespace, hash);
        auto attribute = line.substr (hash + 1, end - hash - 1);

        // TODO: other comparison operators?

        auto equality = attribute.find (L'=');
        if (equality != std::wstring_view::npos) {

            // we have 'abc=def'
            //  - if 'abc'
            //  - if 'abc' != 'def' return false

            if (auto value = option (argc, argw, attribute.substr (0, equality))) {
                if (value != attribute.substr (equality + 1)) {

                    return false;
                }
            } else
                return false;

        } else {

            // just #abc
            //  - if 'abc' doesnt exist, return false

            if (attribute.starts_with (L'!')) {
                attribute.remove_prefix (1);
                if (option (argc, argw, attribute)) {
                    return false;
                }
            } else {
                if (!option (argc, argw, attribute)) {
                    return false;
                }
            }
        }

        hash = line.find (L'#', end);
    }
    return true;
}

bool filter (std::string_view sv, UINT codepage) {
    return filter (a2w (codepage, sv.data (), sv.size ()));
}

template <typename C>
std::basic_string_view <C> rtrim (std::basic_string_view <C> value) {
    return value.substr (0, value.find_last_not_of (traits <C> ::whitespace) + 1);
}

template <typename C, typename... Codepage>
void process (HANDLE hOut, std::basic_string_view <C> data, Codepage... cp) {
    auto offset = data.find_first_not_of (traits <C>::whitespace);
    do {
        auto end = data.find_first_of (traits <C>::separators, offset);
        auto text = data.substr (offset, end - offset);

        text = text.substr (0, text.find (C (';')));
        if (!text.empty ()) {

            std::basic_string <C> replacement;
            if (replace (text, &replacement, cp...)) {

                DWORD n;
                WriteFile (hOut, replacement.data (), (DWORD) (replacement.size () * sizeof (C)), &n, NULL);
                WriteFile (hOut, traits <C>::crlf, 2 * sizeof (C), &n, NULL);
                std::wprintf (L".");

            } else {
                if (filter (text, cp...)) {
                    text = text.substr (0, text.find (C ('#')));
                    text = rtrim (text);

                    DWORD n;
                    WriteFile (hOut, text.data (), (DWORD) (text.size () * sizeof (C)), &n, NULL);
                    WriteFile (hOut, traits <C>::crlf, 2 * sizeof (C), &n, NULL);
                } else
                    std::wprintf (L".");
            }
        }

        offset = data.find_first_not_of (traits <C>::whitespace, end + 1);
    } while (offset != std::basic_string_view <C>::npos);
}

int main () {
    if (argw = CommandLineToArgvW (GetCommandLineW (), &argc)) {
        if (auto input = option (argc, argw, L"in")) {

            bool result = false;
            auto hFile = CreateFile (input, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
            if (hFile != INVALID_HANDLE_VALUE) {

                auto hMapping = CreateFileMapping (hFile, NULL, PAGE_READONLY, 0, 0, NULL);
                if (hMapping) {

                    LARGE_INTEGER li;
                    if (GetFileSizeEx (hFile, &li)) {

                        if (li.QuadPart > 3) {
                            if (auto data = static_cast <const char *> (MapViewOfFile (hMapping, FILE_MAP_READ, 0, 0, 0))) {
                                auto size = (std::size_t) li.QuadPart;

                                std::wprintf (L"%s ", input);

                                // .info file for header

                                if (auto infofile = option (argc, argw, L"info")) {
                                    if (info.load (infofile)) {
                                        std::wprintf (L"& %s (%s %u.%u) ", infofile, info.name, info.major, info.minor);
                                    }
                                }

                                DWORD n;
                                HANDLE hOut;

                                if (auto output = option (argc, argw, L"out")) {
                                    hOut = CreateFile (output, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
                                    std::wprintf (L"> %s", output);
                                } else {
                                    auto defout = std::wstring (input) + L".def";
                                    hOut = CreateFile (defout.c_str (), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
                                    std::wprintf (L"> %s", defout.c_str ());
                                }
                                if (hOut != INVALID_HANDLE_VALUE) {

                                    if ((data [0] == '\xFF') && (data [1] == '\xFE')) { // BOM UTF-16

                                        WriteFile (hOut, data, 2, &n, NULL);
                                        process (hOut, std::wstring_view ((const wchar_t *) (data + 2), (size - 2) / sizeof (wchar_t)));

                                    } else
                                    if ((data [0] == '\xEF') && (data [1] == '\xBB') && (data [2] == '\xBF')) { // BOM UTF-8

                                        WriteFile (hOut, data, 3, &n, NULL);
                                        process (hOut, std::string_view ((const char *) (data + 3), (size - 3)), CP_UTF8);

                                    } else {

                                        // resort to ACP
                                        process (hOut, std::string_view (data, size), CP_ACP);
                                    }

                                    // success
                                    UnmapViewOfFile (data);
                                    CloseHandle (hOut);

                                    SetLastError (ERROR_SUCCESS);
                                }
                                std::wprintf (L"\n");
                            }
                        }
                    }
                    CloseHandle (hMapping);
                }
                CloseHandle (hFile);
            }
        } else
            return ERROR_INVALID_PARAMETER;
    }
    return GetLastError ();
}
