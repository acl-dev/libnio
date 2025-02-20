//
// Created by shuxin ¡¡¡¡zheng on 2025/2/17.
//

#pragma once

namespace nev {

class server_socket {
public:
	explicit server_socket(int backlog = 128);
	~server_socket();

	bool open(const char *ip, int port);
	int accept(std::string *addr = nullptr) const;

	int sock_handle() const { return lfd_; }

private:
	int lfd_ = -1;
	int backlog_;
};

} // namespace
