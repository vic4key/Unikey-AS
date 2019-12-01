#pragma once
#include <Windows.h>

#include <map>
#include <string>

#include <Vu.h>

class CUnikeyNT
{
public:
  CUnikeyNT();
  virtual ~CUnikeyNT();

  typedef enum {
    MODE_EN = 0,
    MODE_VN = 1,
    MODE_COUNT
  } eMode;

  typedef enum {
    FT_PROCESS_NAME = 0,
    FT_WINDOW_NAME  = 1,
    FT_WINDOW_CLASS = 2,
    FT_COUNT
  } eFilterType;

  typedef std::tuple<const std::tstring, const std::tstring, const std::tstring> TWndInfo;

  int Initialize();
  bool IsReady() const;

  eMode GetModeState() const;
  const eMode UpdateModeState();
  int SwitchMode(const eMode mode);

  int LoadFilterList(const std::tstring& filePath);

  eMode Determine(
    const std::tstring& processName,
    const std::tstring& windowName,
    const std::tstring& windowClass,
    bool trimLR = true,
    bool justContain = true
  );

  CUnikeyNT::eMode DetermineByWindowHandle(const HWND windowHandle, bool trimLR = true, bool justContain = true);

  TWndInfo GetInfoByWindowHandle(const HWND windowHandle);

private:
  bool m_Ready;
  ULONG m_PID;
  HANDLE m_Handle;
  eMode m_ModeState;
  ULONG_PTR m_AddressOfImageBase;
  ULONG_PTR m_AddressOfModeState;
  ULONG_PTR m_AddressOfSwitchModeFunction;
  ULONG_PTR m_OffsetOfModeState;
  ULONG_PTR m_OffsetOfSwitchModeFunction;

  typedef std::map<std::tstring, eMode> TFilterData;
  typedef std::map<eFilterType, TFilterData> TFilterLists;
  typedef std::map<eFilterType, std::tstring> TSections;

  TSections m_Sections;
  TFilterLists m_FilterList;

  TFilterData ParseFilter(const std::vector<std::tstring>& filter);

  bool CompareString(const std::tstring& s1, const std::tstring& s2, bool trimLR, bool justContain);

  HMODULE GetpBase(const HANDLE processHandle, vu::ulong PID);
};
