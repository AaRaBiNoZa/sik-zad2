//
// Created by Adam Al-Hosam on 05/05/2022.
//

#ifndef SIK_ZAD3_MESSAGES_H
#define SIK_ZAD3_MESSAGES_H

#include <memory>
#include <map>
#include <sstream>

class Message {
 public:
  typedef std::shared_ptr<Message> (*MessageFactory)(std::stringstream&);
  static std::map<char, MessageFactory> &message_map() {
    static auto *ans = new std::map<char, MessageFactory>();
    return *ans;
  };
  void say_hello()  {
      std::cout << "Hello i am Message";
  };
  static std::shared_ptr<Message> unserialize(std::stringstream& istr) {
    char c;
    istr >> c;
    return message_map()[c](istr);
  }
};

class InputMessage : public Message {
 public:
  typedef std::shared_ptr<InputMessage> (*InputMessageFactory)(std::stringstream&);
  static std::map<char, InputMessageFactory> &input_message_map() {
      static auto *ans = new std::map<char, InputMessageFactory>();
      return *ans;
  };
  static std::shared_ptr<Message> create(std::stringstream& rest) {
    return std::dynamic_pointer_cast<Message>(unserialize(rest));
  }
  void say_hello()  {
    std::cout << "Hello i am InputMessage";
  };
  static std::shared_ptr<InputMessage> unserialize(std::stringstream& istr) {
    char c;
    istr >> c;
    return input_message_map()[c](istr);
  }
};

class PlaceBomb : public InputMessage {
 public:
  static std::shared_ptr<InputMessage> create(std::stringstream& rest) {
    return std::dynamic_pointer_cast<InputMessage>(std::make_shared<PlaceBomb>(rest));
  }
  explicit PlaceBomb(std::stringstream& rest) {};
  void say_hello() {
    std::cout << "I am placebomb";
  }
};

#endif  // SIK_ZAD3_MESSAGES_H
