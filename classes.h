#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>

#include <WinSock2.h>
#include <WS2tcpip.h>

#ifdef CLASSES_H_INCLUDED
#define CLASSES_H_INCLUDED

int inet_pton(int af, const char *src, void *dst) {
    struct sockaddr_storage ss;
    int size = sizeof(ss);
    char src_copy[INET6_ADDRSTRLEN+1];

    ZeroMemory(&ss, sizeof(ss));
    strncpy (src_copy, src, INET6_ADDRSTRLEN+1);
    src_copy[INET6_ADDRSTRLEN] = 0;

    if (WSAStringToAddress(src_copy, af, NULL, (struct sockaddr *)&ss, &size) == 0) {
    switch(af) {
        case AF_INET:
            *(struct in_addr *)dst = ((struct sockaddr_in *)&ss)->sin_addr;
            return 1;
        case AF_INET6:
            *(struct in6_addr *)dst = ((struct sockaddr_in6 *)&ss)->sin6_addr;
            return 1;
        }
    }
    return 0;
}

const char *inet_ntop(int af, const void *src, char *dst, socklen_t size) {
    struct sockaddr_storage ss;
    unsigned long s = size;

    ZeroMemory(&ss, sizeof(ss));
    ss.ss_family = af;

    switch(af) {
    case AF_INET:
        ((struct sockaddr_in *)&ss)->sin_addr = *(struct in_addr *)src;
        break;
    case AF_INET6:
        ((struct sockaddr_in6 *)&ss)->sin6_addr = *(struct in6_addr *)src;
        break;
    default:
        return NULL;
    }
    return (WSAAddressToString((struct sockaddr *)&ss, sizeof(ss), NULL, dst, &s) == 0) ? dst : NULL;
}

class socket_address{
private:
	friend class socket_udp;
	friend class socket_tcp;
	friend class socket_tcp_mini;
	sockaddr_in sock_addr;
public:
	socket_address(int32_t address, int16_t port);
	socket_address(const std::string address, int16_t port);
	socket_address(const sockaddr* sock_address_in);
	socket_address(void);
	void copy(const sockaddr* sock_address_in);
	void copy(const socket_address* sock_address_in);
	size_t size(void);
	void cout_info(void);
};

class socket_udp{
private:
	int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
public:
	socket_udp(socket_address* address);
	void send_to(const char* buffer, int len, socket_address* to);
	void recv_from(char* buffer, int len, socket_address* from, socklen_t* fromlen);
	void set_nonblock_mode(bool mode);
	~socket_udp();

};

class socket_tcp_mini{
private:
	int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
public:
	socket_tcp_mini(socket_address* address);
	void wait_for_con(int backlog);
	int accept_con(socket_address* address);
	void set_nonblock_mode(bool mode);
	~socket_tcp_mini();
};

class socket_tcp{
private:
	int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
public:
	socket_tcp(socket_address* address);
	socket_tcp(int sock);
	void create_con(socket_address* address);
	void send_to(const char* buffer, int len);
	void recv_from(char* buffer, size_t len);
	void set_nonblock_mode(bool mode);
	~socket_tcp();
};

void fatal(const std::string message);

socket_address::socket_address(int32_t address, int16_t port) {
	memset(&sock_addr.sin_zero, 0, sizeof(sock_addr.sin_zero));
	sock_addr.sin_family = AF_INET;
	sock_addr.sin_addr.s_addr = htonl(address);
	sock_addr.sin_port = htons(port);
}

socket_address::socket_address(const std::string address, int16_t port) {
	memset(&sock_addr.sin_zero, 0, sizeof(sock_addr.sin_zero));
	sock_addr.sin_family = AF_INET;
	int error = inet_pton(AF_INET, address.c_str(), &sock_addr.sin_addr);
	if(error != 1)
		fatal("socket_address can't set address from string");
	sock_addr.sin_port = htons(port);
}

socket_address::socket_address(const sockaddr* sock_address_in) {
	memcpy(&sock_addr, sock_address_in, sizeof(sockaddr));
	memset(&sock_addr.sin_zero, 0, sizeof(sock_addr.sin_zero));
}

socket_address::socket_address() {
	memset(&sock_addr, 0, sizeof(sock_addr));
}

void socket_address::copy(const sockaddr* sock_address_in) {
	memcpy(&sock_addr, sock_address_in, sizeof(sockaddr) - sizeof(sock_addr.sin_zero));
}


void socket_address::copy(const socket_address* sock_address_in) {
	memcpy(&sock_addr, &sock_address_in->sock_addr, sizeof(sockaddr) - sizeof(sock_addr.sin_zero));
}


size_t socket_address::size(void) {
	return sizeof(sock_addr);
}

void socket_address::cout_info(void) {
	char q[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &sock_addr.sin_addr, (char *)q, INET_ADDRSTRLEN);
	std::cout << "IP: " << q << '\n';
	std::cout << "PORT: " << ntohs(sock_addr.sin_port) << '\n';
}

socket_udp::socket_udp(socket_address* address) {
	int error = bind(sock_fd, reinterpret_cast<sockaddr*> (&address->sock_addr), sizeof(sockaddr));
	if(error != 0)
		fatal("socket_udp can't bind udp socket with address " + std::to_string(errno));
}

