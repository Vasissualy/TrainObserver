#pragma once
#include "resource.h"
#include "message_interface.h"


class PlayerDlg : public CDialogImpl<PlayerDlg>, public ITickable
{
public:
	enum
	{
		IDD = IDD_PLAYER_DLG
	};

	PlayerDlg(AppManager::GameController*);
	virtual ~PlayerDlg();

	void maxTurn(int val);


	virtual void tick(float deltaTime) override;
	void create(HWND hWnd);
	void destroy();

protected:
	BEGIN_MSG_MAP(SelectGameDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButtonDown)
		MESSAGE_HANDLER(WM_LBUTTONUP, OnLButtonUp)
		MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
		COMMAND_HANDLER(IDC_BUTTON_PREV, BN_CLICKED, OnBnClickedButtonPrev)
		COMMAND_HANDLER(IDC_BUTTON_NEXT, BN_CLICKED, OnBnClickedButtonNext)
		COMMAND_HANDLER(IDC_BUTTON_BEGIN, BN_CLICKED, OnBnClickedButtonBegin)
		COMMAND_HANDLER(IDC_BUTTON_END, BN_CLICKED, OnBnClickedButtonEnd)
		COMMAND_HANDLER(IDC_BUTTON_PLAY, BN_CLICKED, OnBnClickedButtonPlay)
		COMMAND_HANDLER(IDC_BUTTON_STOP, BN_CLICKED, OnBnClickedButtonStop)
		COMMAND_HANDLER(IDC_BUTTON_PAUSE, BN_CLICKED, OnBnClickedButtonPause)
		NOTIFY_HANDLER(IDC_SLIDER, NM_RELEASEDCAPTURE, OnNMReleasedcaptureSlider)
	END_MSG_MAP()

	static DWORD WINAPI ThreadProc(LPVOID);
	DWORD mainLoop();
	HWND m_hParentWnd;
	HANDLE m_hThread;
	HANDLE m_hTerminateEvent;

	LRESULT		OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT		OnLButtonDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT		OnLButtonUp(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT		OnMouseMove(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT		OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT		OnBnClickedButtonPrev(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT		OnBnClickedButtonNext(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT		OnBnClickedButtonBegin(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT		OnBnClickedButtonEnd(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT		OnBnClickedButtonPlay(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT		OnBnClickedButtonStop(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT		OnBnClickedButtonPause(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

private:
	AppManager::GameController* m_pController;
	int m_nMaxTurn;
	CTrackBarCtrl m_tracker;	

	// for move window implement
	CPoint m_initPoint;
	bool m_bMouseCaptured;

	// for implement player
	float m_stepTime = 0.5f;
	bool m_bPause = true;

public:
	
	LRESULT OnNMReleasedcaptureSlider(int /*idCtrl*/, LPNMHDR pNMHDR, BOOL& /*bHandled*/);
};

