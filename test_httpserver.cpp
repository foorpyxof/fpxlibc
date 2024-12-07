#include "test-definitions.h"
#include "fpx_networking/http/httpserver.h"

using namespace fpx;

void WebSocketCallback(HttpServer::websocket_client_t* client, uint16_t metadata, uint64_t len, uint32_t mask_key, uint8_t* data) {
	printf("MESSAGE: %s\n", data);
	HttpServer::websocket_frame_t frame;
	frame.SetBit(128);
	frame.SetOpcode(0x01);
	frame.SetPayload((const char*)data);
	frame.Send(client->FileDescriptor);
}

void UserAgentCallback(HttpServer::http_request_t* req, HttpServer::http_response_t* res) {
	int userAgentIndex = fpx_substringindex(req->Headers, "User-Agent: ") + 12;
	
	res->SetCode("200");
	res->SetStatus("OK");
	res->SetHeaders("Content-Type: text/plain\r\n");
	if (userAgentIndex > -1) {
		char tempBuf[256];
		snprintf(tempBuf, strcspn(&req->Headers[userAgentIndex], "\r\n")+1, "%s", &req->Headers[userAgentIndex]);
		res->SetBody(tempBuf);
	}
}

void RootCallback(HttpServer::http_request_t* req, HttpServer::http_response_t* res) {
	res->SetHeaders("Content-Type: text/plain\r\n");
	res->SetBody("Hello from fpxHTTP :D");
}

int main() {

	HttpServer httpServ("0.0.0.0", 9999);
	
  // httpServ.SetOption(HttpServer::HttpServerOptions::ManualWebSocket);
  // ^^^ this line allows the programmer to handle the websocket connection themselves via the callback. ^^^
  // make sure to add all the required WebSocket headers in the response before returning from the callback.

  httpServ.SetWebSocketTimeout(60);
  httpServ.CreateEndpoint("/", HttpServer::GET, RootCallback);
  httpServ.CreateEndpoint("/useragent", HttpServer::GET | HttpServer::HEAD, UserAgentCallback);
  httpServ.Listen(HttpServer::ServerType::Both, WebSocketCallback);

}