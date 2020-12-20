typedef struct
{
	char *Host;
	char *Port;

	SOCKET Socket;

	SSL *Ssl;
	SSL_CTX *SslCtx;
} connection;

typedef enum
{
	REQUEST_METHOD_GET,
	REQUEST_METHOD_POST,
	REQUEST_METHOD_PUT,
} request_method;

#ifdef _WIN32
internal SOCKET
Win32ConnectSocket(char *Address, char *Port)
{
	SOCKET ConnectedSocket = 0;

	// NOTE(Felix): Init Windows Winsock (newest version as of writing is 2.2)
	WSADATA WindowsSocketInitData = { 0 };
	if (WSAStartup(MAKEWORD(2, 2), &WindowsSocketInitData))
	{
		fprintf(stderr, "Win32 Socket Error: %d\n", WSAGetLastError());
		exit(-1);
	}

	// NOTE(Felix): Info about the connection type, as well as structure to hold more than we care about
	struct addrinfo *AddressInfoResult = { 0 };
	struct addrinfo AddressInfoHints = { 0 };
	{
		AddressInfoHints.ai_family = AF_INET6; // IPv6
		AddressInfoHints.ai_socktype = SOCK_STREAM; // Reliable, two-way, TCP
		AddressInfoHints.ai_protocol = IPPROTO_TCP; 
	}

	// NOTE(Felix): We'll let windows resolve the server address from the dns
	assert(0 == getaddrinfo(Address, Port, &AddressInfoHints, &AddressInfoResult));

	// NOTE(Felix): We received a list from windows with possible connections, 
	// attempt connections until we succeed
	{
		i32 ConnectResult = SOCKET_ERROR;
		struct addrinfo *AddressEntry = AddressInfoResult;
		for (; AddressEntry != 0; AddressEntry = AddressEntry->ai_next)
		{
			// NOTE(Felix): Create socket
			ConnectedSocket = socket(AddressEntry->ai_family, AddressEntry->ai_socktype, AddressEntry->ai_protocol);
			assert(ConnectedSocket != INVALID_SOCKET);

			// NOTE(Felix): Connect
			ConnectResult = connect(ConnectedSocket, AddressEntry->ai_addr, (i32)AddressEntry->ai_addrlen);
			if (ConnectResult == SOCKET_ERROR)
			{
				// NOTE(Felix): This didn't work for some reason,
				// try the next entry
				closesocket(ConnectedSocket);
				continue;
			}

			break;
		}
		freeaddrinfo(AddressInfoResult);
		assert(ConnectResult != SOCKET_ERROR);
	}

	return (ConnectedSocket);
}
#else
#error Unknown environment
#endif

internal connection
CreateConnection(char *Host, char *Port)
{
	connection Connection = { 0 };
	Connection.Host = Host;
	Connection.Port = Port;

	// NOTE(Felix): Create Socket
#ifdef _WIN32
	Connection.Socket = Win32ConnectSocket(Connection.Host, Connection.Port);
#else
#error Unknown environment
#endif


	// NOTE(Felix): Flexible TLS method, 
	// OpenSSL with negotiate the highest mutually available method automatically
	// used for the "client_hello"
	const SSL_METHOD *SslMethod = TLS_client_method();

	// NOTE(Felix): Create SSL context
	Connection.SslCtx = SSL_CTX_new(SslMethod);
	assert(Connection.SslCtx != 0);

#if 0
	// NOTE(Felix): Disable unwanted methods
	SSL_CTX_set_options();
#endif

	// NOTE(Felix): Create SSL connection state object and attach to socket
	Connection.Ssl = SSL_new(Connection.SslCtx);
	SSL_set_tlsext_host_name(Connection.Ssl, Connection.Host);
	SSL_set_fd(Connection.Ssl, (i32)Connection.Socket);

	// NOTE(Felix): Try to SSL connect
	fprintf(stderr, "Attempting connection...\n");
	i32 SslConnectResult = SSL_connect(Connection.Ssl);
	if (SslConnectResult != 1)
	{
		fprintf(stderr, "Creating connection failed with, SSL_connect error %" PFi32 "\n", SslConnectResult);
		assert(!"Error: Could not SSL/TLS-connect.");
	}
	
	return (Connection);
}

