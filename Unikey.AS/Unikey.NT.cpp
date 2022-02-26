#include "stdafx.h"
#include "Unikey.NT.h"

#include <Psapi.h>
#pragma comment(lib, "psapi.lib")

#include <vector>
#include <tuple>
#include <utility>
#include <algorithm>

/* Switch Mode
 * $+0      | 48 83 EC 28          | sub rsp,28                                |
 * $+4      | 48:8B0D 65200800     | mov rcx,qword ptr ds:[14009B330]          |
 * $+B      | 33 C0                | xor eax,eax                               |
 * $+D      | 39 41 04             | cmp dword ptr ds:[rcx+4],eax              | <- Mode State
 * $+10     | 0F 94 C0             | sete al                                   |
 * $+13     | 89 41 04             | mov dword ptr ds:[rcx+4],eax              | <- Mode State
 * $+16     | 48:8B05 53200800     | mov rax,qword ptr ds:[14009B330]          |
 * $+1D     | 83 78 08 00          | cmp dword ptr ds:[rax+8],0                |
 * $+21     | 74 05                | je <unikeynt.loc_1400192E8>               |
 * $+23     | E8 E8 EC FF FF       | call <unikeynt.sub_140017FD0>             |
 * $+28     | 48 83 C4 28          | add rsp,28                                |
 * $+2C     | E9 AF F7 FF FF       | jmp <unikeynt.sub_140018AA0>              |
 */

CUnikeyNT::CUnikeyNT()
{
  m_PID     = 0;
  m_Ready   = false;
  m_Handle  = INVALID_HANDLE_VALUE;
  m_ModeState = eMode::MODE_COUNT;
  m_AddressOfImageBase = 0;
  m_OffsetOfModeState  = 0;
  m_AddressOfModeState = 0;
  m_OffsetOfSwitchModeFunction  = 0;
  m_AddressOfSwitchModeFunction = 0;

  m_Sections.clear();
  m_FilterList.clear();

  m_Sections.insert(std::make_pair(eFilterType::FT_PROCESS_NAME, _T("Process Name")));
  m_Sections.insert(std::make_pair(eFilterType::FT_WINDOW_NAME, _T("Window Name")));
  m_Sections.insert(std::make_pair(eFilterType::FT_WINDOW_CLASS, _T("Window Class")));
}

CUnikeyNT::~CUnikeyNT()
{
  if (!m_Ready) return;

  if (m_Handle != nullptr && m_Handle != INVALID_HANDLE_VALUE)
  {
    CloseHandle(m_Handle);
  }
}

HMODULE CUnikeyNT::GetpBase(const HANDLE processHandle, vu::ulong PID)
{
  HMODULE result = 0;

  TCHAR targetName[MAX_PATH] = {0};
  if (GetModuleBaseName(processHandle, nullptr, targetName, sizeof(targetName)) == 0) return result;

  auto target = vu::lower_string(targetName);
  if (target.length() == 0) return result;

  HMODULE modules[1024];
  DWORD cbNeeded;
  if (!EnumProcessModules(processHandle, modules, sizeof(modules), &cbNeeded)) return result;

  TCHAR modulePath[MAX_PATH];
  for (int i = 0; i < (cbNeeded / sizeof(modules[0])); i++ )
  {
    ZeroMemory(&modulePath, sizeof(modulePath));

    if (!GetModuleFileNameEx(
      processHandle,
      modules[i],
      modulePath,
      sizeof(modulePath) / sizeof(modulePath[0]))
    ) continue;

    auto moduleName = vu::extract_file_name(modulePath);

    auto iter = vu::lower_string(moduleName);
    if (iter == target)
    {
      result = modules[i];
      break;
    }
  }

  return result;
}

int CUnikeyNT::Initialize()
{
  if (m_Ready) return 0;

  HWND windowHandle = FindWindow(_T("UniKey MainWnd"), nullptr);
  if (windowHandle == nullptr || windowHandle == INVALID_HANDLE_VALUE) return 1;

  GetWindowThreadProcessId(windowHandle, &m_PID);
  if (m_PID == 0 || m_PID == (ULONG)-1) return 2;

  m_Handle = OpenProcess(
    PROCESS_QUERY_INFORMATION |
    PROCESS_CREATE_THREAD |
    PROCESS_VM_READ |
    PROCESS_VM_WRITE,
    FALSE,
    m_PID
  );

  if (m_Handle == nullptr || m_Handle == INVALID_HANDLE_VALUE) return 1;

  m_AddressOfImageBase = reinterpret_cast<ULONG_PTR>(this->GetpBase(m_Handle, m_PID));
  m_AddressOfImageBase = (m_AddressOfImageBase == 0 ? 0x140000000 : m_AddressOfImageBase);
  m_OffsetOfModeState  = 0x9B330;
  m_OffsetOfSwitchModeFunction = 0x192C0;

  m_AddressOfModeState = m_AddressOfImageBase + m_OffsetOfModeState;
  m_AddressOfSwitchModeFunction = m_AddressOfImageBase + m_OffsetOfSwitchModeFunction;

  this->UpdateModeState();

  m_Ready = true;

  return 0;
}

