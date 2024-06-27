# Lab0实验报告

## 思考题

### Thinking 0.1

思考下列有关Git的问题。

依次执行命令：

```bash
touch README.txt
git status > Untracked.txt
echo "This a test file" > README.txt
git add -A
git status > Stage.txt
git commit -m "student ID: 22371264"
cat Untracked.txt
cat Stage.txt
```

两次运行，文件 `README.txt` 分别位于**“未跟踪的文件”**和**“要提交的变更”**标签下。

![lab0-1](pic\lab0-1.png)

依次执行命令：

```bash
echo "new content\n" >> README.txt
git status > Modified.txt
cat Modified.txt
```

本次运行，文件 `README.txt` 位于**“尚未暂存以备提交的变更”**标签下。

因为该文件已经被跟踪过，所以不会位于未跟踪标签下，而是相对于上次提交发生了变化，所以会位于已变更标签下。

![lab0-2](pic\lab0-2.png)

### Thinking 0.2

对应的命令如下：

```bash
# add the file
git add
# stage the file
git add
# commit
git commit
```

### Thinking 0.3

使用的命令如下：

```bash
# 代码文件 print.c 被错误删除
git checkout print.c
# 或者
git restore print.c
# 代码文件 print.c 被错误删除后并且执行了 git rm print.c 命令
git reset HEAD print.c
git checkout print.c
# 或者
git reset HEAD print.c
git restore print.c
# 无关文件 hello.txt 已经被添加到暂存区
# 在不删除此文件的前提下将其移出暂存区
git rm --cached hello.txt
```

### Thinking 0.4

依次执行命令：

```bash
echo "Testing 1" >> README.txt
git add README.txt
git commit -m "1"
echo "Testing 2" >> README.txt
git add README.txt
git commit -m "2"
echo "Testing 3" >> README.txt
git add README.txt
git commit -m "3"
git log
```

日志内容如下(更早的日志已省略)：

```
commit daf67700a3390e0617f086e84521d72be2ad3cf8 (HEAD -> master)
Author: HuYunge <silver_wolf@hyg>
Date:   Wed Mar 13 23:15:18 2024 +0800

    3

commit dbf59701cf1a15c4d8b93c137792bc1368166b57
Author: HuYunge <silver_wolf@hyg>
Date:   Wed Mar 13 23:15:03 2024 +0800

    2

commit 009ed016c831acffacb54289c8f07e77a5537749
Author: HuYunge <silver_wolf@hyg>
Date:   Wed Mar 13 23:14:41 2024 +0800

    1
```

提交说明 `3` 的哈希值为 `daf67700a3390e0617f086e84521d72be2ad3cf8` 。

依次执行命令：

```bash
git reset --hard HEAD^
git log
```

日志内容如下(更早的日志已省略)：

```
commit dbf59701cf1a15c4d8b93c137792bc1368166b57 (HEAD -> master)
Author: HuYunge <silver_wolf@hyg>
Date:   Wed Mar 13 23:15:03 2024 +0800

    2

commit 009ed016c831acffacb54289c8f07e77a5537749
Author: HuYunge <silver_wolf@hyg>
Date:   Wed Mar 13 23:14:41 2024 +0800

    1
```

可以发现第三次提交的内容被清除了，回退到了第二次提交后的状态。

依次执行命令：

```bash
git reset --hard 009ed016c831acffacb54289c8f07e77a5537749
git log
```

日志内容如下(更早的日志已省略)：

```
commit 009ed016c831acffacb54289c8f07e77a5537749 (HEAD -> master)
Author: HuYunge <silver_wolf@hyg>
Date:   Wed Mar 13 23:14:41 2024 +0800

    1
```

可以发现回到了第一次提交后的状态。

如果尝试回到最新提交也就是第三次提交后的状态，依次执行如下命令：

```bash
git reset --hard daf67700a3390e0617f086e84521d72be2ad3cf8
git log
```

日志内容如下(更早的日志已省略)：

```bash
commit daf67700a3390e0617f086e84521d72be2ad3cf8 (HEAD -> master)
Author: HuYunge <silver_wolf@hyg>
Date:   Wed Mar 13 23:15:18 2024 +0800

    3

commit dbf59701cf1a15c4d8b93c137792bc1368166b57
Author: HuYunge <silver_wolf@hyg>
Date:   Wed Mar 13 23:15:03 2024 +0800

    2

commit 009ed016c831acffacb54289c8f07e77a5537749
Author: HuYunge <silver_wolf@hyg>
Date:   Wed Mar 13 23:14:41 2024 +0800

    1
```

可以发现回到了第三次提交后的状态，并且第二次提交后的日志记录也被恢复了。

### Thinking 0.5

依次执行命令：

```bash
# 控制台输出一行first
echo first
# 文件一行内容second
echo second > output.txt
cat output.txt
# 文件一行内容third
echo third > output.txt
cat output.txt
# 文件两行内容
# 第一行third
# 第二行fourth
echo fourth >> output.txt
cat output.txt
```

如果不添加重定向，默认输出到标准输出——控制台。

重定向时使用 `>` 意为覆盖，使用 `>>` 意为追加。

### Thinking 0.6

文件 `command.sh` 的内容如下：

