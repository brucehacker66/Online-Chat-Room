#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>
#include "csapp.h"
#include "message.h"
#include "connection.h"
#include "client_util.h"
using std::string;

void checkLoginJoinResponse(Message msg);
void outputMessage (Message server_response, string room_name);

int main(int argc, char **argv) {
  if (argc != 5) {
    std::cerr << "Usage: ./receiver [server_address] [port] [username] [room]\n";
    return 1;
  }

  std::string server_hostname = argv[1];
  int server_port = std::stoi(argv[2]);
  std::string username = argv[3];
  std::string room_name = argv[4];

  Connection conn;
  // TODO: connect to server
  conn.connect(server_hostname, server_port);

  // TODO: send rlogin and join messages (expect a response from the server for each one)
  Message rlogin = Message(TAG_RLOGIN, username); //receeuver sends rlogin
  conn.send(rlogin);
  Message rlogin_response; //the rlogin response sends back by the server
  conn.receive(rlogin_response); //receive the message
  //check for the receive message
  checkLoginJoinResponse(rlogin_response);

  Message join = Message(TAG_JOIN,room_name); //message to join the room 
  conn.send(join);
  Message join_response; //the join response sends back by the server
  conn.receive(join_response);
  checkLoginJoinResponse(join_response); //check if response is valid format

  // TODO: loop waiting for messages from server
  //       (which should be tagged with TAG_DELIVERY)
  while (true) {
    Message server_response;
  if (conn.receive(server_response)) { //receive the message successfully
      if (server_response.tag == TAG_DELIVERY) { //message tag is delivery
        outputMessage(server_response,room_name); //receiver output the message
      } else { //error
        std::cerr << "Invalid Server response message tag"; 
      }
    } else { //does not receive successfully
      if (conn.get_last_result() != Connection::EOF_OR_ERROR) { //check m_last_result
        std::cerr << "Invalid message format";
      } else {
        exit(1);
      }      
    }  
  }

  return 0;
}


/* function to check whether the reponse of the login message is valid

Parameter:
  msg: the login response message 
*/
void checkLoginJoinResponse(Message msg) {
  if (msg.tag == TAG_ERR) { //if tag is ERR
    std::cerr << msg.data.c_str(); //print the rlogin failed message to cerr
    exit(1);
  }
  
  if (msg.tag != TAG_OK) { //if tag is not OK, 
    std::cerr << "Other Error in login/join Response";
    exit(1);
  }
}

/* Function to check and output the deliver message
Parameter:
  server_response: the response message from the server
  room_name: the name of room
*/
void outputMessage (Message server_response, string room_name) {
  size_t colonPosition1 = server_response.data.find(":");  //find the ":" position 
  string room = server_response.data.substr(0,colonPosition1); //get the room name 
  if (room == room_name) { //check if the room of the message is the correct room joined
    string message = server_response.data.substr(colonPosition1 + 1); //get the message from the data
    size_t colonPosition2 = message.find(":");  //find the ":" position 
    string usernameofSender = message.substr(0, colonPosition2);
    string messageText = message.substr(colonPosition2 + 1);
    fprintf(stdout, "%s", (usernameofSender + ": " + messageText).c_str());
  }
}