internal void
CleanupConnection(connection *Connection)
{
	SSL_free(Connection->Ssl);

#ifdef _WIN32
	closesocket(Connection->Socket);
#else
#error Unknown environment
#endif

	SSL_CTX_free(Connection->SslCtx);
}

internal void
RecreateConnection(connection *Connection)
{
	connection NewConnection = CreateConnection(Connection->Host, Connection->Port);
	CleanupConnection(Connection);
	*Connection = NewConnection;
}


// TODO(Felix): Get transfer encoding
// transfer-encoding: chunked -> Do chunked parsing
// Content-Length: NUMBER  -> No chunked parsing, just get NUMBER bytes
// Anything else (zip, inflated) -> Throw error for now, we don't want to deal with this
// TODO(Felix): Replace a bunch of u32 / u64 with umm
internal u32
HttpSendRequestAndFillBufferWithResponse(connection *Connection,
										 request_method RequestMethod, char *Resource, char *Authorization, 
										 void *Data, u32 DataInBytes,
                                         char *RequestBuffer, 
                                         char *ResponseBuffer, u32 ResponseBufferSize)
{
	memset(ResponseBuffer, 0, ResponseBufferSize);

	// Format request
	i32 RequestLengthInBytes = 0;
	{
		char *RequestMethodString = 0;
		char *OptionalContentLength = 0;
		switch (RequestMethod)
		{
			case REQUEST_METHOD_GET: { 
				RequestMethodString = "GET";  
			} break;

			case REQUEST_METHOD_POST: { 
				RequestMethodString = "POST"; 
			} break;

			case REQUEST_METHOD_PUT: {
				RequestMethodString = "PUT";  
			} break;

			default: { assert(!"Unknown request method"); } break;
		}

		RequestLengthInBytes = sprintf(RequestBuffer,
		                               "%s %s HTTP/1.1\r\n"
		                               "HOST: %s\r\n",
									   RequestMethodString, Resource,
									   Connection->Host);
		
		if (Authorization)
		{
			sprintf(RequestBuffer + strlen(RequestBuffer),
					"Authorization: %s\r\n",
					Authorization);
		}
		
		if (Data)
		{
			sprintf(RequestBuffer + strlen(RequestBuffer),
			        "Content-Length: %" PFu32 "\r\n",
			        DataInBytes);
		}

		strcat(RequestBuffer, "\r\n");
		RequestLengthInBytes = (i32)strlen(RequestBuffer);
		
		if (Data)
		{
			memcpy(RequestBuffer + RequestLengthInBytes, Data, DataInBytes);
			RequestLengthInBytes += (i32)DataInBytes;
		}
		else
		{
			// TODO(Felix): Check speck, do we need another empty line?
		}
	}

	u32 ResponseBodyLength = 0;
	b32 Successful = 0;
	while (0 == Successful)
	{
		u32 TotalBytesReceived = 0;

		// NOTE(Felix): Send Request
		{
			i32 BytesSent = SSL_write(Connection->Ssl, RequestBuffer, RequestLengthInBytes);
			if (BytesSent <= 0)
			{
				RecreateConnection(Connection);
				continue;
			}
		}
		
		// NOTE(Felix): Receive header
		{
			b32 ReceivedCompleteHeader = 0;
			b32 ConnectionFailed = 0;
			while (0 == ReceivedCompleteHeader &&
				   0 == ConnectionFailed)
			{
				i32 BytesJustReceived = SSL_read(Connection->Ssl, ResponseBuffer + TotalBytesReceived, (i32)(ResponseBufferSize - TotalBytesReceived));
				ConnectionFailed = (BytesJustReceived <= 0);
				TotalBytesReceived += (u32)BytesJustReceived;
				ReceivedCompleteHeader = (0 != strstr(ResponseBuffer, "\r\n\r\n"));
			}
			
			if (ConnectionFailed)
			{
				RecreateConnection(Connection);
				continue;
			}
		}
		
		// NOTE(Felix): Determine reponse body format
		if (strstr(ResponseBuffer, "Transfer-Encoding: chunked\r\n"))
		{
			// NOTE(Felix): Chunked
			
			// Try to get first chunk size, retrieve more if necessary
			char *StartOfChunkSize = 0;
			char *StartOfChunkContent = 0;
			{
				while (0 == StartOfChunkSize ||
					   0 == StartOfChunkContent)
				{
					StartOfChunkSize = strstr(ResponseBuffer, "\r\n\r\n");
					if (StartOfChunkSize)
					{
						StartOfChunkSize += 4; // skip "\r\n\r\n"
						StartOfChunkContent = strstr(StartOfChunkSize, "\r\n");
						if (StartOfChunkContent)
						{
							StartOfChunkContent += 2; // skip "\r\n"
						}
					}

					if (0 == StartOfChunkSize ||
					    0 == StartOfChunkContent)
					{
						i32 BytesJustReceived = SSL_read(Connection->Ssl, ResponseBuffer + TotalBytesReceived, (i32)(ResponseBufferSize - TotalBytesReceived));
						TotalBytesReceived += (u32)BytesJustReceived;
						assert(BytesJustReceived > 0);
						// We assume that our connection won't fail mid-transmition
					}
				}
			}

			// NOTE(Felix): Parse / get chunk by chunk
			{
				u64 ChunkSize = StringParseUnsignedHexadecimal(StartOfChunkSize);
				u64 IndexWithinChunk = (u64)((ResponseBuffer + TotalBytesReceived) - StartOfChunkContent);
				assert(ChunkSize > 0);

				// TODO(Felix): We sometimes get stuck here, calling SSL_read way too often.
				// I feel like this migh tbe due to the fact that we already received everything but have not
				// "moved" to the latest chunk size (e.g. resizing multiple in one SSL_read call)
				// 
				// On further investigation - we're sometimes just ending up with an
				// empty buffer.
				while (ChunkSize > 0)
				{
					IndexWithinChunk = (u64)((ResponseBuffer + TotalBytesReceived) - StartOfChunkContent);
					if (IndexWithinChunk >= ChunkSize + 2) // +2 for trailing "\r\n" 
					{
						// NOTE(Felix): Received complete chunk, 
						// check if we can also read the size of the next chunk
						char *StartOfNewChunkSize = StartOfChunkContent + ChunkSize + 2; // +2 trailing "\r\n"
						char *StartOfNewChunkContent = strstr(StartOfNewChunkSize, "\r\n");
						if (StartOfNewChunkContent)
						{
							// Update properties to the ones from next chunk
							StartOfChunkSize = StartOfNewChunkSize;
							StartOfChunkContent = StartOfNewChunkContent + 2; // Skip trailing "\r\n"
							ChunkSize = StringParseUnsignedHexadecimal(StartOfChunkSize); // TODO(Felix): Are we failing here weirdly?
							IndexWithinChunk = (u64)((ResponseBuffer + TotalBytesReceived) - StartOfChunkContent);

							// NOTE(Felix): We have received complete chunk and also properties of next chunk,
							// start next iteration to re-do checks
							continue;
						}
					}

					// NOTE(Felix): If any of the if's above fail we'll get here 
					// It means we still have stuff to receive, so we'll get more of the chunk
					i32 BytesJustReceived = SSL_read(Connection->Ssl, ResponseBuffer + TotalBytesReceived, (i32)(ResponseBufferSize - TotalBytesReceived));
					TotalBytesReceived += (u32)BytesJustReceived;
					assert(BytesJustReceived > 0);
				}
			}
			
			// NOTE(Felix): Get rid of http header, stitch chunks together and remove trailing chunk
			{
				// NOTE(Felix): Get rid of header
				u32 ResponseBufferLengthUsed = 0;
				{
					char *HttpBodyStart = strlen("\r\n\r\n") + strstr(ResponseBuffer, "\r\n\r\n");
					u32 SizeOfHttpHeader = (u32)(HttpBodyStart - ResponseBuffer);
					u32 BytesOfBody = TotalBytesReceived - SizeOfHttpHeader;
					memmove(ResponseBuffer, ResponseBuffer+SizeOfHttpHeader, BytesOfBody);
					ResponseBufferLengthUsed = BytesOfBody;
				}

				// NOTE(Felix): Iterate over chunks and stitch them together
				u8 *BufferEndToStitchTo = (void *)ResponseBuffer;
				char *ChunkHeader = ResponseBuffer;
				char *ChunkContent = strstr(ChunkHeader, "\r\n") + strlen("\r\n");
				u64 ChunkSize = StringParseUnsignedHexadecimal(ChunkHeader);
				while (ChunkSize > 0)
				{
					u64 ChunkHeaderInBytes = (u64)(ChunkContent - (char *)BufferEndToStitchTo); // From previous Chunk end
					u64 BytesToMove = (u64)((ResponseBuffer + ResponseBufferLengthUsed) - (char *)BufferEndToStitchTo);
					memmove(BufferEndToStitchTo, BufferEndToStitchTo+ChunkHeaderInBytes, BytesToMove);

					ResponseBufferLengthUsed -= (u32)ChunkHeaderInBytes;
					BufferEndToStitchTo += ChunkSize;

					ChunkHeader = (char *)BufferEndToStitchTo + 2;
					ChunkContent = strstr(ChunkHeader, "\r\n") + 2; // skip "\r\n"
					ChunkSize = StringParseUnsignedHexadecimal(ChunkHeader);
				}

				// NOTE(Felix): Reached last chunk, remove end
				{
					u64 BytesToRemove = strlen("\r\n"
					                           "0\r\n"
					                           "\r\n");
					MemoryClear(BufferEndToStitchTo, BytesToRemove);
					ResponseBufferLengthUsed -= (u32)BytesToRemove;
				}

				ResponseBodyLength = ResponseBufferLengthUsed;
			}
		}
		else if (strstr(ResponseBuffer, "Content-Length: "))
		{
			char *ContentLengthString = strstr(ResponseBuffer, "Content-Length: ");
			u32 BytesOfBody = 0;
			i32 ArgumentsParsed = sscanf(ContentLengthString, "Content-Length: %" SFu32 "\r\n", &BytesOfBody);
			assert(ArgumentsParsed == 1);

			// NOTE(Felix): Just x bytes in body
			char *StartOfBody = strlen("\r\n\r\n") + strstr(ResponseBuffer, "\r\n\r\n");
			u32 BytesOfBodyReceived = (u32)((ResponseBuffer + TotalBytesReceived) - StartOfBody);
			while (BytesOfBodyReceived < BytesOfBody)
			{
				i32 BytesJustReceived = SSL_read(Connection->Ssl, ResponseBuffer + TotalBytesReceived, (i32)(ResponseBufferSize - TotalBytesReceived));
				assert(BytesJustReceived > 0);
				TotalBytesReceived += (u32)BytesJustReceived;
				BytesOfBodyReceived = (u32)((ResponseBuffer + TotalBytesReceived) - StartOfBody);
			}
			assert(BytesOfBodyReceived == BytesOfBody);

			memmove(ResponseBuffer, StartOfBody, BytesOfBody);
			ResponseBuffer[BytesOfBody] = 0; // 0 terminate actual response, everything behind that is junk
			//memset(ResponseBuffer + BytesOfBody, 0, TotalBytesReceived - BytesOfBody);
			
			ResponseBodyLength = BytesOfBody;
		}
		else
		{
			fprintf(stderr, "Header from message we're not able to parse:\n%s\n\n", ResponseBuffer);
			assert(!"Unknown format in response");
		}
		
		Successful = 1;
	}

	//MemoryClear(RequestBuffer, (u64)RequestLength); // TODO(Felix): Do we need this clear? 
	return (ResponseBodyLength);
}
