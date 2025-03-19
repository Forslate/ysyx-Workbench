/**************************************************************************************************
* Copyright (c) 2014-2024 Zihao Yu, Nanjing University                                            *
*                                                                                                 *
* NEMU is licensed under Mulan PSL v2.                                                            *
* You can use this software according to the terms and conditions of the Mulan PSL v2.            *
* You may obtain a copy of Mulan PSL v2 at:                                                       *
*          http://license.coscl.org.cn/MulanPSL2                                                  *
*                                                                                                 *
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,                  *
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,                       *
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.                                                *
*                                                                                                 *
* See the Mulan PSL v2 for more details.                                                          *
**************************************************************************************************/


/*======================================== Header Files =========================================*/
/// Project headers
#include <isa.h>
#include <cpu/cpu.h>

/// System Libraries with GNU C Library
#include <readline/readline.h>
#include <readline/history.h>

/// Project specific modules
#include "sdb.h"
#include "utils.h"
/*======================================== Header Files =========================================*/

/*======================================= Global Settings =======================================*/
/// 是否为批处理模式，这里为 `否`
static int is_batch_mode = false;
/*======================================= Global Settings =======================================*/

void init_regex();
void init_wp_pool();

/*===================================== Function Prototypes =====================================*/
static char *rl_gets();

static int cmd_c(char *args);
static int cmd_q(char *args);
static int cmd_help(char *args);

void sdb_set_batch_mode();
void sdb_mainloop();
void init_sdb();
/*===================================== Function Prototypes =====================================*/

/*================================= rl_gets function definition =================================*/
/* We use the `readline` library to provide more flexibility to read from stdin. */

/// rl_gets == readline gets ?
/// 定义一个返回字符指针的静态函数。
/// static 表示该函数仅在当前文件可见。
static char *rl_gets()
{
    /// 声明一个静态指针变量，初始化为NULL。
    static char *line_read = NULL;

    /// 检查先前是否有读取的内容。
    if (line_read) {
        /// 若存在（即指针为非空）
        /// 则调用 `free` 函数释放先前分配的内存（避免内存泄露）。
        free(line_read);
        /// 将指针重置为 `NULL`（防止野指针）。
        line_read = NULL;
    }

    /// 调用 GNU readline 函数
    /// 显示提示符为 `(nemu) `，等待用户输入。
    line_read = readline("(nemu) ");

    /// 双重检查：
    /// line_read 不为空指针（确保从 `readline` 获取输入）。
    /// `line_read` 是指向字符的指针，存储了从 `readline` 获取的用户输入字符串的地址。
    /// C语言中，指针变量可以指向内存中的某个地址，或者为 `NULL`。
    /// 所以可能性为：非空指针（指向用户输入字符串的首地址）或者为 `NULL`。
    /// 首先检查 `line_read` 是否为非空，如果是，则继续检查第二个条件。
    ///
    /// *line_read 不是空字符（用户没有输入空行）。
    /// 如果 `line_read` 非空，则意味着指向了一个有效的字符串地址，`*line_read` 是对该指针的解引用，
    /// 即访问指针指向的内存位置的值，对于字符串是检查字符串的第一个字符。
    /// 字符串在C语言中以空字符（\0）结尾。
    /// 如果用户直接键入回车键（没有输入任何内容），`readline` 会返回一个空字符串（即一个半酣单个
    /// `\0` 的字符串）。
    /// 在ASCII中对应的是0，在条件判断中视为 假。
    if (line_read && *line_read) {
        /// 如果通过检查，则将输入添加到历史记录中。
        add_history(line_read);
    }

    /// 返回用户输入的字符串指针。
    /// 不需要释放该内存，会在下一次调用本函数时自动释放。
    return line_read;
}
/*================================= rl_gets function definition =================================*/

/*=========================================== cmd_table =========================================*/
/// 匿名结构体（没有类型名），直接用于数组定义
/// 适合一次性使用的结构
static struct {
    /// 不可修改的字符串指针
    /// 存储命令的触发名称
    /// 例如输入 "help" 时会匹配该字段，查找 `name` 为 "help" 的条目
    const char *name;
    /// 帮助信息显示
    const char *description;
    /// 函数指针：接受 char * 的参数，返回 `int` 的参数
    /// `int`：返回值类型
    /// `(*handler)`：指针变量名
    /// `(char *)`：参数类型（指向命令参数的字符串。
    int (*handler) (char *);
} cmd_table [] = {
    {.name = "help", .description = "Display information about all supported commands", .handler = cmd_help},
    {.name = "c", .description = "Continue the execution of the program", .handler = cmd_c},
    {.name = "q", .description = "Exit NEMU", .handler = cmd_q},

    /* TODO: Add more commands */
};

#define NR_CMD ARRLEN(cmd_table)
/*=========================================== cmd_table =========================================*/

/*************************************************************************************************/
static int cmd_c(char *args)
{
    cpu_exec(-1);

    return 0;
}
/*************************************************************************************************/

/*************************************************************************************************/
static int cmd_q(char *args)
{
    nemu_state.state = NEMU_QUIT;

    return -1;
}
/*************************************************************************************************/

/*************************************************************************************************/
static int cmd_help(char *args)
{
    /* extract the first argument */
    char *arg = strtok(NULL, " ");
    int i;

    if (arg == NULL) {
        for (i = 0; i < NR_CMD; i++) {
            printf("%s\t - %s\n", cmd_table[i].name, cmd_table[i].description);
        }
    } else {
        for (i = 0; i < NR_CMD; i++) {
            if (strcmp(arg, cmd_table[i].name) == 0) {
                printf("%s\t - %s\n", cmd_table[i].name, cmd_table[i].description);

                return 0;
            }
        }

        printf("Unknown command '%s'\n", arg);
    }

    return 0;
}
/*************************************************************************************************/

/*************************************************************************************************/
void sdb_set_batch_mode()
{
    is_batch_mode = true;
}
/*************************************************************************************************/

/*************************************************************************************************/
void sdb_mainloop()
{
    if (is_batch_mode) {
        cmd_c(NULL);

        return;
    }

    for (char *str; (str = rl_gets()) != NULL; ) {
        char *str_end = str + strlen(str);

        /* extract the first token as the command */
        char *cmd = strtok(str, " ");
        if (cmd == NULL) continue;

        /* treat the remaining string as the arguments,
         * which may need further parsing
         */
        char *args = cmd + strlen(cmd) + 1;
        if (args >= str_end) {
            args = NULL;
        }

#ifdef CONFIG_DEVICE
        extern void sdl_clear_event_queue();
        sdl_clear_event_queue();
#endif
        int i;
        for (i = 0; i < NR_CMD; i++) {
            if (strcmp(cmd, cmd_table[i].name) == 0) {
                if (cmd_table[i].handler(args) < 0) return;
                break;
            }
        }

        if (i == NR_CMD) printf("Unknown command '%s'\n", cmd);
    }
}
/*************************************************************************************************/

/*************************************************************************************************/
void init_sdb()
{
    /* Compile the regular expressions. */
    init_regex();

    /* Initialize the watchpoint pool. */
    init_wp_pool();
}
/*************************************************************************************************/
