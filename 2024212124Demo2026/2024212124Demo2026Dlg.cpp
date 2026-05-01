
// 2024212124Demo2026Dlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "2024212124Demo2026.h"
#include "2024212124Demo2026Dlg.h"
#include "afxdialogex.h"
#include"CLoginDlg.h"
#include "AiGuideDlg.h"
#include <afxtempl.h>
#include <vector>
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

extern CDatabase my_db;

// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
public:
	
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CMy2024212124Demo2026Dlg 对话框



CMy2024212124Demo2026Dlg::CMy2024212124Demo2026Dlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_MY2024212124DEMO2026_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMy2024212124Demo2026Dlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_SHOW, m_list);
	DDX_Control(pDX, IDC_EDIT_SNO, m_edtsno);
	DDX_Control(pDX, IDC_EDIT_SNAME, m_edtsname);
	DDX_Control(pDX, IDC_EDIT_SSEX, m_edtssex);
	DDX_Control(pDX, IDC_EDIT_SAGE, m_edtsage);
	DDX_Control(pDX, IDC_EDIT_SDEPT, m_edtsdept);
	DDX_Control(pDX, IDC_CHECK_C1, m_btnCourse1);
	DDX_Control(pDX, IDC_CHECK_C2, m_btnCourse2);
	DDX_Control(pDX, IDC_CHECK_C3, m_btnCourse3);
	DDX_Control(pDX, IDC_CHECK_C4, m_btnCourse4);
	DDX_Control(pDX, IDC_CHECK_C5, m_btnCourse5);
}

BEGIN_MESSAGE_MAP(CMy2024212124Demo2026Dlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDCANCEL, &CMy2024212124Demo2026Dlg::OnBnClickedCancel)	
	
	ON_NOTIFY(NM_CLICK, IDC_LIST_SHOW, &CMy2024212124Demo2026Dlg::OnNMClickListShow)
	ON_BN_CLICKED(IDC_BUTTON_ADD, &CMy2024212124Demo2026Dlg::OnBnClickedButtonAdd)
	ON_BN_CLICKED(IDC_BUTTON_UPDATE, &CMy2024212124Demo2026Dlg::OnBnClickedButtonUpdate)
    ON_BN_CLICKED(IDC_BUTTON_DELETE, &CMy2024212124Demo2026Dlg::OnBnClickedButtonDelete)
	ON_BN_CLICKED(IDC_BUTTON_SELECTCOURSE, &CMy2024212124Demo2026Dlg::OnBnClickedButtonSelectcourse)
	ON_BN_CLICKED(IDC_BUTTON_RESETCOURSE, &CMy2024212124Demo2026Dlg::OnBnClickedButtonResetcourse)
	ON_BN_CLICKED(IDC_BUTTON_AI_GUIDE, &CMy2024212124Demo2026Dlg::OnBnClickedButtonAiGuide)
END_MESSAGE_MAP()


// CMy2024212124Demo2026Dlg 消息处理程序

