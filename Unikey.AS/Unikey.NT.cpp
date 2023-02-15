#include "stdafx.h"
#include "Unikey.NT.h"

#include <psapi.h>
#pragma comment(lib, "psapi.lib")

#include <vector>
#include <tuple>
#include <utility>
#include <algorithm>

// Unikey NT

// RC5 64-bit
// $+0  | 48:83EC 28       | sub rsp,28
// $+4  | 48:8D0D 9D801000 | lea rcx,qword ptr ds:[140125F28]
// $+B  | FF15 DF760A00    | call qword ptr ds:[<&RtlEnterCriticalSection>]
// $+11 | 48:8B05 E0881000 | mov rax,qword ptr ds:[140126778]
// $+18 | 33D2             | xor edx,edx
// $+1A | 3910             | cmp dword ptr ds:[rax],edx
// $+1C | 0F94C2           | sete dl
// $+1F | 8910             | mov dword ptr ds:[rax],edx
// $+21 | 48:8B05 D0881000 | mov rax,qword ptr ds:[140126778]
// $+28 | 8378 04 00       | cmp dword ptr ds:[rax+4],0
// $+2C | 74 05            | je unikeynt.14001DEB3
// $+2E | E8 CDEBFFFF      | call <unikeynt.sub_14001CA80>
// $+33 | E8 18F7FFFF      | call <unikeynt.sub_14001D5D0>
// $+38 | 48:8D0D 69801000 | lea rcx,qword ptr ds:[140125F28]
// $+3F | 48:83C4 28       | add rsp,28
// $+43 | 48:FF25 9E760A00 | jmp qword ptr ds:[<&RtlLeaveCriticalSection>]
// $+4A | CC               | int3

// RC5 32-bit
// $+0  | 68 08F64D00      | push unikeynt.4DF608
// $+5  | FF15 50C24900    | call dword ptr ds:[<&RtlEnterCriticalSection>]
// $+B  | A1 38FE4D00      | mov eax,dword ptr ds:[4DFE38]
// $+10 | 33C9             | xor ecx,ecx
// $+12 | 3908             | cmp dword ptr ds:[eax],ecx
// $+14 | 0F94C1           | sete cl
// $+17 | 8908             | mov dword ptr ds:[eax],ecx
// $+19 | A1 38FE4D00      | mov eax,dword ptr ds:[4DFE38]
// $+1E | 8378 04 00       | cmp dword ptr ds:[eax+4],0
// $+22 | 74 05            | je unikeynt.419839
// $+24 | E8 67EEFFFF      | call <unikeynt.sub_4186A0>
// $+29 | E8 92F8FFFF      | call <unikeynt.sub_4190D0>
// $+2E | 68 08F64D00      | push unikeynt.4DF608
// $+33 | FF15 4CC24900    | call dword ptr ds:[<&RtlLeaveCriticalSection>]
// $+39 | C3               | ret
// $+3A | CC               | int3

struct pattern_t
{
  vu::arch arch;
  std::string pattern;
  vu::ulong position;
  vu::ulong offset;
};

static struct
{
  vu::ProcessW process;
  bool         initialized = false;
  size_t       address_mode_state = 0;
  size_t       address_change_mode_function = 0;
  pattern_t    pattern;
} gUnikeyNT;


CUnikeyNT::CUnikeyNT()
{
  m_Sections.clear();
  m_FilterList.clear();

  m_Sections.insert(std::make_pair(eFilterType::FT_PROCESS_NAME, _T("Process Name")));
  m_Sections.insert(std::make_pair(eFilterType::FT_WINDOW_NAME,  _T("Window Name")));
  m_Sections.insert(std::make_pair(eFilterType::FT_WINDOW_CLASS, _T("Window Class")));
}

CUnikeyNT::~CUnikeyNT()
{
}

