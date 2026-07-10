#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ncurses.h>            /* ncurses.h includes stdio.h */  
#include<time.h>
#include <string.h> 

#define MAX_COMMAND_LENGTH 256
#define MAX_MESSAGES 8
#define MAX_MESSAGE_LENGTH 2048

typedef struct{
    char role[32];
    char content[MAX_MESSAGE_LENGTH];
} Message;

typedef struct{
    Message messages[MAX_MESSAGES];
    int count;
} ChatHistory;

void draw_context(WINDOW *context, const ChatHistory *history);
void draw_status(WINDOW *status);
void draw_input(WINDOW *input, char *command);
void read_command(WINDOW *input, char *command);
void process_command(const char *command, ChatHistory *history);
void add_message(ChatHistory *history, const char *role, const char *content);
int system_command(const char *command, char *output, size_t output_size);

int main()
{
    initscr(); /* Start curses mode        */
    if (stdscr == NULL) {
        fprintf(stderr, "initscr() failed\n");
        exit(1);
    }
    start_color();       /* Start color functionality */
    raw();             /* Line buffering disabled, Pass on everty thing to me */
    noecho();
    keypad(stdscr, TRUE);       /* I need that nifty F1 */

    int height_input= 5;          // 留出上下边界
    int height_status = 1;         // 留出上下边界
    int height_context = LINES - height_input - height_status - 2; // 留出上下边界
    int width = COLS - 2;             // 留出左右边界
    int startx = 0;                 // 从第0列开始
    int start_y_context = 0;
    int start_y_status = height_context;
    int start_y_input = height_context + height_status;

    
    char command[MAX_COMMAND_LENGTH]; // 用于存储用户输入的命令


    WINDOW *context = subwin(stdscr, height_context, width, start_y_context, startx);
    WINDOW *status = subwin(stdscr, height_status, width, start_y_status, startx);
    WINDOW *input = subwin(stdscr, height_input, width, start_y_input, startx);

    ChatHistory history = {0};
    draw_context(context, &history);
    draw_status(status);

    while(1){
        draw_input(input, command);
        
        if (strcmp(command, "/exit") == 0) {
            break; // 如果用户输入 "/exit"，则退出循环
        }
        if (command[0] == '\0') {
            continue; // 如果用户输入为空，则继续下一次循环
        }
        process_command(command, &history);
        draw_context(context, &history);
        draw_status(status);

        wrefresh(context);
        wrefresh(status);
        wrefresh(input);

    }  

    delwin(input);
    delwin(status);
    delwin(context);
    
    endwin();           /* End curses mode        */
    return 0;
}

void draw_context(WINDOW *context, const ChatHistory *history) {
    int height, width; // 获取窗口的高度和宽度
    getmaxyx(context, height, width);
    int rows_needed[MAX_MESSAGES];

    for (int i = 0; i < history->count; i++) {
        int rows = 1;
        int column = 0;
        const char *text = history->messages[i].content;
        for (int j = 0; text[j] != '\0'; j++) {
            if (text[j] == '\n') {
                rows++;
                column = 0;
            } else {
                column++;
                if (column >= width - 2) { // 考虑边框
                    rows++;
                    column = 0;
                }
            }
        }
        rows++;
        rows_needed[i] = rows;
    }

        int start = history->count;
        int used_rows = 0;
        for (int i = history->count - 1; i >= 0; i--) {
            if (used_rows + rows_needed[i] > height - 2) { // 考虑边框
                break;
            }
            used_rows += rows_needed[i];
            start = i;
        }

    werase(context);    // 清空窗口内容
    box(context, 0, 0); // 重新绘制边框
    int y = 1; // 从第一行开始绘制内容
    for (int i = start; i < history->count; i++)
    {
        if(y>= height - 1) break; // 如果超过窗口高度，停止绘制
        wattron(context, A_BOLD); // 启用粗体
        mvwprintw(context, y, 1, "%s:", history->messages[i].role);
        wattroff(context, A_BOLD); // 禁用粗体
        y++;
        const char *text = history->messages[i].content;
        int x = 1; // 从第一列开始绘制内容
        for (int j = 0; text[j] != '\0' && y < height - 1; j++) {
            if (text[j] == '\n') {
                y++;
                x = 1;
            } 
            if (y>= height - 1) break; 
            mvwaddch(context, y, x, text[j]);
            x++;
        }
        y += 2;
    }
    wrefresh(context); // 刷新窗口以显示更改
}



