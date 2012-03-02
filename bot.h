// -*- C++ -*-
// Copyright 2012, Evan Klitzke <evan@eklitzke.org>

#ifndef BOT_H_
#define BOT_H_

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/bind.hpp>
#include <boost/random.hpp>

#include <string>
#include <vector>

typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket> ssl_socket;
using boost::asio::ip::tcp;

namespace drakebot {

enum { MAX_LENGTH = 8096 };

enum SendState {
  SEND_PASS,
  SEND_NICK,
  SEND_USER,
  SEND_JOIN,
  SEND_QUOTATIONS
};

void InitializeRNG();

class IRCRobot {
 public:
  explicit IRCRobot(boost::asio::io_service &service,
                    boost::asio::ssl::context &context,
                    const std::string &quotations_file,
                    unsigned int interval,
                    const std::string &nick = "drakebot",
                    const std::string &password = "");
  ~IRCRobot();
  void Connect(boost::asio::ip::tcp::resolver::iterator);
  bool Authenticate(const std::string &, const std::string &,
                    const std::string &);
  void AddChannel(const std::string &);

 private:
  boost::asio::io_service &io_service_;
  boost::asio::deadline_timer timer_;
  ssl_socket socket_;

  const std::string quotations_file_;
  const std::string nick_;
  const std::string password_;
  const unsigned int interval_;
  SendState state_;

  boost::variate_generator<boost::mt19937&, boost::uniform_real<double> > rand_;

  bool waiting_;
  std::vector<std::string> channels_;
  char *reply_;
  boost::asio::streambuf request_;

  void HandleConnect(const boost::system::error_code&, tcp::resolver::iterator);
  void HandleHandshake(const boost::system::error_code&);
  void HandleWrite(const boost::system::error_code&, size_t);
  void HandleTimeout(const boost::system::error_code&);
  void HandleRead(const boost::system::error_code &, size_t);

  void SendLine(const std::string &);
  unsigned int PickWaitTime();
  bool SendMessage(const std::string &);
  bool SendQuotation();
  void JoinChannels();

  std::string GetQuotation();
};
}

#endif  // BOT_H_
