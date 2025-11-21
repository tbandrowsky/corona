#include <windows.h>
#include <string>
#include <optional>
#include <vector>
#include <sstream>
#include <iomanip>

// Reads the specified value from HKLM\SOFTWARE\Microsoft\Cryptography.
// Returns string representation (UTF-8 for text, hex for binary, decimal for DWORD).
std::optional<std::string> ReadMachineIdValue(const std::wstring& valueName = L"MachineId")
{
    constexpr LPCWSTR subKey = L"SOFTWARE\\Microsoft\\Cryptography";
    DWORD type = 0;
    DWORD cbData = 0;

    // Query size and type
    LSTATUS st = ::RegGetValueW(
        HKEY_LOCAL_MACHINE,
        subKey,
        valueName.c_str(),
        RRF_RT_ANY,
        &type,
        nullptr,
        &cbData
    );

    if (st != ERROR_SUCCESS || cbData == 0) {
        return std::nullopt;
    }

    std::vector<BYTE> buffer(cbData);
    st = ::RegGetValueW(
        HKEY_LOCAL_MACHINE,
        subKey,
        valueName.c_str(),
        RRF_RT_ANY,
        &type,
        buffer.data(),
        &cbData
    );
    if (st != ERROR_SUCCESS) {
        return std::nullopt;
    }

    // REG_SZ / REG_EXPAND_SZ -> convert UTF-16 to UTF-8
    if (type == REG_SZ || type == REG_EXPAND_SZ) {
        // Ensure buffer is null-terminated wchar_t
        const wchar_t* wstr = reinterpret_cast<const wchar_t*>(buffer.data());
        int utf8len = ::WideCharToMultiByte(CP_UTF8, 0, wstr, -1, nullptr, 0, nullptr, nullptr);
        if (utf8len == 0) return std::nullopt;
        std::string out;
        out.resize(static_cast<size_t>(utf8len) - 1); // exclude terminating null
        ::WideCharToMultiByte(CP_UTF8, 0, wstr, -1, out.data(), utf8len, nullptr, nullptr);
        return out;
    }

    // REG_DWORD -> decimal string
    if (type == REG_DWORD && cbData >= sizeof(DWORD)) {
        DWORD val = *reinterpret_cast<const DWORD*>(buffer.data());
        return std::to_string(val);
    }

    // REG_QWORD -> decimal string
    if (type == REG_QWORD && cbData >= sizeof(uint64_t)) {
        uint64_t val = *reinterpret_cast<const uint64_t*>(buffer.data());
        return std::to_string(val);
    }

    // REG_BINARY or other -> hex representation
    {
        std::ostringstream oss;
        oss << std::hex << std::setfill('0');
        for (DWORD i = 0; i < cbData; ++i) {
            oss << std::setw(2) << static_cast<int>(buffer[i]);
        }
        return oss.str();
    }
}

// Example usage:
// auto mid = ReadMachineIdValue(L"MachineId");
// if (mid) { /* use *mid */ }