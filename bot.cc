// -*- C++ -*-
// Copyright 2012, Evan Klitzke <evan@eklitzke.org>

#include "./bot.h"

#include <glog/logging.h>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/bind.hpp>
#include <boost/random.hpp>

#include <cassert>
#include <ctime>
#include <fstream>
#include <sstream>

#include "./line_reader.h"

using boost::asio::ssl::context;
using boost::asio::ip::tcp;

typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket> ssl_socket;

namespace drakebot {

boost::mt19937 rng;
boost::uniform_real<double> uniform(0.0, 1.0);

void InitializeRNG() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  rng.seed(tv.tv_usec);
}

IRCRobot::IRCRobot(boost::asio::io_service &service,
                   boost::asio::ssl::context &context,
                   const std::string &quotations_file,
                   unsigned int interval,
                   const std::string &nick,
                   const std::string &password)
    :io_service_(service), timer_(service), quotations_file_(quotations_file),
     socket_(service, context), rand_(rng, uniform), nick_(nick),
     password_(password), state_(SEND_PASS), interval_(interval),
     line_reader_(socket_, boost::bind(&IRCRobot::LineCallback, this))
{
  reply_ = new char[MAX_LENGTH];
  request_ = new char[MAX_LENGTH];
  memset(static_cast<void *>(reply_), 0, sizeof(reply_));
  memset(static_cast<void *>(request_), 0, sizeof(request_));
}

IRCRobot::~IRCRobot() {
  delete request_;
  delete reply_;
}

void IRCRobot::Connect(boost::asio::ip::tcp::resolver::iterator
                       endpoint_iterator) {

  boost::asio::ip::tcp::endpoint endpoint = *endpoint_iterator;
  socket_.lowest_layer().async_connect(
      endpoint,
      boost::bind(&IRCRobot::HandleConnect, this,
                  boost::asio::placeholders::error, ++endpoint_iterator));
}

void IRCRobot::HandleConnect(const boost::system::error_code& error,
                             tcp::resolver::iterator
                             endpoint_iterator) {
  if (!error) {
    socket_.async_handshake(boost::asio::ssl::stream_base::client,
                            boost::bind(&IRCRobot::HandleHandshake, this,
                                        boost::asio::placeholders::error));
  } else if (endpoint_iterator != boost::asio::ip::tcp::resolver::iterator()) {
    socket_.lowest_layer().close();
    boost::asio::ip::tcp::endpoint endpoint = *endpoint_iterator;
    socket_.lowest_layer().async_connect(
        endpoint,
        boost::bind(&IRCRobot::HandleConnect, this,
                    boost::asio::placeholders::error, ++endpoint_iterator));
  } else {
    LOG(FATAL) << "Connect failed: " << error;
    exit(1);
  }
}

void IRCRobot::HandleHandshake(const boost::system::error_code& error) {
  if (!error) {
    if (password_.size()) {
      std::string s = "PASS ";
      s.append(password_);
      SendLine(s);
    } else {
      state_ = SEND_NICK;
      std::string s = "NICK ";
      s.append(nick_);
      SendLine(s);
    }
    EstablishReadCallback();
  } else {
    LOG(FATAL) << "Handshake failed: " << error;
  }
}

void IRCRobot::EstablishReadCallback() {
  boost::asio::async_read(socket_,
                          boost::asio::buffer(reply_, MAX_LENGTH),
                          boost::bind(&IRCRobot::ReadCompletedTest, this,
                                      boost::asio::placeholders::error,
                                      boost::asio::placeholders::bytes_transferred),
                          boost::bind(&IRCRobot::HandleRead, this,
                                      boost::asio::placeholders::error,
                                      boost::asio::placeholders::bytes_transferred));
}

void IRCRobot::SendLine(const std::string &msg) {
  assert(msg.size() < MAX_LENGTH);
  memcpy(request_, msg.c_str(), msg.size());
  request_[msg.size()] = '\n';
  boost::asio::async_write(socket_,
                           boost::asio::buffer(request_, msg.size() + 1),
                           boost::bind(&IRCRobot::HandleWrite, this,
                                       boost::asio::placeholders::error,
                                       boost::asio::placeholders::bytes_transferred));
}

bool IRCRobot::ReadCompletedTest(const boost::system::error_code& error,
                                 size_t bytes_transferred) {
  if (error) {
    LOG(FATAL) << "read error: " << error;
    return true;
  }
  if (bytes_transferred == MAX_LENGTH) {
    reply_[MAX_LENGTH - 1] = '\n';
    return true;
  } else if (bytes_transferred == 0) {
    return false;
  }
  return memrchr(reply_, '\n', bytes_transferred) != NULL;
}

