#include <cstddef>
#include <cstdint>
#include <string>

#define SEND_CALLBACK_TYPE                                                     \
  void (*sendCallback)(void *, size_t, uint32_t id, bool exit)

void message_interface(SEND_CALLBACK_TYPE, std::string msg, uint32_t id);
void open_interface(SEND_CALLBACK_TYPE, uint32_t id);
