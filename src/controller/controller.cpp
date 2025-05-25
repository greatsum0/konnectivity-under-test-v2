// controller.cpp
#include "controller.h"
#include <logger.h>
#include "tcp_client.h"
#include "tcp_server.h"

Controller::Controller(boost::asio::io_context &io_context,
                       ReceiveHandler handler)
    : io_context_(io_context),
      receive_handler_(std::move(handler)),
      mode_(Mode::Client),
      state_(State::Disconnected) {
}

void Controller::startServer(unsigned short port) {
    mode_ = Mode::Server;
    server_ = std::make_shared<TcpServer>(io_context_, port,
                                          [this](const std::string &msg) {
                                              onReceive(msg);
                                          });
    updateState(State::Disconnected);
    server_->start();
    updateState(State::Connected);
}

void Controller::startClient(const std::string &host, unsigned short port) {
    mode_ = Mode::Client;
    client_ = std::make_shared<TcpClient>(io_context_, host, port,
                                          [this](const std::string &msg) {
                                              onReceive(msg);
                                          });
    updateState(State::Connecting);
    client_->start();
    updateState(State::Connected);
}

void Controller::send(const std::string &message) const {
    if (state_ != State::Connected) return;
    if (mode_ == Mode::Server && server_) {
        Logger::instance().debug("Controller::send: " + message);
        server_->syncSend(message);
    } else if (mode_ == Mode::Client && client_) {
        Logger::instance().debug("Controller::send: " + message);
        client_->syncSend(message);
    }
}


void Controller::stop() {
    if (mode_ == Mode::Server && server_) {
        server_->stop();
    } else if (mode_ == Mode::Client && client_) {
        client_->stop();
    }
    updateState(State::Disconnected);
}

Controller::State Controller::state() const {
    return state_;
}

void Controller::onReceive(const std::string &message) const {

    receive_handler_(message);
}

void Controller::updateState(const State new_state) {
    state_ = new_state;
}