BOOL CMy2024212124Demo2026Dlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);// 设置大图标
	SetIcon(m_hIcon, FALSE);// 设置小图标

	// TODO: 在此添加额外的初始化代码
	// 1. 数据库初始化（原工程已在登录阶段建立连接，此处不重复连接）

	// 2. list 控件初始化
	m_list.InsertColumn(0, _T("学号"), LVCFMT_LEFT, 100);
	m_list.InsertColumn(1, _T("姓名"), LVCFMT_LEFT, 70);
	m_list.InsertColumn(2, _T("性别"), LVCFMT_LEFT, 70);
	m_list.InsertColumn(3, _T("年龄"), LVCFMT_LEFT, 70);
	m_list.InsertColumn(4, _T("所在系"), LVCFMT_LEFT, 150);
	m_list.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

	// 3. 展示学生数据
	ShowData();
	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CMy2024212124Demo2026Dlg::OnBnClickedButtonAiGuide()
{
	// ============================================================
	// AI 指导入口说明：
	// 本函数是主界面进入 AI 指导能力的唯一入口。
	// 处理流程可以概括为：
	// 1. 先确认用户在学生列表中选中了哪一名学生；
	// 2. 根据该学生查询已选课程、全部课程，并推导未选课程；
	// 3. 将这些上下文一次性传给新的 AI 指导窗口；
	// 4. 由 AI 指导窗口继续负责会话管理、模型切换、追问与临时文件隔离。
	//
	// 功能：AI选课指导
	// 要求：选中学生后读取学生信息、已选课程、未选课程、全部课程，
	//       打开 AI 对话框并传入上下文。
	// ============================================================

	// 1. 获取当前 List 控件中被选中的学生所在行号
	int nSelectedRow = m_list.GetNextItem(-1, LVNI_SELECTED);

	// 2. 如果没有选中学生，则不能执行 AI 选课指导
	if (nSelectedRow == -1)
	{
		AfxMessageBox(_T("请先在表格中选中一名学生！"));
		return;
	}

	// 3. 读取学生基本信息
	CString strsno = m_list.GetItemText(nSelectedRow, 0);
	CString strsname = m_list.GetItemText(nSelectedRow, 1);
	CString strssex = m_list.GetItemText(nSelectedRow, 2);
	CString strsage = m_list.GetItemText(nSelectedRow, 3);
	CString strsdept = m_list.GetItemText(nSelectedRow, 4);

	// 4. 查询已选课程：
	// 这里除了把课程文本组织出来给界面展示，
	// 还会顺便把课程号收集到 selectedCourseNos 中，
	// 以便后续在“全部课程”里判断哪些课尚未选择。
	CString selectedCoursesText;
	CString allCoursesText;
	CString unselectedCoursesText;

	CArray<CString, CString> selectedCourseNos;

	try
	{
		CRecordset selectedSet(&my_db);
		CString sqlSelected;
		sqlSelected.Format(
			_T("select course.cno, course.cname, sc.grade ")
			_T("from sc join course on sc.cno = course.cno where sc.sno='%s'"),
			(LPCTSTR)strsno
		);

		if (selectedSet.Open(AFX_DB_USE_DEFAULT_TYPE, sqlSelected))
		{
			while (!selectedSet.IsEOF())
			{
				CString cno, cname, grade;
				selectedSet.GetFieldValue((short)0, cno);
				selectedSet.GetFieldValue((short)1, cname);
				selectedSet.GetFieldValue((short)2, grade);
				selectedCourseNos.Add(cno);

				CString line;
				line.Format(_T("%s %s 成绩:%s\r\n"), (LPCTSTR)cno, (LPCTSTR)cname, (LPCTSTR)grade);
				selectedCoursesText += line;

				selectedSet.MoveNext();
			}
		}
		selectedSet.Close();

		// 5. 查询全部课程，并据此反推出未选课程：
		// 逻辑上不是直接查“未选课程表”，
		// 而是通过“全部课程 - 已选课程”的方式构造未选列表，
		// 这样可以保证 AI 始终看到系统当前完整的可选范围。
		CRecordset allSet(&my_db);
		if (allSet.Open(AFX_DB_USE_DEFAULT_TYPE, _T("select cno, cname from course order by cno")))
		{
			while (!allSet.IsEOF())
			{
				CString cno, cname;
				allSet.GetFieldValue((short)0, cno);
				allSet.GetFieldValue((short)1, cname);
				CString line;
				line.Format(_T("%s %s\r\n"), (LPCTSTR)cno, (LPCTSTR)cname);
				allCoursesText += line;

				bool isSelected = false;
				for (INT_PTR i = 0; i < selectedCourseNos.GetSize(); ++i)
				{
					if (selectedCourseNos[i].CompareNoCase(cno) == 0)
					{
						isSelected = true;
						break;
					}
				}

				if (!isSelected)
				{
					unselectedCoursesText += line;
				}

				allSet.MoveNext();
			}
		}
		allSet.Close();
	}
	catch (CDBException* pe)
	{
		pe->ReportError();
		pe->Delete();
		return;
	}

    // 6. 创建新的 AI 指导窗口：
    // 这里显式创建一个新的对话框实例，
    // 保证每次点击“AI 指导”都是新的会话窗口，而不是复用旧窗口。
    CAiGuideDlg* pDlg = new CAiGuideDlg(this);
    pDlg->Create(IDD_DIALOG_AI_GUIDE, this);

    // 7. 传入学生与课程上下文：
    // AI 对话框本身不再回头查询主界面列表，而是直接使用这里传入的数据，
    // 这样能降低耦合，也方便后续做窗口级会话隔离。
    pDlg->SetStudentContext(
        strsno,
        strsname,
        strssex,
        strsage,
        strsdept,
        selectedCoursesText,
        unselectedCoursesText,
        allCoursesText
    );
    // 8. 刷新摘要并显示窗口：
    // 摘要区会显示当前学生信息和课程信息，
    // 聊天区、图片区、会话历史区则由新窗口自行初始化为空。
    pDlg->RefreshStudentSummary();
    pDlg->ShowWindow(SW_SHOW);
    pDlg->SetForegroundWindow();
}

