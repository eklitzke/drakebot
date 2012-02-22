// -*- C++ -*-
// Copyright 2012, Evan Klitzke <evan@eklitzke.org>

#include <stdio.h>
#include <syslog.h>

#include <boost/asio/ssl.hpp>

#include <gflags/gflags.h>
#include <glog/logging.h>

#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>

#include "./bot.h"
#include "./line_reader.h"

using boost::asio::ip::resolver_query_base;
using boost::asio::ssl::context;

DEFINE_bool(daemon, false, "Run as a daemon");
DEFINE_string(host, "irc.freenode.net", "Host to connect to");
DEFINE_int32(port, 6697, "Port to connect to");
DEFINE_string(password, "", "Password to use");
DEFINE_string(nick, "drakebot", "Nick to use");
DEFINE_string(channel, "", "Channel to connect to");
DEFINE_bool(ssl, true, "Use SSL");
DEFINE_string(quotations, "quotations.txt", "Quotations file to use");
DEFINE_int32(interval, 43200, "Mean interval to use");

int main(int argc, char **argv) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);

  if (!FLAGS_channel.size()) {
    printf("Must specify a channel with --channel\n");
    return 1;
  }
  if (!FLAGS_ssl) {
    printf("Sorry, right now only --ssl is supported\n");
    return 1;
  }

  char *real_path_name = realpath(FLAGS_quotations.c_str(), NULL);
  if (real_path_name == NULL) {
	printf("Failed to resolve path to quotations file, \"%s\"\n",
		   FLAGS_quotations.c_str());
	return 1;
  }
  std::string quotations_path(real_path_name, strlen(real_path_name));
  free(real_path_name);

  boost::asio::io_service io_service;

  if (FLAGS_daemon) {
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

  drakebot::InitializeRNG();

  // initiate the name resolution
  std::stringstream numeric_port;
  numeric_port << FLAGS_port;
  boost::asio::ip::tcp::resolver resolver(io_service);
  boost::asio::ip::tcp::resolver::query query(
      FLAGS_host, numeric_port.str(), resolver_query_base::numeric_service);
  boost::asio::ip::tcp::resolver::iterator iterator = resolver.resolve(query);

  // set the SSL context
  context ctx(boost::asio::ssl::context::sslv23);
  ctx.set_default_verify_paths();

  drakebot::IRCRobot robot(io_service, ctx, quotations_path, FLAGS_interval,
						   FLAGS_nick, FLAGS_password);

  std::vector<std::string>::iterator it;
  std::string channel_name = FLAGS_channel;
  if (channel_name[0] != '#') {
    channel_name.insert(0, "#");
  }
  robot.AddChannel(channel_name);

  robot.Connect(iterator);
  io_service.run();

  return 0;
}
