// -*- C++ -*-
// Copyright 2012, Evan Klitzke <evan@eklitzke.org>

#include "./bot.h"

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/bind.hpp>
#include <boost/random.hpp>

#include <cassert>
#include <ctime>
#include <fstream>
#include <sstream>

using boost::asio::ssl::context;
using boost::asio::ip::tcp;
using boost::asio::ip::resolver_query_base;

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
                   const std::string &quotations_file)
    :io_service_(service), quotations_file_(quotations_file), sock_(0),
     rand_(rng, uniform) {
}

IRCRobot::~IRCRobot() {
  if (sock_)
    delete sock_;
}

bool IRCRobot::Connect(const std::string &host, int port, bool use_ssl) {
  assert(use_ssl);

  std::stringstream numeric_port;
  numeric_port << port;

  // Create a context that uses the default paths for
  // finding CA certificates.
  context ctx(boost::asio::ssl::context::sslv23);
  ctx.set_default_verify_paths();

  // Open a socket and connect it to the remote host.
  sock_ = new ssl_socket(io_service_, ctx);
  tcp::resolver resolver(io_service_);
  tcp::resolver::query query(host, numeric_port.str(),
                             resolver_query_base::numeric_service);

  // Connect to the remote host
  boost::asio::connect(sock_->lowest_layer(), resolver.resolve(query));
  sock_->lowest_layer().set_option(tcp::no_delay(true));
  sock_->handshake(boost::asio::ssl::stream_base::client);

  return true;
}

bool IRCRobot::Authenticate(const std::string &nick, const std::string &pass,
                            const std::string &user) {
  std::string comm;

  if (pass.size()) {
    comm = "PASS ";
    comm.append(pass);
    RunCommand(comm);
  }
  comm = "NICK ";
  comm.append(nick);
  RunCommand(comm);

  comm = "USER drakebot drakebot drakebot drakebot";
  RunCommand(comm);
  return true;
}

bool IRCRobot::JoinChannel(const std::string &channel, const std::string &key) {
  std::string s = "JOIN ";
  s.append(channel);
  s.push_back(' ');
  s.append(key);
  RunCommand(s);
  channels_.push_back(channel);
  return true;
}

bool IRCRobot::JoinChannel(const std::string &channel) {
  std::string s = "JOIN ";
  s.append(channel);
  RunCommand(s);
  channels_.push_back(channel);
  return true;
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
      s.append(" :");
      s.append(*mit);
      RunCommand(s);
    }
  }
  return true;
}

bool IRCRobot::SendQuotation() {
  std::string quotation = GetQuotation();
  return SendMessage(quotation);
}

void IRCRobot::RunCommand(const std::string &command) {
  std::string c = command;
  c.push_back('\n');
  // boost::asio::mutable_buffer b(command.c_str(), command.size() + 1);
  // b[command.size()] = '\n';

  boost::asio::write(*sock_, boost::asio::buffer(c, c.size()));
}

std::string IRCRobot::GetQuotation() {
  std::string chosen = "ERROR";
  std::ifstream quotations_file(quotations_file_.c_str());

  int i = 1;
  std::string q;
  std::string s;
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

unsigned int IRCRobot::PickWaitTime(unsigned int avg) {
  double davg = static_cast<double>(avg);
  return static_cast<unsigned int>(davg * (0.5 + rand_()));
}

/*
double NextAnnouncement(double mu, double sigma) {
  return 1.0;
}

std::string GetQuotation(const std::string &quotations_file) {
  return "test";
}
*/
}

