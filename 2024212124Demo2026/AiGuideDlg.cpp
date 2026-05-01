#include "pch.h"
#include "framework.h"
#include "AiGuideDlg.h"
#include "afxdialogex.h"
#include <string>
#include <vector>

// AI 指导对话框实现文件说明：
// 学号：2024212124
// 本文件负责 AI 指导窗口的完整行为实现，包括：
// 1. 初始化窗口内独立会话；
// 2. 读取模型配置并支持同窗口切换模型；
// 3. 维护当前窗口内部的聊天历史；
// 4. 生成当前会话专属 request / response 文件；
// 5. 调用 Python 脚本拿到 AI 回复；
// 6. 在关闭窗口时清理当前会话的运行时文件。

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNAMIC(CAiGuideDlg, CDialogEx)

namespace
{
// 将 ini 文件中的 \uXXXX 场景说明转回正常中文，
// 便于在模型下拉框旁边展示用户可读的说明文本。
CString DecodeUnicodeEscapes(const CString& input)
{
    CString output;
    output.Preallocate(input.GetLength());
    for (int i = 0; i < input.GetLength();)
    {
        if (i + 6 <= input.GetLength() && input[i] == _T('\\') && (input[i + 1] == _T('u') || input[i + 1] == _T('U')))
        {
            CString hex = input.Mid(i + 2, 4);
            wchar_t* endPtr = nullptr;
            unsigned long code = wcstoul(hex, &endPtr, 16);
            if (endPtr != nullptr && *endPtr == 0)
            {
                output.AppendChar((TCHAR)code);
                i += 6;
                continue;
            }
        }
        output.AppendChar(input[i]);
        ++i;
    }
    return output;
}

// 将普通文本转换成可以安全写入 JSON 字符串的形式，
// 避免引号、换行、反斜杠等字符破坏 request.json 结构。
CString EscapeJsonText(const CString& input)
{
    CString escaped;
    for (int i = 0; i < input.GetLength(); ++i)
    {
        TCHAR ch = input[i];
        switch (ch)
        {
        case _T('\\'):
            escaped += _T("\\\\");
            break;
        case _T('"'):
            escaped += _T("\\\"");
            break;
        case _T('\r'):
            escaped += _T("\\r");
            break;
        case _T('\n'):
            escaped += _T("\\n");
            break;
        case _T('\t'):
            escaped += _T("\\t");
            break;
        default:
            escaped += ch;
            break;
        }
    }
    return escaped;
}

// 把界面中“课程文本块”重新解析成 JSON 数组。
// 这样做的原因是主界面和 AI 窗口内部主要以文本形式存课表摘要，
// 而 Python 端更适合读取结构化的课程数组。
CString BuildCoursesJson(const CString& sourceText, bool includeGrade)
{
    CString json = _T("[");
    CString temp = sourceText;
    temp.Replace(_T("\r\n"), _T("\n"));

    CStringArray lines;
    int start = 0;
    while (start <= temp.GetLength())
    {
        int pos = temp.Find(_T('\n'), start);
        CString line = (pos == -1) ? temp.Mid(start) : temp.Mid(start, pos - start);
        line.Trim();
        if (!line.IsEmpty())
        {
            lines.Add(line);
        }
        if (pos == -1)
        {
            break;
        }
        start = pos + 1;
    }

    for (INT_PTR i = 0; i < lines.GetSize(); ++i)
    {
        CString line = lines[i];
        CString cno;
        CString cname;
        CString grade;

        int firstSpace = line.Find(_T(' '));
        int gradePos = line.Find(_T("成绩:"));

        if (firstSpace == -1)
        {
            cno = line;
        }
        else
        {
            cno = line.Left(firstSpace);
            if (includeGrade && gradePos != -1)
            {
                cname = line.Mid(firstSpace + 1, gradePos - firstSpace - 1);
                grade = line.Mid(gradePos + 3);
            }
            else
            {
                cname = line.Mid(firstSpace + 1);
            }
        }

        cno.Trim();
        cname.Trim();
        grade.Trim();

        CString item;
        item += _T("{\"cno\":\"") + EscapeJsonText(cno) + _T("\",");
        item += _T("\"cname\":\"") + EscapeJsonText(cname) + _T("\"");
        if (includeGrade)
        {
            if (grade.IsEmpty())
            {
                item += _T(",\"grade\":null");
            }
            else
            {
                item += _T(",\"grade\":\"") + EscapeJsonText(grade) + _T("\"");
            }
        }
        item += _T("}");

        json += item;
        if (i != lines.GetSize() - 1)
        {
            json += _T(",");
        }
    }

    json += _T("]");
    return json;
}
}

