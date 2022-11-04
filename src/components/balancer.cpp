//
// Created by mrv on 03.11.22.
//

#include "balancer.hpp"
#include <vector>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/clients/http/component.hpp>
#include "../models/Host.hpp"
#include "../utils/random.hpp"
#include "../utils/http/answers.hpp"
namespace {

  auto BuildUrl(const Host& host, const userver::server::http::HttpRequest& request){
      std::string url = host.hostname;
      url += request.GetUrl();
      return url;
  }
  class Balancer final: public userver::server::handlers::HttpHandlerBase{
     private:
      std::vector<Host> hosts_;
      userver::clients::http::Client& http_client_;
     public:
      static constexpr std::string_view kName = "balancer";
      Balancer(const userver::components::ComponentConfig& config,
               const userver::components::ComponentContext& context)
          :
            userver::server::handlers::HttpHandlerBase(config, context),
            http_client_(context.FindComponent<userver::components::HttpClient>().GetHttpClient())
      {
        LOG_DEBUG() << "CONSTRUCTOR";
        for (auto& host_yaml: config["hosts"]){
          Host host;
          host.hostname = host_yaml["hostname"].As<std::string>();
          host.timeout = host_yaml["timeout"].As<std::uint64_t>();
          host.retries = host_yaml["retries"].As<std::uint64_t>();
          LOG_DEBUG() << "hostname: " + host.hostname +
                             ", timeout: " + std::to_string(host.timeout) +
                             ", retries: " + std::to_string(host.retries);
          hosts_.push_back(host);
        }
      }

      auto GetServerResponse(
          const userver::server::http::HttpRequest& user_request,
          const Host& cur_host) const
      {
        auto server_request = http_client_.CreateRequest()->get(BuildUrl(cur_host, user_request))
                        ->retry(cur_host.retries)
                        ->timeout(cur_host.timeout)
                        ->async_perform();
        server_request.Wait();
        auto resp = server_request.Get();
        resp->raise_for_status();
        return resp;
      }

      size_t FindCircleNextHost(const std::vector<std::uint64_t>& attempts,
                                size_t old_index) const
      {
        size_t cur_index = (old_index + 1) % hosts_.size();
        for (; cur_index != old_index; cur_index = (cur_index + 1) % hosts_.size()){
          LOG_DEBUG() << "hosts_[" + std::to_string(cur_index) +"] "
                             "retries: " + std::to_string(hosts_[cur_index].retries)
                             + " attempts[" + std::to_string(cur_index) +"] = "
                             + std::to_string(attempts[cur_index]);
          if (hosts_[cur_index].retries > attempts[cur_index] ||
              hosts_[cur_index].retries == 0){
            break;
          }
        }
        return cur_index;
      }
      std::string RedirectAnswer(
          const userver::server::http::HttpRequest& user_request,
          decltype(http_client_.CreateRequest()->perform())server_request, uint16_t code) const
      {
        auto& resp = user_request.GetHttpResponse();
        resp.SetStatus(static_cast<userver::server::http::HttpStatus>(code));
        return server_request->body();
      }

      void GoNextHost(const std::vector<std::uint64_t>& attempts,
                      size_t& cur_index, bool& all_attempt_used) const {
        auto old_index = cur_index;
        cur_index = FindCircleNextHost(attempts, old_index);
        if (cur_index == old_index){
          all_attempt_used = true;
        }
      }

      std::string HandleRequestThrow(
          const userver::server::http::HttpRequest& user_request,
          userver::server::request::RequestContext& context) const override
      {
        std::vector<std::uint64_t> attempts(hosts_.size(), 0);
        auto cur_index = balancer::utils::my_random(0, hosts_.size() - 1);

        LOG_DEBUG() << "start_index: " + std::to_string(cur_index);
        bool all_attempt_used = false;
        
        while (!all_attempt_used){
          try{
            LOG_DEBUG() << "cur_index: " + std::to_string(cur_index);
            auto& cur_host = hosts_[cur_index];
            attempts[cur_index]++;
            auto serverResponse = GetServerResponse(user_request, cur_host);
            auto code = (std::uint16_t)serverResponse->status_code();
            if (code < 500){
              return RedirectAnswer(user_request, serverResponse, code);
            }
            else {
              LOG_DEBUG() << "5xx code";
              GoNextHost(attempts, cur_index, all_attempt_used);
            }
          }
          catch(std::exception& exception){
            LOG_DEBUG() << "what: " << exception.what();
            GoNextHost(attempts, cur_index, all_attempt_used);
          }
        }
        return balancer::utils::http::AnswerServersDead().HandOver(user_request);
      }
  };
}

void balancer::components::AppendBalancer(
    userver::components::ComponentList& componentList) {
  componentList.Append<Balancer>();
}
