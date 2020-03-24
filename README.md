# leveldb 源代码添加注释

2020-03-24：
之前阅读过的部分添加注释：

* DB类和DBImpl类，提供的接口
* WriteBetch类的含义，操作集合的格式
* DBImpl类，Get操作的流程
* LookupKey类，用于查找的entry的编码方式。
* Version类， Get流程，在当前版本的sstable文件中进行查找的流程。
（到sstable文件cache为止，leveldb采用了LRUcache策略）

##### todo：
Write流程到sstable文件存储格式。
