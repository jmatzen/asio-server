#ifndef included__jm_net_STATUS_CODE_response_hpp
#define included__jm_net_STATUS_CODE_response_hpp

#include "headers.hpp"
#include <builder.hpp>

namespace jm::net::http {
enum class StatusCode {
   // 1xx: Informational
   Continue = 100,
   SwitchingProtocols = 101,
   Processing = 102,
   EarlyHints = 103,

   // 2xx: Success
   Ok = 200,
   Created = 201,
   Accepted = 202,
   NonAuthoritativeInformation = 203,
   NoContent = 204,
   ResetContent = 205,
   PartialContent = 206,
   MultiStatus = 207,
   AlreadyReported = 208,
   ImUsed = 226,

   // 3xx: Redirection
   MultipleChoices = 300,
   MovedPermanently = 301,
   Found = 302,
   SeeOther = 303,
   NotModified = 304,
   UseProxy = 305,
   SwitchProxy = 306, // No longer used
   TemporaryRedirect = 307,
   PermanentRedirect = 308,

   // 4xx: Client Error
   BadRequest = 400,
   Unauthorized = 401,
   PaymentRequired = 402,
   Forbidden = 403,
   NotFound = 404,
   MethodNotAllowed = 405,
   NotAcceptable = 406,
   ProxyAuthenticationRequired = 407,
   RequestTimeout = 408,
   Conflict = 409,
   Gone = 410,
   LengthRequired = 411,
   PreconditionFailed = 412,
   PayloadTooLarge = 413,
   UriTooLong = 414,
   UnsupportedMediaType = 415,
   RangeNotSatisfiable = 416,
   ExpectationFailed = 417,
   ImATeapot = 418,
   MisdirectedRequest = 421,
   UnprocessableEntity = 422,
   Locked = 423,
   FailedDependency = 424,
   TooEarly = 425,
   UpgradeRequired = 426,
   PreconditionRequired = 428,
   TooManyRequests = 429,
   RequestHeaderFieldsTooLarge = 431,
   UnavailableForLegalReasons = 451,

   // 5xx: Server Error
   InternalServerError = 500,
   NotImplemented = 501,
   BadGateway = 502,
   ServiceUnavailable = 503,
   GatewayTimeout = 504,
   VersionNotSupported = 505,
   VariantAlsoNegotiates = 506,
   InsufficientStorage = 507,
   LoopDetected = 508,
   NotExtended = 510,
   NetworkAuthenticationRequired = 511
};
class ResponseBuilder;

class Response {
 public:
   static ResponseBuilder builder();

   const HeaderMap &getHeaders() const { return headers_; }

   StatusCode getStatusCode() const { return statusCode_; }

 private:
   friend class ResponseBuilder;
   HeaderMap headers_;
   StatusCode statusCode_ = StatusCode::InternalServerError;
};

BUILDER_BEGIN(Response)
BUILDER_METHOD(statusCode, StatusCode);
BUILDER_METHOD(headers, HeaderMap);
BUILDER_END(Response);

} // namespace jm::net::http

#endif // included__jm_net_STATUS_CODE_response_hpp