CAiGuideDlg::CAiGuideDlg(CWnd* pParent /*=nullptr*/)
    : CDialogEx(IDD_DIALOG_AI_GUIDE, pParent)
{
}

CAiGuideDlg::~CAiGuideDlg()
{
}

void CAiGuideDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_EDIT_AI_STUDENT_SUMMARY, m_editStudentSummary);
    DDX_Control(pDX, IDC_EDIT_AI_RESPONSE, m_editResponse);
    DDX_Control(pDX, IDC_EDIT_AI_QUESTION, m_editQuestion);
    DDX_Control(pDX, IDC_EDIT_AI_IMAGE_PATHS, m_editImagePaths);
}

BEGIN_MESSAGE_MAP(CAiGuideDlg, CDialogEx)
    ON_BN_CLICKED(IDC_BUTTON_AI_GENERATE, &CAiGuideDlg::OnBnClickedButtonAiGenerate)
    ON_BN_CLICKED(IDC_BUTTON_AI_SEND, &CAiGuideDlg::OnBnClickedButtonAiSend)
    ON_BN_CLICKED(IDC_BUTTON_AI_UPLOAD_IMAGE, &CAiGuideDlg::OnBnClickedButtonAiUploadImage)
    ON_BN_CLICKED(IDC_BUTTON_AI_CLEAR_IMAGE, &CAiGuideDlg::OnBnClickedButtonAiClearImage)
    ON_BN_CLICKED(IDC_BUTTON_AI_CLOSE, &CAiGuideDlg::OnBnClickedButtonAiClose)
    ON_CBN_SELCHANGE(IDC_COMBO_AI_MODEL, &CAiGuideDlg::OnCbnSelchangeComboAiModel)
    ON_WM_CLOSE()
END_MESSAGE_MAP()

CString CAiGuideDlg::ResolveBaseDir() const
{
    // 根据当前可执行文件位置反推工程根目录，
    // 以便可靠定位 tools、ai_kb、runtime 等目录。
    CString exeDir;
    GetModuleFileName(nullptr, exeDir.GetBuffer(MAX_PATH), MAX_PATH);
    exeDir.ReleaseBuffer();

    int lastSlash = exeDir.ReverseFind(_T('\\'));
    if (lastSlash != -1)
    {
        exeDir = exeDir.Left(lastSlash);
    }

    CString baseDir = exeDir;
    CString probe = baseDir + _T("\\tools\\ai_course_guide.py");
    if (GetFileAttributes(probe) == INVALID_FILE_ATTRIBUTES)
    {
        int parentSlash = baseDir.ReverseFind(_T('\\'));
        if (parentSlash != -1)
        {
            baseDir = baseDir.Left(parentSlash);
        }
    }
    return baseDir;
}

CString CAiGuideDlg::GenerateSessionId() const
{
    // 使用“日期时间 + 毫秒 + 进程号 + TickCount”的组合生成唯一会话 id，
    // 目标是尽量避免不同窗口或不同时间点的临时文件重名。
    SYSTEMTIME st = {};
    GetLocalTime(&st);

    CString sessionId;
    sessionId.Format(
        _T("%04d%02d%02d_%02d%02d%02d_%03d_%lu_%llu"),
        st.wYear,
        st.wMonth,
        st.wDay,
        st.wHour,
        st.wMinute,
        st.wSecond,
        st.wMilliseconds,
        GetCurrentProcessId(),
        static_cast<unsigned long long>(GetTickCount64()));
    return sessionId;
}