void draw_status(WINDOW *status) {
    const char *root_dir = "/home/hilda/daytest/daytest-practice/mini-agent"; // 根目录
    char *cwd = getcwd(NULL, 0); // 获取当前工作目录
    char display_dir[256]; // 用于存储显示的目录
    if (cwd != NULL && strncmp(cwd, root_dir, strlen(root_dir)) == 0) {
        // 如果当前目录在根目录下，显示相对路径
        snprintf(display_dir, sizeof(display_dir), "~%s", cwd + strlen(root_dir));
    } else if (cwd != NULL) {
        // 如果当前目录不在根目录下，显示绝对路径
        snprintf(display_dir, sizeof(display_dir), "%s", cwd);
    } else {
        // 获取当前工作目录失败，显示未知目录
        snprintf(display_dir, sizeof(display_dir), "Unknown Directory");
    }


    time_t now = time(NULL); // 获取当前时间
    struct tm *local_time = localtime(&now);
    char time_str[20] = "Unknown Time";
    if (local_time != NULL) {
        strftime(time_str, sizeof(time_str), "%H:%M:%S", local_time);
    }

    int height, width; // 获取窗口的高度和宽度
    getmaxyx(status, height, width); 

    init_pair(1, COLOR_GREEN, COLOR_WHITE); // 设置颜色对

    werase(status); // 清空窗口内容
    wattron(status, COLOR_PAIR(1)); // 启用颜色对
    mvwhline(status,0,0, ' ', width); // 在窗口中绘制一条水平线
    mvwprintw(status, 0, 0, "Current Directory: %s ", display_dir); // 在窗口中写入目录
    mvwprintw(status, 0, width - strlen(time_str) , "%s", time_str); // 在窗口中写入时间
    wattroff(status, COLOR_PAIR(1)); // 禁用颜色对
    wrefresh(status); // 刷新窗口以显示更改
    free(cwd); // 释放内存
}
    

void draw_input(WINDOW *input, char *command) {
    keypad(input, TRUE); // 启用键盘输入
    werase(input); // 清空窗口内容
    box(input, 0, 0); // 重新绘制边框
    mvwprintw(input, 1, 1, ">"); // 在窗口中写入文本
    read_command(input, command); // 调用 read_command 函数读取用户输入的命令

}

void read_command(WINDOW *input, char *command) {
    int ch;
    int pos = 0; // 输入位置
    command[0] = '\0'; // 初始化命令为空字符串

    curs_set(1); // 显示光标
    wmove(input, 1, 2); // 将光标移动到输入位置
    wrefresh(input); // 刷新窗口以显示更改
    
    while (1) {
        ch = wgetch(input);
        if (ch == '\n'|| ch == KEY_ENTER) { // 如果按下回车键
            command[pos] = '\0'; // 在命令末尾添加字符串结束符
            mvwhline(input, 1, 2, ' ', pos); // 清除输入行的内容
            wmove(input, 1, 2); // 将光标移动到输入位置
            curs_set(0); // 隐藏光标
            return; // 退出循环
        } else if (ch == KEY_BACKSPACE || ch == 127) { // 如果按下退格键
            if (pos > 0) {
                pos--; // 移动输入位置向前
                command[pos] = '\0'; // 删除最后一个字符
                mvwaddch(input, 1, pos + 2, ' '); // 删除窗口中的字符（+2 是因为有 "> " 前缀）
                wmove(input, 1, pos + 2); // 将光标移动到新的输入位置
            }
        } else if (ch == 3) { // 如果按下 Ctrl+C
            command[0] = '\0'; // 清空命令
            mvwhline(input, 1, 2, ' ', pos); // 清除输入行的内容
            wmove(input, 1, 2); // 将光标移动到输入位置
            curs_set(0); // 隐藏光标
            return; // 退出循环
        } else if (ch >= 32 && ch <= 126 && pos < 255) { // 限制命令长度为 255 个字符
            command[pos++] = ch; // 将输入的字符添加到命令中
            command[pos] = '\0'; // 在命令末尾添加字符串结束符
            waddch(input, ch); // 在窗口中显示输入的字符
        } 
        wrefresh(input); // 刷新窗口以显示更改
    }
}

