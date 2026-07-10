# daytest-practice
daytest-practice for 种子班
# Mini-Agent Terminal 项目开发进度 README

## 项目简介

本项目基于 **C 语言 + ncurses 库** 实现一个类 Agent 终端交互程序。程序启动后进入全屏终端界面，提供类似聊天式 Agent 的交互体验，包括：

* 固定布局的终端 UI
* 用户输入与历史对话管理
* 本地命令调用
* Markdown 简易渲染
* 状态栏信息展示
* 多窗口刷新与光标控制

当前版本已完成基础框架搭建，实现了终端交互、命令处理、历史消息管理等核心功能。

---

# 一、今日完成内容

## 1. ncurses 界面框架搭建 ✅

完成基于 ncurses 的三区域终端布局设计：

```
+--------------------------------+
|                                |
|            对话区              |
|                                |
+--------------------------------+
| 状态栏             时间        |
+--------------------------------+
| > 用户输入区                   |
|                                |
+--------------------------------+
```

### 窗口划分

* 对话区 `context`
* 状态栏 `status`
* 输入区 `input`

代码实现：

```c
WINDOW *context = subwin(stdscr,
                         height_context,
                         width,
                         start_y_context,
                         startx);

WINDOW *status = subwin(stdscr,
                        height_status,
                        width,
                        start_y_status,
                        startx);

WINDOW *input = subwin(stdscr,
                       height_input,
                       width,
                       start_y_input,
                       startx);
```

当前窗口高度根据：

```c
LINES
```

动态计算：

```c
height_context = LINES - height_input - height_status - 2;
```

---

# 二、界面初始化问题修复

## 1. LINES / COLS 使用位置修正

### 问题

之前：

```c
LINES
COLS
```

在：

```c
initscr()
```

之前调用。

由于 ncurses 尚未初始化，窗口尺寸变量无效。

### 修复

调整为：

```c
initscr();

int height_context = LINES - height_input - height_status - 2;
int width = COLS - 2;
```

---

# 三、刷新机制优化 ✅

## 问题

使用：

```c
printw()
mvwprintw()
```

更新窗口后没有刷新。

### 修复

所有窗口更新后增加：

```c
wrefresh(window);
```

例如：

```c
draw_context(context,&history);

wrefresh(context);
```

---

# 四、窗口属性设置修复

## 问题

子窗口使用：

```c
keypad(stdscr, TRUE);
```

导致输入窗口无法正确获取特殊按键。

## 修复

修改：

```c
keypad(input, TRUE);
```

让输入窗口单独接收：

* Enter
* Backspace
* Ctrl+C
* 方向键

---

# 五、输入系统实现 ✅

完成用户输入模块：

## 功能

支持：

* 输入普通字符
* 回车提交
* Backspace 删除
* Ctrl+C 清空输入
* `/exit`退出

核心函数：

```c
void read_command(
    WINDOW *input,
    char *command
)
```

---

## Ctrl+C行为

需求：

> Ctrl+C 清空输入区，而不是退出程序

实现：

```c
else if(ch == 3)
{
    command[0]='\0';

    mvwhline(
        input,
        1,
        2,
        ' ',
        pos
    );

    return;
}
```

效果：

```
> hello^C

>
```

---

# 六、删除字符逻辑优化 ✅

## 原问题

使用：

```c
wdelch()
```

删除字符。

但是：

`wdelch()` 会删除当前位置字符，并将后续字符整体左移。

导致：

```
hello|
```

删除：

```
hell|
```

时窗口内容发生整体移动。

---

## 当前方案

改为手动覆盖：

```c
mvwaddch(
    input,
    1,
    pos+2,
    ' '
);
```

实现：

* 只删除最后一个字符
* 保留输入布局

---

# 七、历史消息管理完成 ✅

设计消息结构：

```c
typedef struct{

    char role[32];

    char content[MAX_MESSAGE_LENGTH];

}Message;
```

历史：

```c
typedef struct{

    Message messages[MAX_MESSAGES];

    int count;

}ChatHistory;
```

支持：

* User
* Assistant
* Tool Call
* Tool Result

---

# 八、消息滚动显示实现 ✅

## 问题

消息过多会覆盖状态栏和输入区。

## 解决

计算每条消息需要占用行数：

```c
rows_needed[i]
```

根据窗口高度：

```c
height-2
```

只显示最近消息。

核心逻辑：

```c
for(i=history->count-1;i>=0;i--)
{
    if(used_rows + rows_needed[i] > height-2)
        break;

    used_rows += rows_needed[i];

    start=i;
}
```

实现：

* 最近消息优先显示
* 输入区不会被覆盖

---