void CAiGuideDlg::InitializeSessionState()
{
    // 每个新窗口都必须从全新的会话状态开始：
    // 清空聊天历史、清空图片、清空输入框，并显示一条初始提示语，
    // 这样可以明确告诉用户：当前窗口已经成功载入学生信息，但尚未开始提问。
    m_sessionId = GenerateSessionId();
    m_chatHistoryText.Empty();
    m_imagePaths.RemoveAll();
    m_hasRequested = false;
    m_selectedModel = GetCurrentModelApiName();

    if (m_editQuestion.GetSafeHwnd())
    {
        m_editQuestion.SetWindowTextW(L"");
    }
    if (m_editImagePaths.GetSafeHwnd())
    {
        m_editImagePaths.SetWindowTextW(L"");
    }
    if (m_editResponse.GetSafeHwnd())
    {
        m_editResponse.SetWindowTextW(L"当前学生信息已载入，请点击“生成指导”或输入问题。");
    }
}

void CAiGuideDlg::LoadModelsFromIni(const CString& baseDir)
{
    // 读取模型配置文件：
    // ai_models.ini 提供下拉框候选项，
    // ai_config.ini 提供默认模型名称。
    m_models.clear();
    m_selectedModelIndex = -1;

    CString modelsIni = baseDir + _T("\\tools\\ai_models.ini");
    CString configIni = baseDir + _T("\\tools\\ai_config.ini");

    wchar_t defaultModelBuf[256] = { 0 };
    GetPrivateProfileStringW(L"ai", L"default_model", L"Qwen3-VL Plus", defaultModelBuf, 256, configIni);
    CString defaultModelDisplay(defaultModelBuf);
    defaultModelDisplay.Trim();

    int count = GetPrivateProfileIntW(L"models", L"count", 0, modelsIni);
    if (count <= 0)
    {
        return;
    }

    for (int i = 1; i <= count; ++i)
    {
        CString key;
        wchar_t buf[1024] = { 0 };

        ModelItem item;

        key.Format(L"name%d", i);
        GetPrivateProfileStringW(L"models", key, L"", buf, 1024, modelsIni);
        item.displayName = buf;
        item.displayName.Trim();

        key.Format(L"api_model%d", i);
        GetPrivateProfileStringW(L"models", key, L"", buf, 1024, modelsIni);
        item.apiModel = buf;
        item.apiModel.Trim();

        key.Format(L"kind%d", i);
        GetPrivateProfileStringW(L"models", key, L"text", buf, 1024, modelsIni);
        item.kind = buf;
        item.kind.Trim();

        key.Format(L"scene%d", i);
        GetPrivateProfileStringW(L"models", key, L"", buf, 1024, modelsIni);
        item.scene = buf;
        item.scene.Trim();

        if (!item.displayName.IsEmpty())
        {
            m_models.push_back(item);
        }
    }

    for (size_t i = 0; i < m_models.size(); ++i)
    {
        if (m_models[i].displayName.CompareNoCase(defaultModelDisplay) == 0 ||
            m_models[i].apiModel.CompareNoCase(defaultModelDisplay) == 0)
        {
            m_selectedModelIndex = static_cast<int>(i);
            break;
        }
    }

    if (m_selectedModelIndex < 0 && !m_models.empty())
    {
        m_selectedModelIndex = 0;
    }
}

CString CAiGuideDlg::GetCurrentModelDisplayName() const
{
    if (m_selectedModelIndex >= 0 && m_selectedModelIndex < static_cast<int>(m_models.size()))
    {
        return m_models[m_selectedModelIndex].displayName;
    }
    return CString();
}

CString CAiGuideDlg::GetCurrentModelApiName() const
{
    if (m_selectedModelIndex >= 0 && m_selectedModelIndex < static_cast<int>(m_models.size()))
    {
        return m_models[m_selectedModelIndex].apiModel;
    }
    return CString();
}

CString CAiGuideDlg::GetCurrentModelKind() const
{
    if (m_selectedModelIndex >= 0 && m_selectedModelIndex < static_cast<int>(m_models.size()))
    {
        return m_models[m_selectedModelIndex].kind;
    }
    return CString();
}