vu::VUResult CUnikeyNT::Initialize()
{
  if (gUnikeyNT.initialized)
  {
    return vu::VU_OK;
  }

  HWND hWnd = FindWindow(_T("UniKey MainWnd"), nullptr);
  if (hWnd == nullptr)
  {
    return __LINE__;
  }

  vu::ulong process_id = -1;
  GetWindowThreadProcessId(hWnd, &process_id);

  if (!gUnikeyNT.process.attach(process_id))
  {
    return __LINE__;
  }

  auto modules = gUnikeyNT.process.get_modules();
  if (modules.empty())
  {
    return __LINE__;
  }

  auto& module_exe = modules.at(0); // the first module is always EXE
  vu::Buffer data(module_exe.modBaseSize);
  if (!gUnikeyNT.process.read_memory(vu::ulongptr(module_exe.modBaseAddr), data))
  {
    return __LINE__;
  }

  std::vector<pattern_t> patterns;
  {
    patterns.push_back({ vu::arch::x86, "68 ?? ?? ?? ?? FF 15 ?? ?? ?? ?? A1 ?? ?? ?? ?? 33 C9 39 08 0F 94 C1 89 08", 0xC, 0x0 });
    patterns.push_back({ vu::arch::x64, "48 83 EC ?? 48 8D 0D ?? ?? ?? ?? FF 15 ?? ?? ?? ?? 48 8B 05 ?? ?? ?? ?? 33 D2 39 10 0F 94 C2 89 10", 0x14, 0x0 });
    patterns.push_back({ vu::arch::x64, "48 83 EC ?? 48 8B 0D ?? ?? ?? ?? 33 C0 39 41 ?? 0F 94 C0 89 41 ??", 0x7, 0x4 });
  }

  std::vector<size_t> result;

  auto it = std::find_if(patterns.cbegin(), patterns.cend(), [&](const pattern_t& pattern) -> bool
  {
    if (pattern.arch != gUnikeyNT.process.bit())
    {
      return false;
    }

    result = vu::find_pattern_A(data, pattern.pattern, true);

    return !result.empty();
  });

  if (it == patterns.cend() || result.empty())
  {
    return __LINE__;
  }

  auto found_offset = result.at(0);

  gUnikeyNT.pattern = *it;

  gUnikeyNT.address_change_mode_function = ULONG_PTR(module_exe.modBaseAddr) + found_offset;

  if (gUnikeyNT.pattern.arch == vu::arch::x64) // 64-bit
  {
    gUnikeyNT.address_mode_state = *(DWORD*)(data.get_ptr_bytes() + found_offset + gUnikeyNT.pattern.position);
    gUnikeyNT.address_mode_state += gUnikeyNT.pattern.position  + 4; // 64-bit relative address
    gUnikeyNT.address_mode_state += gUnikeyNT.address_change_mode_function;
  }
  else // 32-bit
  {
    gUnikeyNT.address_mode_state = *(DWORD*)(data.get_ptr_bytes() + found_offset + gUnikeyNT.pattern.position);
  }

  gUnikeyNT.initialized = true;

  return vu::VU_OK;
}

void CUnikeyNT::SwitchMode(const eMode mode)
{
  if (!gUnikeyNT.initialized)
  {
    vu::print_A("Unikey NT:error that did not initialize");
    return;
  }

  if (mode == eMode::MODE_NA || mode == this->GetModeState())
  {
    return;
  }

  gUnikeyNT.process.execute_code_at(gUnikeyNT.address_change_mode_function);
}

bool CUnikeyNT::IsReady() const
{
  return gUnikeyNT.initialized;
}

CUnikeyNT::eMode CUnikeyNT::GetModeState() const
{
  if (!gUnikeyNT.initialized)
  {
    vu::print_A("Unikey NT:error that did not initialize");
    return eMode::MODE_NA;
  }

  eMode mode = eMode::MODE_NA;

  auto succeed = vu::read_memory_ex(
    gUnikeyNT.process.bit(),
    gUnikeyNT.process.handle(),
    LPCVOID(gUnikeyNT.address_mode_state),
    &mode, sizeof(mode), true, 1, gUnikeyNT.pattern.offset);

  if (!succeed)
  {
    vu::print_A("Unikey NT:read mode failed");
    return eMode::MODE_NA;
  }

  return mode;
}