void CMy2024212124Demo2026Dlg::ShowData()
{
	m_list.DeleteAllItems();   // 先清空列表原有数据

	CRecordset my_set(&my_db); // 记录集对象，绑定数据库连接,申明CRecordset类的对象，用于调取表格数据

	if (my_set.Open(AFX_DB_USE_DEFAULT_TYPE, _T("select * from student")))
	{
		AfxMessageBox(_T("读取数据集成功"));// 打开成功
	}
	else
	{
		AfxMessageBox(_T("读取数据集失败！请检查连接是否异常，或 student 表是否存在！"));
		return;
	}

	if (my_set.IsBOF())  //判断数据集是否为空
	{
		AfxMessageBox(_T("该表格数据集为空"));
		my_set.Close();
		return;
	}

	int i = 0;
	CString str_sno = _T("");
	CString str_sname = _T("");
	CString str_ssex = _T("");
	CString str_sage = _T("");
	CString str_sdept = _T("");

	while (!my_set.IsEOF())
	{
		my_set.GetFieldValue((short)0, str_sno);  //0 代表第一列，取第一个属性（第一列）
		m_list.InsertItem(i, str_sno); //i表示行，该句表示将取的第一个数据插入第i行第一列

		my_set.GetFieldValue((short)1, str_sname);
		m_list.SetItemText(i, 1, str_sname);//1表示第二列

		my_set.GetFieldValue((short)2, str_ssex);
		m_list.SetItemText(i, 2, str_ssex);

		my_set.GetFieldValue((short)3, str_sage);
		m_list.SetItemText(i, 3, str_sage);

		my_set.GetFieldValue((short)4, str_sdept);
		m_list.SetItemText(i, 4, str_sdept);

		my_set.MoveNext();//数据指针移动到下一行
		i++;
	}

	my_set.Close();
}
void CMy2024212124Demo2026Dlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CMy2024212124Demo2026Dlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CMy2024212124Demo2026Dlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CMy2024212124Demo2026Dlg::OnBnClickedCancel()
{
	// TODO: 在此添加控件通知处理程序代码
	if (my_db.IsOpen())//判断是否处于数据库连接状态
	{
		my_db.Close();//如果当前处于连接状态，从数据源断开连接，释放系统资源
	}
	CDialogEx::OnCancel();
}



void CMy2024212124Demo2026Dlg::OnNMClickListShow(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	CString strsno, strsname, strssex, strsage, strsdept;//用于保存从list控件上获取的值
	if (pNMItemActivate->iItem != -1)//pNMItemActivate->iItem为鼠标焦点处对应的行号
	{
		strsno = m_list.GetItemText(pNMItemActivate->iItem, 0);
		m_edtsno.SetWindowText(strsno);
		strsname = m_list.GetItemText(pNMItemActivate->iItem, 1);
		m_edtsname.SetWindowText(strsname);
		strssex = m_list.GetItemText(pNMItemActivate->iItem, 2);
		m_edtssex.SetWindowText(strssex);
		strsage = m_list.GetItemText(pNMItemActivate->iItem, 3);
		m_edtsage.SetWindowText(strsage);
		strsdept = m_list.GetItemText(pNMItemActivate->iItem, 4);
		m_edtsdept.SetWindowText(strsdept);
		UpdateData(FALSE);//刷新界面将底层代码的数据传递给上层的界面
	}
	*pResult = 0;
}




