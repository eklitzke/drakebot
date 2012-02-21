// -*- C++ -*-
// Copyright 2012, Evan Klitzke <evan@eklitzke.org>

#ifndef LINE_READER_H_
#define LINE_READER_H_

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/function.hpp>

#include <glog/logging.h>

#include <string>

typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket> ssl_socket;
typedef boost::function<void (const std::string &)> callback;
using boost::asio::ip::tcp;

namespace drakebot {

enum { READ_BUF_SIZE = 8096 };

template<typename AsyncReadStream>
class LineReader {
 public:
  LineReader(AsyncReadStream *stream, callback cb);
  ~LineReader();
  void Start();
  void Stop();
 private:
  AsyncReadStream *stream_;
  callback callback_;
  bool keep_going_;
  std::string extra_;

  char *read_buf_;

  bool ReadCompletedTest(const boost::system::error_code&, size_t);
  bool HandleRead(const boost::system::error_code&, size_t);

  void EstablishReadHandler();
};

template<typename AsyncReadStream>
LineReader<AsyncReadStream>::LineReader(AsyncReadStream *stream, callback cb)
    :stream_(stream), callback_(cb), keep_going_(false) {
  read_buf_ = new char[READ_BUF_SIZE];
}

template<typename AsyncReadStream>
LineReader<AsyncReadStream>::~LineReader() {
  delete read_buf_;
}

template<typename AsyncReadStream>
void LineReader<AsyncReadStream>::Start() {
  keep_going_ = true;
  EstablishReadHandler();
}

template<typename AsyncReadStream>
void LineReader<AsyncReadStream>::EstablishReadHandler() {
  boost::asio::async_read(*stream_,
                          boost::asio::buffer(read_buf_, READ_BUF_SIZE),
                          boost::bind(
                              &LineReader<AsyncReadStream>::ReadCompletedTest,
                              this, boost::asio::placeholders::error,
                              boost::asio::placeholders::bytes_transferred),
                          boost::bind(
                              &LineReader<AsyncReadStream>::HandleRead,
                              this, boost::asio::placeholders::error,
                              boost::asio::placeholders::bytes_transferred));
}

template<typename AsyncReadStream>
bool LineReader<AsyncReadStream>::ReadCompletedTest(
    const boost::system::error_code &error, size_t bytes_transferred) {

  if (error) {
    LOG(FATAL) << "read error: " << error;
    return true;
  }
  if (bytes_transferred == 0) {
    return false;
  }

  void *offset = memrchr(read_buf_, '\n', bytes_transferred);
  if (offset == NULL) {
    if (bytes_transferred == READ_BUF_SIZE) {
      LOG(FATAL) << "failed to buffer enough bytes";
      return true;
    }
    return false;
  }

  return true;
}

template<typename AsyncReadStream>
bool LineReader<AsyncReadStream>::HandleRead(
    const boost::system::error_code &error, size_t bytes_transferred) {
  if (error) {
    LOG(FATAL) << "read error: " << error;
  }

  std::string data = extra_;
  data.append(read_buf_, bytes_transferred);
  size_t offset = 0;
  while (true) {
    size_t next = data.find('\n', offset);
    if (next == std::string::npos) {
      extra_ = data.substr(offset, data.size() - offset);
      break;
    }
    std::string line = data.substr(offset, next - offset);
    callback_(line);
    offset = next + 1;
    if (next >= data.size()) {
      extra_.clear();
      break;
    }
  }

  if (keep_going_)
    EstablishReadHandler();
}
}

#endif  // LINE_READER_H_