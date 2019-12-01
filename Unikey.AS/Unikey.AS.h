
// Unikey.AS.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols

// CUnikeyASApp:
// See Unikey.AS.cpp for the implementation of this class
//

class CUnikeyASApp : public CWinApp
{
public:
  CUnikeyASApp();

  // Overrides
public:
  virtual BOOL InitInstance();

  // Implementation
  bool IsRunning();

  DECLARE_MESSAGE_MAP()
};

extern CUnikeyASApp theApp;