void socket_udp::send_to(const char* buffer, int len, socket_address* to) {
	int error = sendto(sock_fd, buffer, len, 0, reinterpret_cast<sockaddr*> (&to->sock_addr), sizeof(sockaddr));
	if(error == -1)
		fatal("socket_udp can't send any information to sockaddr*");
}

void socket_udp::recv_from(char* buffer, int len, socket_address* from, socklen_t* fromlen) {
	memset(&from->sock_addr, 0, sizeof(sockaddr_in));
	memset(buffer, 0, strlen(buffer));
	fromlen = 0;
	len = 0;
	int error = recvfrom(sock_fd, buffer, len, 0, reinterpret_cast<sockaddr*> (&from->sock_addr), fromlen);
	if(error == -1)
        if(WSAGetLastError() != WSAEWOULDBLOCK)
            fatal("some error in revb_from(socket_udp)");
}

void socket_udp::set_nonblock_mode(bool mode) {
    u_long arg = mode ? 1 : 0;
    int error = ioctlsocket(sock_fd, FIONBIO, &arg);
    if(error == SOCKET_ERROR)
        fatal("soket_udp can't set nonblock mode");
}

socket_udp::~socket_udp() {
//	shutdown(sock_fd, SD_BOTH);
	closesocket(sock_fd);
}

socket_tcp_mini::socket_tcp_mini(socket_address* address) {
	int error = bind(sock_fd, reinterpret_cast<sockaddr*> (&address->sock_addr), sizeof(sockaddr));
	if(error != 0)
		fatal("socket_tcp_mini can't bind tcp socket with address");
}

void socket_tcp_mini::wait_for_con(int backlog = SOMAXCONN) {
	int error = listen(sock_fd, backlog);
	if(error != 0)
//        if(WSAGetLastError() != WSAEWOULDBLOCK)
        fatal("socket_tcp_mini has error in waiting for connection");
}

int socket_tcp_mini::accept_con(socket_address* address) {
	memset(&address->sock_addr, 0, sizeof(sockaddr_in));
	socklen_t addrlen = sizeof(sockaddr);
	return accept(sock_fd, reinterpret_cast<sockaddr*> (&address->sock_addr), &addrlen);
}

void socket_tcp_mini::set_nonblock_mode(bool mode) {
    u_long arg = mode ? 1 : 0;
    int error = ioctlsocket(sock_fd, FIONBIO, &arg);
    if(error == SOCKET_ERROR)
        fatal("soket_udp can't set nonblock mode");
}

socket_tcp_mini::~socket_tcp_mini() {
//	shutdown(sock_fd, SHUT_RDWR);
	closesocket(sock_fd);
}

socket_tcp::socket_tcp(socket_address* address) {
	int error = bind(sock_fd, reinterpret_cast<sockaddr*> (&address->sock_addr), sizeof(sockaddr));
	if(error != 0)
		fatal("socket_tcp can't bind tcp socket with address " + std::to_string(errno));
}

socket_tcp::socket_tcp(int sock): sock_fd(sock) {}

void socket_tcp::create_con(socket_address* address) {
        int error = connect(sock_fd, reinterpret_cast<sockaddr*> (&address->sock_addr), sizeof(sockaddr));
        if(error != 0)
                fatal("socket_tcp can't connect to address");
}

void socket_tcp::send_to(const char* buffer, int len) {
	int error = send(sock_fd, buffer, len, 0);
	if(error == -1)
		fatal("socket_tcp can't send buffer to address " + std::to_string(errno));
}

void socket_tcp::recv_from(char* buffer, size_t len) {
	memset(buffer, 0, strlen(buffer));
	int error = recv(sock_fd, buffer, len, 0);
	if(error == -1)
        if(WSAGetLastError() != WSAEWOULDBLOCK)
            fatal("socket_tcp can't recv any information");
}

void socket_tcp::set_nonblock_mode(bool mode) {
    u_long arg = mode ? 1 : 0;
    int error = ioctlsocket(sock_fd, FIONBIO, &arg);
    if(error == SOCKET_ERROR)
        fatal("soket_udp can't set nonblock mode");
}

socket_tcp::~socket_tcp() {
//	shutdown(sock_fd, SHUT_RDWR);
	closesocket(sock_fd);
}

int get_addr_from_domain(const std::string* domain, socket_address* ans) {
	std::string host, service;
	size_t pos = domain->find_last_of(":");
	if(pos != std::string::npos) {
		host = domain->substr(0, pos);
		service = domain->substr(pos + 1);
	} else {
		host = *domain;
		service = "";
	}
	addrinfo hint;
	addrinfo* res = nullptr;
	memset(&hint, 0, sizeof(addrinfo));
	hint.ai_family = AF_INET;
	int error = getaddrinfo(host.c_str(), service.c_str(), &hint, &res);
	if(error != 0)
		return error;
	while((res->ai_addr == nullptr) && (res->ai_next != nullptr))
		res = res->ai_next;
	if(res->ai_addr == nullptr) {
		freeaddrinfo(res);
		return -1;
	}
	ans->copy(res->ai_addr);
	freeaddrinfo(res);
	return 0;
}

void fatal(const std::string message) {
	std::cout << "Error: " << message << '\n';
	exit(-1);
}

#endif // CLASSES_H_INCLUDED