int CUnikeyNT::SwitchMode(const eMode mode)
{
  if (!m_Ready) return 1;

  if (mode == eMode::MODE_COUNT) return 2;

  this->UpdateModeState();

  if (mode == m_ModeState) return 0;

  auto threadHandle = CreateRemoteThread(m_Handle, NULL, 0, (LPTHREAD_START_ROUTINE)m_AddressOfSwitchModeFunction, NULL, 0, NULL);
  if (threadHandle == nullptr || threadHandle == INVALID_HANDLE_VALUE) return 3;

  WaitForSingleObject(threadHandle, INFINITE);

  this->UpdateModeState();

  return 0;
}

bool CUnikeyNT::IsReady() const
{
  return m_Ready;
}

CUnikeyNT::eMode CUnikeyNT::GetModeState() const
{
  return m_ModeState;
}

const CUnikeyNT::eMode CUnikeyNT::UpdateModeState()
{
  vu::rpm_ex(vu::eXBit::x64, m_Handle, LPCVOID(m_AddressOfModeState), &m_ModeState, 1, true, 2, 0, 4);
  return m_ModeState;
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

CUnikeyNT::TWndInfo CUnikeyNT::GetInfoByWindowHandle(const HWND windowHandle)
{
  std::unique_ptr<TCHAR[]> p(new TCHAR[MAXBYTE]);

  DWORD PID = 0;
  GetWindowThreadProcessId(windowHandle, &PID);
  if (PID != 0)
  {
    auto hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, PID);
    if (hProcess != 0 && hProcess != INVALID_HANDLE_VALUE)
    {
      ZeroMemory(p.get(), MAXBYTE*sizeof(TCHAR));
      GetModuleBaseName(hProcess, nullptr, LPTSTR(p.get()), MAXBYTE);
      CloseHandle(hProcess);
    }
  }
  std::tstring processName(std::move(p.get()));

  ZeroMemory(p.get(), MAXBYTE*sizeof(TCHAR));
  GetWindowText(windowHandle, LPTSTR(p.get()), MAXBYTE);
  std::tstring windowName(std::move(p.get()));

  ZeroMemory(p.get(), MAXBYTE*sizeof(TCHAR));
  GetClassName(windowHandle, LPTSTR(p.get()), MAXBYTE);
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

CUnikeyNT::eMode CUnikeyNT::DetermineByWindowHandle(const HWND windowHandle, bool trimLR, bool justContain)
{
  std::tstring processName(_T("")), windowName(_T("")), windowClass(_T(""));

  std::tie(processName, windowName, windowClass) = this->GetInfoByWindowHandle(windowHandle);

  return this->Determine(processName, windowName, windowClass, trimLR, justContain);
}

CUnikeyNT::eMode CUnikeyNT::Determine(
  const std::tstring& processName,
  const std::tstring& windowName,
  const std::tstring& windowClass,
  bool trimLR,
  bool justContain
)
{
  eMode result = eMode::MODE_COUNT;

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
      if (this->CompareString(e.first, arg, trimLR, justContain))
      {
        result = e.second;
        break;
      }
    }

    if (result != eMode::MODE_COUNT) break;
  }

  return result;
}

bool CUnikeyNT::CompareString(const std::tstring& s1, const std::tstring& s2, bool trimLR, bool justContain)
{
  bool result = false;

  auto _s1 = (trimLR ? vu::trim_string(s1) : s1);
  auto _s2 = (trimLR ? vu::trim_string(s2) : s2);

  if (_s1.length() == 0 || _s2.length() == 0) return result;

  if (justContain)
  {
    result = (_s1.find(_s2) != std::tstring::npos || _s2.find(_s1) != std::tstring::npos);
  } 
  else
  {
    result = (_s1 == _s2);
  }

  return result;
}
