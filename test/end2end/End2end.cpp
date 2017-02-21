/*
 * End2end.cpp
 *
 *  Created on: 2017.02.19.
 *      Author: tooma
 */

#include "DavProperty.h"
#include "HttpLogic.h"

#include <iostream>

#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/sendfile.h>

#ifndef F_GETPATH
#define F_GETPATH	(1024 + 7)
#endif

int davRoot;
char* davRootName;

struct Types {
	struct ResourceLocator {
		int fd;
		DIR *dir;
		char name[PATH_MAX];
		struct stat st;

		bool fetchName() {
			char procFsFdPath[256];
			sprintf(procFsFdPath, "/proc/%d/fd/%d", getpid(), fd);
			int r = readlink(procFsFdPath, name, sizeof(name));
			if(r < 0) {
				std::cerr << "Could not readlink " << procFsFdPath << ": " << strerror(errno) << std::endl;
				return false;
			} else
				name[r] = '\0';

			return true;
		}
	};

	static constexpr const uint32_t davStackSize = 192;
	static constexpr const char* username = "foo";
	static constexpr const char* realm = "bar";
	static constexpr const char* RFC2069_A1 = "d65f52b42a2605dd84ef29a88bd75e1d";

	static constexpr const DavProperty davProperties[3] = {
		DavProperty("DAV:", "getcontentlength"),
		DavProperty("DAV:", "getlastmodified"),
		DavProperty("DAV:", "resourcetype")
	};
};

constexpr const DavProperty Types::davProperties[3];

class HttpSession: protected HttpLogic<HttpSession, Types>
{
	friend HttpLogic<HttpSession, Types>;
	using ResourceLocator = Types::ResourceLocator;
	int sockFd;

	void send(const char* str, unsigned int length)
	{
		if(write(sockFd, str, length) < 0)
			std::cerr << "Unable to write client socket " << strerror(errno) << std::endl;
	}

	DavAccess accessible(ResourceLocator* rl, bool authenticated)
	{
		return DavAccess::Dav;
	}

	void resetLocator(ResourceLocator* rl)
	{
		rl->dir = nullptr;
		rl->fd = dup(davRoot);
		std::cout << "reset " << davRoot << " -> " << rl->fd << std::endl;
	}

	HttpStatus enter(ResourceLocator* rl, const char* str, unsigned int length)
	{
		std::string name(str, length);
		int oldFd = rl->fd;
		rl->fd = openat(oldFd, name.c_str(), O_RDONLY);
		close(oldFd);

		std::cout << "enter " << oldFd << " -> " << rl->fd << std::endl;
		if(rl->fd < 0) {
			std::cerr << "Could not enter " << name << ": " << strerror(errno) << std::endl;
			return HTTP_STATUS_NOT_FOUND;
		}

		return HTTP_STATUS_OK;
	}

	HttpStatus remove(ResourceLocator* rl, const char* dstName, uint32_t length)
	{
		HttpStatus status = enter(rl, dstName, length);
		if(status != HTTP_STATUS_OK)
			return status;

		rl->fetchName();
		close(rl->fd);
		if(::remove(rl->name) < 0) {
			std::cerr << "Unable to remove: " << strerror(errno) << std::endl;
			return HTTP_STATUS_FORBIDDEN;
		}

		return HTTP_STATUS_CREATED;
	}

	HttpStatus createDirectory(ResourceLocator* rl, const char* dstName, uint32_t length)
	{
		std::string name(dstName, length);

		if(mkdirat(rl->fd, name.c_str(), 0777) < 0) {
			std::cerr << "Unable to mkdir: " << strerror(errno) << std::endl;
			close(rl->fd);
			return HTTP_STATUS_FORBIDDEN;
		}

		close(rl->fd);
		return HTTP_STATUS_NO_CONTENT;
	}

