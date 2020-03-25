# leveldb 源代码添加注释

2020-03-24：
之前阅读过的部分添加注释：

* DB类和DBImpl类，提供的接口
* WriteBetch类的含义，操作集合的格式
* DBImpl类，Get接口的流程
* LookupKey类，用于查找的entry的编码方式。
* Version类， Get流程，在当前版本的sstable文件中进行查找的流程。
（到sstable文件cache为止，leveldb采用了LRUcache策略）

2020-03-25:
* DBImpl类，Write接口的流程->
    DBImpl::MakeRoomForWrite->
        DBImpl::MaybeScheduleCompaction->
            DBImpl::CompactMemTable->
                DBImpl::WriteLevel0Table->
                    BuildTable->...
BuildTable函数中使用了TableBuilder类，该类描述了sstable文件的具体格式，内存中的memtable就是通过
该类的对象写入磁盘文件。
写入操作是一个漫长的流程，参考文章：https://www.cnblogs.com/cobbliu/articles/10680759.html

##### todo：
TableBuilder类的实现。