CUnikeyNT::TFilterData CUnikeyNT::ParseFilter(const std::vector<std::tstring>& filter)
{
  TFilterData result;

  for (auto& e : filter)
  {
    auto _pair = vu::split_string(e, _T("="));
    if (_pair.size() == 2)
    {
      auto key = vu::lower_string(_pair.at(0));
      auto value = eMode(std::stoi(_pair.at(1)));
      auto item = std::make_pair(key, value);
      result.insert(item);
    }
  }

  return result;
}

int CUnikeyNT::LoadFilterList(const std::tstring& filePath)
{
  vu::INIFile INI(filePath);

  if (m_Sections.empty()) return 1;

  m_FilterList.clear();

  for (auto& section : m_Sections)
  {
    auto sectionData = INI.read_section(section.second, MiB);
    if (sectionData.empty()) continue;

    auto filterData = this->ParseFilter(sectionData);

    if (!filterData.empty()) m_FilterList.insert(std::make_pair(section.first, filterData));
  }

  return 0;
}

CUnikeyNT::TWndInfo CUnikeyNT::GetInfoByWindowHandle(const HWND hWnd)
{
  std::unique_ptr<TCHAR[]> p(new TCHAR[MAXBYTE]);

  DWORD process_id = 0;
  GetWindowThreadProcessId(hWnd, &process_id);
  if (process_id != 0)
  {
    auto hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, process_id);
    if (hProcess != 0 && hProcess != INVALID_HANDLE_VALUE)
    {
      ZeroMemory(p.get(), MAXBYTE*sizeof(TCHAR));
      GetModuleBaseName(hProcess, nullptr, LPTSTR(p.get()), MAXBYTE);
      CloseHandle(hProcess);
    }
  }
  std::tstring processName(std::move(p.get()));

  ZeroMemory(p.get(), MAXBYTE*sizeof(TCHAR));
  GetWindowText(hWnd, LPTSTR(p.get()), MAXBYTE);
  std::tstring windowName(std::move(p.get()));

  ZeroMemory(p.get(), MAXBYTE*sizeof(TCHAR));
  GetClassName(hWnd, LPTSTR(p.get()), MAXBYTE);
  std::tstring windowClass(std::move(p.get()));

  auto s = vu::trim_string(windowClass);
  if (s.length() != 0 && std::all_of(s.cbegin(), s.cend(), [](const TCHAR& v) {
    return ((v >= _T('0') && v <= _T('9')) || (v == _T('#'))); /* Ignore MFC's class name style */
  }))
  {
    windowClass = _T("");
  }

  return std::make_tuple(processName, windowName, windowClass);
}

CUnikeyNT::eMode CUnikeyNT::DetermineByWindowHandle(const HWND hWnd)
{
  std::tstring processName(_T("")), windowName(_T("")), windowClass(_T(""));

  std::tie(processName, windowName, windowClass) = this->GetInfoByWindowHandle(hWnd);

  return this->Determine(processName, windowName, windowClass);
}

CUnikeyNT::eMode CUnikeyNT::Determine(
  const std::tstring& processName,
  const std::tstring& windowName,
  const std::tstring& windowClass
)
{
  eMode result = eMode::MODE_NA;

  if (m_FilterList.empty()) return result;

  std::vector<std::tstring> args;
  args.clear();

  std::tstring v(_T(""));

  v = vu::lower_string(processName);
  args.push_back(v);

  v = vu::lower_string(windowName);
  args.push_back(v);

  v = vu::lower_string(windowClass);
  args.push_back(v);

  for (auto& section : m_FilterList)
  {
    auto arg = args.at(section.first);
    if (arg.length() == 0) continue;

    for (auto& e : section.second)
    {
      auto v1 = vu::trim_string_W(arg);
      auto v2 = vu::trim_string_W(e.first);
      if (vu::contains_string_W(v1, v2, true))
      {
        result = e.second;
        break;
      }
    }

    if (result != eMode::MODE_NA) break;
  }

  return result;
}