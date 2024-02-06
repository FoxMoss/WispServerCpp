#include <cstddef>
#include <cstdint>
#include <string>

#define SEND_CALLBACK_TYPE                                                     \
  void (*sendCallback)(void *, size_t, void *id, bool exit)

void message_interface(SEND_CALLBACK_TYPE, std::string msg, void *id);
void open_interface(SEND_CALLBACK_TYPE, void *id);
