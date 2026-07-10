#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ncurses.h>            /* ncurses.h includes stdio.h */  
#include<time.h>
#include <string.h> 

void draw_context(WINDOW *context, char *command);
void draw_status(WINDOW *status);
void draw_input(WINDOW *input, char *command);
void read_command(WINDOW *input, char *command);

int main()
{
    initscr();          /* Start curses mode        */
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

    
    char command[256]; // 用于存储用户输入的命令

    WINDOW *context = subwin(stdscr, height_context, width, start_y_context, startx);
    WINDOW *status = subwin(stdscr, height_status, width, start_y_status, startx);
    WINDOW *input = subwin(stdscr, height_input, width, start_y_input, startx);

    while(1){
        draw_status(status);
        draw_input(input, command);
        
        if (strcmp(command, "/exit") == 0) {
            break; // 如果用户输入 "/exit"，则退出循环
        }

        draw_context(context, command);

        wrefresh(context);
        wrefresh(status);
        wrefresh(input);

    }  

    refresh();

    endwin();           /* End curses mode        */
    return 0;
}

void draw_context(WINDOW *context, char *command) {
    werase(context); // 清空窗口内容
    box(context, 0, 0); // 重新绘制边框
    mvwprintw(context, 1, 1, "command: %s", command); // 在窗口中写入文本
    wrefresh(context); // 刷新窗口以显示更改
    ust_command(command); // 调用 ust_command 函数处理用户输入的命令
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

void ust_command(char *command) {
    if (strcmp(command, "/exit") == 0) {
        // 如果用户输入 "/exit"，则退出程序
        endwin(); // 结束 ncurses 模式
        exit(0); // 退出程序
    } else if (strncmp(command, "cd ", 3) == 0) {
        // 如果用户输入 "cd "，则尝试更改目录
        char *path = command + 3; // 获取路径部分
        if (chdir(path) != 0) { // 尝试更改目录
            perror("chdir failed"); // 如果失败，打印错误信息
        }
    } else {
        // 对于其他命令，使用系统调用执行
        int ret = system(command); // 执行命令
        if (ret == -1) { // 如果执行失败
            perror("system failed"); // 打印错误信息
        }
    }
}