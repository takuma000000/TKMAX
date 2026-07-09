#include "HintClient.h"

#include <windows.h>
#include <winhttp.h>
#include <string>

#pragma comment(lib, "winhttp.lib")

bool HintClient::Send(const std::string& json) {
	latestHint_.clear();

	HINTERNET hSession = WinHttpOpen(
		L"HintLogClient/1.0",
		WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
		WINHTTP_NO_PROXY_NAME,
		WINHTTP_NO_PROXY_BYPASS,
		0
	);

	if (!hSession) {
		latestHint_ = "WinHttpOpen failed";
		return false;
	}

	HINTERNET hConnect = WinHttpConnect(
		hSession,
		L"localhost",
		3000,
		0
	);

	if (!hConnect) {
		latestHint_ = "WinHttpConnect failed";
		WinHttpCloseHandle(hSession);
		return false;
	}

	HINTERNET hRequest = WinHttpOpenRequest(
		hConnect,
		L"POST",
		L"/hint",
		nullptr,
		WINHTTP_NO_REFERER,
		WINHTTP_DEFAULT_ACCEPT_TYPES,
		0
	);

	if (!hRequest) {
		latestHint_ = "WinHttpOpenRequest failed";
		WinHttpCloseHandle(hConnect);
		WinHttpCloseHandle(hSession);
		return false;
	}

	std::wstring headers = L"Content-Type: application/json\r\n";

	BOOL result = WinHttpSendRequest(
		hRequest,
		headers.c_str(),
		(DWORD)-1L,
		(LPVOID)json.c_str(),
		(DWORD)json.size(),
		(DWORD)json.size(),
		0
	);

	if (!result) {
		latestHint_ = "WinHttpSendRequest failed";
		WinHttpCloseHandle(hRequest);
		WinHttpCloseHandle(hConnect);
		WinHttpCloseHandle(hSession);
		return false;
	}

	result = WinHttpReceiveResponse(hRequest, nullptr);

	if (!result) {
		latestHint_ = "WinHttpReceiveResponse failed";
		WinHttpCloseHandle(hRequest);
		WinHttpCloseHandle(hConnect);
		WinHttpCloseHandle(hSession);
		return false;
	}

	std::string response;

	DWORD size = 0;

	do {
		DWORD downloaded = 0;

		if (!WinHttpQueryDataAvailable(hRequest, &size)) {
			break;
		}

		if (size == 0) {
			break;
		}

		std::string buffer;
		buffer.resize(size);

		if (!WinHttpReadData(
			hRequest,
			buffer.data(),
			size,
			&downloaded
		)) {
			break;
		}

		response.append(buffer.data(), downloaded);

	} while (size > 0);

	WinHttpCloseHandle(hRequest);
	WinHttpCloseHandle(hConnect);
	WinHttpCloseHandle(hSession);

	// 今のExpressは {"hint":"..."} を返すので、超簡易的に中身だけ抜く
	const std::string key = "\"hint\":\"";
	size_t start = response.find(key);

	if (start == std::string::npos) {
		latestHint_ = response;
		return true;
	}

	start += key.size();

	size_t end = response.find("\"", start);

	if (end == std::string::npos) {
		latestHint_ = response;
		return true;
	}

	latestHint_ = response.substr(start, end - start);

	return true;
}