#define CLASSES_H_INCLUDED
#include <fstream>
#include "classes.h"
using namespace std;

std::string q = "HTTP/1.0 200 OK\r\nServer: Tiny webserver\r\nContent-Type: text/html; charset=ISO-8859-1\r\n\r\n";
std::string answer = "";
std::string merror = "HTTP/1.0 404 NOT FOUND\r\nServer: Tiny webserver\r\n\r\n<html><head><title>404 Not Found</title></head><body><h1>URL not found</h1></body></html>\r\n";

bool keep_alive = false;

bool proc(char *buffer) {
    std::string input = buffer;
    int lpos, fpos;
    lpos = input.find(" HTTP/");
    if(lpos == std::string::npos)
        return false;
    else {
        bool good = false;
        if(input.find("GET ") == 0) {
            fpos = 4;
            good = true;
        } else
        if(input.find("HEAD ") == 0) {
            fpos = 5;
            good = true;
        }
        if(!good)
            return false;
        std::cout << "return true\n";
        std::cout << "fpos = " << fpos << ";, lpos = " << lpos << ";\n";
        std::string path = input.substr(fpos, lpos-fpos);
        std::cout << "path = \"" + path + "\"\n";
        if(path[path.size() - 1] != '/')
            path += '/';
        if(path[0] == '/')
            path = path.substr(1);
        path += "index.html";
        std::ifstream file(path);
        if(file.is_open()) {
            answer = q;
//            std::string info;
            char info[512];
            if(input.substr(0, 4) == "GET ") {
//                file.seekp(0, std::ios::end);
//                int sz = fp.tellp();
//                fp.seekp(0, std::ios::beg);
                while(file.getline((char*)info, 512))
                    answer += string((char*)info);
                file.close();
//                std::cout << "answer: " << answer << '\n';
            }
//            if(input.find("Connection: keep-alive") != std::string::npos) {
//                keep_alive = true;
//                answer += "\r\n\r\n";
//            }
            return true;
        } else {
            return false;
        }
    }
}

int main() {
    LPWSADATA q;
    int error = WSAStartup(MAKEWORD(2, 2), (LPWSADATA)&q);
    std::cout << "WSAerror: " << error << '\n';
    char buffer[2048];

    socket_address serv_addr("192.168.0.100", 80);
    socket_tcp_mini server(&serv_addr);
    server.wait_for_con(20);

    socket_address user_addr;
    int user;

    while(true) {
        user = server.accept_con(&user_addr);
        socket_tcp user_sock(user);
        keep_alive = true;
        while(keep_alive) {
            keep_alive = false;
            user_sock.recv_from((char*)&buffer, 2048);
            if(strlen(buffer) > 0)
                std::cout << "input: " << buffer;
            if(proc((char*)&buffer))
                user_sock.send_to(answer.c_str(), answer.size());
            else
                user_sock.send_to(merror.c_str(), merror.size());
        }
        std::cout << "connection closed\n";
        user_sock.~socket_tcp();
    }

    error = WSACleanup();
    std::cout << "WSAerror: " << error << '\n';
	return 0;
}