void IRCRobot::HandleRead(const boost::system::error_code& error,
                          size_t bytes_transferred) {
  if (error) {
    LOG(FATAL) << "read error: " << error;
    exit(1);
  }

  bool keep_going = true;
  size_t pos = 0;
  std::string line;
  std::string reply(reply_, bytes_transferred);
  while (keep_going) {
    size_t next = reply.find('\n', pos);
    if (next == std::string::npos) {
      line = reply.substr(pos, next);
      keep_going = false;
    } else {
      line = reply.substr(pos, next - pos);
      pos = next + 1;
    }

    LineCallback(line);

  }
  EstablishReadCallback();
}

void IRCRobot::LineCallback(const std::string &line) {
  DLOG(INFO) << "Saw line " << line;
  if (line.substr(0, 5) == "PING ") {
    size_t colon_pos = line.find(":");
    assert(colon_pos != std::string::npos);
    std::string server_name = line.substr(colon_pos + 1, std::string::npos);
    std::string command = "PONG ";
    command.append(server_name);
    SendLine(command);
  }
}

void IRCRobot::HandleWrite(const boost::system::error_code& error,
                           size_t bytes_transferred) {
  if (error)  {
    LOG(FATAL) << "Write error: " << error;
  }

  std::string s;
  DLOG(INFO) << "Wrote " << bytes_transferred << " bytes";
  switch (state_) {
    case SEND_PASS:
      state_ = SEND_NICK;
      s = "NICK ";
      s.append(nick_);
      SendLine(s);
      break;
    case SEND_NICK:
      state_ = SEND_USER;
      SendLine("USER drakebot drakebot drakebot drakebot");
      break;
    case SEND_USER:
      state_ = SEND_JOIN;
      JoinChannels();
      break;
    case SEND_JOIN:
      state_ = SEND_QUOTATIONS;
      waiting_ = false;
      break;
  }

  if (!waiting_ && state_ == SEND_QUOTATIONS) {
    waiting_ = true;
    unsigned int wait_time = PickWaitTime();
    DLOG(INFO) << "Scheduling next quotation for " << wait_time << " seconds from now";
    timer_.expires_from_now(boost::posix_time::seconds(wait_time));
    timer_.async_wait(boost::bind(&IRCRobot::HandleTimeout, this, 
                                  boost::asio::placeholders::error));
  }
}

void IRCRobot::HandleTimeout(const boost::system::error_code &error) {
  DLOG(INFO) << "In handle timeout";
  SendQuotation();
  waiting_ = false;
}

void IRCRobot::AddChannel(const std::string &channel) {
  channels_.push_back(channel);
}

bool IRCRobot::SendMessage(const std::string &msg) {
  std::vector<std::string>::iterator cit;
  std::vector<std::string>::iterator mit;
  std::vector<std::string> messages;

  size_t pos = 0;
  while (true) {
    size_t next = msg.find('\n', pos);
    if (next == std::string::npos) {
      messages.push_back(msg.substr(pos, next));
      break;
    } else {
      messages.push_back(msg.substr(pos, next - pos));
      pos = next + 1;
    }
  }

  for (cit = channels_.begin(); cit != channels_.end(); ++cit) {
    for (mit = messages.begin(); mit != messages.end(); ++mit) {
      std::string s = "PRIVMSG ";
      s.append(*cit);
      s.append(" :\001ACTION croons: \"");
      s.append(*mit);
      s.append("\"\001");
      SendLine(s);
    }
  }
  return true;
}

bool IRCRobot::SendQuotation() {
  std::string quotation = GetQuotation();
  return SendMessage(quotation);
}

std::string IRCRobot::GetQuotation() {
  std::string chosen = "ERROR";
  std::ifstream quotations_file(quotations_file_.c_str());

  int i = 1;
  std::string q;
  std::string s;

  // Try to pick a random quotation from the file

  if (quotations_file.is_open()) {
    while (quotations_file.good()) {
      getline(quotations_file, s);
      if (s.size()) {
        if (q.size()) {
          q.push_back('\n');
          q.append(s);
        } else {
          q.append(s);
        }
      } else {
        if (i == 1) {
          chosen = q;
        } else if (rand_() < (1 / static_cast<double>(i))) {
          chosen = q;
        }
        q.clear();
        i++;
      }
    }
  }
  return chosen;
}

void IRCRobot::JoinChannels() {
  std::vector<std::string>::iterator it;
  for (it = channels_.begin(); it != channels_.end(); ++it) {
    std::string s = "JOIN ";
    s.append(*it);
    SendLine(s);
  }
}

unsigned int IRCRobot::PickWaitTime() {
  double davg = static_cast<double>(interval_);
  return static_cast<unsigned int>(davg * (0.5 + rand_()));
}
}

