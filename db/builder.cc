// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "db/builder.h"

#include "db/dbformat.h"
#include "db/filename.h"
#include "db/table_cache.h"
#include "db/version_edit.h"
#include "leveldb/db.h"
#include "leveldb/env.h"
#include "leveldb/iterator.h"

namespace leveldb {

//将iter迭代器中的数据写入磁盘，成为一个level0的sstable文件。
Status BuildTable(const std::string& dbname, Env* env, const Options& options,
                  TableCache* table_cache, Iterator* iter, FileMetaData* meta) {
  Status s;
  meta->file_size = 0;
  //迭代器跳到第一个元素
  iter->SeekToFirst();

  //meta之前向versions_申请了filenumber。
  std::string fname = TableFileName(dbname, meta->number);
  if (iter->Valid()) {
    WritableFile* file;
    s = env->NewWritableFile(fname, &file);
    if (!s.ok()) {
      return s;
    }

    //TableBuilder类是一个辅助类，辅助将key-value按照sstable文件格式写入文件file。
    TableBuilder* builder = new TableBuilder(options, file);
    //meta记录了对应的磁盘文件中最小的key。
    meta->smallest.DecodeFrom(iter->key());
    for (; iter->Valid(); iter->Next()) {
      Slice key = iter->key();
      //更新最大key。
      meta->largest.DecodeFrom(key);
      //向builder写入key-value。
      builder->Add(key, iter->value());
    }

    // Finish and check for builder errors
    //Finish函数中会向文件写入一些特殊的block。
    s = builder->Finish();
    if (s.ok()) {
      //meta更新文件的大小。
      meta->file_size = builder->FileSize();
      assert(meta->file_size > 0);
    }

    //辅助类的任务已经完成。
    delete builder;

    // Finish and check for file errors
    if (s.ok()) {
      //文件刷盘。
      s = file->Sync();
    }
    if (s.ok()) {
      s = file->Close();
    }
    delete file;
    file = nullptr;

    if (s.ok()) {
      // Verify that the table is usable
        //确认刚才写入的sstable可用。
      Iterator* it = table_cache->NewIterator(ReadOptions(), meta->number,
                                              meta->file_size);
      s = it->status();
      delete it;
    }
  }

  // Check for input iterator errors
  if (!iter->status().ok()) {
    s = iter->status();
  }

  //如果file_size为0，则本磁盘文件不需要存在，删除即可。
  if (s.ok() && meta->file_size > 0) {
    // Keep it
  } else {
    env->RemoveFile(fname);
  }
  return s;
}

}  // namespace leveldb