	HttpStatus copy(ResourceLocator* src, ResourceLocator* dstDir, const char* dstName, uint32_t length, bool overwrite)
	{
		std::string name(dstName, length);
		int oldfd = dstDir->fd;

		dstDir->fd = openat(dstDir->fd, name.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
		close(oldfd);

		if(dstDir->fd < 0) {
			std::cerr << "Unable to open file for copy: " << name << strerror(errno) << std::endl;
			close(src->fd);
			return HTTP_STATUS_FORBIDDEN;
		}

		if(fstat(src->fd, &src->st) < 0) {
			std::cerr << "Could not stat source for copy: " << strerror(errno) << std::endl;
			close(src->fd);
			close(dstDir->fd);
			return HTTP_STATUS_FORBIDDEN;
		}

		if(sendfile(dstDir->fd, src->fd, nullptr, src->st.st_size) < 0) {
			std::cerr << "Unable to sendfile for copy " << strerror(errno) << std::endl;
			close(src->fd);
			close(dstDir->fd);
			return HTTP_STATUS_FORBIDDEN;
		}

		close(src->fd);
		close(dstDir->fd);
		return HTTP_STATUS_CREATED;
	}

	HttpStatus move(ResourceLocator* src, ResourceLocator* dstDir, const char* dstName, uint32_t length, bool overwrite)
	{
		std::string name(dstName, length);
		src->fetchName();
		close(src->fd);

		if(renameat(0, src->name, dstDir->fd, name.c_str())) {
			std::cerr << "Unable to renameat for move " << strerror(errno) << std::endl;
			close(dstDir->fd);
			return HTTP_STATUS_FORBIDDEN;
		}

		close(dstDir->fd);
		return HTTP_STATUS_CREATED;
	}

	HttpStatus arrangeReceiveInto(ResourceLocator* rl, const char* dstName, uint32_t length)
	{
		std::string name(dstName, length);
		int oldfd = rl->fd;

		rl->fd = openat(rl->fd, name.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
		close(oldfd);

		if(rl->fd < 0) {
			std::cerr << "Unable to open file for upload: " << name << strerror(errno) << std::endl;
			return HTTP_STATUS_FORBIDDEN;
		}

		return HTTP_STATUS_OK;
	}

	HttpStatus writeContent(ResourceLocator* rl, const char* buff, uint32_t length)
	{
		if(write(rl->fd, buff, length) < 0) {
			std::cerr << "Unable to write out received data " << strerror(errno) << std::endl;
			close(rl->fd);
			return HTTP_STATUS_FORBIDDEN;
		}

		return HTTP_STATUS_OK;
	}

	HttpStatus contentWritten(ResourceLocator* rl)
	{
		close(rl->fd);
		return HTTP_STATUS_CREATED;
	}

	HttpStatus arrangeSendFrom(ResourceLocator* rl, uint32_t &size)
	{
		if(fstat(rl->fd, &rl->st) < 0) {
			std::cerr << "Unable to fstat file for download " << strerror(errno) << std::endl;
			close(rl->fd);
			return HTTP_STATUS_FORBIDDEN;
		}

		size = rl->st.st_size;

		return HTTP_STATUS_OK;
	}

	HttpStatus readContent(ResourceLocator* rl)
	{
		if(sendfile(sockFd, rl->fd, nullptr, rl->st.st_size) < 0) {
			std::cerr << "Unable to sendfile " << strerror(errno) << std::endl;
			close(rl->fd);
			return HTTP_STATUS_FORBIDDEN;
		}

		return HTTP_STATUS_OK;
	}

	HttpStatus contentRead(ResourceLocator* rl)
	{
		close(rl->fd);
		return HTTP_STATUS_OK;
	}

	HttpStatus arrangeListing(ResourceLocator* rl, bool contents)
	{
		std::cout << "arrange " << rl->fd << std::endl;

		if(contents) {
			rl->dir = fdopendir(rl->fd);
			std::cout << "arrange dir" << rl->dir << std::endl;

			if(!rl->dir) {
				std::cerr << "Unable to open directory for listing: " << strerror(errno) << std::endl;
				close(rl->fd);
				return HTTP_STATUS_FORBIDDEN;
			}

			rewinddir(rl->dir);
			struct dirent *de = readdir(rl->dir);
			if(de) {
				strcpy(rl->name, de->d_name);
				if(stat(de->d_name, &rl->st) < 0) {
					std::cerr << "Could not stat " << de->d_name << ": " << strerror(errno) << std::endl;
					closedir(rl->dir);
					close(rl->fd);
					return HTTP_STATUS_FORBIDDEN;
				}
			}

		} else {
			if(!rl->fetchName() || !strstr(rl->name, davRootName))
				return HTTP_STATUS_FORBIDDEN;

			memmove(rl->name, rl->name+strlen(davRootName), strlen(rl->name)-strlen(davRootName)+1);

			if(!strlen(rl->name))
				strcat(rl->name, "/");

			if(fstat(rl->fd, &rl->st) < 0) {
				std::cerr << "Could not stat " << rl->name << ": " << strerror(errno) << std::endl;
				return HTTP_STATUS_FORBIDDEN;
			}

			rl->dir = nullptr;
		}
		return HTTP_STATUS_MULTI_STATUS;
	}

	HttpStatus generateListing(ResourceLocator* rl, const DavProperty* prop)
	{
		std::cout << "generate " << rl->fd << std::endl;
		std::cout << "generate dir " << rl->dir << std::endl;

		if(!prop) {
			sendChunk(rl->name);
		} else if(prop == Types::davProperties + 0) {
			char temp[32];
			sprintf(temp, "%d", (int)rl->st.st_size);
			sendChunk(temp);
		} else if(prop == Types::davProperties + 1) {
			sendChunk("Thu, 01 Jan 1970 00:00:00 GMT");
		} else if(prop == Types::davProperties + 2) {
			if(S_ISDIR(rl->st.st_mode))
				sendChunk("<collection/>");
		} else {
			return HTTP_STATUS_INTERNAL_SERVER_ERROR;
		}

		return HTTP_STATUS_OK;
	}

	bool stepListing(ResourceLocator* rl)
	{
		std::cout << "step " << rl->fd << std::endl;
		std::cout << "step dir " << rl->dir << std::endl;

		struct dirent *de = readdir(rl->dir);
		if(de) {
			strcpy(rl->name, de->d_name);
			if(fstatat(rl->fd, de->d_name, &rl->st, 0) < 0) {
				std::cerr << "Could not stat " << de->d_name << ": " << strerror(errno) << std::endl;
				return false;
			}
			return true;
		} else {
			closedir(rl->dir);
			rl->dir = nullptr;
			return false;
		}
	}

	HttpStatus listingDone(ResourceLocator* rl)
	{
		std::cout << "done " << rl->fd << std::endl;
		close(rl->fd);
		return HTTP_STATUS_MULTI_STATUS;
	}

public:
	inline HttpSession(int sockFd): sockFd(sockFd) {
	    fcntl(sockFd, F_SETFL, O_NONBLOCK);
		reset();
	}

	~HttpSession() {
		close(sockFd);
	}

	void process() {
		char buffer[1024];
		fd_set readset;
		int size;

		std::cerr << "Connection accepted" << std::endl;

		while (1) {
		    FD_ZERO(&readset);
		    FD_SET(sockFd, &readset);
		    select(sockFd + 1, &readset, NULL, NULL, NULL);

	        if (FD_ISSET(sockFd, &readset)) {
	        	size = recv(sockFd, buffer, sizeof(buffer), 0);
				if (size == 0) {
					break;
				} else if (size < 0) {
					if (errno == EAGAIN)
						 continue;
					else
						 break;
				 } else {
					parse(buffer, size);
				 }
			}
		}

		std::cerr << "Connection closed" << std::endl;

		done();
	}
};

int main(int argc, const char *argv[])
{
	if(argc != 2) {
		std::cerr << "Usage: " << argv[0] << " <path to root of dav>" << std::endl;
		return 0;
	}

	davRootName = strdup(argv[1]);

	DIR *rootDir;
	if(!(rootDir = opendir(davRootName))) {
		std::cerr << "Unable to open directory: " << davRootName << " " << strerror(errno) << std::endl;
		return 0;
	}

	davRoot = dirfd(rootDir);
	std::cout << "davRoot " << davRoot << std::endl;

	int listenfd;
	struct sockaddr_in serv_addr, cli_addr;
	int  n;

	listenfd = socket(AF_INET, SOCK_STREAM, 0);

    int one = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

	if (listenfd < 0) {
		std::cerr << "Unable to create listener socket " << strerror(errno) << std::endl;
		return 0;
	}

	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(8080);

	if (bind(listenfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		std::cerr << "Unable to bind listener socket to port 8080 " << strerror(errno) << std::endl;
		return 0;
	}

	listen(listenfd, 5);

	while(1) {
		int clientFd = accept(listenfd, nullptr, nullptr);

		if(clientFd < 0)
			break;

		HttpSession(clientFd).process();
	}

	closedir(rootDir);
	free(davRootName);
	return 0;
}