# 九、状态栏实现 ✅

状态栏显示：

```
Current Directory: ~/project              15:22:52
```

实现内容：

## 1. 当前目录显示

获取：

```c
getcwd()
```

替换项目根目录：

```
/home/user/project
```

显示：

```
~/project
```

代码：

```c
snprintf(
display_dir,
sizeof(display_dir),
"~%s",
cwd+strlen(root_dir)
);
```

---

## 2. 时间显示

使用：

```c
time()
localtime()
strftime()
```

格式：

```
HH:MM:SS
```

---

## 3. 颜色

绿色字体：

```c
init_pair(
1,
COLOR_GREEN,
COLOR_WHITE
);
```

---

# 十、本地命令执行功能完成 ✅

支持：

```
/ls
/pwd
/date
```

流程：

```
用户输入

↓

process_command()

↓

system_command()

↓

popen()

↓

读取输出

↓

显示 Tool Result
```

---

核心：

```c
FILE *fp=popen(command,"r");
```

读取：

```c
fgets()
```

返回：

```c
Tool Result
```

---

# 十一、cd命令支持 ✅

支持：

```
cd path
```

执行：

```c
chdir(path)
```

成功：

```
Tool Result:

Directory changed successfully
```

失败：

```
Directory change failed
```

---

# 十二、字符串安全处理优化 ✅

## 原问题

直接：

```c
strcpy()
```

存在：

* 缓冲区溢出风险

---

## 修改

使用：

```c
snprintf()
```

例如：

```c
snprintf(
message->content,
sizeof(message->content),
"%s",
content
);
```

优势：

* 限制最大长度
* 自动添加 `\0`
* 避免越界

---

# 十三、中文显示支持尝试 ✅

增加：

```c
mbstowcs()
```

实现：

```
多字节字符串

↓

宽字符

↓

ncurses显示
```

用于解决：

中文占用多个字节导致：

* 长度计算错误
* 自动换行异常

---

# 十四、当前代码结构

```
mini-agent

│
├── main()
│
├── draw_context()
│      └── 绘制历史消息
│
├── draw_status()
│      └── 显示目录和时间
│
├── draw_input()
│      └── 输入区域
│
├── read_command()
│      └── 键盘事件处理
│
├── process_command()
│      └── 指令解析
│
├── add_message()
│      └── 保存历史
│
└── system_command()
       └── 执行shell命令
```

当前核心代码已经完成以上模块。

---

# 十五、待完成任务

## 1. Markdown 简易渲染 ⏳

目标：

支持：

### 标题

输入：

```markdown
# Title
## Title
```

效果：

* 保留 #
* 整行绿色

---

### 加粗

输入：

```markdown
**text**
__text__
```

效果：

```
text
```

红色显示。

---

### 行内代码

输入：

```markdown
`code`
```

效果：

```
code
```

蓝色显示。

---

# 2. 多行输入优化 ⏳

当前：

* 单行输入

需要增加：

* Ctrl+J换行
* 自动换行
* 中文宽字符计算

---

# 3. 输入区固定3行 ⏳

需求：

```
+--------------+
| > input      |
|              |
|              |
+--------------+
```

需要优化：

* 光标移动
* 多行显示
* 滚动

---

# 4. 中文宽字符适配 ⏳

当前问题：

普通：

```
hello
```

长度：

```
5
```

中文：

```
你好
```

实际占用：

```
4 bytes
```

需要改为：

```c
wcwidth()
```

计算真实显示宽度。

---

# 今日完成总结

| 模块          | 状态 |
| ----------- | -- |
| ncurses初始化  | ✅  |
| 三窗口布局       | ✅  |
| 状态栏         | ✅  |
| 时间显示        | ✅  |
| 目录显示        | ✅  |
| 输入系统        | ✅  |
| Ctrl+C清空    | ✅  |
| Backspace删除 | ✅  |
| 历史消息        | ✅  |
| 消息滚动        | ✅  |
| 本地命令执行      | ✅  |
| cd命令        | ✅  |
| 安全字符串处理     | ✅  |
| 中文转换基础      | ✅  |
| Markdown渲染  | ⏳  |
| 多行输入        | ⏳  |
| 完整中文排版      | ⏳  |

---

## 今日开发重点总结

今天主要完成了 Mini-Agent 终端框架从“能运行”到“可交互”的基础能力建设，解决了 ncurses 窗口管理、输入事件处理、消息缓存、终端刷新、命令执行等核心问题。目前已经具备一个完整 Agent Terminal 的雏形，下一阶段重点完善 Markdown 渲染、多行输入以及中文宽字符布局，使交互体验接近真实终端 Agent。
