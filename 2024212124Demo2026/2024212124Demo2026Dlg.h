
// 2024212124Demo2026Dlg.h: 头文件
//
// AI 指导主界面入口相关说明：
// 本文件虽然主要声明主界面对话框，但其中也包含“AI 指导”按钮的入口声明。
// 因此这里补充说明：主界面的职责是选中学生、收集学生上下文，然后打开 AI 指导窗口。

#pragma once

#include "AiGuideDlg.h"


// CMy2024212124Demo2026Dlg 对话框
class CMy2024212124Demo2026Dlg : public CDialogEx
{
// 构造
public:
	CMy2024212124Demo2026Dlg(CWnd* pParent = nullptr);	// 标准构造函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_MY2024212124DEMO2026_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedCancel();

	// 学生列表控件与数据展示函数：
	// AI 指导功能依赖用户先在这里选中某名学生，再从对应行读取学生信息。
	CListCtrl m_list;
	void ShowData();

	// 下方编辑框用于展示或编辑学生信息；
	// AI 指导入口会优先以当前列表中选中的学生数据作为上下文来源。
	CEdit m_edtsno;
	CEdit m_edtsname;
	CEdit m_edtssex;
	CEdit m_edtsage;
	CEdit m_edtsdept;
	afx_msg void OnNMClickListShow(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnBnClickedButtonAdd();
	afx_msg void OnBnClickedButtonUpdate();
	afx_msg void OnBnClickedButtonDelete();

	
	CButton m_btnCourse1;
	CButton m_btnCourse2;
	CButton m_btnCourse3;
	CButton m_btnCourse4;
	afx_msg void OnBnClickedButtonSelectcourse();
	afx_msg void OnBnClickedButtonResetcourse();
	CButton m_btnCourse5;

	// AI 指导按钮事件：
	// 该函数在 cpp 中负责查询已选/未选课程并创建新的 AI 指导窗口。
  afx_msg void OnBnClickedButtonAiGuide();

private:
};