void CAiGuideDlg::UpdateModelSceneText()
{
    // 根据当前选中的模型，刷新“适用场景”提示文本。
    // 这里只更新说明，不会触发任何请求，也不会影响现有聊天记录。
    if (!m_staticModelScene.GetSafeHwnd())
    {
        return;
    }

    if (m_selectedModelIndex < 0 || m_selectedModelIndex >= static_cast<int>(m_models.size()))
    {
        m_staticModelScene.SetWindowTextW(L"");
        return;
    }

    CString sceneDecoded = DecodeUnicodeEscapes(m_models[m_selectedModelIndex].scene);
    CString text;
    text.Format(
        L"选择模型：%s\r\n适用场景：%s",
        (LPCWSTR)m_models[m_selectedModelIndex].displayName,
        (LPCWSTR)sceneDecoded);
    m_staticModelScene.SetWindowTextW(text);
}

void CAiGuideDlg::SetStudentContext(
    const CString& sno,
    const CString& sname,
    const CString& ssex,
    const CString& sage,
    const CString& sdept,
    const CString& selectedCoursesText,
    const CString& unselectedCoursesText,
    const CString& allCoursesText)
{
    // 保存主界面传来的学生与课程上下文。
    // 注意这里只做缓存，不做任何 AI 请求；真正的请求要等用户点击生成或发送。
    m_sno = sno;
    m_sname = sname;
    m_ssex = ssex;
    m_sage = sage;
    m_sdept = sdept;
    m_selectedCoursesText = selectedCoursesText;
    m_unselectedCoursesText = unselectedCoursesText;
    m_allCoursesText = allCoursesText;
}

BOOL CAiGuideDlg::OnInitDialog()
{
    // 对话框初始化顺序：
    // 1. 定位工程根目录；
    // 2. 读取模型配置；
    // 3. 动态创建模型相关控件；
    // 4. 初始化当前窗口独立的会话状态；
    // 5. 刷新学生摘要。
    CDialogEx::OnInitDialog();

    m_baseDir = ResolveBaseDir();
    LoadModelsFromIni(m_baseDir);

    CRect rcQuestion;
    CWnd* pQuestion = GetDlgItem(IDC_EDIT_AI_QUESTION);
    if (pQuestion)
    {
        pQuestion->GetWindowRect(&rcQuestion);
        ScreenToClient(&rcQuestion);

        CRect rcCombo = rcQuestion;
        rcCombo.bottom = rcCombo.top - 6;
        rcCombo.top = rcCombo.bottom - 200;
        rcCombo.bottom = rcCombo.top + 200;
        rcCombo.left = rcQuestion.left;
        rcCombo.right = rcQuestion.right;

        CRect rcComboCtrl = rcCombo;
        rcComboCtrl.bottom = rcComboCtrl.top + 200;
        m_comboModel.Create(WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL, rcComboCtrl, this, IDC_COMBO_AI_MODEL);

        CRect rcScene = rcQuestion;
        rcScene.bottom = rcComboCtrl.top - 6;
        rcScene.top = rcScene.bottom - 52;
        m_staticModelScene.Create(L"", WS_CHILD | WS_VISIBLE | SS_LEFT, rcScene, this, IDC_STATIC_AI_MODEL_SCENE);
    }

    if (m_comboModel.GetSafeHwnd() && !m_models.empty())
    {
        for (size_t i = 0; i < m_models.size(); ++i)
        {
            m_comboModel.AddString(m_models[i].displayName);
        }
        if (m_selectedModelIndex >= 0)
        {
            m_comboModel.SetCurSel(m_selectedModelIndex);
        }
        UpdateModelSceneText();
    }

    InitializeSessionState();
    RefreshStudentSummary();
    return TRUE;
}

void CAiGuideDlg::OnCbnSelchangeComboAiModel()
{
    // 模型切换策略：
    // 只更新“当前窗口后续请求要使用的模型”，
    // 不清空聊天记录，不重新发送上一条问题，不重建窗口。
    if (!m_comboModel.GetSafeHwnd())
    {
        return;
    }

    int sel = m_comboModel.GetCurSel();
    if (sel >= 0)
    {
        m_selectedModelIndex = sel;
        m_selectedModel = GetCurrentModelApiName();
        UpdateModelSceneText();

        if (m_staticModelScene.GetSafeHwnd())
        {
            CString text;
            m_staticModelScene.GetWindowText(text);
            if (!text.IsEmpty())
            {
                text += L"\r\n已切换模型，下一次提问生效。";
                m_staticModelScene.SetWindowTextW(text);
            }
        }
    }
}

