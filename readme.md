# 你好哇！

## 这里是我存放C++的各种练习的地方

因为比较杂乱，不讲究什么形式，就是简简单单的名副其实的 PLAYGROUND 。

如果有空的话，我会更新一些经典算法的实现，例如floyd，dijstra，最小生成树，dfs，bfs等等。可能还有本学期数据结构与算法，课程里的练习和作业。

以上

## Project命名和使用规范

### 关于cpp的创建和命名

自己用于调试和练习的文件，一般命名为 `demo+数字+.cpp`

### 关于.h的创建和命名

当一个 project 文件经过自己单文件编译调试，和多次重构比较成熟后，我会将类对象的声明和实现分别存储在新的头文件和cpp文件当中。

其命名格式一般为：
1. 如果project_名称A，A代表某一类型的数据结构，则其中的.h文件会被命名为 `myA.h` 相应的实现文件为 `myA.cpp`.
2. 如果A带代表一类特定的算法，则会命名为 `A_mtd.h` 或者 `A_alg.h`.

## 仓库链接

~~十一作业题~~ （因老师的建议，还是避免直接公布课程作业的代码）

- [链表](./project_list/readme.md)
- [栈结构](./project_stack/readme.md)
- [排序算法](./project_sort/readme.md)


## 考试

40分选择题，40分问答题，20分算法设计题

基本概念3题 数据结构、算法、时间复杂度
### 选择题 2*20

12个选择题：
链式结构：为什么要用链式结构？不同形式的链表？
顺序表、链表检索插入删除的优缺点：向量访问快，插删要搬家：链表访问慢、插删很方便
栈和队列的递归、循环队列的实现、脑模拟

二叉树：各种遍历、给出两个顺序的遍历，确定树的形状
树的层数和节点数的关系，什么叫完全二叉树？满二叉树？计算

二叉排序树：二叉排序树的定义，功能。

霍夫曼树：给一组数据构建一棵树，霍夫曼树的定义、带权路径长度概念。

3个图的题目：邻接矩阵、广度优先搜索（队列实现）、最小生成树（prim，kruscal）。

排序：插入排序、选择排序、冒泡、快速排序、希尔排序、堆排序（要求写出每一次循环排序的状态）。

### 问答题 5*8

1. 链表操作程序，补充代码。（5个空要填，注意审清楚题目要求）
2. 排序排序填代码
3. 二叉树的填代码
4. 栈的填代码，出栈入栈
5. 图的填代码，prim 和  

### 算法设计题 2*10

1. 利用栈实现，表达式的转换，回文字符 （直接调用函数就可以啦，不需要关心它怎么实现的）
2. 看上去很简单，