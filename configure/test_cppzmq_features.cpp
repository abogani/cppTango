#include <zmq.hpp>

int main(int, char**)
{
  zmq::context_t c(1);
  zmq::socket_t s(c, ZMQ_REQ);
  void* plain_socket __attribute__((unused)) = s.handle();

  s.disconnect("some endpoint");

  zmq::message_t m, n;
  zmq::send_result_t sr __attribute__((unused)) = s.send(m, zmq::send_flags::none);
  zmq::recv_result_t rr __attribute__((unused)) = s.recv(m, zmq::recv_flags::none);

  m.copy(n);
  m.move(n);

  s.set(zmq::sockopt::linger, 0);
  int val __attribute__((unused)) = s.get(zmq::sockopt::linger);
  return 0;
}