void CMy2024212124Demo2026Dlg::OnBnClickedButtonAdd()
{
	UpdateData(TRUE);//刷新界面，把数据传递给底层代码
	CString strsno, strsname, strssex, strsage, strsdept;//用于保存从edit控件上获取的值
	//从界面上的edit控件获取用户输入的值
	m_edtsno.GetWindowText(strsno);
	m_edtsname.GetWindowText(strsname);
	m_edtssex.GetWindowText(strssex);
	m_edtsage.GetWindowText(strsage);
	m_edtsdept.GetWindowText(strsdept);
	if (strsno == _T(""))
	{
		AfxMessageBox(_T("学号为主码，取值不能为空！"));
		return;//等待用户重新输入
	}
	else
	{
		if (strsname == _T(""))
			strsname = _T("NULL");
		else
			strsname = _T("'") + strsname + _T("'");
		if (!strssex.CompareNoCase(_T("男")) || !strssex.CompareNoCase(_T("女")))
		{
			strssex = _T("'") + strssex + _T("'");
		}
		else {
			AfxMessageBox(_T("性别只能输入“男”或“女”！"));
			return; return;//等待用户重新输入 
		}

		if (strsage == _T(""))
			strsage = _T("NULL");
		if (strsdept == _T(""))
			strsdept = _T("NULL");
		else
			strsdept = _T("'") + strsdept + _T("'");
		CString mstrsql;
		mstrsql.Format(_T("insert into student values('%s',%s,%s,%s,%s)"), strsno, strsname, strssex,
			strsage, strsdept);
		//AfxMessageBox(mstrsql);//测试生成的sql语言，如果添加数据失败，可打开这个语句查看SQL代码是否符合语法

		try
		{
			my_db.ExecuteSQL(mstrsql);
		}
		catch (CDBException* pe)
		{
			//如果有异常发生，弹出错误消息框，帮助纠正bug
			pe->ReportError();
			pe->Delete();
		}
		ShowData();//加载数据
	}
}

