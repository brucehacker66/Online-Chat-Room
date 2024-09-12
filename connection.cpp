#include <iostream>
#include <sstream>
#include <cctype>
#include <cassert>
#include <string>
#include "csapp.h"
#include "message.h"
#include "connection.h"
using std::string;

Connection::Connection()
  : m_fd(-1)
  , m_last_result(SUCCESS) {
}

Connection::Connection(int fd)
  : m_fd(fd)
  , m_last_result(SUCCESS) {
  // TODO: call rio_readinitb to initialize the rio_t object
  rio_readinitb(&m_fdbuf, fd);
}

void Connection::connect(const std::string &hostname, int port) {
  // TODO: call open_clientfd to connect to the server
  int client_fd = open_clientfd(hostname.c_str(), std::to_string(port).c_str());
  // check for error:
  if (client_fd < 0) {
    std::cerr << "fail to connect to the server";
    exit(1);
  }
  // TODO: call rio_readinitb to initialize the rio_t object
  m_fd =  client_fd;
  rio_readinitb(&m_fdbuf, client_fd);
}

Connection::~Connection() {
  // TODO: close the socket if it is open
  if (is_open()) {
    close(); // clost the socket
  }
}

bool Connection::is_open() const {
  // TODO: return true if the connection is open
  // check for the result:
  if (m_last_result == EOF_OR_ERROR) {
    return false;
  } else {
    return true;    
  }
}

void Connection::close() {
  // TODO: close the connection if it is open
  if (is_open()) {
    ::close(m_fd);
  }
}

bool Connection::send(const Message &msg) {
  // TODO: send a message
  // return true if successful, false if not
  // make sure that m_last_result is set appropriately
  char* message = msg.messageToSent(); // turn the Message struct to a meesage to be sent in char* format
  size_t message_length = strlen(message);
  if (rio_writen(m_fd, message, message_length) == (ssize_t) message_length) { // success to write to server
    delete[] message; //clear memory
    m_last_result = SUCCESS; //change result status
    return true;
    
  } else { //fail to write to server
    delete[] message;
    m_last_result = EOF_OR_ERROR; //change result
    return false;
  }
  
}

bool Connection::receive(Message &msg) {
  // TODO: receive a message, storing its tag and data in msg
  // return true if successful, false if not
  // make sure that m_last_result is set appropriately
  char inBuffer[Message::MAX_LEN]; //buffer for read in message
  ssize_t size_in = rio_readlineb(&m_fdbuf, inBuffer, Message::MAX_LEN); //read messages, get the size of input message
  // check successful read-in:
  if (size_in <= 0) { //fail to read-in, no date read or error situation
    m_last_result = EOF_OR_ERROR; //set m_last_result as 
    msg.tag = TAG_EMPTY; //set the tag to empty
    return false;
  } else { //succeed to read-in, but message format can have some issue
    string inMsg(inBuffer); //convert the char array buffer to string
    size_t colonPosition = inMsg.find(":");  //find the ":" position 
    msg.tag = inMsg.substr(0, colonPosition); // get the tag part
    msg.data = inMsg.substr(colonPosition + 1); // get the data part
    //check whether the message is valid:
    if (msg.tag == TAG_ERR || msg.tag == TAG_OK || msg.tag == TAG_SLOGIN || msg.tag == TAG_RLOGIN || msg.tag == TAG_JOIN || msg.tag == TAG_LEAVE || 
        msg.tag == TAG_SENDALL || msg.tag == TAG_SENDUSER || msg.tag == TAG_QUIT || msg.tag == TAG_DELIVERY || msg.tag == TAG_EMPTY) { //valid
      m_last_result = SUCCESS; //setm_last_result to success 
      return true;
    } else {  //invalid
      m_last_result = INVALID_MSG; //setm_last_result to invalid
      return false;  
    }
  }
}