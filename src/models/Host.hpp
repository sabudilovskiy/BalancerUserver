//
// Created by mrv on 03.11.22.
//
#pragma once
#include <userver/formats/yaml.hpp>

struct Host{
  std::string hostname;
  std::uint64_t retries;
  std::uint64_t timeout;
  Host Parse(const userver::formats::yaml::Value& yaml, userver::formats::parse::To<Host>);
};