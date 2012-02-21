// -*- C++ -*-
// Copyright 2012, Evan Klitzke <evan@eklitzke.org>

#ifndef BOT_H_
#define BOT_H_

#include <string>
#include <vector>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/bind.hpp>
#include <boost/random.hpp>

typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket> ssl_socket;

namespace drakebot {
void InitializeRNG();

class IRCRobot {
 public:
  explicit IRCRobot(boost::asio::io_service &service,
                    const std::string &quotations_file);
  ~IRCRobot();
  bool Connect(const std::string &, int, bool);
  bool Authenticate(const std::string &, const std::string &,
                    const std::string &);
  bool JoinChannel(const std::string &);
  bool JoinChannel(const std::string &, const std::string &);
  bool SendMessage(const std::string &);
  bool SendQuotation();
  unsigned int PickWaitTime(unsigned int avg);

 private:
  const std::string &quotations_file_;
  boost::asio::io_service &io_service_;
  ssl_socket *sock_;
  std::vector<std::string> channels_;

  boost::variate_generator<boost::mt19937&, boost::uniform_real<double> > rand_;

  void RunCommand(const std::string &);
  std::string GetQuotation();
};
}

#endif  // BOT_H_
