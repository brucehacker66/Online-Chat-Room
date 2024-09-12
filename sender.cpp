#include <iostream>
#include <string>
#include <sstream>
#include <stdexcept>
#include "csapp.h"
#include "message.h"
#include "connection.h"
#include "client_util.h"

void checkLoginJoinResponse(Message msg);
int send_loop(Connection &conn, Message send);
void receive_loop(Connection &conn, Message response);

int main(int argc, char **argv) {
  if (argc != 4) {
    std::cerr << "Usage: ./sender [server_address] [port] [username]\n";
    return 1;
  }

  std::string server_hostname;
  int server_port;
  std::string username;

  server_hostname = argv[1];
  server_port = std::stoi(argv[2]);
  username = argv[3];

  Connection conn;
  // TODO: connect to server
  conn.connect(server_hostname, server_port);

  // TODO: send slogin message
  Message slogin = Message(TAG_SLOGIN, username); //receeuver sends rlogin
  conn.send(slogin);
  Message slogin_response; //the rlogin response sends back by the server
  conn.receive(slogin_response); //receive the message
  checkLoginJoinResponse(slogin_response); //check if response is valid format

  // TODO: loop reading commands from user, sending messages to
  //       server as appropriate

  Message send;
  Message response;
  while (true){
    int quit = send_loop(conn, send); //send messages to server
    if(quit == 2){ //start again if user message is invalid
      continue;
    }
    receive_loop(conn, response); //react to messages sent back from the server
    if(quit == 1){ //if user commands quit, stop the loop and return
      break;
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
    std::cerr << "Other error in login response";
    exit(1);
  }
}

/* function for the sender to send commands and message

Parameter:
  conn: current connection object
  send: message object that will be edited and sent to the server
Return:
  quit: 0 means normal, continue the loop
        1 means quit, stop the loop and exit
        2 means invalid user message, end this loop and start over
*/
int send_loop(Connection &conn, Message send){
  int quit = 0;
  std::string input;
  std::getline(std::cin, input);
  std::stringstream ssinput(input);
  std::string tag;
  ssinput >> tag; // identify the tag of the input
  std::string msg;
  if (tag[0] == '/') { // process input if it is a command
    if (tag.compare("/join") == 0) {
      ssinput >> msg;
      send = {TAG_JOIN, msg};
    } else if (tag.compare("/leave") == 0) {
      send = {TAG_LEAVE, "bye"};
    } else if (tag.compare("/quit") == 0) {
      send = {TAG_QUIT, "bye"};
      quit = 1;
    } else { // if tag does not match any expected value, start over
      fprintf(stderr, "%s\n", "invalid command");
      quit = 2;
      return quit;
    }
  } else { // if not command, treat input line as message
    send = {TAG_SENDALL, ssinput.str()};
  }
  if (!conn.send(send)) {  // send message to server
    std::cerr << "cannot send message to server";
    exit(1);
  }
  return quit;
}

/* function for the sender to receive a message from the server

Parameter:
  conn: current connection object
  response: message object that is sent from the server
*/
void receive_loop(Connection &conn, Message response){
  if (conn.receive(response)) {
    if (response.tag == TAG_ERR) {
      std::cerr << response.data.c_str();
    } else if (response.tag != TAG_OK) {
      std::cerr << "unexpected server response tag";
    }
  } else {
    if (!conn.is_open()) {
      std::cerr << "unable to receive due to EOF or Error";
      exit(1);
    } else {
      std::cerr << "unable to receive due to invalid format";
    }
  }
}