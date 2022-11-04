#pragma once
#include <userver/server/handlers/http_handler_base.hpp>

namespace balancer::utils::http{
  struct http_answer{
    std::string body;
    userver::server::http::HttpStatus httpStatus = userver::server::http::HttpStatus::kOk;
    inline std::string HandOver(const userver::server::http::HttpRequest& request){
      request.GetHttpResponse().SetStatus(httpStatus);
      std::string temp = std::move(body);
      return temp;
    }
    inline bool operator==(const http_answer& right) const{
      return httpStatus == right.httpStatus && body == right.body;
    }
    inline bool operator!=(const http_answer& right) const{
      return !(*this == right);
    }
  };
  http_answer AnswerServersDead(){
    http_answer httpAnswer;
    httpAnswer.httpStatus = userver::server::http::HttpStatus::kInternalServerError;
    userver::formats::json::ValueBuilder json;
    json["detail"] = "All servers spent all attempts and exceeded timeouts";
    httpAnswer.body = ToString(json.ExtractValue());
    return httpAnswer;
  }
}