void CAiGuideDlg::AppendHistoryEntry(const CString& question, const CString& answer, bool isFollowUp)
{
    // 会话历史只保存在当前窗口的 m_chatHistoryText 中。
    // 这里刻意不去反读 UI 文本，而是维护独立的历史变量，
    // 目的是避免把界面提示、错误信息、旧窗口残留文本再次塞回 Prompt。
    CString entry;
    if (isFollowUp)
    {
        entry.Format(L"用户追问：%s\r\nAI 回复：%s", (LPCTSTR)question, (LPCTSTR)answer);
    }
    else
    {
        entry.Format(L"用户问题：%s\r\nAI 回复：%s", (LPCTSTR)question, (LPCTSTR)answer);
    }

    if (!m_chatHistoryText.IsEmpty())
    {
        m_chatHistoryText += L"\r\n--------------------\r\n";
    }
    m_chatHistoryText += entry;
}

bool CAiGuideDlg::IsAiErrorMessage(const CString& text) const
{
    // 判断当前返回是否更像“错误提示”而不是“正常 AI 回复”。
    // 如果是错误提示，则只显示给用户，不把它追加进会话历史，
    // 这样下一轮追问就不会被旧错误内容污染。
    CString trimmed = text;
    trimmed.Trim();
    if (trimmed.IsEmpty())
    {
        return true;
    }

    static const wchar_t* kPrefixes[] = {
        L"未检测到",
        L"读取请求文件失败",
        L"接口调用失败",
        L"接口调用异常",
        L"接口调用超时",
        L"网络连接失败",
        L"写入请求文件失败",
        L"AI 脚本不存在",
        L"读取 AI 回复失败",
        L"requests 库未安装",
        L"模型调用失败"
    };

    for (size_t i = 0; i < _countof(kPrefixes); ++i)
    {
        if (trimmed.Find(kPrefixes[i]) == 0)
        {
            return true;
        }
    }
    return false;
}

void CAiGuideDlg::OnBnClickedButtonAiGenerate()
{
    // “生成指导”用于触发当前窗口的首轮指导，或在同窗口下再次要求生成总结。
    // 首轮成功后会把“问题 + 回复”写入当前窗口历史，供后续追问使用。
    CString defaultQuestion =
        L"请根据当前学生信息、已选课程、未选课程和本地知识库，给出个性化选课指导。";

    CString answer = CallAiHelper(defaultQuestion);
    bool isError = IsAiErrorMessage(answer);

    if (m_chatHistoryText.IsEmpty())
    {
        SetAiConversation(defaultQuestion, answer);
    }
    else
    {
        AppendAiConversation(defaultQuestion, answer);
    }

    if (!isError)
    {
        AppendHistoryEntry(defaultQuestion, answer, m_hasRequested);
        m_hasRequested = true;
    }
}

void CAiGuideDlg::OnBnClickedButtonAiSend()
{
    // “发送追问”只读取当前输入框内容，
    // 然后结合当前窗口自己的历史、模型、图片状态发起一次新的请求。
    CString question;
    m_editQuestion.GetWindowText(question);
    question.Trim();

    if (question.IsEmpty())
    {
        AfxMessageBox(L"请输入要咨询的问题");
        return;
    }

    CString answer = CallAiHelper(question);
    bool isError = IsAiErrorMessage(answer);
    AppendAiConversation(question, answer);

    if (!isError)
    {
        AppendHistoryEntry(question, answer, true);
        m_hasRequested = true;
    }

    m_editQuestion.SetWindowTextW(L"");
}

void CAiGuideDlg::UpdateImagePathsDisplay()
{
    // 把当前窗口已上传的图片路径重新拼成字符串显示到界面上，
    // 仅用于用户确认当前会话里附带了哪些图片。
    if (!m_editImagePaths.GetSafeHwnd())
    {
        return;
    }

    CString display;
    for (INT_PTR i = 0; i < m_imagePaths.GetSize(); ++i)
    {
        display += m_imagePaths[i];
        if (i != m_imagePaths.GetSize() - 1)
        {
            display += _T(";");
        }
    }
    m_editImagePaths.SetWindowText(display);
}