```bash
#!/bin/bash
file=test.sh
echo '#!/bin/bash
echo Shell Start...
echo set a = 1
a=1
echo set b = 2
b=2
echo set c = a+b
c=$[$a+$b]
echo c = $c
echo save c to ./file1
echo $c>file1
echo save b to ./file2
echo $b>file2
echo save a to ./file3
echo $a>file3
echo save file1 file2 file3 to file4
cat file1>file4
cat file2>>file4
cat file3>>file4
echo save file4 to ./result
cat file4>>result' > $file
```

文件 `test.sh` 的内容如下：

```bash
#!/bin/bash
# 输出文本内容 Shell Start
echo Shell Start...
# 输出文本内容 set a = 1
echo set a = 1
# 将变量a的值设置为1
a=1
# 输出文本内容 set b = 2
echo set b = 2
# 将变量b的值设置为2
b=2
# 输出文本内容 set c = a+b
echo set c = a+b
# 取变量a的值和变量b的值相加得到临时变量再取该临时变量的值赋给变量c
c=$[$a+$b]
# 输出文本内容 c = 3
# 因为此时c的值为3
echo c = $c
# 输出文本内容 save c to ./file1
echo save c to ./file1
# 输出c的值将标准输出重定向到file1
echo $c>file1
# 输出文本内容 save b to ./file2
echo save b to ./file2
# 输出b的值将标准输出重定向到file2
echo $b>file2
# 输出文本内容 save a to ./file3
echo save a to ./file3
# 输出a的值将标准输出重定向到file3
echo $a>file3
# 输出文本内容 save file1 file2 file3 to file4
echo save file1 file2 file3 to file4
# 以覆盖形式输出file1文件的内容到file4文件
# 此时file4中有一行 3
cat file1>file4
# 以追加形式输出file2文件的内容到file4文件
# 此时file4中有两行
# 第一行内容 3
# 第二行内容 2
cat file2>>file4
# 以追加形式输出file3文件的内容到file4文件
# 此时file4中有三行
# 第一行内容 3
# 第二行内容 2
# 第三行内容 1
cat file3>>file4
# 输出文本内容 save file4 to ./result
echo save file4 to ./result
# 以追加形式输出file4文件的内容到result文件
# 此时result中有三行
# 第一行内容 3
# 第二行内容 2
# 第三行内容 1
cat file4>>result
```

文件 `result` 的内容如下：

```bash
3
2
1
```

解释说明见 `test.sh` 中的注释。

然后是两组代码对比：

```bash
# Group 1
echo echo Shell Start   # 输出 echo Shell Start
echo `echo Shell Start` # 输出 Shell Start
# Group 2
echo echo $c>file1   # 输出 echo $c>file1
echo `echo $c>file1` # 输出 空行
```

不使用反引号的内容会以字符串的形式输出，使用反引号包裹的内容会以指令形式执行，然后返回值作为整段代码的一部分继续执行。

## 难点分析

### Makefile

* 在Makefile文件中可以引用其他Makefile文件，通过多级Makefile实现对复杂程序的编译和链接。
* 使用 `.PHONY` 定义伪目标形成合理的生成层次。
* 在编写Makefile文件时使用通配符、隐式规则和自动变量简化文件编写。
* 定义变量实现代码的复用，同时便于修改。
* 使用 `VPATH` 和 `vpath` 指定搜索路径，避免手动硬编码路径，同时提高Makefile文件的泛用性。

### Shell Programming

* 使用 `sed` 命令对文档进行删除、替换和查找等操作。

* 在使用命令时利用双引号对以 `$` 开头的字符串整体或部分进行值替换，从而执行更灵活的操作。

* 使用通配符进行批量操作。

* 使用管道将多个命令串联操作。

* 使用 `grep` 命令对文本执行特定的查找任务。

* 利用双圆括号的算数扩展功能执行运算和判断操作。

* 使用大括号将需要替换的部分包裹起来，使用 `&` 引用文件描述符指定的文件。

    ```bash
    sed -n "${n}p" err.txt>&2
    ```

* 在 `.sh` 文件中使用 `getopts` 和 `case` 对命令行参数进行解析。

    ```bash
    while getopts "f:q:p:sh" opt; do
        case $opt in
            f) FILE=${OPTARG} ;;
            q) CMD=${OPTARG} ;;
            p) PID=${OPTARG} ;;
            s) SORT=true ;;
            h) usage; exit 0 ;;
            ?) usage; exit 1 ;;
        esac
    done
    ```

* 在 `awk` 命令中使用 `-v` 参数向传给 `awk` 的代码中传递变量的值。

    ```bash
    awk -v input=$CMD '{if(index($5,input)!=0){print $0}}' $FILE
    ```

* 使用 `awk` 命令利用其内置的代码执行功能对文本执行复杂的处理工作。

    ```bash
    awk -v input=$PID '{
                           pid[NR] = $2
                           ppid[NR] = $3
                       } END {
                           p = input
                           for (i = 1; p != 0; ++i) {
                               if (pid[i] == p) {
                                   p = ppid[i]
                                   print p
                                   i = 0
                               }
                           }
                       }' $FILE
    ```

* 使用 `sort` 命令对文本进行排序。

    ```bash
    sort -k4nr -k2n $FILE
    ```

## 实验体会

* 限时实验难度并不低，课下必须做好充足的准备。
* 注意读题，确保文件路径正确。
* 题目中的tips对解题能够提供很多帮助。
* 可以通过 `man` 查看命令说明文档，获取详细的使用信息。
* 确保检查无误后再进行提交，提交冷却时间会逐渐增加，影响做题进度。

