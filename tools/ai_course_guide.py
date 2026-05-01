#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
AI 选课指导辅助脚本
- 读取 request.json
- 读取本地知识库
- 调用 OpenAI-compatible Chat Completions 接口
- 输出 response.txt

学号：2024212124

脚本职责概览：
1. 从 C++ 侧生成的 request.json 中读取当前窗口会话数据；
2. 结合本地知识库与学生选课信息构造 Prompt；
3. 按当前请求指定的模型调用兼容 OpenAI 的接口；
4. 将返回结果写回 response.txt，交还给 MFC 界面显示。
"""

import argparse
import base64
import configparser
import json
import os
import re
import sys
from pathlib import Path

try:
    import requests
except Exception:
    requests = None


def read_text(path):
    """按 UTF-8 读取文本文件，供知识库加载与配置读取复用。"""
    with open(path, "r", encoding="utf-8") as f:
        return f.read()


def load_kb(kb_dir):
    """把本地知识库目录中的多份 Markdown 文本合并成一个长上下文。"""
    kb_texts = []
    for name in [
        "course_knowledge.md",
        "major_rules.md",
        "selection_rules.md",
        "study_advice.md",
        "extended_courses.md",
    ]:
        file_path = kb_dir / name
        if file_path.exists():
            kb_texts.append(read_text(file_path))
    return "\n\n".join(kb_texts)


def to_data_url(image_path):
    """把本地图片转成 data url，供多模态接口直接读取。"""
    suffix = image_path.suffix.lower().strip(".")
    if suffix == "jpg":
        suffix = "jpeg"
    mime = f"image/{suffix}"
    data = image_path.read_bytes()
    b64 = base64.b64encode(data).decode("utf-8")
    return f"data:{mime};base64,{b64}"


def list_courses(items):
    """把课程数组转换成更适合 Prompt 阅读的课程清单文本。"""
    lines = []
    for item in items or []:
        cno = (item.get("cno") or "").strip()
        cname = (item.get("cname") or "").strip()
        grade = item.get("grade")
        base = f"{cno} {cname}".strip()
        if grade not in (None, "", "null"):
            lines.append(f"{base}（成绩：{grade}）")
        else:
            lines.append(base if base else "(未知课程)")
    return "\n".join(lines) if lines else "(无)"


def truncate_text(text: str, max_chars: int) -> str:
    """限制长文本长度，避免 Prompt 无限膨胀。"""
    if not text:
        return ""
    if len(text) <= max_chars:
        return text
    return text[:max_chars] + "\n\n(提示：内容较长，已截断，仅供参考)"


def extract_length_hint(q: str):
    """从用户问题中提取“字数、简短、详细、表格、分点”等输出偏好。"""
    q = (q or "").strip()
    hint = {
        "explicit_max_chars": None,
        "prefer_short": False,
        "prefer_detailed": False,
        "prefer_table": False,
        "prefer_bullets": False,
    }

    m = re.search(r"(\d{2,4})\s*字\s*(以内|内|左右)?", q)
    if m:
        try:
            n = int(m.group(1))
            if 50 <= n <= 2000:
                hint["explicit_max_chars"] = n
        except Exception:
            pass

    short_keywords = ["简短", "简单说", "不要太长", "尽量短", "短一点", "一句话", "概括"]
    long_keywords = ["详细", "展开", "完整", "报告", "越详细越好", "写成报告"]
    hint["prefer_short"] = any(k in q for k in short_keywords)
    hint["prefer_detailed"] = any(k in q for k in long_keywords)
    hint["prefer_table"] = "表格" in q
    hint["prefer_bullets"] = any(k in q for k in ["分点", "要点", "列出", "条目", "清单"])
    return hint

def build_system_prompt() -> str:
    """构造系统提示词，统一约束模型的回答边界、语气和长度。"""
    return (
        "你是一个大学课程选课指导专家助手。\n"
        "你的核心任务是：围绕当前学生的选课问题，结合学生信息、已选课程、未选课程、系统内课程、本地知识库和用户补充信息，给出准确、自然、可执行的建议。\n\n"

        "【最高优先级规则】\n"
        "1. 必须优先回答用户本轮问题，不要机械复述历史对话。\n"
        "2. 如果用户只是问候、确认、闲聊、询问模型身份或系统状态，应简短自然回应，不要输出完整选课报告。\n"
        "3. 只有当用户明确要求“完整选课指导、详细分析、报告形式、全面建议”时，才输出完整结构化选课方案。\n"
        "4. 如果用户要求“不要管前面、忽略前文、重新开始、只回答本句”，必须忽略历史对话，只回答本轮问题。\n"
        "5. 不要引用历史中的错误提示、旧情绪表达、旧学生信息或旧回答，除非用户明确要求回顾当前窗口历史。\n\n"

        "【回答范围控制】\n"
        "1. 用户问什么就答什么。\n"
        "2. 用户问某一门课，例如 MATH 怎么学，就只回答这门课的学习建议。\n"
        "3. 用户问 OS 是否适合现在选，就只分析 OS 与当前学生基础、专业和已选课程的关系。\n"
        "4. 用户问模型身份，就只回答当前调用模型和本系统用途。\n"
        "5. 用户问普通问题，例如“你好”“收到吗”“你在干嘛”，应简短回应，不要强行转入选课分析。\n\n"

        "【长度控制】\n"
        "1. 如果用户要求“200字以内、300字以内、简短、简单说、不要太长”，必须严格压缩回答。\n"
        "2. 如果用户未要求详细，默认控制在 300 到 600 字以内。\n"
        "3. 只有用户明确要求详细展开时，才允许较长回答。\n"
        "4. 默认不要使用长表格，除非用户明确要求表格。\n\n"

        "【格式控制】\n"
        "默认使用简洁小标题或分点，例如：\n"
        "【结论】\n"
        "【理由】\n"
        "【建议】\n"
        "【风险提醒】\n"
        "【学习方法】\n"
        "根据用户问题选择必要部分，不必每次全部输出。\n\n"

        "【课程范围】\n"
        "当前系统内只有 5 门可选课程：MATH、DB_DESIGN、P_DESIGN、OS、ENGLISH。\n"
        "回答应优先围绕这 5 门课展开。\n"
        "如果推荐系统外课程，必须说明：这些课程不在当前系统可选范围内，仅作为后续学习规划建议。\n\n"

        "【语气要求】\n"
        "使用“建议、优先考虑、可以、暂不建议、需要注意”等表达。\n"
        "避免使用“必须、禁止、绝对不能”等过强语气，除非涉及数据库约束或明显错误操作。\n\n"

        "【一致性要求】\n"
        "回答必须前后一致。若给出不同情况的建议，必须写清条件，例如：\n"
        "如果希望冲刺 MATH 高分，可以降低同期开课压力；\n"
        "如果编程基础较好，可以同时考虑 P_DESIGN；\n"
        "如果基础薄弱，OS 建议后置。\n\n"

        "【图片处理】\n"
        "如果用户上传图片，并且本轮问题要求结合图片，应结合图片信息回答。\n"
        "如果图片与选课关系不强，应说明图片信息参考价值有限，再回到选课问题。\n"
        "不要因为有图片就无条件输出长篇分析。\n"
    )
def should_ignore_history(user_question: str, new_session) -> bool:
    """
    判断本轮是否必须忽略历史对话。

    触发条件主要有两类：
    1. 当前请求已经明确标记为新会话；
    2. 用户在本轮问题里明确提出“忽略前文、重新开始”等要求。
    """
    q = (user_question or "").strip()

    # 新会话必须忽略历史
    if new_session is True:
        return True
    if isinstance(new_session, str) and new_session.lower() in ("true", "1", "yes"):
        return True

    ignore_keywords = [
        "不要管前面",
        "别管前面",
        "忽略前文",
        "忽略上文",
        "重新开始",
        "只回答本句",
        "不要参考历史",
        "不要看历史",
        "从头开始",
    ]

    return any(k in q for k in ignore_keywords)

def build_context_prompt(request_data: dict, kb_text: str) -> str:
    """
    构造上下文提示词。

    这里负责把学生信息、课程信息、当前窗口历史、图片清单和知识库拼接起来，
    但不会把其他窗口的历史带入，也不会强行要求模型复述所有上下文。
    """
    student = request_data.get("student", {}) or {}
    selected = request_data.get("selected_courses", []) or []
    unselected = request_data.get("unselected_courses", []) or []
    all_courses = request_data.get("all_courses", []) or []
    image_paths = request_data.get("image_paths", []) or []
    history = (request_data.get("chat_history") or "").strip()
    ignore_history = should_ignore_history(
        request_data.get("user_question", ""),
        request_data.get("new_session"),
    )

    # 如果当前请求属于新会话，或用户明确说“忽略前文”，
    # 则直接清空历史，避免旧消息污染本轮回答。
    if ignore_history:
        history = ""

    kb_excerpt = truncate_text(kb_text or "", 8000)
    history_excerpt = truncate_text(history, 2000) if history else ""

    lines = []
    lines.append("【可用上下文（按需使用，不要机械复述）】")
    lines.append(f"【当前会话】session_id={request_data.get('session_id', '')}；new_session={request_data.get('new_session', False)}")
    lines.append(f"【当前模型】{request_data.get('model_display') or request_data.get('model') or '未指定'}")
    lines.append("【学生信息】")
    lines.append(f"- 学号：{student.get('sno', '')}")
    lines.append(f"- 姓名：{student.get('sname', '')}")
    lines.append(f"- 性别：{student.get('ssex', '')}")
    lines.append(f"- 年龄：{student.get('sage', '')}")
    lines.append(f"- 所在系：{student.get('sdept', '')}")
    lines.append("")
    lines.append("【已选课程】\n" + list_courses(selected))
    lines.append("")
    lines.append("【未选课程】\n" + list_courses(unselected))
    lines.append("")
    lines.append("【系统可选课程范围】\n" + list_courses(all_courses))

    if history_excerpt:
        lines.append("")
        lines.append("【当前窗口历史对话（仅在与本轮问题相关时再引用）】\n" + history_excerpt)
    else:
        lines.append("")
        lines.append("【当前窗口历史对话】\n(空)")

    if image_paths:
        lines.append("")
        lines.append("【用户上传图片】")
        for p in image_paths:
            try:
                lines.append(f"- {Path(p).name}")
            except Exception:
                lines.append(f"- {p}")

    if kb_excerpt:
        lines.append("")
        lines.append("【本地课程知识库摘录（按需引用，不要整段照搬）】\n" + kb_excerpt)

    return "\n".join(lines).strip()


def build_user_task_prompt(user_question: str) -> str:
    """
    构造“本轮任务要求”文本。

    这部分与系统提示词不同：
    - 系统提示词强调长期规则；
    - 本函数强调当前这一次提问到底要长答、短答、分点答还是局部答。
    """
    q = (user_question or "").strip() or "请根据当前学生信息给出选课指导。"
    hint = extract_length_hint(q)

    requirements = []

    if hint["explicit_max_chars"]:
        requirements.append(f"- 用户明确要求字数限制：请严格控制在 {hint['explicit_max_chars']} 字以内。")
    elif hint["prefer_short"] and not hint["prefer_detailed"]:
        requirements.append("- 用户希望简短：请尽量控制在 200 到 350 字。")
    elif hint["prefer_detailed"]:
        requirements.append("- 用户希望详细：可以适当展开，但要结构清晰，不要无关扩写。")
    else:
        requirements.append("- 用户未要求详细时，默认控制在 300 到 600 字以内。")

    if hint["prefer_table"]:
        requirements.append("- 用户要求表格：可以使用小表格，但不要输出过长大表。")
    else:
        requirements.append("- 用户未要求表格时，默认不要使用大表格。")

    if hint["prefer_bullets"]:
        requirements.append("- 用户希望分点：请用清晰条目回答。")

    requirements.append("- 必须优先回答本轮问题。")
    requirements.append("- 如果本轮问题只是问候、确认、模型身份或系统状态，请简短回答，不要输出选课报告。")
    requirements.append("- 如果本轮问题是局部问题，只回答局部，不要展开完整选课分析。")
    requirements.append("- 如果本轮问题要求完整选课指导，再输出完整建议。")
    requirements.append("- 不要复读历史中的旧错误、旧情绪、旧学生信息或旧回答。")

    return (
        "【用户本轮问题】\n"
        f"{q}\n\n"
        "【本轮回答要求】\n"
        + "\n".join(requirements)
    ).strip()


def build_prompts(request_data, kb_text):
    """组合系统提示词与用户提示词，形成最终发送给模型的文本上下文。"""
    system_prompt = build_system_prompt()
    user_prompt = build_context_prompt(request_data, kb_text) + "\n\n" + build_user_task_prompt(
        request_data.get("user_question", "")
    )
    return system_prompt, user_prompt


def build_prompt(request_data, kb_text):
    """Build system + user prompts for a general-purpose course-selection assistant.

    Returns:
        (system_prompt, user_prompt)
    """

    return build_prompts(request_data, kb_text)


def load_ini_config():
    """Load tools/ai_config.ini and tools/ai_models.ini (if present).

    Notes:
    - Never reads/stores API key from files (AI_API_KEY must come from env).
    - This config exists to simplify base_url/model selection and UI.
    """

    tools_dir = Path(__file__).resolve().parent
    cfg_path = tools_dir / "ai_config.ini"
    models_path = tools_dir / "ai_models.ini"

    cfg = configparser.ConfigParser()
    if cfg_path.exists():
        cfg.read(cfg_path, encoding="utf-8")

    models_cfg = configparser.ConfigParser()
    if models_path.exists():
        models_cfg.read(models_path, encoding="utf-8")

    return cfg, models_cfg


def resolve_model_from_ini(models_cfg, display_name: str):
    """Given a display name (or api_model id), return (api_model, kind, scene, matched_display) from ini.

    models ini format: [models] count/nameX/api_modelX/kindX/sceneX
    """
    if not display_name:
        return None, None, None, None
    if not models_cfg.has_section("models"):
        return None, None, None, None

    section = models_cfg["models"]
    try:
        count = int(section.get("count", "0"))
    except Exception:
        count = 0

    for i in range(1, count + 1):
        name = section.get(f"name{i}", "").strip()
        api_model_value = section.get(f"api_model{i}", "").strip()

        # accept either display name or api_model id
        if name == display_name or (api_model_value and api_model_value == display_name):
            api_model = api_model_value.strip() or name
            kind = section.get(f"kind{i}", "").strip()  # text|vision
            scene = section.get(f"scene{i}", "").strip()
            return api_model or None, kind or None, scene or None, (name or display_name)

    return None, None, None, None


def decode_unicode_escapes(s: str) -> str:
    """Decode strings like '\\u4e2d\\u6587' into real Unicode."""
    if not s:
        return s
    try:
        return s.encode("utf-8").decode("unicode_escape")
    except Exception:
        return s


def call_api(
    system_prompt,
    user_prompt,
    image_paths,
    api_key,
    base_url,
    model,
    vision_model,
    warning_prefix="",
    max_tokens: int = 1200,
    temperature: float = 0.3,
):
    """
    调用兼容 OpenAI 的聊天接口。

    这里统一处理：
    - 文字模型与多模态模型的消息体差异；
    - 非流式请求的参数；
    - enable_thinking 兼容问题；
    - 网络异常与接口异常的中文错误包装。
    """
    if requests is None:
        return "requests 库未安装，请先执行：pip install requests"

    headers = {"Authorization": f"Bearer {api_key}", "Content-Type": "application/json"}

    has_images = bool(image_paths)
    use_vision = has_images and bool(vision_model)
    model_name = vision_model if use_vision else model

    warning = ""
    if has_images and not vision_model:
        warning = "（提示：检测到图片，但未配置 AI_VISION_MODEL，将忽略图片，仅按文本生成建议。）\n"

    if not model_name:
        return "未配置 AI_MODEL 或 AI_VISION_MODEL。"

    messages = [{"role": "system", "content": system_prompt}]
    # warning_prefix 只追加到最终展示结果中，
    # 不直接塞进模型的 system prompt，以免干扰模型的核心理解。
    user_text = (warning + user_prompt) if warning else user_prompt
    messages.append({"role": "user", "content": user_text})

    if use_vision:
        content = [{"type": "text", "text": user_text}]
        for path in image_paths:
            try:
                data_url = to_data_url(Path(path))
                content.append({"type": "image_url", "image_url": {"url": data_url}})
            except Exception:
                content.append({"type": "text", "text": f"(图片读取失败: {path})"})
        messages = [{"role": "system", "content": system_prompt}, {"role": "user", "content": content}]

    # 将生成参数约束到一个安全范围内，
    # 避免因为异常输入把 max_tokens 或 temperature 设到不可用值。
    try:
        max_tokens = int(max_tokens)
    except Exception:
        max_tokens = 1200
    max_tokens = max(1, min(max_tokens, 1200))
    try:
        temperature = float(temperature)
    except Exception:
        temperature = 0.3

    # 显式使用非流式调用，并强制 enable_thinking=False，
    # 避免兼容接口因 thinking 模式和非流式模式冲突而报错。
    payload = {
        "model": model_name,
        "messages": messages,
        "max_tokens": max_tokens,
        "temperature": temperature,
        "stream": False,
        "enable_thinking": False,
    }

    url = base_url.rstrip("/") + "/chat/completions"
    try:
        resp = requests.post(url, headers=headers, json=payload, timeout=(10, 180))
        if resp.status_code != 200:
            response_text = resp.text
            # 检查是否是 enable_thinking 错误
            if "enable_thinking must be set to false for non-streaming calls" in response_text:
                msg = "模型调用失败：当前模型开启了思考模式，但本程序使用非流式调用。已建议在非流式请求中设置 enable_thinking=false，请检查脚本配置。"
            else:
                msg = f"接口调用失败，状态码：{resp.status_code}，响应：{response_text}"
            full = warning_prefix + warning + msg
            return full
        data = resp.json()
        content = data.get("choices", [{}])[0].get("message", {}).get("content", "(无返回内容)")
        full = warning_prefix + warning + content
        return full
    except Exception as ex:
        # keep error messages friendly and in Chinese
        msg = None
        try:
            from requests.exceptions import ConnectionError as ReqConnectionError
            from requests.exceptions import Timeout as ReqTimeout
        except Exception:
            ReqTimeout = ()
            ReqConnectionError = ()

        if ReqTimeout and isinstance(ex, ReqTimeout):
            msg = "接口调用超时（连接/读取超时），请检查网络或稍后重试。"
        elif ReqConnectionError and isinstance(ex, ReqConnectionError):
            msg = "网络连接失败，无法访问接口地址，请检查 AI_BASE_URL 或网络状态。"
        else:
            msg = f"接口调用异常：{ex}"
        full = warning_prefix + warning + msg
        return full


def main():
    """
    脚本主入口。

    整体流程：
    1. 校验环境变量与 requests 依赖；
    2. 读取 request.json；
    3. 读取知识库；
    4. 构造 Prompt；
    5. 解析当前请求指定的模型；
    6. 调用接口并把结果写入 response.txt。
    """
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()

    if not os.getenv("AI_API_KEY"):
        Path(args.output).write_text("未检测到 AI_API_KEY 环境变量，请先配置大模型 API Key。", encoding="utf-8")
        return

    cfg, models_cfg = load_ini_config()

    if requests is None:
        Path(args.output).write_text("requests 库未安装，请先执行：pip install requests", encoding="utf-8")
        return

    try:
        # 容忍 UTF-8 BOM，避免 Windows 下生成的 json 因 BOM 读取失败。
        request_data = json.loads(Path(args.input).read_text(encoding="utf-8-sig"))
    except Exception as ex:
        Path(args.output).write_text(f"读取请求文件失败：{ex}", encoding="utf-8")
        return

    kb_dir = Path(__file__).resolve().parent.parent / "ai_kb"
    kb_text = load_kb(kb_dir)

    system_prompt, user_prompt = build_prompt(request_data, kb_text)

    image_paths = request_data.get("image_paths", []) or []

    # 生成参数允许由 request.json 临时覆盖，
    # 但仍会在 call_api 中再次做安全范围限制。
    try:
        max_tokens = int(request_data.get("max_tokens", 1200))
    except Exception:
        max_tokens = 1200
    try:
        temperature = float(request_data.get("temperature", 0.3))
    except Exception:
        temperature = 0.3

    api_key = os.getenv("AI_API_KEY")
    # base_url 优先级：环境变量 > ini 配置。
    base_url = os.getenv("AI_BASE_URL")
    if not base_url:
        base_url = cfg.get("ai", "base_url", fallback="").strip() if cfg.has_section("ai") else ""
    if not base_url:
        Path(args.output).write_text("未检测到 AI_BASE_URL 环境变量或 tools/ai_config.ini 的 base_url，请先配置接口地址。", encoding="utf-8")
        return

    # 模型优先级：
    # request.model > request.model_display > 默认配置/环境变量。
    # 这样可以保证“同一窗口切换模型后，下一轮请求一定使用当前窗口选中的模型”。
    override_model = (request_data.get("model") or "").strip()
    override_display = (request_data.get("model_display") or "").strip()
    override_kind = (request_data.get("model_kind") or "").strip()  # text|vision

    ini_default_display = cfg.get("ai", "default_model", fallback="").strip() if cfg.has_section("ai") else ""
    ini_default_text_display = cfg.get("ai", "default_text_model", fallback="").strip() if cfg.has_section("ai") else ""

    env_text_model = (os.getenv("AI_MODEL") or "").strip()
    env_vision_model = (os.getenv("AI_VISION_MODEL") or "").strip()

    warning_prefix = ""

    # 先根据当前请求显式选择的模型解析 api_model / kind / scene。
    selected_display = override_model or override_display or ini_default_display
    selected_api_model = ""
    selected_kind = override_kind
    if selected_display and (not selected_api_model or not selected_kind):
        ini_api, ini_kind, ini_scene, ini_matched_display = resolve_model_from_ini(models_cfg, selected_display)
        selected_api_model = ini_api or selected_display
        selected_kind = selected_kind or (ini_kind or "")
        if ini_scene:
            display_for_prefix = ini_matched_display or selected_display
            warning_prefix = f"（当前模型：{display_for_prefix}；适用场景：{decode_unicode_escapes(ini_scene)}）\n"

    # 如果当前请求没有带出明确模型，再退回环境变量兜底。
    if not selected_api_model:
        selected_api_model = env_vision_model or env_text_model

    # 根据模型类型决定本轮是文本调用还是多模态调用。
    # 如果当前模型是 vision，则 model 和 vision_model 都指向该模型；
    # 如果当前模型是 text，则只使用文本模型通道。
    model = ""
    vision_model = ""
    if selected_kind == "vision":
        model = selected_api_model
        vision_model = selected_api_model
    else:
        model = selected_api_model
        vision_model = ""

    answer = call_api(
        system_prompt,
        user_prompt,
        image_paths,
        api_key,
        base_url,
        model,
        vision_model,
        warning_prefix=warning_prefix,
        max_tokens=max_tokens,
        temperature=temperature,
    )

    Path(args.output).write_text(answer, encoding="utf-8")


if __name__ == "__main__":
    main()
