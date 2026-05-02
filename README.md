# OpenGauss-MFC-AI-CourseGuide

本仓库用于展示 MFC + OpenGauss 学生选课系统中的 **AI 选课指导模块**。

## 功能简介

该模块在原有学生选课管理系统基础上，新增了“AI选课指导”功能。用户在主界面选中某个学生后，点击 AI 指导按钮，系统会读取该学生的基本信息、已选课程、未选课程，并结合本地知识库调用大模型 API，生成个性化选课建议。

主要功能包括：

1. 自动读取当前选中学生信息；
2. 查询学生已选课程和未选课程；
3. 结合本地课程知识库生成选课建议；
4. 支持用户连续追问；
5. 支持上传图片进行多模态分析；
6. 支持切换不同大模型；
7. API Key 通过环境变量配置，不写入代码。

## 文件说明

```text
2024212124Demo2026/
  AiGuideDlg.h
  AiGuideDlg.cpp
  My2024212124Demo2026.rc
  resource.h

ai_kb/
  course_knowledge.md
  major_rules.md
  selection_rules.md
  study_advice.md
  extended_courses.md

tools/
  ai_course_guide.py
  ai_config.ini
  ai_models.ini

demo/
  show.mp4
```

## 主要文件作用

- `AiGuideDlg.h / AiGuideDlg.cpp`：AI 选课指导对话框代码。
- `ai_kb/`：本地课程知识库，包括课程介绍、专业规则、选课规则和学习建议。
- `tools/ai_course_guide.py`：负责调用大模型 API，并返回 AI 回复。
- `tools/ai_config.ini`：配置 API 地址、默认模型和 Python 命令。
- `tools/ai_models.ini`：配置可选模型列表。
- `demo/show.mp4`：功能演示视频。

## API 环境变量

运行前需要配置：

```text
AI_API_KEY=你的API Key
AI_BASE_URL=https://dashscope.aliyuncs.com/compatible-mode/v1
AI_MODEL=qwen-plus
AI_VISION_MODEL=qwen3-vl-plus
```

注意：不要把 API Key 写入代码或上传到 GitHub。

## Python 依赖

```cmd
pip install requests
```

## 使用流程

1. 启动 MFC 学生选课系统；
2. 登录系统；
3. 在学生列表中选中一个学生；
4. 点击“AI选课指导”；
5. 点击“生成指导”或输入问题进行追问；
6. 可上传图片，让 AI 结合图片内容进行分析。

## 说明

本仓库主要用于展示 AI 选课指导功能相关文件，不包含完整学生选课系统的所有源码。若要完整运行，需要结合原 MFC + OpenGauss 工程环境。

## 上传命令

```cmd
notepad README.md
git add README.md
git commit -m "add README"
git push
```
