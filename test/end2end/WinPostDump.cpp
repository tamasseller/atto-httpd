/*******************************************************************************
 *
 * Copyright (c) 2017 Tamás Seller. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *******************************************************************************/

#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>

#include <iostream>

#include "DavProperty.h"
#include "HttpLogic.h"
#include "JsonParser.h"

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"

struct Types {
	static constexpr const char* username = "foo";
	static constexpr const char* realm = "bar";
	static constexpr const char* RFC2069_A1 = "d65f52b42a2605dd84ef29a88bd75e1d";

	static constexpr const uint32_t davStackSize = 192;
	static constexpr const DavProperty davProperties[1] = {DavProperty("DAV:", "getcontentlength")};
};

constexpr const DavProperty Types::davProperties[1];

class HttpSession: protected HttpLogic<HttpSession, Types>
{
	friend HttpLogic<HttpSession, Types>;
	SOCKET& socket;

    bool silenced = false, stunned = false, disarmed = false, hexed = false, muted = false;
    BoolExtractor silencedExtractor, stunnedExtractor, disarmedExtractor, hexedExtractor, mutedExtractor;

    int healthPercent = -1, manaPercent = -1;
    NumberExtractor healthPercentExtractor, manaPercentExtractor;

	ObjectFilter<7> propertiesFilter = ObjectFilter<7>(
			FilterEntry("health_percent", &healthPercentExtractor),
			FilterEntry("mana_percent", &manaPercentExtractor),
			FilterEntry("silenced", &silencedExtractor),
			FilterEntry("stunned", &stunnedExtractor),
			FilterEntry("disarmed", &disarmedExtractor),
			FilterEntry("hexed", &hexedExtractor),
			FilterEntry("muted", &mutedExtractor)
		);

	ObjectFilter<1> rootFilter = ObjectFilter<1>(FilterEntry("hero", &propertiesFilter));

	JsonParser<8> jsonParser;

	void send(const char* str, unsigned int length) {
		::send(socket, str, length, 0);
	}

	void flush() {}

	inline HttpStatus arrangeReceiveInto(const char* dstName, uint32_t length) {
		jsonParser.reset(&rootFilter);
		return HTTP_STATUS_OK;
	}

	inline HttpStatus writeContent(const char* buff, uint32_t length)
	{
		jsonParser.parse(buff, length);
		return HTTP_STATUS_OK;
	}

	inline HttpStatus contentWritten() {
		if(jsonParser.done()) {
			std::cout << "silenced: " << silenced << ", ";
			std::cout << "stunned: " << stunned << ", ";
			std::cout << "disarmed: " << disarmed << ", ";
			std::cout << "hexed: " << hexed << ", ";
			std::cout << "muted: " << muted << ", ";
			std::cout << "healthPercent: " << healthPercent << ", ";
			std::cout << "manaPercent: " << manaPercent << std::endl;
		}
		return HTTP_STATUS_OK;
	}

public:

	inline HttpSession(SOCKET& socket):
		socket(socket),
    	silencedExtractor(silenced),
    	stunnedExtractor(stunned),
    	disarmedExtractor(disarmed),
    	hexedExtractor(hexed),
    	mutedExtractor(muted),
    	healthPercentExtractor(healthPercent),
    	manaPercentExtractor(manaPercent) {
		reset();
	}

	void process() {
	    int iSendResult;
	    int iResult;
	    char recvbuf[DEFAULT_BUFLEN];
	    int recvbuflen = DEFAULT_BUFLEN;

	    // Receive until the peer shuts down the connection
	    while(true) {
	        iResult = recv(socket, recvbuf, recvbuflen, 0);

	        if (iResult > 0)
	            parse(recvbuf, iResult);
	        else
	        	break;

	        if(isError(getStatus())) {
	        	beginHeaders();
	        	send("\r\n", 2);
	        	reset();
	        }
	    };

		done();
	}
};

int main(void)
{
    WSADATA wsaData;
    int iResult;

    SOCKET ListenSocket = INVALID_SOCKET;
    SOCKET ClientSocket = INVALID_SOCKET;

    struct addrinfo *result = NULL;
    struct addrinfo hints;


    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the server address and port
    iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    if ( iResult != 0 ) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Create a SOCKET for connecting to server
    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET) {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    // Setup the TCP listening socket
    iResult = bind( ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(result);

    iResult = listen(ListenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        printf("listen failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    // Accept a client socket
    ClientSocket = accept(ListenSocket, NULL, NULL);
    if (ClientSocket == INVALID_SOCKET) {
        printf("accept failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    HttpSession(ClientSocket).process();

    // No longer need server socket
    closesocket(ListenSocket);

    // shutdown the connection since we're done
    iResult = shutdown(ClientSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(ClientSocket);
        WSACleanup();
        return 1;
    }

    // cleanup
    closesocket(ClientSocket);
    WSACleanup();

    return 0;
}
