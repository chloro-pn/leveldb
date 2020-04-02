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
该类的对象写入磁盘文件。write操作是一个漫长的流程，参考文章：https://www.cnblogs.com/cobbliu/articles/10680759.html

2020-03-27:
* BlockBuilder类的分析。(metaindex block 和 footer block除外)

2020-03-28:
* meta_index_block , filter block , footer block分析。

2020-03-30:
* TableCache::Get， TableCache::FindTable，Table::InternalGet

2020-03-31:
* TableCache, SharededLRUCache, LRUCache, LRUHandle类
LRUCache对象中含有两个链表和一个hash表，hash表用来根据key快速找到对应的value。
两个链表分别为lru_双向链表和in_use双向链表。执行LRU缓存算法。in_use链表用来记录
那些被客户使用的缓存项，ref >= 2 （因为LRUCache本身持有其一个ref），当所有用户都
release该缓存项时，ref == 1，然后将其放回给lru_链表。

ps：个人觉得这种将结构逻辑和内容放在一起的做法并不会让代码变得更清晰。
url:https://blog.csdn.net/u011693064/article/details/77485631

ps：尝试实现lru策略，发现智能指针的引用计数确实不能解决上述问题，因为LRU缓存策略的关键
在于缓存和持有资源的用户共同决定资源的生命周期。智能指针对于这个问题难以解决，当缓存本身
持有资源的智能指针时，客户持有的资源无法释放。当缓存不持有资源本身的智能指针时，如何为新
的客户分配当前被其他客户共享生命周期的资源？所以侵入式的引用技术方法无法被智能指针完全代替：）
实现见：https://github.com/chloro-pn/MagicPocket

##### todo：
* major compaction
* memtable及其内存管理
* version机制与recover机制