void CAiGuideDlg::OnBnClickedButtonAiUploadImage()
{
    // 上传图片不会立即调用 AI；
    // 图片只是先挂在当前窗口会话上，等用户下一次发送问题时一起带过去。
    CFileDialog dlg(TRUE, _T("png"), nullptr, OFN_FILEMUSTEXIST,
        L"图片文件 (*.png;*.jpg;*.jpeg;*.bmp)|*.png;*.jpg;*.jpeg;*.bmp||", this);

    if (dlg.DoModal() == IDOK)
    {
        m_imagePaths.Add(dlg.GetPathName());
        UpdateImagePathsDisplay();
    }
}

void CAiGuideDlg::OnBnClickedButtonAiClearImage()
{
    // 清空当前窗口已挂载的图片路径，不影响已生成的历史文本。
    m_imagePaths.RemoveAll();
    UpdateImagePathsDisplay();
}

void CAiGuideDlg::CleanupSessionRuntimeFiles()
{
    // 删除当前窗口会话产生的 request / response 文件。
    // 由于文件名中带 sessionId，因此这里不会误删其他窗口的临时文件。
    if (m_baseDir.IsEmpty() || m_sessionId.IsEmpty())
    {
        return;
    }

    CString runtimeDir = m_baseDir + _T("\\tools\\runtime");
    CString requestPath = runtimeDir + _T("\\ai_request_") + m_sessionId + _T(".json");
    CString responsePath = runtimeDir + _T("\\ai_response_") + m_sessionId + _T(".txt");

    try
    {
        if (GetFileAttributes(requestPath) != INVALID_FILE_ATTRIBUTES)
        {
            CFile::Remove(requestPath);
        }
    }
    catch (...)
    {
    }

    try
    {
        if (GetFileAttributes(responsePath) != INVALID_FILE_ATTRIBUTES)
        {
            CFile::Remove(responsePath);
        }
    }
    catch (...)
    {
    }
}

void CAiGuideDlg::OnBnClickedButtonAiClose()
{
    CleanupSessionRuntimeFiles();
    DestroyWindow();
}

void CAiGuideDlg::OnClose()
{
    CleanupSessionRuntimeFiles();
    DestroyWindow();
}

void CAiGuideDlg::PostNcDestroy()
{
    CDialogEx::PostNcDestroy();
    delete this;
}

void CAiGuideDlg::RefreshStudentSummary()
{
    // 学生摘要区只展示当前学生的静态背景信息，
    // 不承担聊天历史功能，因此刷新它不会影响聊天区。
    if (!m_editStudentSummary.GetSafeHwnd())
    {
        return;
    }

    CString summary;
    summary.Format(
        L"学号：%s\r\n姓名：%s\r\n性别：%s\r\n年龄：%s\r\n所在系：%s\r\n\r\n已选课程：\r\n%s\r\n未选课程：\r\n%s",
        (LPCTSTR)m_sno,
        (LPCTSTR)m_sname,
        (LPCTSTR)m_ssex,
        (LPCTSTR)m_sage,
        (LPCTSTR)m_sdept,
        (LPCTSTR)m_selectedCoursesText,
        (LPCTSTR)m_unselectedCoursesText);
    m_editStudentSummary.SetWindowText(summary);
}

void CAiGuideDlg::AppendAiConversation(const CString& question, const CString& answer)
{
    // 负责把“追问 + 回复”追加到界面聊天框。
    // 聊天框主要是给用户看的显示层；真正送给 Python 的历史以 m_chatHistoryText 为准。
    CString existing;
    m_editResponse.GetWindowText(existing);

    CString newText = existing;
    newText.Trim();
    if (newText.IsEmpty() || newText == L"当前学生信息已载入，请点击“生成指导”或输入问题。")
    {
        newText.Empty();
    }

    if (!newText.IsEmpty())
    {
        newText += L"\r\n--------------------\r\n";
    }

    CString appended;
    appended.Format(
        L"用户追问：%s\r\nAI 回复：%s\r\n",
        (LPCTSTR)question,
        (LPCTSTR)answer);
    newText += appended;
    m_editResponse.SetWindowTextW(newText);
}