void process_command(const char *command, ChatHistory *history) {
    add_message(history, "User", command);
    if (strncmp(command, "cd ", 3) == 0) {
        const char *path = command + 3;

        char summary[MAX_MESSAGE_LENGTH];
        snprintf(
            summary,
            sizeof(summary),
            "Calling chdir, target directory: %s",
            path
        );

        add_message(history, "Tool Call", summary);

        if (chdir(path) == 0) {
            char *cwd = getcwd(NULL, 0);

            char result[MAX_MESSAGE_LENGTH];
            snprintf(
                result,
                sizeof(result),
                "Directory changed successfully: %s",
                cwd != NULL ? cwd : "Unknown"
            );

            add_message(history, "Tool Result", result);
            free(cwd);
        } else {
            add_message(
                history,
                "Tool Result",
                "Directory change failed"
            );
        }

        return;
    }

    /*
     * 以 / 开头的内容作为本地命令。
     * 跳过最前面的 /。
     */
    if (command[0] == '/' && command[1] != '\0') {
        const char *local_command = command + 1;

        char summary[MAX_MESSAGE_LENGTH];
        snprintf(
            summary,
            sizeof(summary),
            "Executing local command: %s",
            local_command
        );

        add_message(history, "Tool Call", summary);

        char output[MAX_MESSAGE_LENGTH];

        if (
            system_command(
                local_command,
                output,
                sizeof(output)
            ) == 0
        ) {
            add_message(history, "Tool Result", output);
        } else {
            add_message(
                history,
                "Tool Result",
                "Failed to execute local command."
            );
        }

        return;
    }

    /*
     * 普通文本固定回复。
     */
    add_message(
        history,
        "Assistant",
        "## `429` Too many Requests\n\n"
        "**busy服务器繁忙，请稍后再试**"
    );
}


void add_message(ChatHistory *history, const char *role, const char *content) {
    // 将新消息添加到消息数组中
    if (history->count >= MAX_MESSAGES) {
        memmove(&history->messages[0], &history->messages[1], sizeof(Message)*(MAX_MESSAGES - 1)); // 将现有消息向后移动一位
        history->count = MAX_MESSAGES - 1;
    }
    Message *message = &history->messages[history->count];
    snprintf(message->role, sizeof(message->role), "%s", role); // 设置消息角色
    snprintf(message->content, sizeof(message->content), "%s", content); // 设置消息内容
    history->count++; // 增加消息计数
}

int system_command(const char *command, char *output, size_t output_size) {
    // 使用 popen 执行命令并获取输出
    FILE *fp = popen(command, "r");
    if (fp == NULL) {
        snprintf(output, output_size, "Failed to run command: %s", command);
        return -1;
    }

    output[0] = '\0'; // 初始化输出为空字符串
    char line[512];
    while (fgets(line, sizeof(line), fp) != NULL) {
        size_t used = strlen(output);
        if (used >= output_size -1) {
            break; // 如果输出缓冲区已满，停止读取
        }
        strncat(output, line, output_size - used - 1);
    }
    int result = pclose(fp); // 关闭文件指针并获取命令执行结果
    if (output[0] == '\0') {
        snprintf(output, output_size, "Command executed with no output.");
    }
    return result == -1? -1 : 0; // 返回命令执行结果
}
