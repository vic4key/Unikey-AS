#pragma once

#include <vu>
#include <map>
#include <string>
#include <windows.h>

class CUnikeyNT
{
public:
  CUnikeyNT();
  virtual ~CUnikeyNT();

  typedef enum
  {
    MODE_NA = -1,
    MODE_EN = 0,
    MODE_VN = 1,
  } eMode;

  typedef enum
  {
    FT_PROCESS_NAME = 0,
    FT_WINDOW_NAME  = 1,
    FT_WINDOW_CLASS = 2,
    FT_COUNT
  } eFilterType;

  typedef std::tuple<const std::tstring, const std::tstring, const std::tstring> TWndInfo;

  vu::VUResult Initialize();
  bool IsReady() const;

  eMode GetModeState() const;
  void  SwitchMode(const eMode mode);

  int LoadFilterList(const std::tstring& filePath);

  eMode Determine(
    const std::tstring& processName,
    const std::tstring& windowName,
    const std::tstring& windowClass);

  CUnikeyNT::eMode DetermineByWindowHandle(const HWND hWnd);

private:
  typedef std::map<std::tstring, eMode> TFilterData;
  typedef std::map<eFilterType, TFilterData> TFilterLists;
  typedef std::map<eFilterType, std::tstring> TSections;

  TSections m_Sections;
  TFilterLists m_FilterList;

  TFilterData ParseFilter(const std::vector<std::tstring>& filter);
  TWndInfo GetInfoByWindowHandle(const HWND windowHandle);
};