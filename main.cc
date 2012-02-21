// -*- C++ -*-
// Copyright 2012, Evan Klitzke <evan@eklitzke.org>

#include <syslog.h>

#include <boost/program_options.hpp>
#include <boost/asio/ssl.hpp>

#include <glog/logging.h>

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "./bot.h"

namespace po = boost::program_options;

using boost::asio::ip::resolver_query_base;
using boost::asio::ssl::context;

int main(int argc, char **argv) {
  google::InitGoogleLogging(argv[0]);

  int port;
  unsigned int interval;
  bool use_ssl = false;
  std::string host;
  std::string nick;
  std::string password;
  std::string quotations_file;
  std::vector<std::string> channels;
  po::options_description desc("Allowed options");
  desc.add_options()
      ("help,h", "Produce this help message.")
      ("daemon,d", "Run as a daemon")
      ("host,H", po::value<std::string>(&host)->
       default_value("irc.freenode.net"), "The host to connect to.")
      ("port,p", po::value<int>(&port)->default_value(6667),
       "The port to connect on.")
      ("password", po::value<std::string>(&password)->default_value(""),
       "The password to use (optional)")
      ("nick,n", po::value<std::string>(&nick)->default_value("drakebot"),
       "The nick to use.")
      ("channels,c", po::value<std::vector<std::string> >(), "Channels to join")
      ("ssl,s", "Use SSL when connecting.")
      ("quotations,q", po::value<std::string>(&quotations_file)->
       default_value("quotations.txt"), "The path to the quotations file")
      ("interval,i", po::value<unsigned int>(&interval)->default_value(28800),
       "The average amount of time to wait between sending quotations");

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);

  if (vm.count("help")) {
    std::cout << desc << std::endl;
    return 1;
  }
  if (!vm.count("channels")) {
    std::cout << "Must specify one or more channels with -c/--channels"
              << std::endl;
    return 1;
  }
  if (!vm.count("ssl")) {
    // FIXME
    std::cout << "Sorry, right now only SSL mode is supported!" << std::endl;
    return 1;
  }
  if (vm.count("ssl")) {
    use_ssl = true;
  }

  boost::asio::io_service io_service;

  if (vm.count("daemon")) {
    if (pid_t pid = fork()) {
      if (pid > 0) {
        // We're in the parent process and need to exit.
        exit(0);
      } else {
        syslog(LOG_ERR | LOG_USER, "First fork failed: %m");
        return 1;
      }
    }
    setsid();
    chdir("/");
    umask(0);

    // A second fork ensures the process cannot acquire a controlling terminal.
    if (pid_t pid = fork()) {
      if (pid > 0) {
        exit(0);
      } else {
        syslog(LOG_ERR | LOG_USER, "Second fork failed: %m");
        return 1;
      }
    }

    // Close the standard streams
    close(0);
    close(1);
    close(2);

    // Inform the io_service that we have finished becoming a daemon.
    io_service.notify_fork(boost::asio::io_service::fork_child);
  }

  channels = vm["channels"].as<std::vector<std::string> >();

  drakebot::InitializeRNG();

  // initiate the name resolution
  std::stringstream numeric_port;
  numeric_port << port;
  boost::asio::ip::tcp::resolver resolver(io_service);
  boost::asio::ip::tcp::resolver::query query(host, numeric_port.str(), resolver_query_base::numeric_service);
  boost::asio::ip::tcp::resolver::iterator iterator = resolver.resolve(query);

  // set the SSL context
  context ctx(boost::asio::ssl::context::sslv23);
  ctx.set_default_verify_paths();

  drakebot::IRCRobot robot(io_service, ctx, quotations_file, interval, nick,
                           password);

  std::vector<std::string>::iterator it;
  for (it = channels.begin(); it != channels.end(); ++it) {
    std::string channel = *it;
    if (!channel.size())
      continue;
    if (channel[0] != '#')
      channel.insert(0, "#");
    robot.AddChannel(channel);
  }

  robot.Connect(iterator);
  io_service.run();

  return 0;
}