void CMy2024212124Demo2026Dlg::OnBnClickedButtonUpdate()
{
	// ============================================================
	// 2024212124 刘泽宇
	// 功能：修改 student 表中的学生信息
	// 说明：
	// 1. 如果没有修改学号，则直接更新 student 表。
	// 2. 如果修改了学号，并且该学生在 SC 表中存在选课记录，
	//    则先暂存 SC 表中的选课记录，再删除旧学号选课记录，
	//    然后修改 student 表中的学号，最后用新学号恢复选课记录。
	// ============================================================

	// 1. 获取当前 List 控件中被选中的行号
	int nSelectedRow = m_list.GetNextItem(-1, LVNI_SELECTED);

	if (nSelectedRow == -1)
	{
		AfxMessageBox(_T("请先在表格中选中一行需要修改的数据！"));
		return;
	}

	// 2. 获取原学号，用于定位原始学生记录
	CString strOldSno;
	strOldSno = m_list.GetItemText(nSelectedRow, 0);

	// 3. 读取编辑框中的新数据
	CString strsno;
	CString strsname;
	CString strssex;
	CString strsage;
	CString strsdept;

	m_edtsno.GetWindowText(strsno);
	m_edtsname.GetWindowText(strsname);
	m_edtssex.GetWindowText(strssex);
	m_edtsage.GetWindowText(strsage);
	m_edtsdept.GetWindowText(strsdept);

	// 4. 学号不能为空
	if (strsno.IsEmpty())
	{
		AfxMessageBox(_T("学号为主码，不能为空！"));
		return;
	}

	// 5. 姓名为空时写入 NULL，否则作为字符串写入数据库
	if (strsname.IsEmpty())
	{
		strsname = _T("NULL");
	}
	else
	{
		strsname = _T("'") + strsname + _T("'");
	}

	// 6. 性别只能输入“男”或“女”
	if (!strssex.CompareNoCase(_T("男")) || !strssex.CompareNoCase(_T("女")))
	{
		strssex = _T("'") + strssex + _T("'");
	}
	else
	{
		AfxMessageBox(_T("性别只能输入“男”或“女”！"));
		return;
	}

	// 7. 年龄为空时写入 NULL，否则作为数字写入数据库
	if (strsage.IsEmpty())
	{
		strsage = _T("NULL");
	}

	// 8. 所在系为空时写入 NULL，否则作为字符串写入数据库
	if (strsdept.IsEmpty())
	{
		strsdept = _T("NULL");
	}
	else
	{
		strsdept = _T("'") + strsdept + _T("'");
	}

	CString mstrsql;

	// 事务是否已经开启。用于出错时判断是否需要回滚。
	BOOL bTransStarted = FALSE;

	try
	{
		// ========================================================
		// 情况一：学号没有变化，只修改姓名、性别、年龄、所在系
		// ========================================================
		if (strOldSno == strsno)
		{
			mstrsql.Format(
				_T("update student set sname=%s, ssex=%s, sage=%s, sdept=%s where sno='%s'"),
				(LPCTSTR)strsname,
				(LPCTSTR)strssex,
				(LPCTSTR)strsage,
				(LPCTSTR)strsdept,
				(LPCTSTR)strOldSno
			);

			my_db.ExecuteSQL(mstrsql);
			AfxMessageBox(_T("修改成功！"));
			ShowData();
			return;
		}

		// ========================================================
		// 情况二：学号发生变化
		// 先检查新学号是否已经存在，避免主键冲突
		// ========================================================
		CString strCheckSql;
		strCheckSql.Format(
			_T("select sno from student where sno='%s'"),
			(LPCTSTR)strsno
		);

		CRecordset rsCheck(&my_db);
		rsCheck.Open(AFX_DB_USE_DEFAULT_TYPE, strCheckSql);

		if (!rsCheck.IsEOF())
		{
			rsCheck.Close();
			AfxMessageBox(_T("修改失败：新学号已经存在，不能重复！"));
			return;
		}

		rsCheck.Close();

		// ========================================================
		// 读取旧学号在 SC 表中的选课记录，暂存在程序变量中
		// ========================================================
		std::vector<CString> vecCno;
		std::vector<CString> vecGrade;

		CString strQuerySc;
		strQuerySc.Format(
			_T("select cno, grade from sc where sno='%s'"),
			(LPCTSTR)strOldSno
		);

		CRecordset rsSc(&my_db);
		rsSc.Open(AFX_DB_USE_DEFAULT_TYPE, strQuerySc);

		CString strCno;
		CString strGrade;

		while (!rsSc.IsEOF())
		{
			rsSc.GetFieldValue((short)0, strCno);
			rsSc.GetFieldValue((short)1, strGrade);

			vecCno.push_back(strCno);
			vecGrade.push_back(strGrade);

			rsSc.MoveNext();
		}

		rsSc.Close();

		// ========================================================
		// 开启事务，保证 student 和 SC 两张表要么一起修改成功，
		// 要么一起回滚，避免只改了一半导致数据不一致。
		// ========================================================
		if (my_db.CanTransact())
		{
			my_db.BeginTrans();
			bTransStarted = TRUE;
		}

		// 1）先删除旧学号在 SC 表中的选课记录
		mstrsql.Format(
			_T("delete from sc where sno='%s'"),
			(LPCTSTR)strOldSno
		);
		my_db.ExecuteSQL(mstrsql);

		// 2）再修改 student 表中的学号及其他信息
		mstrsql.Format(
			_T("update student set sno='%s', sname=%s, ssex=%s, sage=%s, sdept=%s where sno='%s'"),
			(LPCTSTR)strsno,
			(LPCTSTR)strsname,
			(LPCTSTR)strssex,
			(LPCTSTR)strsage,
			(LPCTSTR)strsdept,
			(LPCTSTR)strOldSno
		);
		my_db.ExecuteSQL(mstrsql);

		// 3）最后用新学号恢复原来的选课记录
		for (size_t i = 0; i < vecCno.size(); i++)
		{
			CString strGradeValue;

			// 如果成绩为空，则插入 NULL
			if (vecGrade[i].IsEmpty())
			{
				strGradeValue = _T("NULL");
			}
			else
			{
				strGradeValue = vecGrade[i];
			}

			mstrsql.Format(
				_T("insert into sc(sno, cno, grade) values('%s', '%s', %s)"),
				(LPCTSTR)strsno,
				(LPCTSTR)vecCno[i],
				(LPCTSTR)strGradeValue
			);

			my_db.ExecuteSQL(mstrsql);
		}

		// 4）提交事务
		if (bTransStarted)
		{
			my_db.CommitTrans();
			bTransStarted = FALSE;
		}

		AfxMessageBox(_T("修改成功！相关选课记录已同步更新。"));
		ShowData();
	}
	catch (CDBException* pe)
	{
		// 如果执行过程中出错，回滚事务，避免 student 和 SC 数据不一致
		if (bTransStarted)
		{
			my_db.Rollback();
			bTransStarted = FALSE;
		}

		CString strMsg;
		strMsg.Format(
			_T("修改失败：数据库执行出错！\n错误信息：%s"),
			(LPCTSTR)pe->m_strError
		);
		AfxMessageBox(strMsg);

		pe->Delete();
	}
}

