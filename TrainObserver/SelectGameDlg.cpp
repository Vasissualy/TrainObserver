#include "stdafx.h"
#include "SelectGameDlg.h"



SelectGameDlg::SelectGameDlg(const std::map<uint32_t, std::string>& games)
	: m_games(games)
	, m_gameId(0)
{
	
}


SelectGameDlg::~SelectGameDlg()
{
}

LRESULT SelectGameDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	CenterWindow(::GetDesktopWindow());
	m_listBox.Attach(GetDlgItem(IDC_LIST));
	
	int nItem = 0;
	for (const auto& pair : m_games)
	{
		int idx = m_listBox.AddString(pair.second.c_str());
		if (idx != LB_ERR)
		{
			m_listBox.SetItemData(idx, pair.first);
			if (nItem == 0)
			{
				m_listBox.SetCurSel(idx);
			}
		}
		++nItem;
	}

	return TRUE;
}

LRESULT SelectGameDlg::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	if (wID == IDOK)
	{
		m_gameId = m_listBox.GetItemData(m_listBox.GetCurSel());
	}
	EndDialog(wID);
	return 0;
}



LRESULT SelectGameDlg::OnLbnSelchangeList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	int idx = m_listBox.GetCurSel();
	::EnableWindow(GetDlgItem(IDOK), idx == LB_ERR ? FALSE : TRUE);
	return 0;
}


LRESULT SelectGameDlg::OnLbnDblclkList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	int idx = m_listBox.GetCurSel();
	if (idx != LB_ERR)
	{
		m_gameId = m_listBox.GetItemData(m_listBox.GetCurSel());
		EndDialog(IDOK);
	}

	return 0;
}
