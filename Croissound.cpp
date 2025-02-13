#include <Windows.h>
#include <ShlObj.h>
#include <iostream>
#include <vector>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

struct RegistryEntry {
	std::wstring keyName;
	std::wstring defaultValue;
};
std::wstring getLocalLowPath() {
	WCHAR* localLowPath = nullptr;

	// Get the AppData\LocalLow path using SHGetKnownFolderPath()
	if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppDataLow, 0, NULL, &localLowPath))) {
		std::wstring path(localLowPath);
		CoTaskMemFree(localLowPath);
		return path;
	}
	return L"";
}

std::vector<RegistryEntry> listFilesInDirectory(const std::wstring& localLowPath, const std::wstring& programDirectory) {
	std::vector<RegistryEntry> registryArray;
	WCHAR searchPath[MAX_PATH];
	WIN32_FIND_DATAW findData;
	HANDLE hFind;

	swprintf(searchPath, MAX_PATH, L"%s\\%s\\*", localLowPath.c_str(), programDirectory.c_str());
	hFind = FindFirstFileW(searchPath, &findData);
	if (hFind == INVALID_HANDLE_VALUE) {
		std::wcout << L"Failed to open directory: " << searchPath << std::endl;
	}
	else {
		do {
			if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
				RegistryEntry entry;

				std::wstring filename = findData.cFileName;
				size_t extPos = filename.rfind(L'.');
				if (extPos != std::wstring::npos) {
					entry.keyName = filename.substr(0, extPos);
				}
				else {
					entry.keyName = filename;
				}

				entry.defaultValue = localLowPath + L"\\" + programDirectory + L"\\" + findData.cFileName;
				registryArray.push_back(entry);
			}
		} while (FindNextFileW(hFind, &findData));

		FindClose(hFind);
	}

	return registryArray;
}


int listSubkeys(LPCWSTR subKey, WCHAR*** subkeyNames) {
	HKEY hKey;
	DWORD dwSubKeys = 0;
	DWORD dwMaxSubKeyLen = 0;
	int subkeyCount = 0;

	if (RegOpenKeyExW(HKEY_CURRENT_USER, subKey, 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
		wprintf(L"Failed to open registry key: %s\n", subKey);
		return -1;
	}

	LONG result = RegQueryInfoKeyW(hKey, NULL, NULL, NULL, &dwSubKeys, &dwMaxSubKeyLen, NULL, NULL, NULL, NULL, NULL, NULL);

	if (result != ERROR_SUCCESS) {
		wprintf(L"Failed to query registry key information\n");
		RegCloseKey(hKey);
		return -1;
	}

	*subkeyNames = (WCHAR**)malloc(dwSubKeys * sizeof(WCHAR*));
	if (*subkeyNames == NULL) {
		wprintf(L"Memory allocation failed\n");
		RegCloseKey(hKey);
		return -1;
	}

	for (DWORD index = 0; index < dwSubKeys; index++) {
		(*subkeyNames)[index] = (WCHAR*)malloc((dwMaxSubKeyLen + 1) * sizeof(WCHAR));
		if ((*subkeyNames)[index] == NULL) {
			wprintf(L"Memory allocation failed for subkey name %d\n", index);
			for (int i = 0; i < subkeyCount; i++) {
				free((*subkeyNames)[i]);
			}
			free(*subkeyNames);
			RegCloseKey(hKey);
			return -1;
		}

		DWORD subkeyNameSize = dwMaxSubKeyLen + 1;
		result = RegEnumKeyExW(hKey, index, (*subkeyNames)[index], &subkeyNameSize, NULL, NULL, NULL, NULL);
		if (result != ERROR_SUCCESS) {
			wprintf(L"Failed to enumerate subkey at index %lu\n", index);
			break;
		}

		subkeyCount++;
	}

	RegCloseKey(hKey);

	return subkeyCount;
}

int setRegistryValue(const std::wstring& subKey, const std::wstring& valueName, const std::wstring& newValue) {
	HKEY hKey;
	LONG result = RegOpenKeyExW(HKEY_CURRENT_USER, subKey.c_str(), 0, KEY_SET_VALUE, &hKey);
	if (result != ERROR_SUCCESS) {
		std::wcerr << L"Failed to open registry key: " << subKey << std::endl;
		return 1;
	}

	result = RegSetValueExW(hKey, valueName.c_str(), 0, REG_SZ, reinterpret_cast<const BYTE*>(newValue.c_str()), static_cast<DWORD>((newValue.size() + 1) * sizeof(wchar_t)));
	if (result != ERROR_SUCCESS) {
		std::wcerr << L"Failed to set registry value: " << valueName << std::endl;
		RegCloseKey(hKey);
		return 1;
	}

	RegCloseKey(hKey);
	return 0;
}


int copyFolderToLocalLow(const fs::path& localLowPath, const std::wstring& folderName) {
	fs::path currentDir = fs::current_path();
	fs::path sourceFolder = currentDir / folderName;

	fs::path targetFolder = localLowPath / folderName;

	try {

		if (!fs::exists(sourceFolder)) {
			std::wcerr << folderName << L" doesn't exist in current directory, aborting." << std::endl;
			return 1;
		}

		if (fs::exists(targetFolder)) {
			std::wcout << folderName << L" already exists in LocalLow, skipping copy." << std::endl;
			return 0;
		}

		std::wcout << L"Copying folder from: " << sourceFolder << L" to: " << targetFolder << std::endl;
		fs::copy(sourceFolder, targetFolder, fs::copy_options::recursive);

		std::wcout << L"Folder copied successfully!" << std::endl;
		return 0;
	}
	catch (const std::exception& e) {
		std::wcerr << e.what() << std::endl;
		return 1;
	}
}

int main() {

	const std::wstring programDirectory = L"Croissound";
	const std::wstring schemesKey = L"AppEvents\\Schemes";
	const std::wstring prefix = L"AppEvents\\Schemes\\Apps\\.Default\\";
	const std::wstring suffix = L"\\.Current";

	std::wstring localLowPathW = getLocalLowPath();
	if (localLowPathW.empty()) {
		std::wcerr << L"Failed to get LocalLow path." << std::endl;
		return 1;
	}
	fs::path localLowPath = localLowPathW;
	if (copyFolderToLocalLow(localLowPath, L"Croissound")) {
		return 1;
	}

	// Modifying every sound entries
	std::vector<RegistryEntry> registryArray = listFilesInDirectory(localLowPath, programDirectory);
	for (const auto& entry : registryArray) {
		if (!setRegistryValue(prefix + entry.keyName + suffix, L"", entry.defaultValue)) {
			std::wcout << entry.keyName << L" updated successfully!" << std::endl;
		}
	}
	// Modifying the used scheme
	setRegistryValue(schemesKey, L"", L".Current");

	return 0;
}