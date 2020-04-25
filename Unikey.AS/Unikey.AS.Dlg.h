
// Unikey.AS.Dlg.h : header file
//

#pragma once
#include "afxwin.h"
#include "Unikey.AS.h"
#include "Unikey.NT.h"

#include <memory>

// CUnikeyASDlg dialog
class CUnikeyASDlg : public CDialogEx
{
  // Construction
public:
  CUnikeyASDlg(CWnd* pParent = NULL);	// standard constructor

  // Dialog Data
  enum { IDD = IDD_UNIKEYSWITCHER_DIALOG };

protected:
  virtual BOOL OnInitDialog();
  virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

  // Implementation
protected:
  HICON m_hIcon;
  BOOL m_TopMost;
  BOOL m_ForceMode;
  CStatic m_ModeIcon;
  CString m_Status;
  CButton m_SwitchMode;
  CButton m_ToggleButtonED;
  CBitmap m_EN, m_VN;

  // Generated message map functions
  afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
  afx_msg void OnPaint();
  afx_msg HCURSOR OnQueryDragIcon();
  DECLARE_MESSAGE_MAP()

public:
  afx_msg int  OnCreate(LPCREATESTRUCT lpCreateStruct);
  afx_msg void OnClose();
  afx_msg void OnDestroy();

  afx_msg void OnTimer(UINT_PTR nIDEvent);

  afx_msg void OnBnClickedOk();
  afx_msg void OnBnClickedOption();
  afx_msg void OnBnClickedTopMost();
  afx_msg void OnBnClickedMode();

  afx_msg void OnMenuShowMain();
  afx_msg void OnMenuAbout();
  afx_msg void OnMenuExit();

  afx_msg LRESULT OnTrayNotify(WPARAM wParam, LPARAM lParam);

private:
  CUnikeyNT::eMode m_ModeState;
  CString m_EVmode;
  UINT_PTR m_Timer;
  CWnd* m_pActiveWindow;

  bool m_StateToggleButtonED;

  CMenu m_TrayMenu;
  NOTIFYICONDATA m_Tray;

  std::unique_ptr<CUnikeyNT> m_pUnikeyNT;

  std::tstring m_AppDir;
  std::tstring m_AppName;
  std::tstring m_AppPath;
  std::tstring m_CfgPath;

  std::tstring m_AppTitle;

private:
  int  Initialize();
  void Setup();
  void SetWindowOnTopMost(bool flag = true);
  void UpdateSwitchModeButton(bool force = false);

  void UpdateToggleButtonED();

  int  TrayInitialize();
  void TrayDestroy();
  void CenterWindow();
};