void CMy2024212124Demo2026Dlg::OnBnClickedButtonDelete()
{
	// ============================================================
	// 2024212124 刘泽宇
	// 功能：删除 student 表中选中的学生信息
	// 说明：
	// 如果该学生在 SC 表中存在选课记录，直接删除 student 表记录
	// 会因为外键约束失败。因此本函数先删除 SC 表中的关联选课记录，
	// 再删除 student 表中的学生记录，并使用事务保证操作一致性。
	// ============================================================

	// 1. 获取当前 List 控件中被选中的行号
	int nSelectedRow = m_list.GetNextItem(-1, LVNI_SELECTED);

	// 2. 如果没有选中任何一行，则不能执行删除
	if (nSelectedRow == -1)
	{
		AfxMessageBox(_T("请先在表格中选中一行需要删除的数据！"));
		return;
	}

	// 3. 从选中行中取出学号
	// 学号 sno 是 student 表的主码，也是 SC 表中的外键字段。
	CString strsno;
	strsno = m_list.GetItemText(nSelectedRow, 0);

	// 4. 学号为空时，不能继续删除
	if (strsno.IsEmpty())
	{
		AfxMessageBox(_T("当前选中行的学号为空，无法删除！"));
		return;
	}

	// 5. 删除前进行二次确认，避免误删
	CString strConfirm;
	strConfirm.Format(
		_T("确定要删除学号为 %s 的学生记录吗？\n该操作会同时删除该学生在 SC 表中的选课记录。"),
		(LPCTSTR)strsno
	);

	if (AfxMessageBox(strConfirm, MB_YESNO | MB_ICONQUESTION) != IDYES)
	{
		return;
	}

	CString mstrsql;

	// 6. 事务标志，用于异常时判断是否需要回滚
	BOOL bTransStarted = FALSE;

	try
	{
		// 7. 开启事务
		// 目的是保证 SC 表和 student 表要么一起删除成功，要么一起回滚。
		if (my_db.CanTransact())
		{
			my_db.BeginTrans();
			bTransStarted = TRUE;
		}

		// 8. 先删除 SC 表中该学生的选课记录
		// 因为 SC.sno 引用了 student.sno，如果不先删除 SC 记录，
		// 直接删除 student 记录可能会违反外键约束。
		mstrsql.Format(
			_T("delete from sc where sno='%s'"),
			(LPCTSTR)strsno
		);
		my_db.ExecuteSQL(mstrsql);

		// 9. 再删除 student 表中的学生记录
		mstrsql.Format(
			_T("delete from student where sno='%s'"),
			(LPCTSTR)strsno
		);
		my_db.ExecuteSQL(mstrsql);

		// 10. 提交事务
		if (bTransStarted)
		{
			my_db.CommitTrans();
			bTransStarted = FALSE;
		}

		AfxMessageBox(_T("删除成功！该学生及其选课记录已同步删除。"));

		// 11. 删除成功后刷新 List 控件
		ShowData();

		// 12. 清空下方编辑框，避免继续显示已经删除的数据
		m_edtsno.SetWindowText(_T(""));
		m_edtsname.SetWindowText(_T(""));
		m_edtssex.SetWindowText(_T(""));
		m_edtsage.SetWindowText(_T(""));
		m_edtsdept.SetWindowText(_T(""));

		// 13. 清空选课区复选框状态
		m_btnCourse1.SetCheck(BST_UNCHECKED);
		m_btnCourse2.SetCheck(BST_UNCHECKED);
		m_btnCourse3.SetCheck(BST_UNCHECKED);
		m_btnCourse4.SetCheck(BST_UNCHECKED);
		m_btnCourse5.SetCheck(BST_UNCHECKED);
	}
	catch (CDBException* pe)
	{
		// 14. 如果删除过程中发生错误，回滚事务，避免只删了一部分数据
		if (bTransStarted)
		{
			my_db.Rollback();
			bTransStarted = FALSE;
		}

		CString strMsg;
		strMsg.Format(
			_T("删除失败：数据库执行出错！\n错误信息：%s"),
			(LPCTSTR)pe->m_strError
		);
		AfxMessageBox(strMsg);

		pe->Delete();
	}
}