void CAiGuideDlg::SetAiConversation(const CString& question, const CString& answer)
{
    // 直接覆盖聊天框内容，通常用于首轮生成指导时构造一个干净的起始聊天界面。
    CString text;
    text.Format(
        L"用户问题：%s\r\n\r\nAI 回复：%s\r\n",
        (LPCTSTR)question,
        (LPCTSTR)answer);
    m_editResponse.SetWindowTextW(text);
}

CString CAiGuideDlg::EscapeJsonString(const CString& input)
{
    return EscapeJsonText(input);
}

std::string CAiGuideDlg::CStringToUtf8(const CString& input)
{
    int required = WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)input, -1, nullptr, 0, nullptr, nullptr);
    if (required <= 0)
    {
        return std::string();
    }

    std::string result;
    result.resize(required);
    WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)input, -1, &result[0], required, nullptr, nullptr);
    if (!result.empty() && result.back() == '\0')
    {
        result.pop_back();
    }
    return result;
}

CString CAiGuideDlg::Utf8ToCString(const std::string& input)
{
    int len = MultiByteToWideChar(CP_UTF8, 0, input.c_str(), -1, nullptr, 0);
    CString result;
    if (len > 0)
    {
        MultiByteToWideChar(CP_UTF8, 0, input.c_str(), -1, result.GetBuffer(len), len);
        result.ReleaseBuffer();
    }
    return result;
}

