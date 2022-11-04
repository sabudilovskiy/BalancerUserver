//
// Created by mrv on 03.11.22.
//

#include "Host.hpp"
Host Host::Parse(const userver::formats::yaml::Value& yaml,
                 userver::formats::parse::To<Host>) {
  Host host;
  host.hostname = yaml["hostname"].As<std::string>();
  host.retries = yaml["retries"].As<std::uint64_t>();
  host.timeout = yaml["timeout"].As<std::uint64_t>();
  return host;
}
