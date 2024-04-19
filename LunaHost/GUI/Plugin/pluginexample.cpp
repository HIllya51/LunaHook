#include"plugindef.h"

bool ProcessSentence(std::wstring& sentence, SentenceInfo sentenceInfo);

extern "C" __declspec(dllexport) wchar_t* OnNewSentence(wchar_t* sentence, const InfoForExtension* sentenceInfo)
{
	try
	{
		std::wstring sentenceCopy(sentence);
		int oldSize = sentenceCopy.size();
		if (ProcessSentence(sentenceCopy, SentenceInfo{ sentenceInfo }))
		{
			if (sentenceCopy.size() > oldSize) sentence = (wchar_t*)HeapReAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS, sentence, (sentenceCopy.size() + 1) * sizeof(wchar_t));
			wcscpy_s(sentence, sentenceCopy.size() + 1, sentenceCopy.c_str());
		}
	}
	catch (std::exception &e)
	{
		*sentence = L'\0';
	}
	return sentence;
}
bool sendclipboarddata(const std::wstring&text,HWND hwnd);
bool ProcessSentence(std::wstring& sentence, SentenceInfo sentenceInfo) 
{
	if (sentenceInfo["current select"] && sentenceInfo["process id"] != 0 &&sentenceInfo["toclipboard"])
	{
		sendclipboarddata(sentence,(HWND)sentenceInfo["HostHWND"]);
	}
	return false;
}