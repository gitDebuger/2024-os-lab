#!/bin/sh
# 用于以后台方式运行 QEMU 并与 GDB 调试器连接
# 将 GDB 调试器的 PID 赋值给变量 gdb_pid
gdb_pid=$1
# 设置一个陷阱捕获 SIGINT 信号 Ctrl+C 并将其忽略
# 即按下 Ctrl+C 时不会中断脚本的执行
trap '' SIGINT
# 检查是否存在名为 /tmp/qemu_stdout 的命名管道文件
# 如果不存在则创建一个
# 这个命名管道用于将 QEMU 的标准输出写入文件以便后续处理
[ ! -p /tmp/qemu_stdout ] && mkfifo /tmp/qemu_stdout
# 启动 QEMU 进程并以后台方式运行
# -s -S 参数启用了 QEMU 的 GDB 调试模式
# /dev/null 用于将标准输入重定向到空设备以确保 QEMU 不会从标准输入读取任何内容
# >/tmp/qemu_stdout 将 QEMU 的标准输出重定向到 /tmp/qemu_stdout 文件中
$QEMU $QEMU_FLAGS -kernel $mos_elf </dev/null -s -S >/tmp/qemu_stdout &
# 获取 QEMU 进程的 PID 并赋值给变量 qemu_pid
qemu_pid=$!
# 读取 /tmp/qemu_stdout 中的内容并将其同时输出到终端和 .qemu_log 文件中
# tee -a 选项表示追加写入文件
cat /tmp/qemu_stdout | tee -a .qemu_log &
# 不断检查 GDB 进程和 QEMU 进程是否仍在运行
# kill -0 测试命令用于检查进程是否存在
# 如果 GDB 进程或 QEMU 进程不存在则退出循环
while kill -0 $gdb_pid 2>/dev/null && kill -0 $qemu_pid 2>/dev/null ; do
    sleep 1;
done
# 杀死 QEMU 进程
# 使用 -9 选项表示强制终止进程
# 2>/dev/null 用于将错误输出重定向到空设备以防止输出错误信息到终端
# true 命令确保即使 kill 命令失败也不会导致脚本退出
kill -9 $qemu_pid 2>/dev/null;true
# 如果存在 .qemu_log 文件则删除它
[ -f .qemu_log ] && rm .qemu_log
# 为什么要先写入 .qemu_log 再删除它