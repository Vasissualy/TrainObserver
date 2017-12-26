#pragma once

#include <map>
#include "resource.h"


class SelectGameDlg : public CDialogImpl<SelectGameDlg>
{	
public:
	enum
	{
		IDD = IDD_DLG_SELECT_GAME
	};

	SelectGameDlg(const std::map<uint32_t, std::string>& games);
	~SelectGameDlg();

	uint32_t getGameID() { return m_gameId;  }

protected:

	BEGIN_MSG_MAP(SelectGameDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
		COMMAND_HANDLER(IDC_LIST, LBN_SELCHANGE, OnLbnSelchangeList)
		COMMAND_HANDLER(IDC_LIST, LBN_DBLCLK, OnLbnDblclkList)
	END_MSG_MAP()

	LRESULT		OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT		OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnLbnSelchangeList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

private:
	std::map<uint32_t, std::string> m_games;
	CListBox m_listBox;
	uint32_t m_gameId;
public:
	LRESULT OnLbnDblclkList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
};

