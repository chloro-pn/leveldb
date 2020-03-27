// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.
//
// BlockBuilder generates blocks where keys are prefix-compressed:
//
// When we store a key, we drop the prefix shared with the previous
// string.  This helps reduce the space requirement significantly.
// Furthermore, once every K keys, we do not apply the prefix
// compression and store the entire key.  We call this a "restart
// point".  The tail end of the block stores the offsets of all of the
// restart points, and can be used to do a binary search when looking
// for a particular key.  Values are stored as-is (without compression)
// immediately following the corresponding key.
//
// An entry for a particular key-value pair has the form:
//     shared_bytes: varint32
//     unshared_bytes: varint32
//     value_length: varint32
//     key_delta: char[unshared_bytes]
//     value: char[value_length]
// shared_bytes == 0 for restart points.
//
// The trailer of the block has the form:
//     restarts: uint32[num_restarts]
//     num_restarts: uint32
// restarts[i] contains the offset within the block of the ith restart point.

#include "table/block_builder.h"

#include <assert.h>

#include <algorithm>

#include "leveldb/comparator.h"
#include "leveldb/options.h"
#include "util/coding.h"

namespace leveldb {

BlockBuilder::BlockBuilder(const Options* options)
    : options_(options), restarts_(), counter_(0), finished_(false) {
  assert(options->block_restart_interval >= 1);
  //第一个重启点就是0，这里可以看到每个重启点记录的是非共享键首地址的index。
  restarts_.push_back(0);  // First restart point is at offset 0
}

//重用当前block，恢复构造时候的状态即可。
void BlockBuilder::Reset() {
  buffer_.clear();
  restarts_.clear();
  restarts_.push_back(0);  // First restart point is at offset 0
  counter_ = 0;
  finished_ = false;
  last_key_.clear();
}

//估计当前block占用的地址，等于buffer_的size + 重启点占用的大小 + 重启点个数的大小。
size_t BlockBuilder::CurrentSizeEstimate() const {
  return (buffer_.size() +                       // Raw data buffer
          restarts_.size() * sizeof(uint32_t) +  // Restart array
          sizeof(uint32_t));                     // Restart array length
}

// Finish函数将重启点数组写入block，最后将重启点个数写入block，
// 这里的编码都是固定编码。可以想象，解析每一个block的时候，
// 首先找到block的起始和终止地址，然后从终止地址向前读入4字节，得到重启点个数n。
// 接着向前读n个重启点数据，然后从block起始地值解析kv对，根据重启点判断key的真实内容即可。
// 其实根据后面的分析，每个block还可能经过压缩，后面跟着1字节压缩类型和4字节crc用于检测错误。
// 那么首先用crc验证，然后根据压缩类型解压，最后再回到这里的处理方法。
Slice BlockBuilder::Finish() {
  // Append restart array
  for (size_t i = 0; i < restarts_.size(); i++) {
    PutFixed32(&buffer_, restarts_[i]);
  }
  PutFixed32(&buffer_, restarts_.size());
  finished_ = true;
  return Slice(buffer_);
}

void BlockBuilder::Add(const Slice& key, const Slice& value) {
  // 块内的add操作，首先没有调用finish。
  Slice last_key_piece(last_key_);
  assert(!finished_);
  // 每两个重启点之间的kv数量有限制
  assert(counter_ <= options_->block_restart_interval);
  // 按序插入，新插入的值总是最大的
  assert(buffer_.empty()  // No values yet?
         || options_->comparator->Compare(key, last_key_piece) > 0);
  size_t shared = 0;
  if (counter_ < options_->block_restart_interval) {
    // See how much sharing to do with previous string
    const size_t min_length = std::min(last_key_piece.size(), key.size());
    while ((shared < min_length) && (last_key_piece[shared] == key[shared])) {
      shared++;
    }
  } else {
    // Restart compression
    // 如果当前重启点后的kv数量达到上限，则记录新的重启点，并将counter_置为0。
    restarts_.push_back(buffer_.size());
    counter_ = 0;
  }
  //计算出没有共享的部分。
  const size_t non_shared = key.size() - shared;

  // Add "<shared><non_shared><value_size>" to buffer_
  //按照上述格式记录键共享长度，键非共享长度，值的长度。
  PutVarint32(&buffer_, shared);
  PutVarint32(&buffer_, non_shared);
  PutVarint32(&buffer_, value.size());

  // Add string delta to buffer_ followed by value
  // 将键的非共享部分写入buffer_，然后将值写入buffer_。
  buffer_.append(key.data() + shared, non_shared);
  buffer_.append(value.data(), value.size());

  // Update state
  // 将本次插入的键作为新的last_key_。
  last_key_.resize(shared);
  last_key_.append(key.data() + shared, non_shared);
  assert(Slice(last_key_) == key);
  // counter_加一。
  counter_++;
}

}  // namespace leveldb
