/*
    Project: Wolf Engine. Copyright © 2014-2023 Pooya Eimandar
    https://github.com/WolfSource/wolf
*/

#if defined(WOLF_STREAM_GRPC) && defined(WOLF_SYSTEM_COROUTINE)

#pragma once

#include <grpcpp/channel.h>
#include <grpcpp/create_channel.h>

#include <agrpc/asio_grpc.hpp>
#include <boost/asio.hpp>
#include <wolf/wolf.hpp>

namespace wolf::stream::rpc {

class w_grpc_client {
 public:
  // default constructor
  W_API w_grpc_client() noexcept = default;
  // default destructor
  W_API ~w_grpc_client() noexcept = default;
  // default move constructor
  W_API w_grpc_client(w_grpc_client&& p_src) noexcept = default;
  // default move assignment operator
  W_API auto operator=(w_grpc_client&& p_src) noexcept
      -> w_grpc_client& = default;
  // default copy constructor(deleted)
  w_grpc_client(const w_grpc_client&) noexcept = delete;
  // default copy assignment operator (deleted)
  auto operator=(const w_grpc_client&) noexcept -> w_grpc_client& = delete;

  /*
   * @brief: initialize grpc client
   * @param: p_io: boost io context
   * @param: p_endpoint: grpc server endpoint
   * @param: p_ssl: use ssl or not
   * @param: p_max_msg_size: max message size
   * @return: 0 on success
   */
  auto init(_In_ boost::asio::io_context& p_io,
            _In_ std::string_view p_endpoint, _In_ bool p_ssl = false,
            _In_ int p_max_msg_size = 0) noexcept -> boost::leaf::result<int> {
    try {
      if (p_endpoint.empty()) {
        return W_FAILURE(std::errc::invalid_argument,
                         "missing grpc server endpoint");
      }

      grpc::ChannelArguments channel_args{};
      if (p_max_msg_size > 0) {
        channel_args.SetInt(GRPC_ARG_MAX_RECEIVE_MESSAGE_LENGTH,
                            p_max_msg_size);
        channel_args.SetInt(GRPC_ARG_MAX_SEND_MESSAGE_LENGTH, p_max_msg_size);
      }

      std::shared_ptr<grpc::ChannelCredentials> credentials;
      if (p_ssl) {
        // TODO(POOYA): fix the secure credentials
        //_credentials =
        // std::make_shared<grpc::ChannelCredentials>(grpc::SslCredentialsOptions());
      } else {
        credentials = grpc::InsecureChannelCredentials();
      }

      this->_channel = grpc::CreateCustomChannel(p_endpoint.data(), credentials,
                                                 channel_args);
      if (this->_channel == nullptr) {
        return W_FAILURE(std::errc::operation_canceled,
                         "could not create channel to connect to grpc serve");
      }

      boost::asio::post(p_io, [&] {
        p_io.get_executor().on_work_finished();
        agrpc::run(this->_context, p_io);
        p_io.get_executor().on_work_started();
      });

      return 0;

    } catch (const std::exception& exc) {
      return W_FAILURE(
          std::errc::operation_canceled,
          wolf::format(
              "could not initialize grpc, an exception just happened: {}",
              exc.what()));
    }
  }

  /*
   * @brief: send unary request via grpc
   * @param: p_request: the grpc request
   * @param: p_timeout: the grpc timeout
   * @param: p_meta_data: the meta data of request
   * @return: a tuple of grpc status and grpc response
   */
  template <auto PrepareAsyncFun, typename Stub, typename Request,
            typename Response>
  auto send_unary(
      _In_ Request&& p_request,
      _In_ std::chrono::system_clock::duration p_timeout,
      _In_ std::unordered_map<std::string, std::string> p_meta_data) noexcept
      -> boost::asio::awaitable<std::tuple<grpc::Status, Response>> {
    grpc::ClientContext client_context;

    for (const auto& pair : p_meta_data) {
      if (!pair.first.empty()) {
        client_context.AddMetadata(pair.first, pair.second);
      }
    }

    const auto deadline = std::chrono::system_clock::now() + p_timeout;
    client_context.set_deadline(deadline);

    Stub stub{this->_channel};
    Response respose{};

    using RPC = agrpc::ClientRPC<PrepareAsyncFun>;
    auto status =
        co_await RPC::request(this->_context, stub, client_context, p_request,
                              respose, boost::asio::use_awaitable);

    co_return std::make_tuple(status, respose);
  }

 private:
  agrpc::GrpcContext _context{};
  std::shared_ptr<grpc::Channel> _channel = nullptr;
};
}  // namespace wolf::stream::rpc

#endif  // WOLF_STREAM_GRPC && WOLF_SYSTEM_COROUTINE
