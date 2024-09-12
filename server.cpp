#include <pthread.h>
#include <iostream>
#include <sstream>
#include <memory>
#include <set>
#include <vector>
#include <cctype>
#include <cassert>
#include "message.h"
#include "connection.h"
#include "user.h"
#include "room.h"
#include "guard.h"
#include "server.h"
using namespace std;

////////////////////////////////////////////////////////////////////////
// Server implementation data types
////////////////////////////////////////////////////////////////////////

// TODO: add any additional data types that might be helpful
//       for implementing the Server member functions
class ConnectionException: public exception {
  protected: 
    string msg; //error message
  public:
    ConnectionException(void) : msg("") {}
    ConnectionException(const string &message) : msg(message) {}
    const char *what(void) {return msg.c_str();}
};

// struct ConnInfo: represents a client connection
typedef struct ConnInfo{
  Server* server;
  Connection* newconn;
  
  ConnInfo(Connection * connection, Server *server) { //constructor
    this->newconn = connection;
    this->server = server;
  }

  ~ConnInfo(){
    delete newconn;
  }
} ConnInfo;

////////////////////////////////////////////////////////////////////////
// Client thread functions
////////////////////////////////////////////////////////////////////////

namespace {

/* check the status of a new connection
    Parameter:
      conn - new connection
*/
void connectionCheck(Connection* conn){ //check if there's error reveiving the message 
  if (conn->get_last_result() == Connection::EOF_OR_ERROR) {
    throw ConnectionException();
  }
}

/* send message of gor connection
    Parameter:
      conn - new connection
      msg -error message
*/
void reportError(Connection* conn, string msg) {
  Message MsgtoSend = {TAG_ERR,msg};
  conn->send(MsgtoSend);
  connectionCheck(conn); //check the connection status 
}

/* send errpr message to the user
    Parameter:
      conn - new connection
      msg -error message
*/
void reportGood(Connection* conn, string msg) {
  Message MsgtoSend = {TAG_OK,msg};
  conn->send(MsgtoSend);
  connectionCheck(conn); //check the connection status 
}

/* remove the trailing whitespace from the message
      Parameter:
        s - string 
*/
string rtrim(const string &s){
    const string WHITESPACE = " \n\r\t\f\v";
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

/* check whether the message terminate with a new line character "\n"
    Parameter:
      conn - new connection
*/
bool msgHasNewLineTerminator(Message &msg) {
  if (msg.data.find("\n") != string::npos) { //there is a ""\n"
    return true;
  } else { //there is no "\n" at the end
    return false;
  }
}

/* check whether the user name are in correct format
    Parameter:
      msg - the input message
*/
bool usernameCorrect(Message &msg) {
  if (!msgHasNewLineTerminator(msg)) { //there is no new line terminator in message
    return false;
  } else {
    if ( (rtrim(msg.data).find_first_not_of("1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz") == string::npos) && (msg.data.length() >= 1)) {
      return true;
    } else{ //incorrect format 
      return false;
    }
  }
}

/* chat with the receiver
    Parameter:
      conn - connection
      server - server
      user - user
*/
void chat_with_receiver(Server* server, Connection* conn,  User* user) {
  Message request;

  //check if the connection receive the message request
  if (!conn->receive(request)) { //if the connection does not receive message
    reportError(conn, "receiver doesn't receive message");
    throw ConnectionException();
  }

  //the message request is not in the correct format
  if (!msgHasNewLineTerminator(request)|| request.tag != TAG_JOIN) {
    reportError(conn, "receiver message request is in invalid format");
    throw ConnectionException();
  }

  //creat chat room
  Room *chatroom = server->find_or_create_room(rtrim(request.data));
  chatroom->add_member(user);  //add users
  conn->send(Message(TAG_OK, ""));

  if (conn->get_last_result() == Connection::EOF_OR_ERROR) { //connection error
    chatroom->remove_member(user); //remove the suer from the chat room
    throw ConnectionException();
  }

  //loop to retrieve message
  while (true) {
    Message* resmsg = user->mqueue.dequeue(); //get a message by dequeue from user queue
    
    if (resmsg != nullptr) {
      conn->send(*resmsg);
      delete resmsg;
    }

    if (conn->get_last_result() == Connection::EOF_OR_ERROR) {
      break;
    }
  }
  chatroom->remove_member(user);
}

/* chat with the sender
    Parameter:
      conn - connection
      server - server
      username - username for sender
*/
void chat_with_sender( Server* thisServer, Connection* conn, string username) {
  Message resmsg; //the response message
  Room* chatroom = nullptr; //room to chat 

  //loop to chat with the sender
  while (true) {
    conn->receive(resmsg);
    connectionCheck(conn); //check the connection
    //tag can be quit, leave, sendall, join
    if (conn->get_last_result() != Connection::INVALID_MSG) { //the message is valid: 
      if (resmsg.tag == TAG_QUIT) { //tag is quit
        reportGood(conn,"");
        throw ConnectionException();
      } else if (resmsg.tag == TAG_JOIN) { //tag is join 
        if (! msgHasNewLineTerminator(resmsg)) { //if the message format is wrong
          reportError(conn, "sender send invalid room name");
        } else { //the room name format is right
          chatroom = thisServer->find_or_create_room(rtrim(resmsg.data));
          reportGood(conn,""); //report connection status
        }
      } else if (resmsg.tag == TAG_LEAVE) { //tag is to leava
        if (chatroom != nullptr) {
          chatroom = nullptr;
          reportGood(conn,"");
        } else {
          reportError(conn, "sender is not in the room cannot leave");
        }
      } else if (resmsg.tag == TAG_SENDALL) { //tag is to send to sendall
        if (chatroom != nullptr) {
          if (!msgHasNewLineTerminator(resmsg)) {
            reportError(conn, "invalid sendall message format");
          } else {
            chatroom->broadcast_message(username, rtrim(resmsg.data));
            reportGood(conn,"");
          }          
        } else {//the room is not exist, invalid
          reportError(conn, "sender is not in the room cannot send message");
        }
      } else { //the tag is invalid
        reportError(conn, "invalid tag");
      }
    } else { //the message is not valid, report error
      reportError(conn, "invalid message format");
    }
  }
}

void *worker(void *arg) {
  pthread_detach(pthread_self());

  // TODO: use a static cast to convert arg from a void* to
  //       whatever pointer type describes the object(s) needed
  //       to communicate with a client (sender or receiver)
  ConnInfo* info = static_cast<ConnInfo *>(arg); //get connection info
  Connection* newConn = info->newconn;     //new connection
  Server* newServer = info->server; //new server

  // TODO: read login message (should be tagged either with
  //       TAG_SLOGIN or TAG_RLOGIN), send resmsg
  User* newUser = new User("");
  Message inputMsg; //the message received by the server

  try { //receive the message from connection
    newConn->receive(inputMsg); 
    connectionCheck(newConn); //check the connection is not EOF
    
    //check for invalid message
    if (newConn->get_last_result() == Connection::INVALID_MSG) {
      reportError(newConn, "MESSAGE INVALID");
    }

    //check the input username
    if (!usernameCorrect(inputMsg)) { //the user name into does not have correct format
      reportError(newConn, "USERNAME INVALID");
      throw ConnectionException();      
    } else { //the user name is in correct format 
      reportGood(newConn,"");
      newUser->username = rtrim(inputMsg.data); //remove the whitespace from the message
    }

  // TODO: depending on whether the client logged in as a sender or
  //       receiver, communicate with the client (implementing
  //       separate helper functions for each of these possibilities
  //       is a good idea)

    //deal with sender and receiver login message
    if (inputMsg.tag == TAG_RLOGIN) { //receiver login message
      chat_with_receiver(newServer, newConn, newUser); //chat with receiver here
    } else if (inputMsg.tag == TAG_SLOGIN) { //sender login message
      chat_with_sender(newServer, newConn, newUser->username); //chat with sender here, not creating user for sender
    } else { //login tag has problem 
      reportError(newConn, "LOGIN TAG INVALID");
      throw ConnectionException();      
    }
  } catch (ConnectionException& e) {
    free(info);
    return nullptr;
  } 
  free(info);
  return nullptr;
}

}


////////////////////////////////////////////////////////////////////////
// Server member function implementation
////////////////////////////////////////////////////////////////////////

Server::Server(int port)
  : m_port(port)
  , m_ssock(-1) {
  // TODO: initialize mutex
    pthread_mutex_init(&m_lock, NULL);
}

Server::~Server() {
  // TODO: destroy mutex
  close (m_ssock);
  pthread_mutex_destroy(&m_lock);
}

bool Server::listen() {
  // TODO: use open_listenfd to create the server socket, return true
  //       if successful, false if not
  int server_fd = open_listenfd(std::to_string(m_port).c_str());
  if (server_fd < 0) {
    return false;
  }else{
    m_ssock = server_fd;
    return true;
  }
}

void Server::handle_client_requests() {
  // TODO: infinite loop calling accept or Accept, starting a new
  //       pthread for each connected client
  vector<ConnInfo*> threadArgs;
  while(true){
    int client_fd = Accept(m_ssock,NULL,NULL);
    Connection* new_connection = new Connection(client_fd);
    ConnInfo* new_info = new ConnInfo(new_connection, this);
    threadArgs.push_back(new_info);
    pthread_t thr_id;
    if (pthread_create(&thr_id, NULL, worker, new_info) != 0) {
      fprintf(stderr, "%s\n","Error occured while processing client request");
      exit(-1);
    }
  }
}

Room *Server::find_or_create_room(const string &room_name) {
  // TODO: return a pointer to the unique Room object representing
  //       the named chat room, creating a new one if necessary
  Guard g(m_lock);
  RoomMap::iterator target_room = m_rooms.find(room_name);
  if (target_room == m_rooms.end()){
  //create a new room
    Room * new_room = new Room(room_name);
    m_rooms.insert({room_name,new_room});
    return new_room;
  }else{
    return target_room->second;
  }
}
