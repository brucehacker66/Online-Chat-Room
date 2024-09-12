#ifndef MESSAGE_H
#define MESSAGE_H

#include <cstring>
#include <vector>
#include <string>
using std::string;

struct Message {
  // An encoded message may have at most this many characters,
  // including the trailing newline ('\n'). Note that this does
  // *not* include a NUL terminator (if one is needed to
  // temporarily store the encoded message.)
  static const unsigned MAX_LEN = 255;

  std::string tag;
  std::string data;

  Message() { }

  Message(const std::string &tag, const std::string &data)
    : tag(tag), data(data) { }

  // TODO: you could add helper functions
  /* a function that creat a message that is used to be sent to server
     return a char* type buffer
  */
  char* messageToSent() const {
    string message = tag + ':' + data; // the message format
    size_t mess_len = message.length(); //size of the message 
    char* result = new char[mess_len + 2];
    memcpy(result, message.c_str(), mess_len); //copy the message string memory address to 
    result[mess_len] = '\n'; //add new line terminator to the message 
    result[mess_len+1] = '\0'; //end the message 
    return result;
  }
};

// standard message tags (note that you don't need to worry about
// "senduser" or "empty" messages)
#define TAG_ERR       "err"       // protocol error
#define TAG_OK        "ok"        // success response
#define TAG_SLOGIN    "slogin"    // register as specific user for sending
#define TAG_RLOGIN    "rlogin"    // register as specific user for receiving
#define TAG_JOIN      "join"      // join a chat room
#define TAG_LEAVE     "leave"     // leave a chat room
#define TAG_SENDALL   "sendall"   // send message to all users in chat room
#define TAG_SENDUSER  "senduser"  // send message to specific user in chat room
#define TAG_QUIT      "quit"      // quit
#define TAG_DELIVERY  "delivery"  // message delivered by server to receiving client
#define TAG_EMPTY     "empty"     // sent by server to receiving client to indicate no msgs available

#endif // MESSAGE_H