CString CAiGuideDlg::CallAiHelper(const CString& userQuestion)
{
    // 这是 C++ 侧 AI 调用链的核心函数：
    // 1. 计算当前窗口的 runtime 文件路径；
    // 2. 组装当前窗口专属 request.json；
    // 3. 调用 Python 脚本；
    // 4. 读取当前窗口专属 response.txt；
    // 5. 返回给界面层显示。
    //
    // 这里特别强调“当前窗口专属”：
    // request/response 文件名都带 sessionId，
    // 从而避免多个 AI 窗口之间通过固定文件相互串话。
    CString scriptPath = m_baseDir + _T("\\tools\\ai_course_guide.py");
    if (m_baseDir.IsEmpty() || GetFileAttributes(scriptPath) == INVALID_FILE_ATTRIBUTES)
    {
        return L"AI 脚本不存在，请检查 tools/ai_course_guide.py 是否存在。";
    }

    CString runtimeDir = m_baseDir + _T("\\tools\\runtime");
    CreateDirectory(runtimeDir, nullptr);

    CString requestPath = runtimeDir + _T("\\ai_request_") + m_sessionId + _T(".json");
    CString responsePath = runtimeDir + _T("\\ai_response_") + m_sessionId + _T(".txt");

    try
    {
        if (GetFileAttributes(responsePath) != INVALID_FILE_ATTRIBUTES)
        {
            CFile::Remove(responsePath);
        }
    }
    catch (...)
    {
    }

    CString currentModelApi = GetCurrentModelApiName();
    CString currentModelDisplay = GetCurrentModelDisplayName();
    CString currentModelKind = GetCurrentModelKind();
    m_selectedModel = currentModelApi;

    CString escapedQuestion = EscapeJsonString(userQuestion);
    CString escapedHistory = EscapeJsonString(m_chatHistoryText);

    // request.json 中的关键字段：
    // session_id：区分不同窗口；
    // new_session：区分本窗口第一次请求和后续追问；
    // chat_history：只包含当前窗口内部已确认的历史；
    // model：当前下拉框选择的模型；
    // image_paths：当前窗口挂载的图片。
    CString requestJson;
    requestJson += _T("{");
    requestJson += _T("\"session_id\":\"") + EscapeJsonString(m_sessionId) + _T("\",");
    requestJson += _T("\"new_session\":") + CString(m_hasRequested ? _T("false") : _T("true")) + _T(",");
    requestJson += _T("\"student\":{");
    requestJson += _T("\"sno\":\"") + EscapeJsonString(m_sno) + _T("\",");
    requestJson += _T("\"sname\":\"") + EscapeJsonString(m_sname) + _T("\",");
    requestJson += _T("\"ssex\":\"") + EscapeJsonString(m_ssex) + _T("\",");
    requestJson += _T("\"sage\":\"") + EscapeJsonString(m_sage) + _T("\",");
    requestJson += _T("\"sdept\":\"") + EscapeJsonString(m_sdept) + _T("\"},");
    requestJson += _T("\"selected_courses\":") + BuildCoursesJson(m_selectedCoursesText, true) + _T(",");
    requestJson += _T("\"unselected_courses\":") + BuildCoursesJson(m_unselectedCoursesText, false) + _T(",");
    requestJson += _T("\"all_courses\":") + BuildCoursesJson(m_allCoursesText, false) + _T(",");
    requestJson += _T("\"user_question\":\"") + escapedQuestion + _T("\",");
    requestJson += _T("\"chat_history\":\"") + escapedHistory + _T("\",");
    requestJson += _T("\"image_paths\":[");
    for (INT_PTR i = 0; i < m_imagePaths.GetSize(); ++i)
    {
        requestJson += _T("\"") + EscapeJsonString(m_imagePaths[i]) + _T("\"");
        if (i != m_imagePaths.GetSize() - 1)
        {
            requestJson += _T(",");
        }
    }
    requestJson += _T("],");
    requestJson += _T("\"model\":\"") + EscapeJsonString(currentModelApi) + _T("\",");
    requestJson += _T("\"model_display\":\"") + EscapeJsonString(currentModelDisplay) + _T("\",");
    requestJson += _T("\"model_kind\":\"") + EscapeJsonString(currentModelKind) + _T("\"");
    requestJson += _T("}");

    try
    {
        CFile outFile;
        if (!outFile.Open(requestPath, CFile::modeCreate | CFile::modeWrite | CFile::typeBinary))
        {
            return L"写入请求文件失败，请检查 tools/runtime 目录是否存在。";
        }
        std::string utf8Json = CStringToUtf8(requestJson);
        outFile.Write(utf8Json.data(), static_cast<UINT>(utf8Json.size()));
        outFile.Close();
    }
    catch (...)
    {
        return L"写入请求文件失败，请检查 tools/runtime 目录是否存在。";
    }

    CString configIni = m_baseDir + _T("\\tools\\ai_config.ini");
    wchar_t pythonCmdBuf[512] = { 0 };
    GetPrivateProfileStringW(L"ai", L"python_cmd", L"python", pythonCmdBuf, 512, configIni);
    CString pythonCmd(pythonCmdBuf);
    pythonCmd.Trim();
    if (pythonCmd.IsEmpty())
    {
        pythonCmd = _T("python");
    }

    CString pythonCmdForShell = pythonCmd;
    if (pythonCmdForShell.Find(_T(' ')) >= 0 &&
        (pythonCmdForShell.Find(_T('\\')) >= 0 || pythonCmdForShell.Find(_T('/')) >= 0) &&
        pythonCmdForShell[0] != _T('"'))
    {
        pythonCmdForShell = _T("\"") + pythonCmdForShell + _T("\"");
    }

    CString cmd;
    cmd.Format(_T("%s \"%s\" --input \"%s\" --output \"%s\""),
        (LPCTSTR)pythonCmdForShell,
        (LPCTSTR)scriptPath,
        (LPCTSTR)requestPath,
        (LPCTSTR)responsePath);

    // 执行 Python 脚本并读取当前会话的响应文件。
    // 即使脚本报错，也优先尝试读取 responsePath，便于拿到更具体的中文错误原因。
    int ret = _wsystem(cmd);

    CFile inFile;
    if (!inFile.Open(responsePath, CFile::modeRead | CFile::typeBinary))
    {
        if (ret != 0)
        {
            return L"接口调用失败，请检查 Python、API Key、AI_BASE_URL 与脚本配置。";
        }
        return L"读取 AI 回复失败，请检查 tools/runtime 下当前会话响应文件是否生成。";
    }

    ULONGLONG fileLength = inFile.GetLength();
    std::string buffer;
    buffer.resize(static_cast<size_t>(fileLength));
    if (fileLength > 0)
    {
        inFile.Read(&buffer[0], static_cast<UINT>(fileLength));
    }
    inFile.Close();

    CString resultText = Utf8ToCString(buffer);
    CString trimmed = resultText;
    trimmed.Trim();
    if (trimmed.IsEmpty() && ret != 0)
    {
        return L"接口调用失败，请检查 Python、API Key、AI_BASE_URL 与脚本配置。";
    }

    return resultText;
}
