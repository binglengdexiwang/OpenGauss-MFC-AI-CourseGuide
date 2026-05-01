#pragma once

// AI 指导对话框头文件说明：
// 学号：2024212124
// 本文件负责声明 AI 指导窗口的界面控件、会话状态、模型状态、
// 临时文件清理能力，以及与 Python 辅助脚本交互所需的方法。
// 这里主要描述“窗口内状态长什么样、有哪些能力”，具体执行流程在 cpp 中实现。

#include <string>
#include <vector>
#include "resource.h"

// CAiGuideDlg dialog
class CAiGuideDlg : public CDialogEx
{
    DECLARE_DYNAMIC(CAiGuideDlg)

public:
    CAiGuideDlg(CWnd* pParent = nullptr);
    virtual ~CAiGuideDlg();

#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_DIALOG_AI_GUIDE };
#endif

    // Receive student/course context from the main dialog
    void SetStudentContext(
        const CString& sno,
        const CString& sname,
        const CString& ssex,
        const CString& sage,
        const CString& sdept,
        const CString& selectedCoursesText,
        const CString& unselectedCoursesText,
        const CString& allCoursesText
    );

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    virtual void PostNcDestroy();

    DECLARE_MESSAGE_MAP()

private:
    // 学生与课程上下文：
    // 这些字段由主界面在打开 AI 指导窗口时传入，
    // 用于让当前窗口始终围绕“当前选中的学生”展开分析。
    CString m_sno;
    CString m_sname;
    CString m_ssex;
    CString m_sage;
    CString m_sdept;
    CString m_selectedCoursesText;
    CString m_unselectedCoursesText;
    CString m_allCoursesText;

    // 界面控件句柄：
    // 这些成员与对话框上的编辑框/静态控件绑定，
    // 便于代码直接读写 UI 内容，而不用频繁查找控件。
    CEdit m_editStudentSummary;
    CEdit m_editResponse;
    CEdit m_editQuestion;
    CEdit m_editImagePaths;

    // 动态创建的模型选择相关控件：
    // 模型下拉框和模型场景说明不是在资源编辑器中固定摆放，
    // 而是在窗口初始化时按当前布局动态创建。
    CComboBox m_comboModel;
    CStatic m_staticModelScene;

    struct ModelItem
    {
        // displayName：给用户看的名称
        // apiModel：真正发送给 Python / 大模型接口的模型名
        // kind：区分文本模型还是多模态模型
        // scene：界面上展示的“适用场景”说明
        CString displayName;
        CString apiModel;
        CString kind;  // "text" | "vision"
        CString scene;
    };
    std::vector<ModelItem> m_models;
    int m_selectedModelIndex = -1;

    // 当前窗口的会话级状态：
    // m_selectedModel 记录当前窗口正在使用的模型，
    // m_sessionId 用于隔离不同窗口的请求/响应文件，
    // m_chatHistoryText 只保存当前窗口内部的历史对话，
    // m_baseDir 记录工程根目录，
    // m_hasRequested 用于区分“本窗口第一轮请求”与“后续追问”。
    CString m_selectedModel;
    CString m_sessionId;
    CString m_chatHistoryText;
    CString m_baseDir;
    bool m_hasRequested = false;

    // 当前窗口上传的图片路径集合：
    // 只在本窗口内有效，关闭窗口后即随会话一起销毁。
    CStringArray m_imagePaths;

    // 调用 Python 辅助脚本，完成 request.json 写入、脚本执行、
    // response.txt 读取等完整调用链。
    CString CallAiHelper(const CString& userQuestion);

    // 从配置文件中读取模型列表与默认模型，并刷新模型说明文本。
    void LoadModelsFromIni(const CString& baseDir);
    void UpdateModelSceneText();

    // 将文本做 JSON 转义，避免引号、换行等字符破坏请求结构。
    CString EscapeJsonString(const CString& input);

    // 字符编码转换：
    // MFC 中界面常用 CString / UTF-16，
    // 写 request.json 与读 response.txt 时需要转成 UTF-8。
    std::string CStringToUtf8(const CString& input);

    CString Utf8ToCString(const std::string& input);

    // 初始化当前窗口的会话状态，确保新窗口打开时不会继承旧窗口内容。
    void InitializeSessionState();

    // 生成安全且唯一的会话 id，用于隔离不同窗口的运行时文件。
    CString GenerateSessionId() const;

    // 计算工程根目录，便于定位 tools、ai_kb、runtime 等目录。
    CString ResolveBaseDir() const;

    // 获取当前下拉框对应模型的显示名、接口名、模型类型。
    CString GetCurrentModelDisplayName() const;
    CString GetCurrentModelApiName() const;
    CString GetCurrentModelKind() const;

    // 刷新图片路径显示，并在窗口关闭时清理当前会话的运行时文件。
    void UpdateImagePathsDisplay();
    void CleanupSessionRuntimeFiles();

    // 只维护当前窗口的聊天历史，并判断当前返回是否属于错误消息。
    // 这样做的目的，是避免把旧错误提示继续塞回下一轮 Prompt。
    void AppendHistoryEntry(const CString& question, const CString& answer, bool isFollowUp);
    bool IsAiErrorMessage(const CString& text) const;

    // 将新一轮“用户问题/AI 回复”追加到界面上的聊天显示区域。
    void AppendAiConversation(const CString& question, const CString& answer);

    // 直接设置聊天显示区域，一般用于首轮生成指导时覆盖显示。
    void SetAiConversation(const CString& question, const CString& answer);

public:
    afx_msg void OnBnClickedButtonAiGenerate();
    afx_msg void OnBnClickedButtonAiSend();
    afx_msg void OnBnClickedButtonAiUploadImage();
    afx_msg void OnBnClickedButtonAiClearImage();
    afx_msg void OnBnClickedButtonAiClose();
	afx_msg void OnCbnSelchangeComboAiModel();
	afx_msg void OnClose();

	// 刷新学生摘要区域，但不影响当前窗口已保留的聊天记录。
	void RefreshStudentSummary();
};