void CMy2024212124Demo2026Dlg::OnBnClickedButtonSelectcourse()
{
	// ============================================================
	// 2024212124 刘泽宇
	// 功能：选课
	// 说明：
	// 1. 在 List 控件中选中一名学生。
	// 2. 根据勾选的课程，向 SC 表插入选课记录。
	// 3. 插入前先检查该学生是否已经选择过该课程。
	//    如果已经选过，则跳过；如果没有选过，则插入。
	// ============================================================

	// 1. 获取当前 List 控件中被选中的学生所在行号
	int nSelectedRow = m_list.GetNextItem(-1, LVNI_SELECTED);

	// 2. 如果没有选中学生，则不能执行选课
	if (nSelectedRow == -1)
	{
		AfxMessageBox(_T("请先在表格中选中一名学生！"));
		return;
	}

	// 3. 从选中行中获取学号
	CString strsno;
	strsno = m_list.GetItemText(nSelectedRow, 0);

	// 4. 学号不能为空
	if (strsno.IsEmpty())
	{
		AfxMessageBox(_T("学号不能为空，无法执行选课操作！"));
		return;
	}

	// 5. 判断是否至少勾选了一门课程
	if (m_btnCourse1.GetCheck() != BST_CHECKED &&
		m_btnCourse2.GetCheck() != BST_CHECKED &&
		m_btnCourse3.GetCheck() != BST_CHECKED &&
		m_btnCourse4.GetCheck() != BST_CHECKED &&
		m_btnCourse5.GetCheck() != BST_CHECKED)
	{
		AfxMessageBox(_T("请至少勾选一门课程！"));
		return;
	}

	// 6. 用数组保存课程号和对应 Check Box 的勾选状态
	CString courseNos[5] = {
		_T("1"),
		_T("2"),
		_T("3"),
		_T("4"),
		_T("5")
	};

	BOOL courseChecked[5] = {
		m_btnCourse1.GetCheck() == BST_CHECKED,
		m_btnCourse2.GetCheck() == BST_CHECKED,
		m_btnCourse3.GetCheck() == BST_CHECKED,
		m_btnCourse4.GetCheck() == BST_CHECKED,
		m_btnCourse5.GetCheck() == BST_CHECKED
	};

	CString mstrsql;

	// 7. 记录实际插入数量和跳过数量
	int insertCount = 0;
	int skipCount = 0;

	try
	{
		// 8. 逐门课程处理
		for (int i = 0; i < 5; i++)
		{
			// 如果当前课程没有被勾选，则跳过
			if (!courseChecked[i])
			{
				continue;
			}

			// 9. 插入前先查询 SC 表，判断该学生是否已经选过该课程
			CString strCheckSql;
			strCheckSql.Format(
				_T("select sno from sc where sno='%s' and cno='%s'"),
				(LPCTSTR)strsno,
				(LPCTSTR)courseNos[i]
			);

			CRecordset rsCheck(&my_db);
			rsCheck.Open(AFX_DB_USE_DEFAULT_TYPE, strCheckSql);

			// 10. 如果查询结果不为空，说明已经选过该课程，不能重复插入
			if (!rsCheck.IsEOF())
			{
				rsCheck.Close();
				skipCount++;
				continue;
			}

			rsCheck.Close();

			// 11. 如果没有选过该课程，则插入新的选课记录
			mstrsql.Format(
				_T("insert into sc(sno, cno, grade) values('%s', '%s', NULL)"),
				(LPCTSTR)strsno,
				(LPCTSTR)courseNos[i]
			);

			my_db.ExecuteSQL(mstrsql);
			insertCount++;
		}

		// 12. 根据插入结果给出提示
		CString strMsg;

		if (insertCount > 0 && skipCount > 0)
		{
			strMsg.Format(
				_T("选课完成！成功插入 %d 门新课程，跳过 %d 门已选课程。"),
				insertCount,
				skipCount
			);
			AfxMessageBox(strMsg);
		}
		else if (insertCount > 0 && skipCount == 0)
		{
			strMsg.Format(
				_T("选课成功！成功插入 %d 门新课程。"),
				insertCount
			);
			AfxMessageBox(strMsg);
		}
		else if (insertCount == 0 && skipCount > 0)
		{
			AfxMessageBox(_T("选课失败：所勾选课程均已存在，不能重复选课！"));
		}
	}
	catch (CDBException* pe)
	{
		CString strError = pe->m_strError;

		CString strMsg;
		strMsg.Format(
			_T("选课失败：数据库执行出错！\n错误信息：%s"),
			(LPCTSTR)strError
		);
		AfxMessageBox(strMsg);

		pe->Delete();
	}
}

void CMy2024212124Demo2026Dlg::OnBnClickedButtonResetcourse()
{
	// ============================================================
	// 2024212124 刘泽宇
	// 功能：重置学生选课信息
	// 要求：在 List 控件中选中一名学生后，点击“重置”按钮，
	//      删除 SC 表中该学生对应的选课记录，并同步更新到底层数据库。
	// ============================================================

	// 1. 获取当前 List 控件中被选中的学生所在行号
	int nSelectedRow = m_list.GetNextItem(-1, LVNI_SELECTED);

	// 2. 如果没有选中学生，则不能执行重置
	if (nSelectedRow == -1)
	{
		AfxMessageBox(_T("请先在表格中选中一名学生！"));
		return;
	}

	// 3. 从选中行中获取学号
	//    SC 表通过 sno 字段保存学生选课记录，所以重置时以 sno 为条件删除。
	CString strsno;
	strsno = m_list.GetItemText(nSelectedRow, 0);

	// 4. 学号不能为空
	if (strsno.IsEmpty())
	{
		AfxMessageBox(_T("学号不能为空，无法执行重置操作！"));
		return;
	}

	// 5. 删除前进行二次确认，避免误删该学生的选课记录
	CString strConfirm;
	strConfirm.Format(
		_T("确定要重置学号为 %s 的学生选课信息吗？\n该操作会删除该学生在 SC 表中的所有选课记录。"),
		(LPCTSTR)strsno
	);

	if (AfxMessageBox(strConfirm, MB_YESNO | MB_ICONQUESTION) != IDYES)
	{
		return;
	}

	// 6. 拼接 SQL 语句
	//    删除 SC 表中该学生的所有选课记录。
	CString mstrsql;
	mstrsql.Format(
		_T("delete from sc where sno='%s'"),
		(LPCTSTR)strsno
	);

	// 7. 可以临时打开这一句，用来检查 SQL 是否正确
	// AfxMessageBox(mstrsql);

	// 8. 执行 SQL
	try
	{
		my_db.ExecuteSQL(mstrsql);

		// 9. 重置成功后，取消界面上所有课程复选框的勾选状态
		m_btnCourse1.SetCheck(BST_UNCHECKED);
		m_btnCourse2.SetCheck(BST_UNCHECKED);
		m_btnCourse3.SetCheck(BST_UNCHECKED);
		m_btnCourse4.SetCheck(BST_UNCHECKED);
		m_btnCourse5.SetCheck(BST_UNCHECKED);

		AfxMessageBox(_T("重置成功！该学生的选课记录已删除。"));
	}
	catch (CDBException* pe)
	{
		// 10. 如果 SC 表不存在、字段名错误或数据库连接异常，会进入这里
		CString strError = pe->m_strError;

		CString strMsg;
		strMsg.Format(_T("重置失败：数据库执行出错！\n错误信息：%s"), (LPCTSTR)strError);
		AfxMessageBox(strMsg);

		pe->Delete();
	}
}
